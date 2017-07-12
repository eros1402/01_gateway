/*******************************************************************************
* FILENAME: mylocalfunc.c							       
*
* Version V1.0		
* Author: Pham Hoang Chi
*
* An implementation of supporting functions - System Software
* 
*******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>   //thread
#include <assert.h>
#include <string.h> 
#include <unistd.h>
#include <poll.h>
#include "config.h"
#include "err_handler.h"
#include "myqueue.h"
#include "list.h"
#include "tcpsocket.h"
#include "mongoose.h"
#include "mylocalfunc.h"

int temp = 0;

int send_request_run(struct mg_connection *conn, gateway_conf_t *input, FILE *fp) {
  char port_num[10];
  char db_name[50], dropdown[10];
  char high_temp[3];
  char low_temp[3];
  char log_line[100]; 
  int len;
  
  if (strcmp(conn->uri, "/gw_config_set") == 0) {    
	mg_get_var(conn, "port_num", port_num, sizeof(port_num));
    mg_get_var(conn, "dropdown", dropdown, sizeof(dropdown));
    mg_get_var(conn, "db_name", db_name, sizeof(db_name));
	mg_get_var(conn, "high_temp", high_temp, sizeof(high_temp));
	mg_get_var(conn, "low_temp", low_temp, sizeof(low_temp));
	input->gw_port = atoi(port_num);
	asprintf(&(input->gw_db_type), "%s", dropdown);
	asprintf(&(input->gw_db_name), "%s", db_name); 
	if(strcmp(high_temp, "")) {	input->gw_high_temp = atoi(high_temp); }
    if(strcmp(low_temp, "")) {	input->gw_low_temp = atoi(low_temp); }	
	mg_send_file(conn, "gateway_running.html", NULL);  
	input->gw_flag = 1;
	return MG_MORE;
  } else {   
	if(strcmp(conn->uri, "/gw_general_info") == 0) {		
		if(strncmp(conn->content, "Get_log", strlen("Get_log")) == 0)
		{				
			if(fgets(log_line, 100, fp) !=  NULL) {
				len = strlen(log_line) - 1;
				if(log_line[len] == '\n') log_line[len] = '\0';				
			}
			else {
				strcpy(log_line, "");
			}			
			mg_printf_data(conn, "{\"log_line\": \"%s\", \"gw_ss_num\": %ld, \"gw_packet_num\": %ld}", log_line, input->gw_ss_num, input->gw_packet_num);
			temp++;			
		} else {
			mg_printf_data(conn, "{\"gw_port\": %ld, \"gw_db\": \"%s\", \"gw_db_name\": \"%s\", \"gw_high_temp\": %ld, \"gw_low_temp\": %ld,\"gw_ss_num\": %ld, \"gw_packet_num\": %ld}", input->gw_port, input->gw_db_type, input->gw_db_name, input->gw_high_temp, input->gw_low_temp, 0, 0);
		}				
		return MG_TRUE;
	}
	else {		
		
		if(input->gw_flag != 1) { mg_send_file(conn, "config.html", NULL); return MG_MORE;}
		else {
			mg_printf_data(conn,                   
						   "GW PORT: [%d]\n"
						   "Database: [%s] - name: [%s]\n",
							input->gw_port, input->gw_db_type, input->gw_db_name);
			return MG_TRUE;
		}					
	}	
  }
}

/*
 * Print help to stdout
 */
void print_help(void) {
  printf("Use this program with command line options: PORT=<port_number> DB_TYPE=<type_of_SQL_database>\n");
  printf("If the DB_TYPE is not mentioned, the default is MYSQL database\n");
  printf("\tE.g: ~$ ./a.out PORT=1234\n");
  printf("\tE.g: ~$ ./a.out PORT=1234 DB_TYPE=MYSQL\n");   
  printf("\tE.g: ~$ ./a.out PORT=1234 DB_TYPE=SQLITE\n");
}

/*
 * Check command-line argument with 2 input set for number of Port_Number and SQL database type
 * return 0 - If no input argument
 * return -1 - If there is error in one of two argument
 * return 1 - If there is no error
 */
int check_command_line_argument(int argc, char *argv[], gateway_conf_t *input) {
	char *temp;
	
	if(argc<=2) {
		print_help();	
		return 0;
	}
	else {
		temp = strchr(argv[1],'=');
		if(temp != NULL) {input->gw_port = atoi(temp+1);}
		else { print_help(); return -1; }	

		temp = strchr(argv[2],'=');
		if(temp != NULL) input->gw_db_type = (temp+1);
		else { input->gw_db_type = DEFAULT_DB_TYPE;}	
		
		input->gw_db_name = DB_NAME;
		return 1;	
	}
} 

/*
 *  check if the list size reaches max_list_size -> realloc memory'
 */
void dynamic_sensor_list_size (int list_size, int *max_list_size, struct pollfd *poll_fd, time_t *last_timestamp) {
	if(list_size == *max_list_size) {
		poll_fd = realloc(poll_fd, sizeof(struct pollfd)*(*max_list_size + INCREASE_MAX));
		malloc_check(poll_fd, "realloc() to poll_fd");
		last_timestamp = realloc(last_timestamp, sizeof(time_t)*(*max_list_size + INCREASE_MAX));
		malloc_check(last_timestamp, "realloc() to last_timestamp");
		
		*max_list_size = *max_list_size + INCREASE_MAX;
	}
	return;
}

/*
 *  The function supports read data received on the socket 's'
 */
int receive_sensor_data(Socket s, sensor_data_t *data_packet) {
	int bytes = 0;
	
	bytes = tcp_receive( (Socket)s, (void *)&(data_packet->id), sizeof(sensor_id_t));					
	assert( (bytes == sizeof(sensor_id_t)) || (bytes == 0) );	
	bytes = tcp_receive( (Socket)s, (void *)&(data_packet->value), sizeof(sensor_value_t));
	assert( (bytes == sizeof(sensor_value_t)) || (bytes == 0) );
	bytes = tcp_receive( (Socket)s, (void *)&(data_packet->ts), sizeof(sensor_ts_t));
	assert( (bytes == sizeof(sensor_ts_t)) || (bytes == 0) );
	 
	return bytes;
} 
 
 /* 
 * Generate a log event by writing to FIFO
 */
 void generate_log_event(int fifo_fd, int event_id, int sensorNodeID, int sensor_id) {
	char *event_msg;
	int presult;	
	presult = pthread_mutex_lock( &fifo_mutex );	
	pthread_err_handler( presult, "pthread_mutex_lock", __FILE__, __LINE__ );
	switch (event_id) {
		case EV_STOP: // Event: Gateway stop
			asprintf( &event_msg, "stop" );
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);
				// exit( EXIT_FAILURE );
			}
			break;
		
		case EV_START: // Event: Gateway start
			asprintf( &event_msg, "start" );
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
		
		case EV_NEW_CONN: // Event: A sensor node with <sensorNodeID> has opened a new connection
			asprintf( &event_msg, "A sensor node (Socket_id = %d, sensor_id = %d) has opened a new connection", sensorNodeID, sensor_id );
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;	
			
		case EV_CLOSE_CONN:
			asprintf( &event_msg, "The sensor node (Socket_id = %d, sensor_id = %d) has closed the connection", sensorNodeID, sensor_id);
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
			
		case EV_T_HOT:
			asprintf( &event_msg, "The sensor node (sensor_id = %d) reports it’s too hot", sensor_id);
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
			
		case EV_T_COLD:
			asprintf( &event_msg, "The sensor node (sensor_id = %d) reports it’s too cold", sensor_id);
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
		
		case EV_NON_ID_MAPPED:
			asprintf( &event_msg, "The sensor with id = %d  is not available in sensor mapping file", sensor_id);
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
		
		case EV_Q_FULL:
			asprintf( &event_msg, "Circular queue is full");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
		
		case EV_Q_FULL_OVR:
			asprintf( &event_msg, "Circular queue is full: started to overwrite data");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
			
		case EV_MYSQL_CONN:
			asprintf( &event_msg, "Connection to MySQL server established");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
			
		case EV_MYSQL_LOST:
			asprintf( &event_msg, "Connection to MySQL server lost");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
			
		case EV_MYSQL_UNCONN:
			asprintf( &event_msg, "Unable to connect to SQL server");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}			
			break;
		
		case EV_SQLITE_CONN:
			asprintf( &event_msg, "Connection to SQLite database established");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}
			break;
		
		case EV_SQLITE_UNCONN:
			asprintf( &event_msg, "Unable to connect to SQLite database");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}			
			break;

		case EV_SQL_RECONNECT:
			asprintf( &event_msg, "Reconnect to SQL server");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}			
			break;
#ifdef DEBUG
		case EV_THE_CONNECTION_EXIT:
			asprintf( &event_msg, "The Connection thread exit");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}			
			break;
			
		case EV_THE_DATA_EXIT:
			asprintf( &event_msg, "The Data thread exit");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}			
			break;
		
		case EV_THE_STORAGE_EXIT:
			asprintf( &event_msg, "The Storage thread exit");
			/* Write event msg to FIFO */
			if ( write( fifo_fd, event_msg, strlen(event_msg)+1 ) < 0 )	{
				fprintf( stderr, "Event_id %d: Error writing msg to fifo\n", event_id);				
			}			
			break;	
#endif			
	}	
	presult = pthread_mutex_unlock( &fifo_mutex );
	pthread_err_handler( presult, "pthread_mutex_unlock", __FILE__, __LINE__ );	
	free(event_msg);
	event_msg = NULL;
 }
 
/*------------------------------------------------------------------------------
		Implemented functions for queue.h 			       
------------------------------------------------------------------------------*/
/*
 * Copy the 'content' of src_element to dst_element (deep copy)
 */
void queue_element_copy(element_t *dest_element, element_t src_element)
{   
	sensor_data_t *p;
	p = (sensor_data_t *) malloc(sizeof(sensor_data_t));	
	assert(p != NULL);	
	p->id = ((sensor_data_t *)src_element)->id;
	p->value = ((sensor_data_t *)src_element)->value;
	p->ts = ((sensor_data_t *)src_element)->ts;
    *dest_element = p; 
}

/*
 * Clean up element, including freeing memory if needed
 */
void queue_element_free(element_t *element)
{
  // implementation goes here
  if(*element != NULL) {	
	   free(*element);
	   *element = NULL;
  }  	
}

/*
 * Print 1 element to stdout. 
 */
void queue_element_print(element_t element)
{   
	if(element != NULL) {
		printf("sensor_ID = %d - value = %g - timestamp = %ld\n", (int)((sensor_data_t *)element)->id, (double)((sensor_data_t *)element)->value, (long)((sensor_data_t *)element)->ts);	
	}	
}

/*
 * Compare two element elements; returns 0(equal) or 1(unequal)  or -1 (one of elements is NULL)
 */
int queue_element_compare(element_t x, element_t y)
{
	if(x == NULL || y == NULL) return -1;
	
	if((((sensor_data_t *)x)->id == ((sensor_data_t *)y)->id) && (((sensor_data_t *)x)->value == ((sensor_data_t *)y)->id) && (((sensor_data_t *)x)->ts == ((sensor_data_t *)y)->ts)) return 0;
	else return 1;  
}

/*------------------------------------------------------------------------------
		Implemented functions for list.h 			       
------------------------------------------------------------------------------*/
/*
 * Print 1 element to stdout. 
 */
void list_element_print(element_ptr_t element){
	printf("Socket descriptor: %d\n", get_socket_descriptor(*(Socket *)element));
}

/*
 * Clean up element, including freeing memory if needed
 */
void list_element_free(element_ptr_t *element){
	tcp_close(*element);
	free(*element);	
}

/*
 * Compare two element elements; returns -1, 0 or 1 
 */
int list_element_compare(element_ptr_t x, element_ptr_t y)
{
//Not use
  return 0;
}

/*
 * Copy the 'content' of src_element to dst_element (deep copy)
 */
void list_element_copy(element_ptr_t *dest_element, element_ptr_t src_element)
{
	list_element_t *p;
	p = (list_element_t *) malloc(sizeof(list_element_t));	
	assert(p != NULL);	
	*p = *(list_element_t *)src_element;
    *dest_element = (element_ptr_t) p; 	
}

/*
 * Check timeout condition to stop running thread
 * Threads will stop when: queue is empty/full and there is no data put/get in TIMEOUT seconds
 */
 void check_timeout_running(time_t last_timestamp) {
	return;
 }