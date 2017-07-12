/*******************************************************************************
* FILENAME: err_handler.c							       
*
* Version V1.0		
* Author: Pham Hoang Chi
*
* An implementation of supporting functions - System Software
* 
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>

/*
 * definition of error codes
 * */
#define LIST_NO_ERROR 0
#define LIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define LIST_EMPTY_ERROR 2  //error due to an operation that can't be executed on an empty list
#define LIST_INVALID_ERROR 3 //error due to a list operation applied on a NULL list 
#define ELEMENT_INVALID_ERROR 4 //error due to a NULL element

/*
 * Error handler for pthread activities
 */
void pthread_err_handler( int err_code, char *msg, char *file_name, int line_nr )
{
	if ( 0 != err_code ) {
		fprintf( stderr, "\n%s failed with error code %d in file %s at line %d\n", msg, err_code, file_name, line_nr );		
	}
}

/*
 * Error handler for memory allocation
 */
void malloc_check(void *p, char *msg) {
	if(p == NULL) {
		fprintf(stderr, "\n%s: memory alloc error\n", msg);
		exit(-1);
	}
}

/*
 * Error handler for reading/writing file I/O 
 */
void file_err_handler(int fresult, char *msg) {
	if(fresult <=-1) {
		fprintf(stderr, "\n%s failed with file I/O\n", msg); 
		exit(-1);	
	}	
}

/*
 * Error handler for queue 
 */
 void queue_err_handler(void *queue_p, char *msg) {
	 if(queue_p == NULL) {
		fprintf(stderr, "\n%s\n", msg);
		exit(-1);
	 }
 }

/*
 * Error handler for list 
 */
void list_err_handler(int err_code, char *msg) {	
	switch(err_code) {
		case LIST_MEMORY_ERROR:
			fprintf(stderr, "\n%s failed :  List memory error\n", msg);
			exit(EXIT_FAILURE);
			break;
		case LIST_EMPTY_ERROR:
			fprintf(stderr, "\n%s failed :  List empty error\n", msg);
			exit(EXIT_FAILURE);
			break;
		case LIST_INVALID_ERROR:
			fprintf(stderr, "\n%s failed :  List invalid error\n", msg);
			exit(EXIT_FAILURE);
			break;
		case ELEMENT_INVALID_ERROR:
			fprintf(stderr, "\n%s failed :  List element invalid error\n", msg);
			exit(EXIT_FAILURE);
			break;
		default :
			break;
	}		
}

