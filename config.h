#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <time.h>


/*------------------------------------------------------------------------------
		definitions (defines, typedefs, ...)
------------------------------------------------------------------------------*/
#ifdef DEBUG
	#define DB_SERVER_NAME "localhost"
	#define USER_NAME "root"
	#define PASSWORD "7777"
	#define DB_NAME "mylocaldb"
	#define TABLE_NAME "PhamHoangChi"
#else
	#define DB_SERVER_NAME "studev.groept.be"
	#define USER_NAME "a13_syssoft"
	#define PASSWORD "a13_syssoft"
	#define DB_NAME "a13_syssoft"
	#define TABLE_NAME "PhamHoangChi"
#endif

#define QUEUE_SIZE 5
#define NUM_OF_WRITER 1
#define NUM_OF_READER 2
#define TIMEOUT 10 // timeout (sec)
#define GW_TIMEOUT 10 // gateway timeout (sec)
#define SS_TIMEOUT 3 // sensor node timeout (sec)
#define MAX 50
#define ARG1_DEFAULT_VAL 1
#define DEFAULT_DB_TYPE "MYSQL"
#define ALTERN_DB_TYPE "SQLITE"
#define DATA_FILE 	"sensor_data"
#define DATA_FILE_TEXT "sensor_data_text.txt"
#define LOG_FILE 	"gateway.log"
#define MAP_FILE 	"room_sensor.map"
#define FIFO_NAME 	"logFifo"
#define INCREASE_MAX 2
#define DATA_READ_DELAY 1 //second
#define CONN_LOST_DELAY 3 //second
#define STOP_COMM "stop"
#define CLEAR_UP_FLAG 1
#define ROOM_MIN_TEMP 17 // unit [degrees Celsius]
#define ROOM_MAX_TEMP 24 // unit [degrees Celsius]

enum event_id { EV_STOP = 0, EV_START, EV_NEW_CONN, EV_CLOSE_CONN, EV_T_HOT, EV_T_COLD, EV_NON_ID_MAPPED, EV_Q_FULL, EV_Q_FULL_OVR, EV_MYSQL_CONN, EV_MYSQL_LOST, EV_MYSQL_UNCONN, EV_SQLITE_CONN, EV_SQLITE_UNCONN, EV_SQL_RECONNECT, EV_THE_CONNECTION_EXIT, EV_THE_DATA_EXIT, EV_THE_STORAGE_EXIT };

typedef uint16_t sensor_id_t;
typedef double sensor_value_t;    
typedef time_t sensor_ts_t;         // UTC timestamp as returned by time()

typedef struct{
	sensor_id_t id;
	sensor_value_t value;
	sensor_ts_t ts;
}sensor_data_t, * sensor_data_ptr_t;

typedef void * list_element_t;			

typedef struct{
	int gw_flag;
	int gw_port;
	char *gw_db_type;
	char *gw_db_name;
	int gw_high_temp;
	int gw_low_temp;
	int gw_ss_num;
	int gw_packet_num;
} gateway_conf_t;
#endif /* _CONFIG_H_ */

