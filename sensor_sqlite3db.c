/*******************************************************************************
* FILENAME: sensor_sqlite3db.c							       
*
* Version V1.10		
* Author: Pham Hoang Chi
*
* An implementation of SQLite3 database Interaction with sensor data
* - System Software Course
* 
*******************************************************************************/
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "sensor_sqlite3db.h"

#define SQLITE3_ERROR(err, db, error_msg) 					\
		do {												\
			if ( (err) != SQLITE_OK )   					\
			{												\
				printf("%s: Error executing syscall\n", error_msg);			\
				fprintf(stderr, "Error: %s - %s", error_msg, sqlite3_errmsg(db));	\
				sqlite3_close(db);							\
				return NULL;						        \
			}												\
		} while(0)	

#define SQLITE3_ERROR_2(err, db, error_msg) 				\
		do {												\
			if ( (err) != SQLITE_OK )   					\
			{												\
				fprintf(stderr, "Error: %s - %s", error_msg, sqlite3_errmsg(db));	\
				sqlite3_close(db);							\
				return (-1);						        \
			}												\
		} while(0)	

/*
 * Make a connection to SQLite database
 * Create a table named 'yourname' if the table does not exist
 * If the table existed, clear up the existing data if clear_up_flag is set to 1
 * return the connection for success, NULL if an error occurs
 */
sqlite3 *sqlite_init_connection(int clear_up_flag) {
	sqlite3 *db;
	sqlite3_stmt *res;
	char *query1, *query2;
	
	// establish a connection to the database:
	int rc = sqlite3_open(SQLITE3_DB_FILE, &db);
	SQLITE3_ERROR(rc, db, "sqlite3_open");
		
	// Check clear_up_flag and then check if the table is already existing
	if((clear_up_flag) && (db != NULL)) {
		asprintf(&query1, "DROP TABLE IF EXISTS %s", TABLE_NAME);
		rc = sqlite3_prepare_v2(db, query1, -1, &res, 0);
		SQLITE3_ERROR(rc, db, "sqlite3_prepare_v2");
		
		rc = sqlite3_step(res);
		if (rc == SQLITE_ROW) {
			printf("%s\n", sqlite3_column_text(res, 0));
		}
		sqlite3_finalize(res);
		
		asprintf(&query2, "CREATE TABLE %s (id INTEGER PRIMARY KEY AUTOINCREMENT, sensor_id INTEGER, sensor_value REAL, timestamp TEXT)", TABLE_NAME);
		rc = sqlite3_prepare_v2(db, query2, -1, &res, 0);	
		SQLITE3_ERROR(rc, db, "sqlite3_prepare_v2");
		rc = sqlite3_step(res);
		if (rc == SQLITE_ROW) {
			printf("%s\n", sqlite3_column_text(res, 0));
		}		
		free(query1);
		free(query2);
		sqlite3_finalize(res);
	}	
	
	return db;	
}

/*
 * Write an INSERT query to insert a single sensor measurement
 * return zero for success, and non-zero if an error occurs
 */
int sqlite_insert_sensor(sqlite3 *db, sensor_id_t id, sensor_value_t value, sensor_ts_t ts) {
	
	if(db == NULL) return -1;
	
	sqlite3_stmt *res;
	char *query;
	char strtime[20];  	
	strftime(strtime, 20, "%F %T",localtime(&ts)); 
	int rc;
	
	asprintf(&query, "INSERT INTO %s(sensor_id, sensor_value, timestamp) VALUES (%d, %g, '%s')", TABLE_NAME, (int)id,(double)value, strtime);	
	rc = sqlite3_prepare_v2(db, query, -1, &res, 0);	
	SQLITE3_ERROR_2(rc, db, "sqlite3_prepare_v2");
	rc = sqlite3_step(res);
	if (rc == SQLITE_ROW) {
		printf("%s\n", sqlite3_column_text(res, 0));
	}
	
	free(query);	
	sqlite3_finalize(res);
	return 0;
}

/*
 * Disconnect SQLite database
 */
void sqlite_disconnect(sqlite3 *db) {
	if(db == NULL) exit(-1);
	int rc = sqlite3_close(db);
	if(rc != SQLITE_OK)	exit(-1);
}

/*
 * Reconnect to MySQL database
 */
 int sqlite_reconnect(sqlite3 *db) {
	 if(db == NULL) return -1;
	 int rc = sqlite3_open(SQLITE3_DB_FILE, &db);
	 SQLITE3_ERROR_2(rc, db, "sqlite3_open");
	 return 1;
 }
 
/* 
int main(void) {
    
    sqlite3 *db;
	sensor_ts_t ts1, ts2, ts3, ts4;
	int rc;
	    
	// Open connection to database and create a table
	db = sqlite_init_connection(1);
	
	ts1 = (sensor_ts_t)time(NULL);
	rc = sqlite_insert_sensor(db, 11, 10.1, ts1);
	SQLITE3_ERROR_2(rc, db, "SQLite Error: sqlite_insert_sensor");
	sleep(1);
	ts2 = (sensor_ts_t)time(NULL);
	rc = sqlite_insert_sensor(db, 22, 10.02, ts2);
	SQLITE3_ERROR_2(rc, db, "SQLite Error: sqlite_insert_sensor");
	sleep(1);
	ts3 = (sensor_ts_t)time(NULL);
	rc = sqlite_insert_sensor(db, 33, 11.02, ts3);
	SQLITE3_ERROR_2(rc, db, "SQLite Error: sqlite_insert_sensor");
	sleep(1);
	ts4 = (sensor_ts_t)time(NULL);
	rc = sqlite_insert_sensor(db, 44, 22.02, ts4);
	SQLITE3_ERROR_2(rc, db, "SQLite Error: sqlite_insert_sensor");
	
	sqlite_disconnect(db);	
    return 0;
}
// */