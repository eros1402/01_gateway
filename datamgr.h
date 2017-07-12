#ifndef DATAMGR_H_
#define DATAMGR_H_

#include <inttypes.h>
#include "list.h"
#include "config.h"

/*
 * definition of preprocessor directives
 * */
#ifndef SET_MIN_TEMP
	#define SET_MIN_TEMP 17 // unit [degrees Celsius]
#endif	
#ifndef SET_MAX_TEMP	
	#define SET_MAX_TEMP 24 // unit [degrees Celsius]
#endif
#define NUM_AVG		5
#define MAX_SENSOR	100

// typedef uint16_t sensor_id_t;
// typedef double sensor_value_t;    
// typedef time_t sensor_ts_t;         // UTC timestamp as returned by time()

// typedef struct {
	// sensor_id_t id;
	// sensor_value_t value;
	// sensor_ts_t ts;
// } sensor_data_t;


/*-- error codes --*/
enum err_code { ERR_NONE = 0, ERR_EMPTY, ERR_FULL, ERR_MEM, ERR_INIT, ERR_FILE, ERR_UNDEFINED };
typedef enum err_code err_code_t;

list_ptr_t ss_node_list_create(const char *mapping_path);

void ss_node_list_print(list_ptr_t ss_list_p);

void ss_node_list_free(list_ptr_t ss_list_p);

void ss_node_update_from_file(list_ptr_t ss_list_p, const char *data_path);

int ss_node_update_from_packet(list_ptr_t ss_list_p, sensor_data_t *packet);

int ss_node_check_temp_avg (list_ptr_t ss_list_p, sensor_id_t id);

#endif  //DATAMGR_H_
