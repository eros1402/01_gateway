/*******************************************************************************
* FILENAME: datamgr.c							       
*
* Version V1.1		
* Author: Pham Hoang Chi
*
* An implementation of Lab 6 assignment - System Software Course
* 
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include "datamgr.h"

#define FILE_ERROR(fp,error_msg) 	do { \
					  if ((fp)==NULL) { \
					    printf("%s\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)
						
#define LIST_ERROR(erro_no,error_msg) 	do { \
					  if ((erro_no)==LIST_MEMORY_ERROR) { \
					    printf("%s : List memory error\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					  if ((erro_no)==LIST_EMPTY_ERROR) { \
					    printf("%s : List empty error\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					  if ((erro_no)==LIST_INVALID_ERROR) { \
					    printf("%s : List invalid error\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					  if ((erro_no)==ELEMENT_INVALID_ERROR) { \
					    printf("%s : List element invalid error\n",(error_msg)); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)

#define BUF_MAX 100


typedef struct {	
	sensor_id_t sensor_id;
	uint16_t room_ID;
	sensor_value_t running_avg_temp;
	sensor_ts_t last_timestamp;
} ss_node_t;


int list_errno;
sensor_id_t ss_id_arr[MAX_SENSOR];
static sensor_value_t ss_temp_record[MAX_SENSOR][NUM_AVG] = {{0}};
static int temp_record_pos[MAX_SENSOR] = {0};
static int count[MAX_SENSOR] = {0}; // static global varibale: variable is protected inside the file, other file could not access this variable

void element_print(element_ptr_t element);
void element_copy(element_ptr_t *dest_element, element_ptr_t src_element);
void element_free(element_ptr_t *element);
int element_compare(element_ptr_t x, element_ptr_t y);

/*
 * Local function to calculate the running avarage temperature 
 */
sensor_value_t cal_avg_temp (int node_pos, sensor_value_t temperature)
{
	double avg=0;
	int i;
	
	ss_temp_record[node_pos][temp_record_pos[node_pos]] = temperature;		

	for( i=0; i<NUM_AVG ; i++) {
		avg+=ss_temp_record[node_pos][i];
	}
	
	if(count[node_pos] < NUM_AVG) count[node_pos]++; //the count stops increasing when exceeding NUM_AVG
	
	avg /= count[node_pos];
	temp_record_pos[node_pos]++;
	if(temp_record_pos[node_pos] == NUM_AVG) temp_record_pos[node_pos] = 0;
	
	return avg;
}

/*
 * Function to create a list of sensor nodes 
 */
list_ptr_t ss_node_list_create(const char *mapping_path)
{
	FILE *fp;
	list_ptr_t ss_list_p = NULL;
	char buf[BUF_MAX];
	char s1[BUF_MAX], s2[BUF_MAX];
	int i,j;	
	ss_node_t *element;
	
	list_errno = ERR_NONE;
	
	element = (ss_node_t *) malloc(sizeof(ss_node_t));		
	ss_list_p = mylist_create( &element_copy, &element_free, &element_compare, &element_print);
	LIST_ERROR(list_errno,"mylist_create() failed");
		
	/* Open mapping file */
	fp = fopen( mapping_path, "r" );
	FILE_ERROR(fp,"File open failed\n");	
	
	while(fgets(buf, BUF_MAX, fp) != NULL) // Check each line of the file
	{
		// get room_ID
		for(i=0; i<strlen(buf); i++) {
			if(buf[i] != ' ') {
				s1[i] = buf[i];
			}
			else {
				s1[i] = '\0';
				break;
			}			
		}		
		i++;
		j=0;
		// get id
		while(buf[i] != '\n')
		{
			s2[j] = buf[i];
			i++;
			j++;
		}		
		s2[j] = '\0';

		// Set init values for each sensor node	
		element->room_ID = (uint16_t) atof(s1);
		element->sensor_id = (uint16_t) atof(s2);
		element->running_avg_temp = 20.0;	
		element->last_timestamp = 0;

		// Insert the sensor node to the list	
		ss_list_p = mylist_insert_at_index(ss_list_p, element, mylist_size(ss_list_p)); //insert at the end of the list	
		LIST_ERROR(list_errno,"mylist_insert_at_index() failed");
	}
	
	
	#ifdef DEBUG
		ss_node_list_print(ss_list_p); //for debug
	#endif
	
	// Check EOF of the file
	if(feof(fp) == 0) {
		perror("File read failed: ");		
		return NULL;
	}
	
	free(element);
	fclose( fp );	
	return ss_list_p;
}

/*
 * Function to print a list of sensor nodes 
 */
void ss_node_list_print(list_ptr_t ss_list_p) {	
	mylist_print(ss_list_p); //for debug
	LIST_ERROR(list_errno,"ss_node_list_print() failed");
}

/*
 * Function to free a list of sensor nodes 
 */
void ss_node_list_free(list_ptr_t ss_list_p) {
	mylist_free(&ss_list_p);
	LIST_ERROR(list_errno,"ss_node_list_free() failed");
	ss_list_p = NULL;
}

/*
 * Function to read sensor_data file and update to the list of sensor nodes
 */
void ss_node_update_from_file(list_ptr_t ss_list_p, const char *data_path)
{
	FILE *fp;
	int i,j;
	int count;
	int result;
	long fsize;
	sensor_data_t *ss_data_p;
	
	int num_of_node = mylist_size(ss_list_p);
	ss_node_t *node;
	
	list_errno = ERR_NONE;
	
	ss_data_p = (sensor_data_t *) malloc(sizeof(sensor_data_t));	
	if(ss_data_p == NULL) {
		list_errno = ERR_MEM;
		return;
	}
	
	fp = fopen( data_path, "r" );
	FILE_ERROR(fp,"File open failed\n");	
	
	// obtain file size:
	fseek (fp , 0 , SEEK_END);
	fsize = ftell (fp);
	rewind (fp);	
	
	count = fsize/(sizeof(uint16_t) + sizeof(double) + sizeof(time_t));		
		
	for(i=0; i<count; i++) { // Check each data package=<sensor ID><temperature><timestamp>
		result = fread( &(ss_data_p->id), sizeof(ss_data_p->id), 1, fp );
		result = fread( &(ss_data_p->value), sizeof(ss_data_p->value), 1, fp );
		result = fread( &(ss_data_p->ts), sizeof(ss_data_p->ts), 1, fp );
		
		if ( result < 1 ) {
			perror("File read failed: ");
			fclose( fp );
			//~ exit(EXIT_FAILURE);			
		}		
		// Check ss id
		for(j=0; j<num_of_node; j++) {
			node = (ss_node_t *) mylist_get_element_at_index(ss_list_p, j);
			LIST_ERROR(list_errno,"mylist_get_element_at_index() failed");
			if(node->sensor_id == ss_data_p->id) {						
				// Calculate running average - Update to sensor list				
				node->last_timestamp = ss_data_p->ts;
				node->running_avg_temp = cal_avg_temp(j, ss_data_p->value);
				break;
			}			
		}			
	}
	
	if ( fclose(fp) == EOF ) {
		perror("File close failed: ");			
		//~ exit(EXIT_FAILURE);
	}
	
	free(ss_data_p);	
	ss_data_p = NULL;		
}

/*
 * Function to read sensor_data packet and update to the list of sensor nodes
 * return the index of updated node if successful, return -1 if unsuccessful
 */
int ss_node_update_from_packet(list_ptr_t ss_list_p, sensor_data_t *packet)
{
	int j;	
	int flag = 0;
	int num_of_node = mylist_size(ss_list_p);
	ss_node_t *node;
	
	list_errno = ERR_NONE;
		
	for(j=0; j<num_of_node; j++) {
		node = (ss_node_t *)mylist_get_element_at_index(ss_list_p, j);
		LIST_ERROR(list_errno,"mylist_get_element_at_index() failed");
		if(node->sensor_id == packet->id) {			
			// Update timestamp to sensor node			
			node->last_timestamp = packet->ts;			
			// Calculate running average
			node->running_avg_temp = cal_avg_temp(j, packet->value);
			flag = 1;
			return j;			
		}			
	}	
	if (flag == 0) return -1;
	return 0;
}


/*
 * Function to check temperature avarage in boundaries
 * return: 0 if temperature is normal, 1 if temperature is too hot, -1 if temperature is too cold 
 */
int ss_node_check_temp_avg (list_ptr_t ss_list_p, sensor_id_t id)
{
	ss_node_t *node;	
	int j;
	int num_of_node = mylist_size(ss_list_p);
	
	for(j=0; j<num_of_node; j++) {
		node = mylist_get_element_at_index(ss_list_p, j);		
		LIST_ERROR(list_errno,"ss_node_check_temp_avg() -> mylist_get_element_at_index() failed");
		// check avg boundaries
		if(node->sensor_id == id) {
			if (node->running_avg_temp < SET_MIN_TEMP)	{
				// fprintf(fp_log,"SS_id %d - Room %" PRIu16 " is too cold\n", node->sensor_id, node->room_ID);
				return -1;	
			}
			if (node->running_avg_temp > SET_MAX_TEMP)	{
				// fprintf(fp_log,"SS_id %d - Room %" PRIu16 " is too hot\n", node->sensor_id, node->room_ID);
				return 1;	
			}
			return 0;
		}			
	}
	return 0;
}

/*
 * Copy the 'content' of src_element to dst_element.
 */
void element_copy(element_ptr_t *dest_element, element_ptr_t src_element)
{  
  ss_node_t *p;
  p = (ss_node_t *) malloc(sizeof(ss_node_t));
  assert(p != NULL);
  p->room_ID = ((ss_node_t *)src_element)->room_ID;
  p->sensor_id = ((ss_node_t *)src_element)->sensor_id;
  p->running_avg_temp = ((ss_node_t *)src_element)->running_avg_temp;
  p->last_timestamp = ((ss_node_t *)src_element)->last_timestamp;
  *dest_element = (element_ptr_t) p;  
}


/*
 * Clean up element, including freeing memory if needed
 */
void element_free(element_ptr_t *element)
{ 
  free(*element);
}

/*
 * Print 1 element to stdout. 
 */
void element_print(element_ptr_t element)
{ 	
	printf("SS Node : id = %d - room_ID = %d - running_avg_temp = %g - last_timestamp = %ld\n", (int)((ss_node_t *)element)->sensor_id, (int)((ss_node_t *)element)->room_ID, (double)((ss_node_t *)element)->running_avg_temp, (long)((ss_node_t *)element)->last_timestamp);
}

/*
 * Compare two element elements; returns -1, 0 or 1 
 */
int element_compare(element_ptr_t x, element_ptr_t y)
{
	//not use
  return 0;
}
