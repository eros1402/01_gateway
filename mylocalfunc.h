#ifndef __mylocalfunc_h__
#define __mylocalfunc_h__

extern pthread_mutex_t fifo_mutex; // mutex 

int send_request_run(struct mg_connection *conn, gateway_conf_t *input, FILE *fp);

void print_help(void);
int check_command_line_argument(int argc, char *argv[], gateway_conf_t *input);
void check_timeout_running(time_t last_timestamp);
void dynamic_sensor_list_size (int list_size, int *max_list_size, struct pollfd *poll_fd, time_t *last_timestamp);
int receive_sensor_data(Socket s, sensor_data_t *data_packet);
void generate_log_event(int fifo_fd, int event_id, int sensorNodeID, int sensor_id);

void queue_element_print(element_t element);
void queue_element_copy(element_t *dest_element, element_t src_element);
void queue_element_free(element_t *element);
int queue_element_compare(element_t x, element_t y);

void list_element_print(element_ptr_t element);
void list_element_copy(element_ptr_t *dest_element, element_ptr_t src_element);
void list_element_free(element_ptr_t *element);
int list_element_compare(element_ptr_t x, element_ptr_t y);

#endif //__mylocalfunc_h__