/*******************************************************************************
*  FILENAME: tcp_comm.c							       
*
* Version V1.0		
* Author: Luc Vandeurzen
*
* An implementation of a simple-to-use TCP socket API 
* 
* TO DO 	
* - More error checking (errno & perror) 
* - Reentrant code
* - Thread safety
*									    
*******************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tcpsocket.h"

#define CHAR_IP_ADDR_LENGTH 16     // 4 numbers of 3 digits, 3 dots and \0
#define ERROR_SD -1
#define ERROR_PORT 0
#define MIN_PORT 1
#define MAX_PORT 65536
 
#define	PROTOCOLFAMILY	AF_INET		// internet protocol suite
#define	TYPE		SOCK_STREAM	// streaming protool type
#define	PROTOCOL	IPPROTO_TCP 	// TCP protocol 

typedef struct {
  int sd;
  char *ip_addr;
  int port;  
  uint16_t ss_id; //update: 23 Jul 2015
  } MySocket;		// My definition of a socket: a socket descriptor, 
			// the IP address and port number of the PC hosting this socket 

// private functions used for error checking
static void die(char* message);
static void check_socket_ptr(char *pre_msg, Socket s);
static void check_sd(char *pre_msg, int sd);
static void check_ip_addr(char *pre_msg, char *ip_addr);
static void check_port(char *pre_msg, int port);

// private error message string
static char error_msg[256]; 

/*-------------------------------------------------------------------------------------*/
Socket tcp_passive_open(int port) 
/*-------------------------------------------------------------------------------------*/
{
  // parameter check                                                                                         
  check_port("tcp_open_server() failed", port);

  MySocket *s = (MySocket *)malloc( sizeof(MySocket) );
  if ( s == NULL ) 
    die("tcp_open__socket() failed: mem alloc error");

  struct sockaddr_in addr;

  s->sd = socket(PROTOCOLFAMILY, TYPE, PROTOCOL);
  check_sd("tcp_open_server failed(): socket creation error", s->sd);

  /* Construct the server address structure */
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = PROTOCOLFAMILY;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if ( bind(s->sd, (struct sockaddr *)&addr, sizeof(addr)) != 0 ) {
    die("tcp_open_server failed(): bind() failed");  //will fail if e.g; port is in use
  }

  if( listen(s->sd, MAX_PENDING) != 0 )
	die("tcp_open_server failed(): listen() failed");

  s->port = port;
  s->ip_addr = NULL;  //INADDR_ANY ...	

  return (Socket)s;  
}

/*-------------------------------------------------------------------------------------*/
Socket tcp_active_open( int remote_port, char *remote_ip ) 
/*-------------------------------------------------------------------------------------*/
{
  // parameter check                                                                                         
  check_port("tcp_open_client() failed", remote_port);
  check_ip_addr("tcp_open_client() failed", remote_ip);
    
  MySocket *client = (MySocket *)malloc( sizeof(MySocket) );
  if ( client == NULL ) 
    die("tcp_open_client() failed: mem alloc error");

  struct sockaddr_in addr;
  int length;
  char *p;

  client->sd = socket(PROTOCOLFAMILY, TYPE, PROTOCOL);
  check_sd("tcp_open_client() failed: socket creation error", client->sd);
    
  /* Construct the server address structure */
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = PROTOCOLFAMILY;
  if ( inet_aton(remote_ip, (struct in_addr *) &addr.sin_addr.s_addr) == 0 ) 
    die("tcp_open_client failed(): invalid ip address");

  addr.sin_port = htons(remote_port);

  if ( connect(client->sd, (struct sockaddr *) &addr, sizeof(addr) ) < 0 )
    die("tcp_open_client failed(): connect () failed");

  memset(&addr, 0, sizeof(struct sockaddr_in));
  length = sizeof(addr);
  if ( getsockname(client->sd, (struct sockaddr *)&addr, (socklen_t *)&length) != 0 )
    die("tcp_open_client failed(): getsockname() failed");
  
  p = inet_ntoa(addr.sin_addr);  //returns addr to statically allocated buffer
  
  client->ip_addr = (char *)malloc( sizeof(char)*CHAR_IP_ADDR_LENGTH);
  if ( client->ip_addr == NULL )
 	die("tcp_open_client failed(): mem alloc error");
  client->ip_addr = strcpy( client->ip_addr, p );
  client->port = ntohs(addr.sin_port);

  return (Socket)client;
}

/*-------------------------------------------------------------------------------------*/
Socket tcp_wait_for_connection( Socket socket ) 
/*-------------------------------------------------------------------------------------*/
{
  // parameter check                                                                                         
  check_socket_ptr("tcp_wait_for_connection() failed", socket);

  MySocket *serv = (MySocket *)socket;
  MySocket *clie = (MySocket *)malloc( sizeof(MySocket) );
  if ( clie == NULL ) 
    die("tcp_wait_for_connection() failed: mem alloc error");

  struct sockaddr_in addr;
  unsigned int length = sizeof(struct sockaddr_in);
  char *p;
  
  check_sd("tcp_wait_for_connection() failed", serv->sd);

  clie->sd = accept(serv->sd, (struct sockaddr*) &addr, &length);

  check_sd("tcp_wait_for_connection() failed: accept() error", clie->sd);
		
  p = inet_ntoa(addr.sin_addr);  //returns addr to statically allocated buffer
  clie->ip_addr = (char *)malloc( sizeof(char)*CHAR_IP_ADDR_LENGTH);
  if ( clie->ip_addr == NULL )
    die("tcp_wait_for_connection failed(): mem alloc error");
  clie->ip_addr = strcpy( clie->ip_addr, p );
  clie->port = ntohs(addr.sin_port);

  return (Socket)clie;
}

/*-------------------------------------------------------------------------------------*/
void tcp_close( Socket *socket )
/*-------------------------------------------------------------------------------------*/
{
  // parameter check                
  check_socket_ptr("tcp_close() failed", socket);                                                                         
  check_socket_ptr("tcp_close() failed", *socket);

  MySocket *s = (MySocket *)*socket;
  
  check_sd("tcp_close() failed", s->sd);
  
  close( s->sd );
  
  if (s->ip_addr != NULL) {
    free(s->ip_addr);
  }
  free(s);

  *socket = NULL;
}

/*-------------------------------------------------------------------------------------*/
void tcp_send(Socket socket, void *buffer, int bufsize ) 
/*-------------------------------------------------------------------------------------*/
{
  // parameter check                                                                                         
  check_socket_ptr("tcp_send() failed", socket);

  if ( buffer == NULL ) 
    die("tcp_send failed(): buffer param is NULL");
                                                                                             
  MySocket *s = (MySocket *)socket;
  int result;
  int sen = 0;
  int to_sen = bufsize;

  check_sd("tcp_send() failed", s->sd);

  do {
    result = send(s->sd, (const void*) (buffer+sen), to_sen, 0);
    if (result < 0)
      die("tcp_send() failed: not able to send");
    sen += result;
    to_sen -= result;
  } while ( to_sen > 0 );

}

/*-------------------------------------------------------------------------------------*/
int tcp_receive (Socket socket, void* buffer, int bufsize) 
/*-------------------------------------------------------------------------------------*/
{
  // parameter check                                                                             
  check_socket_ptr("tcp_receive() failed", socket);
 
  if ( buffer == NULL ) 
    die("tcp_receive() failed: buffer param is NULL");
  if ( bufsize == 0 ) 
    die("tcp_receive() failed: bufsize is zero");
                                                                                             
  MySocket *s = (MySocket *)socket;

  check_sd("tcp_receive() failed", s->sd);
  
  int rec = recv(s->sd, buffer, bufsize, 0);

  return rec;
}

/*-------------------------------------------------------------------------------------*/
char * get_ip_addr( Socket socket ) 

/*-------------------------------------------------------------------------------------*/
{   
  // parameter check
  check_socket_ptr("get_ip_addr() failed", socket);

  MySocket *s = (MySocket *)socket;
  
  check_ip_addr("get_ip_addr() failed", s->ip_addr);

  return s->ip_addr;
}

/*-------------------------------------------------------------------------------------*/
int get_port( Socket socket ) 

/*-------------------------------------------------------------------------------------*/
{
  // parameter check
  check_socket_ptr("get_port() failed", socket);
    
  MySocket *s = (MySocket *)socket;
  
  check_port("get_port() failed", s->port);

  return s->port;
}


/*-------------------------------------------------------------------------------------*/
int get_socket_descriptor( Socket socket )
/*-------------------------------------------------------------------------------------*/
{
  // parameter check
  check_socket_ptr("get_socket_descriptor() failed", socket);
    
  MySocket *s = (MySocket *)socket;

  check_sd("get_socket_descriptor() failed", s->sd);

  return s->sd;
}

/*-------------------------------------------------------------------------------------*/
int get_sensor_id( Socket socket ) //update: 23 Jul 2015
/*-------------------------------------------------------------------------------------*/
{
  // parameter check
  check_socket_ptr("get_socket_descriptor() failed", socket);
    
  MySocket *s = (MySocket *)socket;

  return s->ss_id;
}

/*-------------------------------------------------------------------------------------*/
void set_sensor_id( Socket socket, int id ) //update: 23 Jul 2015
/*-------------------------------------------------------------------------------------*/
{
  // parameter check
  check_socket_ptr("get_socket_descriptor() failed", socket);
    
  MySocket *s = (MySocket *)socket;

  if(id < 0) s->ss_id = 0;
  else s->ss_id = id;
}

/*-------------------------------------------------------------------------------------*/
static void die(char* message)
/*-------------------------------------------------------------------------------------*/
{
  perror(message);
  exit(-1);
}

/*-------------------------------------------------------------------------------------*/
static void check_socket_ptr(char *pre_msg, Socket socket) 
/*-------------------------------------------------------------------------------------*/
{
  if ( socket == NULL ) 
  {
    sprintf(error_msg, "%s: socket ptr is NULL", pre_msg);
    die(error_msg);   
  }                                                                                      
}
       
/*-------------------------------------------------------------------------------------*/       
static void check_sd(char *pre_msg, int sd)
/*-------------------------------------------------------------------------------------*/
{ 
  if ( sd <= ERROR_SD ) 
  {
    sprintf(error_msg, "%s: invalid socket descriptor", pre_msg);
    die(error_msg); 
  } 
}

/*-------------------------------------------------------------------------------------*/
static void check_ip_addr(char *pre_msg, char *ip_addr)
/*-------------------------------------------------------------------------------------*/
{
  if ( ip_addr == NULL )
  { 
    sprintf(error_msg, "%s: invalid socket ip address", pre_msg);
    die(error_msg);
  } 
}

/*-------------------------------------------------------------------------------------*/
static void check_port(char *pre_msg, int port)
/*-------------------------------------------------------------------------------------*/
{
  if ( (port < MIN_PORT) || (port > MAX_PORT) )
  {
    sprintf(error_msg, "%s: invalid socket port", pre_msg);
    die(error_msg);
  } 
}