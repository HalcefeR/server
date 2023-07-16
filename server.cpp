#include <stdio.h>
#include <string.h> //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h> //close
#include <arpa/inet.h> //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros
#include <iostream>

#define PORT 8888

class Server {
private:
	int max_clients; // maximum number of clients
private:
	int master_socket; // the main socket
	int client_socket[30];
	struct sockaddr_in address;
	fd_set readfds; // set of socket descriptors
public:
	Server(int max_clients) {
		this->max_clients = max_clients;
		for (int client_number = 0; client_number < max_clients; client_number++)
			client_socket[client_number] = 0; // initializing client sockets

		if((master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0)
			raise_error(1001);

		int option_data = 1;
		if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&option_data, sizeof(option_data)) < 0)
			raise_error(1002);

		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(PORT);
		if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
			raise_error(1003);
		if (listen(master_socket, 3) < 0)
			raise_error(1004);
		printf("Listener on port %d \n", PORT);
	}
	int start() {
		int socket_descriptor, max_socket_descriptor, activity, client_number, new_socket;
		int addrlen = sizeof(address);
		char message[32] = "ECHO Daemon v1.0 \r\n";
		char buffer[1024];
		int message_size;
		while(true) {
		    FD_ZERO(&readfds);
		    FD_SET(master_socket, &readfds);
		    max_socket_descriptor = master_socket; // the largest number of current sockets descriptors; argument of the select() function

			// add child sockets to set
			for (client_number = 0; client_number < max_clients; client_number++) {
				if(client_socket[client_number] > 0)
					FD_SET(client_socket[client_number], &readfds);
				if(client_socket[client_number] > max_socket_descriptor)
					max_socket_descriptor = client_socket[client_number];
			}

			// waiting for an activity on one of the sockets, timeout is NULL, so wait indefinitely
			activity = select(max_socket_descriptor + 1, &readfds, NULL, NULL, NULL);
			if ((activity < 0) && (errno!=EINTR))
				raise_error(1004);
			// if something happened on the master socket, then its an incoming connection
			if (FD_ISSET(master_socket, &readfds))
			{
				if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
					raise_error(1005);
				// inform user of socket number - used in send and receive commands
				printf("New connection, socket fd is %d, ip is : %s, port : %d \n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));
				// send new connection greeting message
				if(send(new_socket, message, strlen(message), 0) != strlen(message))
					raise_error(1006);
				puts("Welcome message sent successfully");

				// add new socket to array of sockets
				for (client_number = 0; client_number < max_clients; client_number++) {
					// if position is empty
					if(client_socket[client_number] == 0) {
						client_socket[client_number] = new_socket;
						printf("Adding to list of sockets as %d\n", client_number);
						break;
					}
				}
			}

			for (client_number = 0; client_number < max_clients; client_number++) {
				if (FD_ISSET(client_socket[client_number], &readfds)) {
					// check if it was for closing , and also read the
					// incoming message
					if ((message_size = read(client_socket[client_number], buffer, 1024)) == 0) { // !!!!
						// somebody disconnected , get his details and print
						getpeername(client_socket[client_number], (struct sockaddr*)&address, (socklen_t*)&addrlen);
						printf("Host disconnected , ip %s , port %d \n", inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
						// close the socket and mark as 0 in list for reuse
						close(client_socket[client_number]);
						client_socket[client_number] = 0;
					}
					// echo back the message that came in
					else {
						// set the string terminating NULL byte on the end
						// of the data read
						buffer[message_size] = '\0';
						send(client_socket[client_number], buffer, strlen(buffer), 0);
					}
				}
			}
		}
	}
	int print() {
		std::cout << max_clients << std::endl;
		return 0;
	}
private:
	void raise_error(int error_code = 0) {
		switch(error_code) {
			case 1001:
				perror("Master socket initializing failed");
				exit(EXIT_FAILURE);
				break;
			case 1002:
				perror("Setting options, associated with the master socket, failed");
				exit(EXIT_FAILURE);
				break;
			case 1003:
				perror("Master socket binding failed");
				exit(EXIT_FAILURE);
				break;
			case 1004:
				perror("Function listen() on the master socket caused an error");
				exit(EXIT_FAILURE);
				break;
			case 1005:
				perror("Function select() caused an error");
				exit(EXIT_FAILURE);
				break;
			case 1006:
				perror("Attempt to accept the master socket caused an error");
				exit(EXIT_FAILURE);
				break;
			case 1007:
				perror("Attempt to send message caused an error");
				exit(EXIT_FAILURE);
				break;
			default:
				perror("Error");
				exit(EXIT_FAILURE);
				break;
		}
	}
};

int main(int argc , char *argv[])
{
	Server my_server(2);
	my_server.start();
	return 0;
}
