#ifndef __err_handler_h__
#define __err_handler_h__

/*
 * Error handler for pthread activities
 */
void pthread_err_handler( int err_code, char *msg, char *file_name, int line_nr );

/*
 * Error handler for memory allocation
 */
void malloc_check(void *p, char *msg) ;

/*
 * Error handler for reading/writing file I/O 
 */
void file_err_handler(int fresult, char *msg) ;

/*
 * Error handler for queue 
 */
 void queue_err_handler(void *queue_p, char *msg);

/*
 * Error handler for list 
 */
void list_err_handler(int err_code, char *msg) ;

#endif //__err_handler_h__