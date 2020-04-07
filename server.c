#include "server.h"

void ser_process(int sockfd);
int server_data_conn(int sock_control);

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
	}

	//listen process
	while(1) {
		//wait for client request
		//create control socket for a client request
		if((sock_control = sock_accept(sock_listen)) < 0){
			printf("create control socket error\n");
			break;
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
	struct sockaddr_in dest_addr;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("socket in sock_connect error\n");
		return -1;
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htonl(port);
	dest_addr.sin_addr.s_addr = inet_addr(host);

	if(connect(sockfd, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0){
		perror("error in connect to dest");
		return -1;
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
	
	server_data_conn(sock_control);
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

	//get client addr
	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);
	getpeername(sock_control, (struct sockaddr*)&client_addr, &len);
	inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf));
	printf("buf info:%s,%d\n", buf, ntohs(client_addr.sin_port));

	//init data conn with client 
	if((sock_data = sock_connect(DATA_PORT, buf)) < 0){
		printf("sock_data conn error\n");
		return -1;
	}

	return sock_data;
}
