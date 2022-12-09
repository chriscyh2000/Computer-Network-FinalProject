#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
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

#define HEADER_MAX 8192

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)

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

char process_buf[HEADER_MAX+10];

server httpsvr;
struct sockaddr_in httpSvrAddr, tcpSvrAddr, clientAddr;
int welcome_fd, connect2svr_fd, client_fd;
int client_len;
unordered_map<string, string> req_map;

static void init_server(int port, string tcpSvrIP, int tcpSvrPort) {
    int tmp;

    gethostname(process_buf, sizeof(process_buf));
    // httpsvr.hostname = (string)process_buf;
    // httpsvr.port = port;
    welcome_fd = socket(AF_INET, SOCK_STREAM, 0);

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

    cout << "HTTP server listen on port: " << httpsvr.port << ", fd: " << welcome_fd << '\n';

    // tcp client connecting to server
    // connect2svr_fd = socket(AF_INET, SOCK_STREAM, 0);
    // if(connect2svr_fd < 0) {
    //     ERR_EXIT("socket");
    // }
    // if(inet_addr(tcpSvrIP.c_str()) < 0) {
    //     fprintf(stderr, "server ip should be a valid ip.\n");
    //     exit(1);
    // }
    // if(tcpSvrPort < 0 || tcpSvrPort > 65535) {
    //     fprintf(stderr, "server port should be a number between 0 to 65535.\n");
    //     exit(1);
    // }
    // bzero(&tcpSvrAddr, sizeof(tcpSvrAddr));
    // tcpSvrAddr.sin_family = AF_INET;
    // tcpSvrAddr.sin_addr.s_addr = inet_addr(tcpSvrIP.c_str());
    // tcpSvrAddr.sin_port = htons((unsigned short)tcpSvrPort);

    // tmp = 1;
    // if(setsockopt(connect2svr_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
    //     ERR_EXIT("setsockopt");
    // }
    // if(connect(connect2svr_fd, (struct sockaddr*)&tcpSvrAddr, sizeof(tcpSvrAddr)) < 0) {
    //     ERR_EXIT("connect");
    // }

    // cout << "connected to " + tcpSvrIP << '\n';
}

string set_200_header(string file_type, string extra_field = "", int body_len = 0) {
    string res_header;
    res_header.append("HTTP/1.1 200 OK\r\n");
    res_header.append("Content-Length: " + to_string(body_len) + "\r\n");
    res_header.append("Content-Type: " + file_type + "\r\n");
    res_header.append("Connection: keep-alive\r\n");
    res_header.append(extra_field + "\r\n");
    return res_header;
} 

int read_httpreq() {
    memset(process_buf, 0, sizeof(process_buf));
    req_map = {};
    ssize_t bytes = read(client_fd, process_buf, HEADER_MAX);
    if(bytes <= 0) return bytes;
    string req = (string)process_buf;

    // check if "\r\n\r\n" in req string
    int eoh;
    if((eoh = req.find("\r\n\r\n")) == string::npos) {
        fprintf(stderr, "did not read a complete http request\n");
        return -1;
    }

    // cout << "\nreq: \n" << req <<'\n';
    // seperate header and body
    string header = req.substr(0, eoh), subheader = header, body = req.substr(eoh + 4), tmpstr;
    eoh += 4;

    int sz, sep;
    sz = subheader.find("\r\n");
    tmpstr = subheader.substr(0, sz);
    subheader = subheader.substr(sz + 2);

    // method
    sz = tmpstr.find(" ");
    req_map["method"] = tmpstr.substr(0, sz);
    cout << "method: " << req_map["method"]  << '\n';
    tmpstr = tmpstr.substr(sz + 1);
    // reqpath
    sz = tmpstr.find(" ");
    req_map["reqpath"] = tmpstr.substr(0, sz);
    cout << "reqpath: " << req_map["reqpath"]  << '\n';

    // by traversing the header, split each field out and store it into a map
    while((sz = subheader.find("\r\n")) != string::npos) {
        tmpstr = subheader.substr(0, sz);
        subheader = subheader.substr(sz + 2);
        sep = tmpstr.find(": ");
        req_map[tmpstr.substr(0, sep)] = tmpstr.substr(sep + 2);
        cout << tmpstr.substr(0, sep) + ": " + tmpstr.substr(sep + 2) + "\n";
    }

    // get the last field in request
    sep = subheader.find(": ");
    req_map[subheader.substr(0, sep)] = subheader.substr(sep + 2);
    cout << subheader.substr(0, sep) + ": " + subheader.substr(sep + 2) + "\n";

    return 1;
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        fprintf(stderr, "usage: %s [SERVER_ADDR]:[SERVER_PORT] [CLIENT_PORT]\n", argv[0]);
        exit(1);
    }

    string arg1 = (string)argv[1];
    if(arg1.find(':') == string::npos) {
        fprintf(stderr, "usage: %s [SERVER_ADDR]:[SERVER_PORT] [CLIENT_PORT]\n", argv[0]);
        exit(1);
    }
    string tcpSvrIP = arg1.substr(0, arg1.find(':')), tcpSvrPortStr = arg1.substr(arg1.find(':') + 1);
    string myPortStr = (string)argv[2];

    cout << tcpSvrIP << ' ' << tcpSvrPortStr << '\n';

    int tcpSvrPort = stoi(tcpSvrPortStr), myPort = stoi(myPortStr);
    init_server(myPort, tcpSvrIP, tcpSvrPort);
    
    while(1) {
        client_len = sizeof(clientAddr);
        client_fd = -1;
        if((client_fd = accept(welcome_fd, (struct sockaddr*)&clientAddr, (socklen_t *)&client_len)) < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;  // try again
            if (errno == ENFILE) {
                fprintf(stderr, "out of file descriptor table\n");
                continue;
            }
            ERR_EXIT("accept");
        }

        // read the fd when it is ready to read
        struct pollfd pfd[2];
        pfd[0].events = POLLIN;
        pfd[0].fd = client_fd;

        if(poll(pfd, (nfds_t)1, -1) < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                continue;
            }
        }
        if(read_httpreq() < 0) {
            fprintf(stderr, "read error\n");
        } 
        else {
            
        }
        close(client_fd);
    }
    return 0;
}