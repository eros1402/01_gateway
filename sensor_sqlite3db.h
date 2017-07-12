#ifndef _SENSOR_SQLITE3DB_H_
#define _SENSOR_SQLITE3DB_H_

#include <sqlite3.h>
#include "config.h"

#ifndef SQLITE3_DB_FILE
	#define SQLITE3_DB_FILE "mysqlitedb.db"
#endif

#ifndef TABLE_NAME
	#define TABLE_NAME "PhamHoangChi"
#endif


/*
 * Make a connection to SQLite database
 * Create a table named 'yourname' if the table does not exist
 * If the table existed, clear up the existing data if clear_up_flag is set to 1
 * return the connection for success, NULL if an error occurs
 */
sqlite3 *sqlite_init_connection(int clear_up_flag);

/*
 * Write an INSERT query to insert a single sensor measurement
 * return zero for success, and non-zero if an error occurs
 */
int sqlite_insert_sensor(sqlite3 *db, sensor_id_t id, sensor_value_t value, sensor_ts_t ts);

/*
 * Disconnect SQLite database
 */
void sqlite_disconnect(sqlite3 *db);

/*
 * Reconnect to MySQL database
 */			
int sqlite_reconnect(sqlite3 *db);
			
#endif /* _SENSOR_SQLITE3DB_H_ */