/*******************************************************************************
* FILENAME: list.c							       
*
* Version V1.1		
* Author: Pham Hoang Chi
*
* An implementation of Lab 5 assignment - System Software course
* 
*******************************************************************************/

#include <stdio.h>
#include "list.h"
#include <stdlib.h>
#include <assert.h>

#ifdef DEBUG
	#define DEBUG_PRINT(...) 															\
	  do {					  															\
		printf("In %s - function %s at line %d: ", __FILE__, __func__, __LINE__);		\
		printf(__VA_ARGS__);															\
	  } while(0)
#else
	#define DEBUG_PRINT(...) (void)0
#endif

extern int list_errno;

/*
 * The real definition of 'struct list'
 */ 
struct list_node {	
	list_node_ptr_t prev;
	list_node_ptr_t next;
	element_ptr_t element;
};

struct list {
	list_node_ptr_t head;
	int num_of_element;
	element_copy_func *element_copy; //callback function
	element_free_func *element_free;
	element_compare_func *element_compare;
	element_print_func *element_print; 
}; 

/*
 * Public functions
 */ 
list_ptr_t mylist_create(element_copy_func *element_copy, element_free_func *element_free, element_compare_func *element_compare, element_print_func *element_print){
	list_ptr_t mylist=NULL;	
	list_errno = LIST_NO_ERROR;
	mylist = (list_ptr_t) malloc(sizeof(list_t)); // list allocated
	if(mylist == NULL)
	{
		DEBUG_PRINT( "DEBUG:: Error in list allocating\n" );
		list_errno = LIST_MEMORY_ERROR;
		return NULL;
	}	
	mylist->head = NULL;
	mylist->num_of_element = 0;
	mylist->element_copy = element_copy;
	mylist->element_free = element_free;
	mylist->element_compare = element_compare;
	mylist->element_print = element_print;
	return mylist;
} 
// Returns a pointer to a newly-allocated list.
// Returns NULL if memory allocation failed and list_errno is set to LIST_MEMORY_ERROR 

void mylist_free( list_ptr_t* list )
{	
	int i=0;
	list_node_ptr_t temp = (*list)->head;
		
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(*list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return;	
	}	
	//Check the list is empty
	if((*list)->num_of_element == 0)
	{	  
		free(*list);
		*list = NULL;
		return;
	}			
	//free element of each node
	for(i=0; i < (*list)->num_of_element; i++)
	{
		(*list)->element_free(&(temp->element));
		if(i != (*list)->num_of_element-1) temp=temp->next;			
	}	
	//free each node
	for(i=0; i < (*list)->num_of_element; i++)
	{
		if(i != (*list)->num_of_element-1) temp=temp->prev;				
		free(temp->next);
		temp->next = NULL;
	}	
	free((*list)->head);
	(*list)->head = NULL;
	free(*list);
	*list = NULL;
}
// Every list node and node element of the list needs to be deleted (free memory)
// The list itself also needs to be deleted (free all memory) and set to NULL

int mylist_size( list_ptr_t list )
{	
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG::List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return -1;	
	}		
	return list->num_of_element;
}
// Returns the number of elements in 'list'.

list_ptr_t mylist_insert_at_index( list_ptr_t list, element_ptr_t element, int index)
{	
	list_node_ptr_t new_node;
	int i;
	
	list_errno = LIST_NO_ERROR;	
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return NULL;	
	}		
	new_node = (list_node_ptr_t)malloc(sizeof(list_node_t));
	if(new_node == NULL)
	{
		DEBUG_PRINT( "DEBUG:: Error in allocating a new list_node\n" );
		list_errno = LIST_MEMORY_ERROR;
		return NULL;
	}		
	//new_node->element = element; //Deep copy???	
	list->element_copy(&(new_node->element), element); //make a deep copy
	
	//the list node is inserted at the start of 'list'
	if(index <= 0)
	{		
		new_node->prev = NULL;		
		if(list->num_of_element == 0) //The list is empty
		{
			new_node->next = NULL;
		}
		else
		{
			new_node->next = list->head;
			list->head->prev = new_node;
		}		
		list->head = new_node;		
	}
	else
	{		
		list_node_ptr_t temp = list->head;		
		//the list node is inserted at the end of 'list'
		if(index >= list->num_of_element)
		{
			new_node->next = NULL;			
			for(i=1; i < list->num_of_element; i++)
			{			
				temp = temp->next;
			}			
			new_node->prev = temp;
			temp->next = new_node;				
		}
		else
		{
			//the list node is inserted in the middle of 'list'
			for(i=1; i < index; i++)
			{
				temp = temp->next;
			}
			new_node->prev = temp;
			new_node->next = temp->next;
			new_node->next->prev = new_node;
			temp->next = new_node;
		}		
	}	
	
	list->num_of_element++;
	return list;
}
// Inserts a new list node containing 'element' in 'list' at position 'index'  and returns a pointer to the new list.
// Remark: the first list node has index 0.
// If 'index' is 0 or negative, the list node is inserted at the start of 'list'. 
// If 'index' is bigger than the number of elements in 'list', the list node is inserted at the end of 'list'.
// Returns NULL if memory allocation failed and list_errno is set to LIST_MEMORY_ERROR 

list_ptr_t mylist_remove_at_index( list_ptr_t list, int index)
{		
	list_node_ptr_t temp;
	
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return NULL;	
	}	
	//Check the list is empty
	if(list->num_of_element == 0)
	{	  
	  list_errno = LIST_EMPTY_ERROR;
	  DEBUG_PRINT( "DEBUG:: List is empty\n" );
	  return list;
	}	
	//Check if index is negative or out of list range
	if(index < 0) index = 0;
	if(index >= (list->num_of_element)) index = list->num_of_element-1;	
		
	temp = mylist_get_reference_at_index(list, index);
	//If 'index' is 0 or negative, the first list node is removed
	if(temp->prev == NULL)  
	{
		if(temp->next != NULL) temp->next->prev = NULL;
		list->head = temp->next;
		//temp->next = NULL;
		free(temp);
		list->num_of_element--;	
		return list;	
	}	
	//If 'index' is bigger than the number of elements in 'list', the last list node is removed
	if(temp->next == NULL)
	{
		if(temp->prev != NULL) temp->prev->next = NULL;
		free(temp);
		list->num_of_element--;
		return list;
	}	
	//the list node is removed in the middle of 'list'
	temp->prev->next = temp->next;
	temp->next->prev = temp->prev;
	free(temp);	
	list->num_of_element--;
	return list;
}
// Removes the list node at index 'index' from 'list'. NO free() is called on the element pointer of the list node. 
// If 'index' is 0 or negative, the first list node is removed. 
// If 'index' is bigger than the number of elements in 'list', the last list node is removed.
// If the list is empty, return list and list_errno is set to LIST_EMPTY_ERROR (to see the difference with removing the last element from a list)

list_ptr_t mylist_free_at_index( list_ptr_t list, int index)
{	
	
	list_node_ptr_t temp;
	
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return NULL;	
	}	
	//Check the list is empty
	if(list->num_of_element == 0)
	{	  
	  list_errno = LIST_EMPTY_ERROR;
	  DEBUG_PRINT( "DEBUG:: List is empty\n" );
	  return list;
	}	
	//Check if index is negative or out of list range
	if(index < 0) index = 0;
	if(index >= (list->num_of_element)) index = list->num_of_element-1;	
	
	temp = mylist_get_reference_at_index(list, index);
	// If 'index' is 0 or negative, the first list node is deleted
	if(temp->prev == NULL)  
	{
		if(temp->next != NULL)	temp->next->prev = NULL;
		list->head = temp->next;		
		// list->element_free(temp->element);	// bug found: 11-May-15
		list->element_free(&(temp->element)); //Fixed bug: 11-May-15
		free(temp);
		list->num_of_element--;	
		return list;	
	}	
	//If 'index' is bigger than the number of elements in 'list', the last list node is deleted.
	if(temp->next == NULL)
	{
		if(temp->prev != NULL) temp->prev->next = NULL;
		// list->element_free(temp->element);	// bug found: 11-May-15
		list->element_free(&(temp->element)); //Fixed bug: 11-May-15		
		free(temp);
		list->num_of_element--;
		return list;
	}	
	//the list node is deleted in the middle of 'list'
	temp->prev->next = temp->next;
	temp->next->prev = temp->prev;
	// list->element_free(temp->element);	// bug found: 11-May-15
	list->element_free(&(temp->element)); //Fixed bug: 11-May-15
	free(temp);
	
	list->num_of_element--;
	return list;
}
// Deletes the list node at index 'index' in 'list'. 
// A free() is called on the element pointer of the list node to free any dynamic memory allocated to the element pointer. 
// If 'index' is 0 or negative, the first list node is deleted. 
// If 'index' is bigger than the number of elements in 'list', the last list node is deleted.
// If the list is empty, return list and list_errno is set to LIST_EMPTY_ERROR (to see the difference with freeing the last element from a list)

list_node_ptr_t mylist_get_reference_at_index( list_ptr_t list, int index )
{	
	int i;
	list_node_ptr_t node_ptr;
	
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return NULL;	
	}	
	//Check the list is empty
	if(list->num_of_element == 0)
	{	  
	  list_errno = LIST_EMPTY_ERROR;
	  DEBUG_PRINT( "DEBUG:: List is empty\n" );
	  return NULL;
	}	
	//Check if index is negative or out of list range
	if(index < 0) index = 0;
	if(index >= (list->num_of_element)) index = list->num_of_element-1;	
	node_ptr = list->head;
	for(i=1; i <= index; i++)
	{			
		node_ptr = node_ptr->next; //point to index pos
	}		
	return node_ptr;
}
// Returns a reference to the list node with index 'index' in 'list'. 
// If 'index' is 0 or negative, a reference to the first list node is returned. 
// If 'index' is bigger than the number of list nodes in 'list', a reference to the last list node is returned. 
// If the list is empty, NULL is returned.

element_ptr_t mylist_get_element_at_index( list_ptr_t list, int index )
{	
	list_errno = LIST_NO_ERROR;	
	list_node_ptr_t temp = mylist_get_reference_at_index(list, index);
	if(temp == NULL) return NULL;	
	return temp->element; //return an element pointer of the list (not a copy)-> be careful!!!
}
// Returns the list element contained in the list node with index 'index' in 'list'. 
// If 'index' is 0 or negative, the element of the first list node is returned. 
// If 'index' is bigger than the number of elements in 'list', the element of the last list node is returned.
// If the list is empty, NULL is returned.

int mylist_get_index_of_element( list_ptr_t list, element_ptr_t element )
{		
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return -1;	
	}	
	//Check the list is empty
	if(list->num_of_element == 0)
	{	  
		list_errno = LIST_EMPTY_ERROR;
		DEBUG_PRINT( "DEBUG:: List is empty\n" );
		return -1;
	}	
	//Check the element is NULL
	if(element == NULL)
	{
		list_errno = ELEMENT_INVALID_ERROR;
		DEBUG_PRINT( "DEBUG:: Input element is NULL\n" );
		return -1;
	}	
	int i=0;
	int isEqual = -1;
	list_node_ptr_t temp = list->head;
	while(i < list->num_of_element)
	{
		isEqual = list->element_compare(temp->element, element);
		if(isEqual == 1) return i;
		temp = temp->next;
		i++;
	}	
	// If 'element' is not found in 'list'
	return -1;
}
// Returns an index to the first list node in 'list' containing 'element'.  
// If 'element' is not found in 'list', -1 is returned.

void mylist_print( list_ptr_t list )
{	
	int i;
	list_node_ptr_t temp;
	
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return;	
	}	
	//Check the list is empty
	if(list->num_of_element == 0)
	{
	  //List is empty
	  list_errno = LIST_EMPTY_ERROR;
	  DEBUG_PRINT( "DEBUG:: List is empty\n" );
	  return;
	}	
	temp = list->head;
	for(i=0; i < list->num_of_element; i++)
	{
		list->element_print(temp->element);
		temp = temp->next;
	}
}
// for testing purposes: print the entire list on screen

#ifdef LIST_EXTRA
  list_ptr_t list_insert_at_reference( list_ptr_t list, element_ptr_t element, list_node_ptr_t reference )
  {
	  int index = list_get_index_of_reference(list, reference);
	  if(index != -1) { return mylist_insert_at_index(list, element, index);}
	  return list;
  }
  // Inserts a new list node containing 'element' in the 'list' at position 'reference'  and returns a pointer to the new list. 
  // If 'reference' is NULL, the element is inserted at the end of 'list'.

  list_ptr_t list_insert_sorted( list_ptr_t list, element_ptr_t element )
  {
	  int index = 0;
	  //~ int i,j;
	  list_errno = LIST_NO_ERROR;	
		//check if the list is NULL
		if(list == NULL) 
		{
			DEBUG_PRINT( "DEBUG:: List invalid error\n" );
			list_errno = LIST_INVALID_ERROR;
			return NULL;	
		}
	  list = mylist_insert_at_index(list, element, index);
	  
	  // Sort the list
	  //~ for(i = 0; i < list->num_of_element-1; i++)
	  //~ {
		  //~ for(j = i+1; j < list->num_of_element; j++)
		  //~ {
			  //~ 
		  //~ }
	  //~ }
	  return list;
  }
  // Inserts a new list node containing 'element' in the sorted 'list' and returns a pointer to the new list. 
  // The 'list' must be sorted before calling this function. 
  // The sorting is done in ascending order according to a comparison function.  
  // If two members compare as equal, their order in the sorted array is undefined.

  list_ptr_t list_remove_at_reference( list_ptr_t list, list_node_ptr_t reference )
  {		
	// Check If 'reference' is NULL
	if(reference == NULL)	return mylist_remove_at_index(list, list->num_of_element-1);		
		
	int index = list_get_index_of_reference(list, reference);	
	if(index != -1) return mylist_remove_at_index(list, index);	
	return list;
  }
  // Removes the list node with reference 'reference' in 'list'. 
  // NO free() is called on the element pointer of the list node. 
  // If 'reference' is NULL, the last list node is removed.
  // If the list is empty, return list and list_errno is set to LIST_EMPTY_ERROR

  list_ptr_t list_free_at_reference( list_ptr_t list, list_node_ptr_t reference )
  {
	// Check If 'reference' is NULL
	if(reference == NULL)	return mylist_free_at_index(list, list->num_of_element-1);		
	 
	int index = list_get_index_of_reference(list, reference);	
	if(index != -1) return mylist_free_at_index(list, index);	
	return list;
  }
  // Deletes the list node with position 'reference' in 'list'. 
  // A free() is called on the element pointer of the list node to free any dynamic memory allocated to the element pointer. 
  // If 'reference' is NULL, the last list node is deleted.
  // If the list is empty, return list and list_errno is set to LIST_EMPTY_ERROR

  list_ptr_t list_remove_element( list_ptr_t list, element_ptr_t element )
  {		
	int index;	
	index = mylist_get_index_of_element( list, element );
	if(index != -1) return mylist_remove_at_index(list, index);	
	return list;	
  }
  // Finds the first list node in 'list' that contains 'element' and removes the list node from 'list'. 
  // NO free() is called on the element pointer of the list node.
  // If the list is empty, return list and list_errno is set to LIST_EMPTY_ERROR
  
  list_node_ptr_t list_get_first_reference( list_ptr_t list )
  {			
		return mylist_get_reference_at_index(list, 0);
  }
  // Returns a reference to the first list node of 'list'. 
  // If the list is empty, NULL is returned.

  list_node_ptr_t list_get_last_reference( list_ptr_t list )
  {			
		return mylist_get_reference_at_index(list, list->num_of_element-1);	
  }
  // Returns a reference to the last list node of 'list'. 
  // If the list is empty, NULL is returned.

  list_node_ptr_t list_get_next_reference( list_ptr_t list, list_node_ptr_t reference )
  {	 
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return NULL;	
	}	
	//Check the list is empty
	if(list->num_of_element == 0)
	{	  
	  list_errno = LIST_EMPTY_ERROR;
	  DEBUG_PRINT( "DEBUG:: List is empty\n" );
	  return NULL;
	}	
	// Check If 'reference' is NULL
	if(reference == NULL) return NULL;
	
	// Check If the next element doesn't exists
	int index = list_get_index_of_reference(list, reference);
	if(index == -1) return NULL;			
	return reference->next; 	
  } 
  // Returns a reference to the next list node of the list node with reference 'reference' in 'list'. 
  // If the next element doesn't exists, NULL is returned.

  list_node_ptr_t list_get_previous_reference( list_ptr_t list, list_node_ptr_t reference )
  {	  
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return NULL;	
	}	
	//Check the list is empty
	if(list->num_of_element == 0)
	{	  
	  list_errno = LIST_EMPTY_ERROR;
	  DEBUG_PRINT( "DEBUG:: List is empty\n" );
	  return NULL;
	}	
	// Check If 'reference' is NULL
	if(reference == NULL) return NULL;	
	// Check If the next element doesn't exists
	int index = list_get_index_of_reference(list, reference);
	if(index != -1){ return reference->prev;}	
	else {	return NULL; }
  }
  // Returns a reference to the previous list node of the list node with reference 'reference' in 'list'. 
  // If the previous element doesn't exists, NULL is returned.

  element_ptr_t list_get_element_at_reference( list_ptr_t list, list_node_ptr_t reference )
  {		
	int index = list_get_index_of_reference(list, reference);
	if(index != -1) { return mylist_get_element_at_index(list, index); }
	else {	return NULL; }
  }
  // Returns the element pointer contained in the list node with reference 'reference' in 'list'. 
  // If 'reference' is NULL, the element of the last element is returned.

  list_node_ptr_t list_get_reference_of_element( list_ptr_t list, element_ptr_t element )
  {		
	int index;	
	index = mylist_get_index_of_element( list, element );
	if(index == -1) {return NULL;}
	else {return mylist_get_reference_at_index(list, index);}
  }
  // Returns a reference to the first list node in 'list' containing 'element'. 
  // If 'element' is not found in 'list', NULL is returned.

  int list_get_index_of_reference( list_ptr_t list, list_node_ptr_t reference )
  {	  
	list_errno = LIST_NO_ERROR;
	//check if the list is NULL
	if(list == NULL) 
	{
		DEBUG_PRINT( "DEBUG:: List invalid error\n" );
		list_errno = LIST_INVALID_ERROR;
        return -1;	
	}	
	//Check the list is empty
	if(list->num_of_element == 0)
	{	  
		list_errno = LIST_EMPTY_ERROR;
		DEBUG_PRINT( "DEBUG:: List is empty\n" );
		return -1;
	}	
	if(reference == NULL)
	{
		return (list->num_of_element-1);
	}	  
	int i=0;	
	list_node_ptr_t temp = list->head;
	while(i < list->num_of_element)
	{
		if(temp == reference) return i;		//if 2 pointer point to same node
		temp = temp->next;
		i++;
	}
	return -1; //reference is not belong to the list
  }
  // Returns the index of the list node in the 'list' with reference 'reference'. 
  // If 'reference' is NULL, the index of the last element is returned.

  list_ptr_t list_free_element( list_ptr_t list, element_ptr_t element )
  {		
	int index;	
	index = mylist_get_index_of_element( list, element );
	if(index != -1) return mylist_free_at_index(list, index);	
	return list;
  }
  // Finds the first list node in 'list' that contains 'element' and deletes the list node from 'list'. 
  // A free() is called on the element pointer of the list node to free any dynamic memory allocated to the element pointer. 
 #endif
