/*******************************************************************************
* FILENAME: sensor_db.c							       
*
* Version V1.1		
* Author: Pham Hoang Chi
*
* An implementation of Lab 7 assignment - System Software Course
* 
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <my_global.h>
#include "sensor_db.h"

//install lib: 
//Compile: gcc -Wall -g -D_GNU_SOURCE -o out test.c sensor_db.c $(mysql_config --cflags --libs)


#ifdef DEBUG
	#define DEBUG_PRINT(...) 															\
	  do {					  															\
		printf("In %s - function %s at line %d: ", __FILE__, __func__, __LINE__);		\
		printf(__VA_ARGS__);															\
	  } while(0)
#else
	#define DEBUG_PRINT(...) 		\
		do {						\
			printf(__VA_ARGS__);	\
		} while(0)
#endif

#define CHECK_QUERY(query) 								\
		do {												\
			if(query < 0) {									\
				DEBUG_PRINT( "Error: incorrect query\n" );	\
				exit(EXIT_FAILURE);							\
			}												\
		} while (0)

					
void tstamp_to_tstring(sensor_ts_t ts, char *strtime) {
	strftime(strtime, 20, "%F %T",localtime(&ts));  	
}

/*
 * Make a connection to MySQL database
 * Create a table named 'yourname' if the table does not exist
 * If the table existed, clear up the existing data if clear_up_flag is set to 1
 * return the connection for success, NULL if an error occurs
 */

MYSQL *init_connection(int clear_up_flag)
{
	if (mysql_library_init(0, NULL, NULL)) {
		fprintf(stderr, "could not initialize MySQL library\n");
		exit(1);
	}
	
	// initialises a MYSQL object	
	MYSQL *con = mysql_init(NULL);	
	if(con == NULL) return NULL;
	char *query1, *query2;	
		
	// establish a connection to the database: server_name(studev.groept.be) - user_name(a13_syssoft) - pw(a13_syssoft) - database_name(a13_syssoft) 
	if (mysql_real_connect(con, DB_SERVER_NAME, USER_NAME, PASSWORD, DB_NAME, 0, NULL, 0) == NULL) {
		fprintf(stderr, "%s\n", mysql_error(con));
		return NULL;
	} 
		
	// Check clear_up_flag and then check if the table is already existing	
	if(clear_up_flag) {
		asprintf(&query1, "DROP TABLE IF EXISTS %s", TABLE_NAME);		
		if (mysql_query(con, query1)) {
			fprintf(stderr, "%s\n", mysql_error(con));			
		}

		// Create new table with columns: id (primary_key and auto_increment), sensor_id(INT), sensor_value DECIMAL(4,2), timestamp (TIMESTAMP)
		asprintf(&query2, "CREATE TABLE %s (id INT PRIMARY KEY AUTO_INCREMENT, sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP)", TABLE_NAME);
		if (mysql_query(con, query2)) {      
			fprintf(stderr, "%s\n", mysql_error(con));
		}	
		
		free(query2);
	}	
		
	free(query1);		
	return con;
}

/*
 * Reconnect to MySQL database
 */
int reconnect(MYSQL *conn) {
	// my_bool recon = 1;
	if(conn == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		return -1;
	} 
	// Enable Auto-reconnect option
	// mysql_options(conn, MYSQL_OPT_RECONNECT, &recon);
	if (mysql_real_connect(conn, DB_SERVER_NAME, USER_NAME, PASSWORD, DB_NAME, 0, NULL, 0) == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		return -1;
	}
	
	return 1;
}

/*
 * Disconnect MySQL database
 */
void disconnect(MYSQL *conn) {
	mysql_close(conn);
	conn = NULL;
	mysql_library_end();
}

/*
 * Checks whether the connection to the server is working
 * return zero for success, and non-zero if an error occurs
 */
int check_connection(MYSQL *conn) {
	if(mysql_ping(conn)) return -1;
	else return 0;
} 

/*
 * Write an INSERT query to insert a single sensor measurement
 * return zero for success, and non-zero if an error occurs
 */

int insert_sensor(MYSQL *conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts) {
	char *query;	
	char strtime[20];  
	tstamp_to_tstring(ts, (char *)strtime);
	if(conn == NULL) return -1;
	
	asprintf(&query, "INSERT INTO %s(sensor_id, sensor_value, timestamp) VALUES (%d, %g, '%s')", TABLE_NAME, (int)id,(double)value, strtime);		
	if (mysql_query(conn, query)) {    
		fprintf(stderr, "%s\n", mysql_error(conn));  
		// if(conn == NULL) return -1;
		return -1;
	}
	free(query);
	return 0;
}

/*
 * Write an INSERT query to insert all sensor measurements existed in the file pointed by sensor_data
 * return zero for success, and non-zero if an error occurs
 */

int insert_sensor_from_file(MYSQL *conn, FILE *sensor_data_file) {
	sensor_id_t id;
	sensor_value_t value;
	sensor_ts_t ts;
	int fsize;
	int i, count, result;
	
	if(conn == NULL) {
		fprintf(stderr, "Error in connection\n");  	
		return -1;
	}
	// Check the file
	if ( sensor_data_file ==NULL) {
		fprintf(stderr, "Error: File is not opened\n");  	
		return -1;
	}	
	// obtain file size:
	fseek (sensor_data_file , 0 , SEEK_END);
	fsize = ftell (sensor_data_file);
	rewind (sensor_data_file);		
	count = fsize/(sizeof(uint16_t) + sizeof(double) + sizeof(time_t));	
	
	// Check each data package=<sensor ID><temperature><timestamp>
	for(i=0; i<count; i++) { 
		result = fread( &id, sizeof(sensor_id_t), 1, sensor_data_file );
		if ( result < 1 ) {
			fprintf(stderr, "Error: File read failed\n"); return -1;			
		}
		
		result = fread( &value, sizeof(sensor_value_t), 1, sensor_data_file );
		if ( result < 1 ) {
			fprintf(stderr, "Error: File read failed\n"); return -1;			
		}
		
		result = fread( &ts, sizeof(sensor_ts_t), 1, sensor_data_file );
		if ( result < 1 ) {
			fprintf(stderr, "Error: File read failed\n"); return -1;			
		}
		
		if(insert_sensor(conn, id, value, ts) == -1) {
			fprintf(stderr, "Error: Insert to database failed\n");
			return -1;
		}		
	}
	
	return 0;
}

/*
 * Write a SELECT query to return all sensor measurements existed in the table
 * return MYSQL_RES with all the results
 */

MYSQL_RES *find_sensor_all(MYSQL *conn) {
	if(conn == NULL) return NULL;
	
	char *query;	
	asprintf(&query, "SELECT * FROM %s", TABLE_NAME);
	
	if (mysql_query(conn, query)) {    
		fprintf(stderr, "%s\n", mysql_error(conn));  		
	}	
	
	MYSQL_RES *result = mysql_store_result(conn);
	if(result == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
	}	
	
	free(query);
	return result;		
}

/*
 * Write a SELECT query to return all sensor measurements containing 'value_t'
 * return MYSQL_RES with all the results
 */

MYSQL_RES *find_sensor_by_value(MYSQL *conn, sensor_value_t value_t) {
	char *query;	
	asprintf(&query, "SELECT * FROM %s WHERE sensor_value=%g", TABLE_NAME, (double)value_t);
	
	if(mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));   
		if(conn == NULL) return NULL;
	}	
	MYSQL_RES *result = mysql_store_result(conn);	
	free(query);
	return result;			
}

/*
 * Write a SELECT query to return all sensor measurement that its value exceeds 'value_t'
 * return MYSQL_RES with all the results
 */
MYSQL_RES *find_sensor_exceed_value(MYSQL *conn, sensor_value_t value_t) {
	char *query;	
	asprintf(&query, "SELECT * FROM %s WHERE sensor_value >= %g", TABLE_NAME, (double)value_t);
	
	if(mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));   
		if(conn == NULL) return NULL;
	}	
	
	MYSQL_RES *result = mysql_store_result(conn);	
	free(query);
	return result;			
}

/*
 * Write a SELECT query to return all sensor measurement containing timestamp 'ts_t'
 * return MYSQL_RES with all the results
 */
MYSQL_RES *find_sensor_by_timestamp(MYSQL *conn, sensor_ts_t ts_t) {
	char *query;
	char strtime[20];  
	tstamp_to_tstring(ts_t, (char *)strtime);	
	asprintf(&query, "SELECT * FROM %s WHERE timestamp= '%s'", TABLE_NAME, strtime);
	
	if(mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));   
		if(conn == NULL) return NULL;
	}	
	
	MYSQL_RES *result = mysql_store_result(conn);	
	free(query);
	return result;			
}

/*
 * Write a SELECT query to return all sensor measurement recorded later than timestamp 'ts_t'
 * return MYSQL_RES with all the results
 */
MYSQL_RES *find_sensor_later_timestamp(MYSQL *conn, sensor_ts_t ts_t) {
	char *query;
	char strtime[20];  
	tstamp_to_tstring(ts_t, (char *)strtime);	
	asprintf(&query, "SELECT * FROM %s WHERE timestamp >= '%s'", TABLE_NAME, strtime);
	
	if(mysql_query(conn, query)) {
		fprintf(stderr, "%s\n", mysql_error(conn));   
		if(conn == NULL) return NULL;
	}	
	
	MYSQL_RES *result = mysql_store_result(conn);	
	free(query);
	return result;			
}

/*
 * Return the number of records contained in the result
 */
int get_result_size(MYSQL_RES *result) {	
	return mysql_num_rows(result);
}

/*
 * Print all the records contained in the result
 */
void print_result(MYSQL_RES *result) {
	int num_fields = mysql_num_fields(result);
	MYSQL_ROW row;
	int i;
  
	while ((row = mysql_fetch_row(result))) { 
		for(i = 0; i < num_fields; i++) 
		{ 
			printf("%s ", row[i] ? row[i] : "NULL"); 
		} 
        printf("\n"); 
	}
}

/*
 * Free the records contained in the result
 */
void free_sensor_data(MYSQL_RES *result) {
	mysql_free_result(result);
}

