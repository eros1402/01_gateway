/*******************************************************************************
*  FILENAME: tcp_comm.h							       
*
* Version V1.0		
* Author: Luc Vandeurzen
*
* A simple-to-use TCP client/server API 
*									    
*******************************************************************************/

#ifndef __TCP_COMM_H__
#define __TCP_COMM_H__

#define	ERROR	-1
#define MAX_PENDING 10

typedef void * Socket;

Socket tcp_passive_open(int port);
/* Creates a new TCP socket and opens the socket in 'passive listening mode' (waiting for an 
active connection setup request). The socket is bound to port number 'port' and to any 
IP address of the PC. The number of pending connection setup requests is set to the 
maximum. The newly created socket is return as the function result. This function is 
typically called by a server. */


Socket tcp_active_open(int remote_port, char *remote_ip );
/* Creates a new TCP socket and opens a TCP connection to a socket on the PC with IP address 
'remote_ip' on port 'remote_port'. The IP address that the host PC has to use to connect 
to the remote PC, is not defined (it is left to the system to choose an available IP interface). 
The newly created socket is return as the function result. This function is typically 
called by a client. */


Socket tcp_wait_for_connection( Socket socket );
/* Puts the socket 'socket' in a blocking waiting mode. The function will only return 
when an incoming TCP connection setup request is received. The remote socket (the 
remote IP address and port number) that initiated the connection request is returned 
as the function result.  */


void tcp_close( Socket *socket );
/* The socket 'socket' is closed and any allocated resources are freed. */


void tcp_send(Socket socket, void *buffer, int bufsize );
/* The function initiates a send command on the socket 'socket'. Recall that the 
send command might block. In total 'bufsize' bytes of data in 'buffer' will be send. 
Of course, the socket 'socket' needs to be connected to a remote socket before this 
function can be called.  */

int tcp_receive (Socket socket, void* buffer, int bufsize);
/* The function initiates a receive command on the socket 'socket'. Recall that the 
receive command might block. Pending data in 'socket' will be copied to 'buffer'. The 
function returns the number of bytes that were really copied to 'buffer', which might 
be less than 'bufsize'. No more than 'bufsize' bytes however will be copied. Of course, 
the socket 'socket' needs to be connected to a remote socket before this function can 
be called.*/


char * get_ip_addr( Socket socket );
/* Returns the IP address to which the socket 'socket' is bound. The IP address is 
returned as a string. */

int get_port( Socket socket ); 
/* Returns the port number to which the socket 'socket' is bound. */

int get_socket_descriptor( Socket socket ); 
/* Returns the socket descriptor to which the socket 'socket' is bound. */


int get_sensor_id( Socket socket ); //update: 23 Jul 2015
/* Returns the sensor id to which the socket 'socket' is bound. */

void set_sensor_id( Socket socket, int id ); //update: 23 Jul 2015

#endif  //__TCP_COMM_H__
