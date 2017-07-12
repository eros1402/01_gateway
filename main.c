/*******************************************************************************
* FILENAME: main.c							       
*
* Version V1.2		
* Author: Pham Hoang Chi
*
* An implementation of a Sensor Gateway - Final assignment - System Software
* 
*******************************************************************************/


// export LD_LIBRARY_PATH=/home/chi/Desktop:$LD_LIBRARY_PATH
// #define _GNU_SOURCE
#include <pthread.h>   //thread
#include <sys/types.h> //FIFO
#include <sys/stat.h> //FIFO
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

#include "tcpsocket.h"
#include "sensor_db.h"
#include "sensor_sqlite3db.h"
#include "myqueue.h"
#include "list.h"
#include "datamgr.h"

#include "mongoose.h"
#include "config.h"
#include "my_macros.h"
#include "err_handler.h"
#include "mylocalfunc.h"

/*------------------------------------------------------------------------------
		Global variables			       
------------------------------------------------------------------------------*/
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER; // mutex
pthread_mutex_t fifo_mutex = PTHREAD_MUTEX_INITIALIZER; // mutex
// static global variable: variable is protected inside the file, other files could not access this variable
static queue_t* queue; 
static int gw_alive = 0;
static int read_busy = 0;
int list_errno = 0;
list_ptr_t socket_list_p = NULL;
static int fifo_fd; 
int allowed_overide_flag = 0;

static int web_alive = 1;
gateway_conf_t *gw_input;
FILE *fp_log;
 
static int ev_handler(struct mg_connection *conn, enum mg_event ev) {
	if (ev == MG_REQUEST) {
		return send_request_run(conn, gw_input, fp_log);		
	} else if (ev == MG_AUTH) {
		return MG_TRUE;
	} else {
		return MG_FALSE;
	}
}

/*------------------------------------------------------------------------------
		Multiple Threads			       
------------------------------------------------------------------------------*/
/* Thread writer functions : the_connection*/
void *the_connection(void *arg) {	
	int max_ss_node = MAX;	
	int init_flag = 1;
	struct pollfd *poll_fd;
	int bytes;
	
	sensor_data_t *data_packet;	
	Socket server, client;	
	int port_num = *(int *)arg; // Get port number from command-line
	list_element_t *socket_p = NULL;	
	
	int i, size, poll_result;
	double dif_time_sec;
	time_t cur_timestamp;
	time_t *last_timestamp;	
	
	int new_ss_node = 0;
	
	#ifdef DEBUG
		FILE *fp_text; // for debug
	#endif
	
	/* allocate memory */
	poll_fd = malloc(max_ss_node*sizeof(struct pollfd));
	malloc_check(poll_fd, "malloc() to poll_fd");
	last_timestamp = malloc(max_ss_node*sizeof(time_t));
	malloc_check(last_timestamp, "malloc() to last_timestamp");	
	data_packet = malloc(sizeof(sensor_data_t));
	malloc_check(data_packet, "malloc() to data_packet");	
	
	/* Create list of client */
	socket_list_p = mylist_create( &list_element_copy, &list_element_free, &list_element_compare, &list_element_print);
	list_err_handler( list_errno, "mylist_create()" );
	
	/* open server socket */
	server = tcp_passive_open( port_num );
	gw_alive++;
	//insert server socket to the list
	socket_list_p = mylist_insert_at_index(socket_list_p, &server, mylist_size(socket_list_p)); //remark: server is cloned to new socket and added into the list 
	list_err_handler( list_errno, "mylist_insert_at_index()" );
	printf("Server socket is created - ");
	mylist_print(socket_list_p);	
	
	/* Generate a log event for starting gateway */
	generate_log_event(fifo_fd, EV_START, 0, 0);
	
	#ifdef DEBUG
		/* For debug: Open the sensor_data_text file */
		fp_text = fopen(DATA_FILE_TEXT, "w");
		FILE_ERROR(fp_text,"Couldn't create sensor_data_text file");
	#endif
	/* Get current timestamp as a start time for server listening */
	last_timestamp[0]=time(NULL);
	
	do {		
		//update the size of socket list
		size = mylist_size(socket_list_p);
		gw_input->gw_ss_num = size - 1;
		// Dynamic memory: check if the list size reaches max_ss_node -> realloc memory		
		dynamic_sensor_list_size (size, &max_ss_node, poll_fd, last_timestamp);
		
		// put all read fds in poll-array and set poll event type
		for( i = 0; i < size; i++) {
			socket_p = (list_element_t *)mylist_get_element_at_index(socket_list_p, i);
			poll_fd[i].fd = get_socket_descriptor(*socket_p);
			poll_fd[i].events = POLLIN;
		}
		
		if(init_flag) poll_result = poll(poll_fd, size, -1); // block until there is at least one sensor node connected
		else poll_result = poll(poll_fd, size, 1000); // block in max 1s 
		if(poll_result == -1) { fprintf(stderr, "\npoll() error\n");}
		
		//Loop for polling sockets
		for( i = 0; i < size; i++) {
			if( i == 0) { // position of server socket => listen to new socket
				// listen to new connection						
				if(poll_fd[i].revents & POLLIN) {
					init_flag = 0;
					socket_p = (list_element_t *)mylist_get_element_at_index(socket_list_p, i);
					client = tcp_wait_for_connection( (Socket)(*socket_p) );
					printf("\nThe_connection thread: Connected to new client (Socket descriptor = %d)\n", get_socket_descriptor(client));
					// add new client to the socket list
					socket_list_p = mylist_insert_at_index(socket_list_p, &client, mylist_size(socket_list_p));					
					gw_alive++;
					last_timestamp[0]=time(NULL);
					/* Location flag for a new sensor node*/				
					new_ss_node = mylist_size(socket_list_p) - 1;						
				}
			}
			else {				
				/* Check data received on each socket */								
				//If the data are availble for receiving
				if(poll_fd[i].revents & POLLIN) {
					// Get the socket from the list position					
					socket_p = (list_element_t *)mylist_get_element_at_index(socket_list_p, i);
					bytes = receive_sensor_data((Socket)(*socket_p), data_packet);	
					// bytes == 0 indicates tcp connection teardown					
					if (bytes) 
					{
						if( i == new_ss_node) {
							generate_log_event(fifo_fd, EV_NEW_CONN, get_socket_descriptor(*socket_p), data_packet->id);
							set_sensor_id(*socket_p, data_packet->id);
							new_ss_node = 0;							
						}
						printf("\nThe_connection received: sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", data_packet->id, data_packet->value, (long int)(data_packet->ts) );						
					    gw_input->gw_packet_num++;
						// Check if the queue is FULL
						if(queue_size(queue) == QUEUE_SIZE) {
							/* Generate a log event: Circular queue is full*/
							printf("\nThe_connection event: Circular queue is full\n");
							if(allowed_overide_flag) generate_log_event(fifo_fd, EV_Q_FULL_OVR, 0, 0);
							else generate_log_event(fifo_fd, EV_Q_FULL, 0, 0);	
						}
						// Insert data in the queue 
						queue_enqueue(queue, (sensor_data_t *)data_packet);	
						
						#ifdef DEBUG
							//write sensor data to the text file (for debug)
							fprintf(fp_text,"%" PRIu16 " %g %ld\n", data_packet->id, data_packet->value, (long int)(data_packet->ts));	
						#endif
						//save the last timestamp of this sensor node
						last_timestamp[i] = data_packet->ts;
						last_timestamp[0] = data_packet->ts;
					}						
				}	
			}			
		}			
		
		/* get current timestamp */
		cur_timestamp = time(NULL);
		
		/* Check timeout for each socket (include server socket)*/
		for(i = size-1; i >= 0; i--) {
			dif_time_sec = difftime(cur_timestamp, last_timestamp[i]);
			
			if(dif_time_sec > (i?SS_TIMEOUT:GW_TIMEOUT)) {	
				printf("Socket descriptor = %d is timeout\n", poll_fd[i].fd);
				socket_p = (list_element_t *)mylist_get_element_at_index(socket_list_p, i);					
				
				/* Generate a log event: The sensor node with <sensorNodeID> has closed the connection */				
				if(i == 0) {
					printf("\nThe_connection: All connections are time out - Gateway is closing !!!\n");					
				} 
				else {
					printf("\nThe_connection event: A sensor node with id=%d has closed the connection\n", get_sensor_id(*socket_p));										
					generate_log_event(fifo_fd, EV_CLOSE_CONN, get_socket_descriptor(*socket_p), get_sensor_id(*socket_p));
					last_timestamp[0]=time(NULL);
				}
				
				poll_fd[i].fd = -1; 
				poll_fd[i].events = 0;								
				socket_list_p = mylist_free_at_index(socket_list_p, i);
				gw_alive--;
			}				
		}		
	}while(gw_alive);
	
	#ifdef DEBUG
	/* Close files (for debug)*/		
		fclose(fp_text);
	#endif
	/* free memory allocated */	
	FREE(data_packet);	
	FREE(poll_fd);	
	FREE(last_timestamp);		
	mylist_free(&socket_list_p);

	/* exit thread */
	sleep(1);
#ifdef DEBUG
	printf("\nThe_connection event: Thread exit\n");
	// generate_log_event(fifo_fd, EV_THE_CONNECTION_EXIT, 0, 0);
#endif
	sleep(2);
	generate_log_event(fifo_fd, EV_STOP, 0, 0);	
	sleep(1);
	web_alive = 0;
	pthread_exit( NULL );
}

/* Thread reader functions : the_data*/
void *the_data(void *arg) {	
	int presult;
	list_ptr_t ss_node_list = NULL;
	sensor_data_t **data_packet_read = NULL;	
	sensor_data_t *last_data = NULL;
	int temp_check = 0;
	int sensorNodeID;
	
	printf("\nThread the_data is running...\n");
	ss_node_list = ss_node_list_create(MAP_FILE);  
	
	do {		
			/* Read data out from queue */
			data_packet_read = (sensor_data_t **)queue_top(queue);		
		
			if(data_packet_read == NULL) {
				printf("\nThe_data read: Queue is empty\n");
			}
			else {
				printf("\nThe_data read: ");	
				queue_element_print(*data_packet_read); //for debug
				
				// Check if last_data read is unequal with current data read
				if(queue_element_compare((sensor_data_t *)last_data, (sensor_data_t *)*data_packet_read)) {
					if(last_data != NULL) FREE(last_data);
					queue_element_copy( (void **)(&last_data), (sensor_data_t *)*data_packet_read);
					
					/* Write data read to sensor node list */
					sensorNodeID = ss_node_update_from_packet(ss_node_list, last_data);
					
					if(sensorNodeID >= 0) {
						/* Check temperature at this sensor id */
						temp_check = ss_node_check_temp_avg(ss_node_list, (sensor_id_t)last_data->id);					
						if(temp_check == 1) {
							/* Generate a log event: The sensor node with <sensorNodeID> reports it’s too hot */
							printf("\nThe_data event: The sensor node with id = %d reports it’s too hot\n", last_data->id);
							generate_log_event(fifo_fd, EV_T_HOT, 0, last_data->id);
						} else {
							if(temp_check == -1) {
								/* Generate a log event: The sensor node with <sensorNodeID> reports it’s too cold */
								printf("\nThe_data event: The sensor node with id = %d reports it’s too cold\n", last_data->id);
								generate_log_event(fifo_fd, EV_T_COLD, 0, last_data->id);
							}
						}
					}
					else {
						/* Generate a log event: The sensor with id is not available in sensor mapping file */
						fprintf(stderr, "\nThe_data thread: The sensor with id=%d  is not available in sensor mapping file\n", last_data->id);
						generate_log_event(fifo_fd, EV_NON_ID_MAPPED, 0, last_data->id);
					}											
						
					//update shared flag
					presult = pthread_mutex_lock( &data_mutex );	
					pthread_err_handler( presult, "pthread_mutex_lock", __FILE__, __LINE__ );
					read_busy--; 
					if(read_busy <= 0) {						
						queue_dequeue(queue);
						read_busy = NUM_OF_READER;
					}
					presult = pthread_mutex_unlock( &data_mutex );
					pthread_err_handler( presult, "pthread_mutex_unlock", __FILE__, __LINE__ );
				}				
			}
		
		sleep(DATA_READ_DELAY);
	} while (gw_alive);
	
	FREE(last_data);	
	ss_node_list_free(ss_node_list);
#ifdef DEBUG
	printf("\nThe_data event: Thread exit\n");
	// generate_log_event(fifo_fd, EV_THE_DATA_EXIT, 0, 0);
#endif	
	pthread_exit( NULL );
}

/* Thread reader functions : the_storage*/
void *the_storage(void *arg) {
	int presult;	
	int conn_flag = 0;
	int conn_cnt = 0;	
	char* db_type; 
	db_type = (char *)arg; // Get SQL database type from command-line
	sensor_data_t **data_packet_read = NULL;	
	sensor_data_t *last_data = NULL;
	int default_sql_db_flag = 1;
	void *conn;
	
	printf("\nThread the_storage is running...\n");
	if( strncmp( db_type, DEFAULT_DB_TYPE, strlen(DEFAULT_DB_TYPE)) == 0) {
		conn = (MYSQL *)init_connection(CLEAR_UP_FLAG);
		if(conn != NULL) {
			printf("\nThe_storage event: Connection to MySQL server established\n");			
			generate_log_event(fifo_fd, EV_MYSQL_CONN, 0, 0);
		}
		else {
			printf("\nThe_storage event: Unable to connect to MySQL server\n");			
			generate_log_event(fifo_fd, EV_MYSQL_UNCONN, 0, 0);			
			db_type = ALTERN_DB_TYPE;
			default_sql_db_flag = 0;
		}
	}

	if( strncmp( db_type, ALTERN_DB_TYPE, strlen(ALTERN_DB_TYPE)) == 0) {
		conn = (sqlite3 *) sqlite_init_connection(CLEAR_UP_FLAG);
		default_sql_db_flag = 0;
		if(conn != NULL) {
			printf("\nThe_storage event: Connection to SQLite server established\n");			
			generate_log_event(fifo_fd, EV_SQLITE_CONN, 0, 0);			
		}
		else {
			printf("\nThe_storage event: Unable to connect to SQLite server\n");			
			generate_log_event(fifo_fd, EV_SQLITE_UNCONN, 0, 0);			
		}
	}	
	
	do {		
			/* Read data out from queue */
			data_packet_read = (sensor_data_t **)queue_top(queue);		
		
			if(data_packet_read == NULL) {
				printf("\nThe_storage read: Queue is empty\n");
			}
			else {					
				printf("\nThe_storage read: ");	
				queue_element_print(*data_packet_read); //for debug		
				
				// Check if last_data read is unequal with current data read
				if(queue_element_compare((sensor_data_t *)last_data, (sensor_data_t *)*data_packet_read)) {					
					if(last_data != NULL) FREE(last_data);
					queue_element_copy( (void **)(&last_data), (sensor_data_t *)*data_packet_read);
						
						/* Write data read to SQL database */
					if(conn_cnt == 0) {
						if(default_sql_db_flag) conn_flag = insert_sensor(conn, last_data->id, last_data->value, last_data->ts);	
						else conn_flag = sqlite_insert_sensor(conn, last_data->id, last_data->value, last_data->ts);	
					}						
					if(conn_flag == 0) {
						//update shared flag
						presult = pthread_mutex_lock( &data_mutex );	
						pthread_err_handler( presult, "pthread_mutex_lock", __FILE__, __LINE__ );	
							read_busy--; 
							if(read_busy <= 0) {							
								queue_dequeue(queue);
								read_busy = NUM_OF_READER;
							}
						presult = pthread_mutex_unlock( &data_mutex );
						pthread_err_handler( presult, "pthread_mutex_unlock", __FILE__, __LINE__ );
						conn_cnt = 0;
					}
					else {
						if(conn_cnt == 0) {
							printf("\nThe_storage run: Connection to SQL server lost\n");
							/* Generate a log event: Connection to SQL server lost */
							generate_log_event(fifo_fd, EV_MYSQL_LOST, 0, 0);							
						}
						FREE(last_data);
							// wait a bit before trying reconnection
						if(conn_cnt < CONN_LOST_DELAY) {
							conn_cnt++;
						}
						else {
							printf("\nThe_storage run: Try to reconnect to SQL server\n");													
							if(default_sql_db_flag) conn_flag = reconnect(conn);
							else conn_flag = sqlite_reconnect(conn);
							if(conn_flag < 0) {
								printf("\nThe_storage run: Fail to reconnect SQL server\n");
								allowed_overide_flag = 1;
							}
							else {
								printf("\nThe_storage run: Reconnected SQL server\n");
								generate_log_event(fifo_fd, EV_SQL_RECONNECT, 0, 0);	
								conn_cnt = 0;
								allowed_overide_flag = 0;
							}
						}
					}	
				}												
			}		
		sleep(DATA_READ_DELAY);
	} while(gw_alive);
	
	if( default_sql_db_flag ) disconnect(conn);
	else sqlite_disconnect(conn);
	FREE(last_data);	
#ifdef DEBUG
	printf("\nThe_storage event: Thread exit\n");
	// generate_log_event(fifo_fd, EV_THE_STORAGE_EXIT, 0, 0);
#endif
	pthread_exit( NULL );
}

/* Thread web server functions : the_websv*/
void *the_websv(void *arg) {
	struct mg_server *server = mg_create_server(NULL, ev_handler);	
		
	printf("\nThread the_websv is running...\n");
	mg_set_option(server, "listening_port", "8080");
	printf("Use Web browser to config the gateway server at URL: 127.0.0.1:8080\n");
	printf("Web server is starting on port: %s\n", mg_get_option(server, "listening_port"));	
	
	fp_log = fopen(LOG_FILE, "r");
	FILE_ERROR(fp_log,"Couldn't open log file");
	while(web_alive){
		mg_poll_server(server, 1000);			
	}	
	sleep(2);
	fclose(fp_log);	
	mg_destroy_server(&server);
	pthread_exit( NULL );
}
/*------------------------------------------------------------------------------
		LOG PROCESS			       
------------------------------------------------------------------------------*/
void run_log ( int exit_code ) {
	pid_t my_pid; 
	my_pid = getpid();
	time_t curtime;
	char *lctime;
	
	int presult; 
	int read_bytes;	
	char recv_buf[256];	
	
	FILE *fp_text;
	int log_sq_num = 0;
	
	printf("\nStarting LOG process (pid = %d) \n", my_pid);
	
	/* Open FIFO */
	fifo_fd = open(FIFO_NAME, O_RDONLY ); 	
	SYSCALL_ERROR(fifo_fd, "Open FIFO");
	sleep(1);
	
	/* Open log file */
	fp_text = fopen(LOG_FILE, "w");
	FILE_ERROR(fp_text,"Couldn't open log file");
	fprintf(fp_text,"Seq_No | Time_stamp               | Log_Event\n");
	fprintf(fp_text,"---------------------------------------------------\n");	
	do {		
		/* Read FIFO */		
		presult = pthread_mutex_lock( &fifo_mutex );	
		pthread_err_handler( presult, "pthread_mutex_lock", __FILE__, __LINE__ );
			read_bytes = read(fifo_fd, recv_buf, sizeof(recv_buf));
			SYSCALL_ERROR(read_bytes, "Read FIFO");	
		presult = pthread_mutex_unlock( &fifo_mutex );
		pthread_err_handler( presult, "pthread_mutex_unlock", __FILE__, __LINE__ );	
		
		if(read_bytes > 0) {
			printf( "\nLOG process received %d bytes: %s \n", read_bytes, recv_buf );
			log_sq_num++;
			
			/* Write log event to log file */
			curtime = time(NULL);
			lctime = ctime(&curtime);			
			printf( "\nLOG process writes to log: %d | %.*s | %s\n", log_sq_num, strlen(lctime)-1 ,lctime, recv_buf );			
			fseek(fp_text, 0, SEEK_END);
			fprintf(fp_text,"%6d | %.*s | %s\n", log_sq_num, strlen(lctime)-1, lctime, recv_buf);	
		}		
	} while ((strncmp( recv_buf, STOP_COMM, strlen(STOP_COMM) ) != 0) && (read_bytes >= 0));
	sleep(1);
	read_bytes = close( fifo_fd );
	SYSCALL_ERROR(read_bytes, "Close FIFO");	
	fclose(fp_text); 
	printf("\nLOG process (pid = %d) is terminating ...\n", my_pid);
	
	/* Destroy mutex */
	presult = pthread_mutex_destroy( &data_mutex );
	pthread_err_handler( presult, "pthread_mutex_destroy", __FILE__, __LINE__ );	
	presult = pthread_mutex_destroy( &fifo_mutex );
	pthread_err_handler( presult, "pthread_mutex_destroy", __FILE__, __LINE__ );	
	exit( exit_code );
}

/*------------------------------------------------------------------------------
		MAIN PROCESS			       
------------------------------------------------------------------------------*/
int main( int argc, char *argv[] )
{	
	pid_t my_pid, child_pid; 	
	int result;
	
	my_pid = getpid();
	printf("Gateway process (pid = %d) is started ...\n", my_pid);
	
	/* Create the FIFO if it does not exist */ 
	result = mkfifo(FIFO_NAME, 0666);
	CHECK_MKFIFO(result);
	
	/* Create a new process by fork */ 
	child_pid = fork();	
	SYSCALL_ERROR(child_pid, "fork");
	
	int presult;	
	pthread_t thread_writer, thread_reader_1, thread_reader_2, thread_websv;		
	
	if ( child_pid == 0  ) {  
		run_log( 0 );
	}
	else {		
		/* Open FIFO */
		fifo_fd = open(FIFO_NAME, O_WRONLY );		
		SYSCALL_ERROR(fifo_fd, "Open FIFO");	
		
		read_busy = NUM_OF_READER; // Shared flag for reader threads (if read_busy > 0 -> the value is still not read by another thread)			
		
		gw_input = malloc(sizeof(gateway_conf_t));
		malloc_check(gw_input, "malloc() to gw_input");
		gw_input->gw_high_temp = ROOM_MAX_TEMP;
		gw_input->gw_low_temp = ROOM_MIN_TEMP;
		gw_input->gw_ss_num = 0;
		gw_input->gw_packet_num = 0;
		/* Check input arguments */	
		gw_input->gw_flag = check_command_line_argument(argc, argv, gw_input);
		
		/* Create thread web server */	
		presult = pthread_create( &thread_websv, NULL, &the_websv, NULL);
		pthread_err_handler( presult, "pthread_create", __FILE__, __LINE__ );
		
		while(gw_input->gw_flag != 1);//wait for gateway configuration by user (command line or Web Browser)
		printf("\nGW configuration: FLAG=%d, PORT=%d - DATABASE=%s - DB_NAME=%s\n",gw_input->gw_flag, gw_input->gw_port, gw_input->gw_db_type, gw_input->gw_db_name);
		/* Create the queue */
		queue = queue_create(&queue_element_copy, &queue_element_free, &queue_element_print); 
		queue_err_handler(queue, "queue_create() error: queue is NULL");
			
		/* Create thread connection */		
		presult = pthread_create( &thread_writer, NULL, &the_connection, &(gw_input->gw_port));
		pthread_err_handler( presult, "pthread_create", __FILE__, __LINE__ );
		
		/* Delay to let the server socket is opened */
		sleep(2);
		
		/* Create thread reader: the data & the storage */		
		presult = pthread_create( &thread_reader_1, NULL, &the_data, NULL);
		pthread_err_handler( presult, "pthread_create", __FILE__, __LINE__ );
		presult = pthread_create( &thread_reader_2, NULL, &the_storage, gw_input->gw_db_type);
		pthread_err_handler( presult, "pthread_create", __FILE__, __LINE__ );		
		
		/* Wait to close threads */	
		presult = pthread_join( thread_writer, NULL);
		pthread_err_handler( presult, "pthread_join thread_writer", __FILE__, __LINE__ );	
		presult = pthread_join( thread_reader_1, NULL);
		pthread_err_handler( presult, "pthread_join thread_reader_1", __FILE__, __LINE__ );
		presult = pthread_join( thread_reader_2, NULL);
		pthread_err_handler( presult, "pthread_join thread_reader_2", __FILE__, __LINE__ );	
		presult = pthread_join( thread_websv, NULL);
		pthread_err_handler( presult, "pthread_join thread_websv", __FILE__, __LINE__ );	
		
		/* Free queue */
		queue_free(&queue); 
		
		/* wait on termination of log process */
		waitpid(child_pid, NULL, 0);
		SYSCALL_ERROR(close( fifo_fd ), "Close FIFO");
		printf("Main process (pid = %d) is terminating ...\n", my_pid);		
	}
	FREE(gw_input->gw_db_type);
	FREE(gw_input->gw_db_name);
    FREE(gw_input);
	return 0;
}
