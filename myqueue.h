#ifndef MYQUEUE_
#define MYQUEUE_

/*
 ** The default queue size is 5
 */
#ifndef QUEUE_SIZE
    #define QUEUE_SIZE 15
#endif

extern int allowed_overide_flag;

/*
 **  Make some changes here to define the type of element that will be stored in the queue
 */

typedef void* element_t;
typedef struct queue queue_t;

//*define CALLBACK function (function pointer)
typedef void element_copy_func(element_t *, element_t);
typedef void element_free_func(element_t *);
typedef void element_print_func(element_t);

/*
 **  Creates (memory allocation!) and initializes the queue and prepares it for usage
 **  Return a pointer to the newly created queue
 **  Returns NULL if queue creation failed
 */
// queue_t* queue_create();
queue_t* queue_create(element_copy_func *element_copy, element_free_func *element_free, element_print_func *element_print);

/*  
 **  Add an element to the queue
 **  Does nothing if queue is full
 */
void queue_enqueue(queue_t* queue, element_t element);

/*
 **  Delete the queue from memory; set queue to NULL
 **  The queue can no longer be used unless queue_create is called again
 */
void queue_free(queue_t** queue);

/*
 **  Return the number of elements in the queue
 */
int queue_size(queue_t* queue);

/*
 **  Return a pointer to the top element in the queue
 **  Returns NULL if queue is empty
 */
element_t* queue_top(queue_t* queue);

/*
 **  Remove the top element from the queue
 **  Does nothing if queue is empty
 */
void queue_dequeue(queue_t* queue);

/*
 **  Print all elements in the queue, starting from the front element
 */
void queue_print(queue_t *queue);

#endif //MYQUEUE_


