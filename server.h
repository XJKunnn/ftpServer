#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>

#define MAXSIZE 1024
#define PORT 8000
#define DATA_PORT 9000

int sock_create(int port);

int sock_accept(int sock_listen);

int sock_connect(int port, char* host);

int send_response_code(int sockfd, int code);
