#ifndef __my_macros_h__
#define __my_macros_h__

#include <errno.h>

#define FREE(p) do { free(p); p = NULL; } while(0)

#define FILE_ERROR(fp,error_msg) 	do { 					\
					  if ((fp)==NULL) { 					\
					    printf("%s\n",(error_msg)); 		\
					    exit(EXIT_FAILURE); 				\
					  }										\
					} while(0)
						
#define SYSCALL_ERROR(err, error_msg) 						\
		do {												\
			if ( (err) == -1 )								\
			{												\
				printf("%s: Error executing syscall\n", error_msg);			\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)

#define CHECK_MKFIFO(err) 									\
		do {												\
			if ( (err) == -1 )								\
			{												\
				if ( errno != EEXIST )						\
				{											\
					perror("Error executing mkfifo");		\
					exit( EXIT_FAILURE );					\
				}											\
			}												\
		} while(0)
			
#define FILE_CLOSE_ERROR(err) 								\
		do {												\
			if ( (err) == -1 )								\
			{												\
				perror("File close failed");				\
				exit( EXIT_FAILURE );						\
			}												\
		} while(0)

			
#endif //__my_macros_h__