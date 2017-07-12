#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include "mongoose.h"
#include "my_macros.h"
#include "gateway.h"


// static const char *html_form =
  // "<html>"
  // "<body>SENSOR GATEWAY CONFIGURATION <br>"
  // "<form method=\"POST\" action=\"/handle_post_request\">"
  // "Port Number : <input type=\"text\" name=\"port_num\" /> <br>"
  // "Database : "
  // "<form>"
  // "<select name=\"dropdown\"> <br>"
  // "<option value = \"MYSQL\" selected>MYSQL</option>"
  // "<option value = \"SQLITE\" >SQLITE</option>"
  // "</selected>"
  // "</form>" 
  // "<p>Name : </p><input type=\"text\" name=\"db_name\"> <br>"
  // "<input type=\"submit\"/>"
  // "</form></body></html>";
static const char *html_form =  
"<html>"
"<body> SENSOR GATEWAY CONFIGURATION <br>"
"<br>"
"<form method=\"POST\" action=\"/handle_post_request\">"
"PORT NUMBER:   <tab id=t1> <input type=\"text\" name=\"port_num\"> <br>"
"<br>"
"DATABASE: <tab to=t1> <select name=\"dropdown\"> <option value=\"MYSQL\" selected>MYSQL</option> <option value=\"SQLITE\">SQLITE</option> </select>    NAME: <input type=\"text\" name=\"db_name\"> <br>"
"<br>"
"<input type=\"submit\" name=\"bt_run\" value=\"RUN\">"
"</form>"
"</body>"
"</html>";

int sv_alive = 1;
pid_t my_pid, child_pid = 1;
char *argv[3];
  
static void send_reply(struct mg_connection *conn) {
  char port_num[500], dropdown[500];
  char db_name[50];
  
  if (strcmp(conn->uri, "/handle_post_request") == 0) {
    // User has submitted a form, show submitted data and a variable value
    // Parse form data. port_num and dropdown are guaranteed to be NUL-terminated
    mg_get_var(conn, "port_num", port_num, sizeof(port_num));
    mg_get_var(conn, "dropdown", dropdown, sizeof(dropdown));
    mg_get_var(conn, "db_name", db_name, sizeof(db_name));
	
    // Send reply to the client, showing submitted form values.
    // POST data is in conn->content, data length is in conn->content_len
    mg_send_header(conn, "Content-Type", "text/plain");
    mg_printf_data(conn,
                   "Submitted data: [%.*s]\n"
                   "Submitted data length: %d bytes\n"
                   "GW PORT: [%s]\n"
                   "Database: [%s] - name: [%s]\n",
                   conn->content_len, conn->content,
                   conn->content_len, port_num, dropdown, db_name);
	argv[0]	= " ";	
	asprintf(&argv[1], "PORT=%s", port_num);
	asprintf(&argv[2], "DB=%s", dropdown);		
	child_pid = fork();
	if(child_pid < 0) {
		printf("Error: fork");
		exit(0);
	}
  } else {
    // Show HTML form.chi
    mg_send_data(conn, html_form, strlen(html_form));
  }
}

static int ev_handler(struct mg_connection *conn, enum mg_event ev) {
  if (ev == MG_REQUEST) {
    send_reply(conn);
    return MG_TRUE;
  } else if (ev == MG_AUTH) {
    return MG_TRUE;
  } else {
    return MG_FALSE;
  }
}

void run_child(int exit_code) {
	do {
		run_gateway(3, argv);
		sv_alive = 0;
	} while(sv_alive);
	exit( exit_code );
}

int main(void) {
	struct mg_server *server = mg_create_server(NULL, ev_handler);
	
	
	my_pid = getpid();
	printf("Main process (pid = %d) is started ...\n", my_pid);
	
	mg_set_option(server, "listening_port", "8080");
	printf("Use Web browser to config the gateway server at URL: 127.0.0.1:8080\n");
	printf("Web server is starting on port: %s\n", mg_get_option(server, "listening_port"));
	
	// child_pid = fork();
	// SYSCALL_ERROR(child_pid, "fork");
	if ( child_pid == 0  ) {  
		run_child( 0 );
	}
	else {
		for (;;) {
			mg_poll_server(server, 1000);
			if(sv_alive == 0) break;
		}
		waitpid(child_pid, NULL, 0);
		printf("Main process (pid = %d) is terminating ...\n", my_pid);
	}

  mg_destroy_server(&server);

  return 0;
}  
