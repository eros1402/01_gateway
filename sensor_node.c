#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "tcpsocket.h"

#define LOOPS 25
// conditional compilation option to control the number of measurements this sensor node wil generate
#if (LOOPS > 1)
  #define UPDATE(i) (i--)
#else
  #define LOOPS 1 
  #define UPDATE(i) (void)0 //create infinit loop
#endif

// conditional compilation option to log all sensor data to a text file
#ifdef LOG_SENSOR_DATA

  #define LOG_FILE	"sensor_log"

  #define LOG_OPEN()						\
    FILE *fp_log; 						\
    do { 							\
      fp_log = fopen(LOG_FILE, "w");				\
      if ((fp_log)==NULL) { 					\
	printf("%s\n","couldn't create log file"); 		\
	exit(EXIT_FAILURE); 					\
      }								\
    } while(0)

  #define LOG_PRINTF(sensor_id,temperature,timestamp)							\
      do { 												\
	fprintf(fp_log, "%" PRIu16 " %g %ld\n", (sensor_id), (temperature), (long int)(timestamp));	\
	fflush(fp_log);											\
      } while(0)

  #define LOG_CLOSE()	fclose(fp_log);	

#else
  #define LOG_OPEN(...) (void)0
  #define LOG_PRINTF(...) (void)0
  #define LOG_CLOSE(...) (void)0
#endif

#define INITIAL_TEMPERATURE 	20
#define TEMP_DEV 		5	// max afwijking vorige temperatuur in 0.1 celsius


void print_help(void);

/*
 * argv[1] = sensor ID
 * argv[2] = sleep time
 * argv[3] = server IP
 * argv[4] = server port
 */

int main( int argc, char *argv[] )
{
  double temperature = INITIAL_TEMPERATURE;
  uint16_t sensor_id;
  time_t timestamp;
  long int sleep_time;
  int server_port;
  char server_ip[] = "000.000.000.000"; 
  Socket client;
  int i;
  
  LOG_OPEN();
  
  if (argc != 5)
  {
    print_help();
    exit(EXIT_SUCCESS);
  }
  else
  {
    // to do: user input validation!
    sensor_id = atoi(argv[1]);
    sleep_time = atoi(argv[2]);
    strncpy(server_ip, argv[3],strlen(server_ip));
    server_port = atoi(argv[4]);
    //printf("%d %ld %s %d\n", sensor_id, sleep_time, server_ip, server_port);
  }
  
  srand48( time(NULL) );
  
  // open TCP connection to the server; server is listening to SERVER_IP and PORT
  client = tcp_active_open( server_port, server_ip ); 
  
  i=LOOPS;
  while(i) 
  {
    temperature = temperature + TEMP_DEV * ((drand48() - 0.5)/10); 
    time(&timestamp);
    // send data to server in this order (!!): <sensor_id><temperature><timestamp>
    // remark: don't send as a struct!
    tcp_send( client, (void *)&sensor_id, sizeof(sensor_id));
    tcp_send( client, (void *)&temperature, sizeof(temperature));
    tcp_send( client, (void *)&timestamp, sizeof(timestamp));
    LOG_PRINTF(sensor_id,temperature,timestamp);
    sleep(sleep_time);
    UPDATE(i);
  }
  
  tcp_close( &client );
  
  LOG_CLOSE();
  
  exit(EXIT_SUCCESS);
}
  
  
void print_help(void)
{
  printf("Use this program with 4 command line options: \n");
  printf("\t%-15s : a unique sensor node ID\n", "\'ID\'");
  printf("\t%-15s : node sleep time (in sec) between two measurements\n","\'sleep time\'");
  printf("\t%-15s : TCP server IP address\n", "\'server IP\'");
  printf("\t%-15s : TCP server port number\n", "\'server port\'");
}
