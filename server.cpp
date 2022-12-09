#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <memory.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <vector>
#include <unordered_map>

#define ERR(a) {perror(a); return EXIT_FAILURE;}

using namespace std;

typedef struct request {
    int conn_fd;
    int filesize;
    string username;
} request;

typedef struct server {
    string hostname;
    unsigned short port;
    int listen_fd;
} server;

server http_svr;
request *req_arr;

static void init_server(unsigned short port) {
    int tmp;

    gethostname(process_buf, sizeof(process_buf));
    httpsvr.hostname = (string)process_buf;
    httpsvr.port = port;
    httpsvr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    welcome_fd = httpsvr.listen_fd;

    if(welcome_fd < 0) {
        ERR_EXIT("socket");
    }
    bzero(&httpSvrAddr, sizeof(httpSvrAddr));
    httpSvrAddr.sin_family = AF_INET;
    httpSvrAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    httpSvrAddr.sin_port = htons((unsigned short)port);

    tmp = 1;
    if(setsockopt(welcome_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if(bind(welcome_fd, (struct sockaddr*)&httpSvrAddr, sizeof(httpSvrAddr))) {
        ERR_EXIT("bind");
    }
    if(listen(welcome_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    cout << "HTTP server listen on " << httpsvr.hostname << ':' << httpsvr.port << ", fd: " << welcome_fd << '\n';
}

int main(int argc, char *argv) {
    if(argc != 2 || argc != 3) {
        fprintf(stderr, "usage: %s [SERVER_PORT] [OPTIONAL]--init\n", argv[0]);
        exit(1);
    }

    string arg1 = (string)argv[1];
    struct sockaddr_in tcpServerAddr, client;

    // check if --init flag is on, if so initialize the database

    // 
}
