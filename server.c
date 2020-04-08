#include "server.h"

void ser_process(int sockfd);
int server_data_conn(int sock_control);
int data_conn(int sock_control);

int main(int argc, char* argv[]){
	int sock_listen;	//listen socket in father process
	int sock_control;	//control socket for client connection
	int port;
	int pid;

	if(argc != 2){
		printf("input the server port\n");
		exit(0);
	}

	port = atoi(argv[1]);

	//create listen socket
	if((sock_listen = sock_create(port)) < 0){
		printf("error in create listen socket\n");
		exit(1);
	} else{
		printf("sock_listen create succeed\n");
	}

	//listen process
	while(1) {
		//wait for client request
		//create control socket for a client request
		if((sock_control = sock_accept(sock_listen)) < 0){
			printf("create control socket error\n");
			break;
		} else{
			printf("sock_control create succeed\n");
		}

		//create child process to serve the client
		if((pid = fork()) < 0){
			printf("child process error\n");
		} else if(pid == 0){	//child process
			printf("into child process\n");
			close(sock_listen);
			ser_process(sock_control);
			close(sock_control);
			exit(0);
		}

		close(sock_control);
	}

	close(sock_listen);

	return 0;
}



//create socket of server
int sock_create(int port){
	int sockfd;
	int yes = 1;
	struct sockaddr_in sockaddr;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("sock_create() error\n");
		return -1;
	}

	//memset(&sockaddr, sizeof(sockaddr_in));
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);
	sockaddr.sin_addr.s_addr = htons(INADDR_ANY);

	if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		close(sockfd);
		printf("setsockopt() error\n");
		return -1;
	}

	//bind
	if(bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0){
		close(sockfd);
		printf("bind error\n");
		return -1;
	}

	//listen
	if(listen(sockfd, 5) < 0){
		close(sockfd);
		printf("listen error\n");
		return -1;
	}

	return sockfd;
}

//accept connection from client and return the sockfd created by listen socket
int sock_accept(int sock_listen){
	int sockfd;
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	sockfd = accept(sock_listen, (struct sockaddr*)&client_addr, &len);

	if(sockfd < 0){
		printf("accept error\n");
		return -1;
	}

	return sockfd;
}

//create data_socket to client
int sock_connect(int port, char *host){
	int sockfd;
	struct sockaddr_in client_addr;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("sockfd in sock_conn error\n");
		return -1;
	}

	bzero(&client_addr, sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = 18888;
	client_addr.sin_addr.s_addr = inet_addr("192.168.16.107");

	inet_pton(AF_INET, "192.168.16.107", &client_addr.sin_addr);

	if((connect(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr))) < 0){
		perror("conn in sock_conn");
		return -1;
	} else {
		printf("success");
	}

	return sockfd;
}

//sock_control send reponse_code to client's sock_control
/*
 *220: client coonect successfully
 *230: find the request file in current direction
 *430: coundn't find the request file
 *221: get "QUIT" command
 *200: 
 *226: "LIST" succeed or "RETR" succeed
 *550:
 */
int send_response_code(int sockfd, int code){
	int con_code = htonl(code);
	if(send(sockfd, &con_code, sizeof(con_code), 0) < 0){
		printf("send_reponse_code error\n");
		return -1;
	} else {
		printf("send success\n");
	}
	return 0;
}

void ser_process(int sock_control){
	int sock_data;
	
	send_response_code(sock_control, 220);
	
	sock_data = server_data_conn(sock_control);
	int rd;
	if(recv(sock_data, &rd, sizeof(rd), 0) < 0){
		perror("data recv");
	}
	send_response_code(sock_data, 626);
}

int server_data_conn(int sock_control){
	char buf[1024];
	int wait;
	int sock_data;

	//wait for the client ack
	if(recv(sock_control, &wait, sizeof(wait), 0) < 0){
		printf("error in wait ack\n");
		return -1;
	} else {
		printf("get ack: %d\n", wait);
	}
	
	if((sock_data = data_conn(sock_control)) < 0){
		perror("sock_data = data_conn");
		return -1;
	}
	
	return sock_data;
}

int data_conn(int sock_control){
	int data_listen;
	int sock_data;

	if((data_listen = sock_create(18888)) < 0){
		perror("datalisten socket error");
		return -1;
	}

	send_response_code(sock_control, 221);

	if((sock_data = sock_accept(data_listen)) < 0){
		perror("datalisten accept error");
		return -1;
	}

	return sock_data;
}


