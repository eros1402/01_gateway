/*******************************************************************************
* FILENAME: myqueue.c							       
*
* Version V1.1		
* Author: Pham Hoang Chi
*
* An implementation of Lab 4 assignment - System Software Course
* 
*******************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h> 
#include "myqueue.h"

/*
 * The real definition of 'struct queue'
 */ 
struct queue {
  element_t *arr; // dynamic array containing data elements
  int current_size; // Counts number of elements in the queue
  int front, rear;
  
  element_copy_func *element_copy; //callback function
  element_free_func *element_free;  
  element_print_func *element_print;
  // Remark for later: extra fields need to be added here to make it a thread-safe queue as needed for the assignment 
  pthread_mutex_t data_mutex;  
};

void mem_alloc_check(void *p, char *msg) {
	if(p == NULL) {
		fprintf(stderr, "\n%s: memory alloc error\n", msg);
		exit(-1);
	}
}

void queue_check(queue_t* queue, char *msg) {
	if(queue == NULL) {
		fprintf(stderr, "\n%s: queue input is NULL\n", msg);		
		exit(-1);        
	}
}

void pthread_check( int err_code, char *msg, char *file_name, int line_nr )
{
	if ( 0 != err_code ) {
		fprintf( stderr, "\n%s failed with error code %d in file %s at line %d\n", msg, err_code, file_name, line_nr );		
	}
}

/*
 **  Creates (memory allocation!) and initializes the queue and prepares it for usage
 **  Return a pointer to the newly created queue
 **  Returns NULL if queue creation failed
 */
queue_t* queue_create(element_copy_func *element_copy, element_free_func *element_free, element_print_func *element_print)
{	
  // implementation goes here	
	queue_t* myqueue;	
	myqueue = (queue_t *) malloc(sizeof(queue_t)); // Queue allocated	
	if(myqueue == NULL) {return NULL;}	
	myqueue->arr = (element_t *) malloc(QUEUE_SIZE*sizeof(element_t));
	mem_alloc_check((element_t *) myqueue->arr, "malloc() return NULL");
	myqueue->current_size = 0;
	myqueue->front = 0;
	myqueue->rear = -1;		
	
	myqueue->element_copy = element_copy;
	myqueue->element_free = element_free;	
	myqueue->element_print = element_print;
	
	pthread_mutex_init(&(myqueue->data_mutex), NULL);	
	return myqueue;
}

/*
 **  Delete the queue from memory; set queue to NULL
 **  The queue can no longer be used unless queue_create is called again
 */
void queue_free(queue_t** queue)
{
	// Check if the queue is NULL
	queue_check(*queue, "Error queue_free()");		
	int i=0;
	// Check if the queue is empty
	if((*queue)->current_size != 0)
	{	  
		// free element in each node of the queue	
		for(i = 0; i < QUEUE_SIZE; i++) {
			if((*queue)->arr[i] != NULL) {
				(*queue)->element_free(&((*queue)->arr[i]));
			}
		}
	}	
	
	pthread_mutex_destroy( &((*queue)->data_mutex) );	
	free((*queue)->arr); 	//free element array in queue
	free(*queue); 			//free the queue
	*queue = NULL; 		
}

/*  
 **  Add an element to the queue
 **  (Does nothing if queue is full)
 **  Update 22-Jul-15: If the queue is full, overide the the rear when the allowed flag is set. Otherwise, do nothing
 */
void queue_enqueue(queue_t* queue, element_t element)
{
	int presult;
  // implementation goes here
	queue_check(queue, "Error queue_enqueue()");	
	
	presult = pthread_mutex_lock( &(queue->data_mutex) );	
	pthread_check( presult, "pthread_mutex_lock", __FILE__, __LINE__ );
	
	if(queue->current_size == QUEUE_SIZE) {		
		// printf("Queue is full !!!\n");
		if(allowed_overide_flag) {
			// free the current rear
			queue->element_free(&(queue->arr[queue->rear]));
			// overide the current rear
			queue->element_copy(&(queue->arr[queue->rear]), element);	
		}		        
	}
	else {
		queue->current_size = queue->current_size + 1;	
		queue->rear = queue->rear + 1;
		if(queue->rear == QUEUE_SIZE) {
			queue->rear = 0; //circulating
		}		
		// Make a deep copy
		queue->element_copy(&(queue->arr[queue->rear]), element);
	}
	
	presult = pthread_mutex_unlock( &(queue->data_mutex) );
	pthread_check( presult, "pthread_mutex_unlock", __FILE__, __LINE__ );	
	// signal the condition variable
}

/*
 **  Return the number of elements in the queue
 */
int queue_size(queue_t* queue)
{
	queue_check(queue, "Error queue_size()");
    return queue->current_size;
}

/*
 **  Return a pointer to the top element in the queue
 **  Returns NULL if queue is empty
 */
element_t* queue_top(queue_t* queue){
	int presult;
	element_t *p;
	// implementation goes here
	queue_check(queue, "Error queue_top()");
	
	presult = pthread_mutex_lock( &(queue->data_mutex) );	
	pthread_check( presult, "pthread_mutex_lock", __FILE__, __LINE__ );
  	//Check if the queue is empty
  	if(queue->current_size != 0) {		
		p = &(queue->arr[queue->front]);
		presult = pthread_mutex_unlock( &(queue->data_mutex) );
		pthread_check( presult, "pthread_mutex_unlock", __FILE__, __LINE__ );	
		return p;
	}
  	else {		
		presult = pthread_mutex_unlock( &(queue->data_mutex) );
		pthread_check( presult, "pthread_mutex_unlock", __FILE__, __LINE__ );	
		return NULL;
	}
}

/*
 **  Remove the top element from the queue
 **  Does nothing if queue is empty
 */
void queue_dequeue(queue_t* queue)
{
	int presult;
   //Check the queue is Null
	queue_check(queue, "Error queue_dequeue()");
	
	presult = pthread_mutex_lock( &(queue->data_mutex) );	
	pthread_check( presult, "pthread_mutex_lock", __FILE__, __LINE__ );
	if(queue->current_size != 0) {		
		queue->element_free(&(queue->arr[queue->front]));
		queue->front = queue->front + 1;
		if(queue->front == QUEUE_SIZE) {
			queue->front = 0; //circulating
		}
		queue->current_size = queue->current_size - 1;
	}	
	else {
		//Queue is empty, do nothing
		printf("Queue is empty !!!\n");	
	}
	presult = pthread_mutex_unlock( &(queue->data_mutex) );
	pthread_check( presult, "pthread_mutex_unlock", __FILE__, __LINE__ );	
}

/*
 **  Print all elements in the queue, starting from the front element
 */
void queue_print(queue_t *queue)
{
  // implementation goes here  
  queue_check(queue, "Error queue_print()");
  if(queue->current_size == 0) {	  
	  printf("Queue is empty !!!\n");	
	  return;
  }
  
  int pos;  
  pos= queue->front;  
  element_t e = queue->arr[pos];  
  
  printf("Current queue (starting from the front element): \n");
  
  queue->element_print(e);
  while(pos != queue->rear)  
  {
	  pos++;	  
	  if(pos == QUEUE_SIZE) { pos = 0;} 
	  e = queue->arr[pos];
	  queue->element_print(e);
  } 
  printf("\n");
}

