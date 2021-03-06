#include "server.h"

void ser_process(int sockfd);

int data_accept(int data_port);
int create_data_port(int sock_control);
int server_recv_cmd(int sock_control);
void file_to_client(int sock_data, int sock_control, char* filename);
void file_to_server(int sock_data, int sock_control);
int getFileSize(char *filename);
int fileList(int sock_control, int sock_data);

int main(int argc, char* argv[]){
	int sock_listen;	//listen socket in father process
	int sock_control;	//control socket for client connection
	int port;
	int pid;
    //xixi
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

void ser_process(int sock_control){
	int sock_data;
	int data_listen;
	int data_port = 9000;					//set random
	int cmd = 0, detail = 0;

	send_response_code(sock_control, 220);	//send welcome code
	
	while(1){
		if((cmd = server_recv_cmd(sock_control)) < 0){
			perror("get cmd error");
			continue;
		}
		if(cmd == 280){		//data request
			data_listen = create_data_port(data_port);
			if(send_response_code(sock_control,data_port) < 0){
				perror("send data port error");
				exit(1);
			}
			sock_data = data_accept(data_listen);
			send_response_code(sock_data, 626);		//sock_data accept code
			if((detail = server_recv_cmd(sock_control)) < 0){
				perror("get detail failed");
				exit(1);
			}
			if(detail == 282){
				char filename[128];
				if((recv(sock_control, &filename, sizeof(filename), 0)) < 0){
					perror("get filename failed");
					exit(1);
				}
				file_to_client(sock_data, sock_control, filename);
				send_response_code(sock_control, 383);
			} else if(detail == 284){	//recv file from client
			
				file_to_server(sock_data, sock_control);
			
			}else if(detail == 286){
				//send the current direction info
				fileList(sock_control, sock_data);

			} else {
				send_response_code(sock_control, 300);	//bad detail code
			}
			close(sock_data);
		} else if(cmd == 1000){
			//quit
			break;
		} else {
			break;
		}
	}
	close(data_listen);
	close(sock_control);
	printf("child process end...\n");
	return ;
}

//get server filelist
int fileList(int sock_control, int sock_data){
	char data[MAXSIZE];
	size_t num_read;
	FILE* fd;

	int rs = system("ls -l ../ftpFile/ | grep ^[^d] | awk '{print $9}' > ../ftpFile/.filelist");
	if(rs < 0){
		printf("system shell fail\n");
		return -1;
	}

	fd = fopen("../ftpFile/.filelist","r");
	if(!fd){
		printf("open .filelist error\n");
		return -1;
	}

	fseek(fd, SEEK_SET, 0);

	send_response_code(sock_control, 600);

	memset(data, 0 ,MAXSIZE);
	while((num_read = fread(data, 1, MAXSIZE, fd)) > 0){
		if(send(sock_data, data, num_read, 0) < 0){
			perror("error in send .filelist");
			return -1;
		}
		memset(data, 0 ,MAXSIZE);
	}

	fclose(fd);
	server_recv_cmd(sock_control);

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
		perror("send_reponse_code error");
		return -1;
	} 
	return 0;
}



int server_recv_cmd(int sock_control){
	int command;

	if(recv(sock_control, &command, sizeof(command), 0) < 0){
		perror("recv command error");
		return -1;
	}

	command = ntohl(command);
	printf("[cmd from client]:%d\n",command);
	return command;
}

int create_data_port(int data_port){
	int data_listen;

	if((data_listen = sock_create(data_port)) < 0){
		perror("data_listen create error");
		return -1;
	}

	return data_listen;
}

int data_accept(int data_listen){
	int sock_data;

	if((sock_data = sock_accept(data_listen)) < 0){
		perror("sock_data accept error");
		return -1;
	} else {
		printf("sock_data create succeed\n");
	}

	return sock_data;
}

void file_to_client(int sock_data, int sock_control, char* filename){
	printf("in file to client\n");
	int count = 0;
	for(int i = 0; i < 128; i++){
		if(filename[i] != ';'){
			count++;
		} else{
			break;
		}
	}
	char real_name[count];
	strncpy(real_name, filename, count);
	real_name[count] = '\0';
	printf("%s\n", real_name);

	char url[11 + count];
	strcpy(url, "../ftpFile/");
	strcat(url, real_name);

	FILE* fd = NULL;
	char data[MAXSIZE];
	size_t num_read;

	int filesize = getFileSize(url);
	send_response_code(sock_control, filesize);		//send file size

	fd = fopen(url,"r");
	int percent = 0, sum = 0, res = filesize;

	if(!fd){
		send_response_code(sock_control, 283);		//file not exist
	} else {
		send_response_code(sock_control, 382);		//file ready
		printf("already send:  0%%");
		/*
		do{
			num_read = fread(data, 1, MAXSIZE, fd);
			if(num_read < 0){
				printf("error in fread");
			}

			if(send(sock_data, data, num_read, 0) < 0){
				perror("send file error");
			}
			sum += num_read;
			res -= num_read;
			percent = sum * 100 / filesize;
			if(percent < 10){
				printf("\b\b");
			} else {
				printf("\b\b\b");
			}
			printf("%d%%", percent);
		} while(num_read > 0 && res > 0);
		*/
		int loop = filesize / MAXSIZE;
		for(int j=0; j<loop; j++){
			num_read = fread(data, 1, MAXSIZE, fd);
			if(send(sock_data, data, num_read, 0) < 0){
				perror("send file error");
			}
			sum += num_read;
			res -= num_read;
			printf("send sum:%d\n", sum);
		}
		num_read = fread(data, 1, res, fd);
		if(send(sock_data, data, res, 0) < 0){
			perror("send res error");
		}
		printf("send res:%d\n", res);
		printf("finish send\n");
		fclose(fd);
		server_recv_cmd(sock_control);
		printf("\nsend fd closed\n");
	}
}

void file_to_server(int sock_data, int sock_control){
	char filename[128];
	if(recv(sock_control, &filename, sizeof(filename), 0) < 0){
		perror("error in unload get filename");
		return;
	}
	int count = 0;
	for(int i = 0; i < 128; i++){
		if(filename[i] != ';'){
			count++;
		} else {
			break;
		}
	}
	char real_name[count];
	strncpy(real_name,filename, count);
	real_name[count] = '\0';
	
	printf("filename:%s\n", real_name);

	send_response_code(sock_control, 285);	//server recv ready

	int filesize = server_recv_cmd(sock_control);
	
	printf("filesize:%d\n", filesize);

	
	char url[11 + count];
	strcpy(url, "../ftpFile/");
	strcat(url, real_name);
	FILE* fd = fopen(url, "w");
	int size;
	char* data[MAXSIZE];
	int res = filesize;
	int percent;
	
	printf("file accept:  0%%");
	while(res > 0){
		if((size = recv(sock_data, &data, sizeof(data), 0)) > 0){
			fwrite(data, 1, size, fd);
			//printf("size:%d\n", size);
		}
		res = res - size;
		percent = (filesize - res) * 100 / filesize;
		if(percent < 10){
			printf("\b\b");
		} else {
			printf("\b\b\b");
		}
		printf("%d%%", percent);
	}
	

	if(size < 0){
		perror("size error in upload file");
	}

	send_response_code(sock_control, 385);
	//printf("fd not close\n");
	fclose(fd);
	printf("\nfile ac succeed\n");
	//printf("fd closed\n");
	//send_response_code(sock_control, 385);	//server recv succeed
}

int getFileSize(char *filename){
	FILE *fp = fopen(filename,"r");

	if(!fp){
		return -1;
	} 
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fclose(fp);
	return size;
}

