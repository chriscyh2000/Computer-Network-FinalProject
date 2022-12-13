#include <arpa/inet.h>
// #include <dirent.h>
#include <poll.h>
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
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

#define ERR_EXIT(a) do { perror(a); exit(1); } while(0)
#define MSGMAX 10000

using namespace std;
using namespace std::__fs::filesystem;

#define LOGINLEN 16
#define REGISTERLEN 24
#define BOARDLEN 7

char process_buf[MSGMAX + 10];

typedef struct package {
    int type, buf_size;
    char sender[256];
    char password[28];
    char reqpath[256];
    char message[256];
    char buf[2048];
    time_t Time;
} package;

typedef struct request {
    int conn_fd;
    package pkg;
} request;

typedef struct server {
    string hostname;
    unsigned short port;
    int listen_fd;
} server;

server tcpsvr;
struct sockaddr_in frontendAddr, backendAddr;
int backend_fd, maxfd;
int client_len;

request *request_array;

static void init_server(unsigned short port) {
    int tmp;

    gethostname(process_buf, sizeof(process_buf));
    // tcpsvr.hostname = (string)process_buf;
    // tcpsvr.port = port;
    backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    // tcpsvr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    // backend_fd = httpsvr.listen_fd;

    if(backend_fd < 0) {
        ERR_EXIT("socket failed");
    }
    bzero(&backendAddr, sizeof(backendAddr));
    backendAddr.sin_family = AF_INET;
    backendAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    backendAddr.sin_port = htons((unsigned short)port);

    tmp = 1;
    if(setsockopt(backend_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if(::bind(backend_fd, (struct sockaddr*)&backendAddr, sizeof(backendAddr))) {
        ERR_EXIT("bind");
    }
    if(listen(backend_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    cout << "TCP server listen on port: " << port << ", fd: " << backend_fd << '\n';

    maxfd = getdtablesize();
    request_array = (request *)malloc(sizeof(request) * maxfd);
}

void init_database() {
    remove_all("database");
    create_directory("database");

    ofstream user_info("./database/user_passwd.txt");
    user_info << "";
    user_info.close();

    ofstream comments("./database/comments.txt");
    comments << "";
    comments.close();
}

static void init_request(request *reqP) {
    reqP->conn_fd = -1;
    memset(&(reqP->pkg), 0, sizeof(package));
}

static void free_request(request *reqP) {
    /*if (reqP->filename != NULL) {
        free(reqP->filename);
        reqP->filename = NULL;
    }*/
    init_request(reqP);
}

static int check_connection(struct pollfd *fdarray, int &i, nfds_t &nfds) {
    int error_code;
    int error_code_size = sizeof(error_code);
    getsockopt(fdarray[i].fd, SOL_SOCKET, SO_ERROR, &error_code, (socklen_t *)&error_code_size);
    return error_code;
}

static int read_package(int fd) {
    int bytes = 0, tmp;
    memset(&(request_array[fd].pkg), 0, sizeof(package));
    while(bytes < sizeof(package)) {
        tmp = read(fd, &(request_array[fd].pkg), sizeof(package));
        if(tmp <= 0) {
            if(errno == EAGAIN) {
                continue;
            }
            return 0;
        }
        bytes += tmp;
    }
    return bytes;
}

/* Homwpage Error Detection */
static void homepage_message(string &response, string error, int line_num) {
    string data;
    fstream data_file;
    data_file.open("./template/index.html");
    for(int i = 1; i <= line_num; ++i) {
        getline(data_file, data);
        response.append(data + "\n");
    }
    response.append("<h4>" + error + "<h4>\n");
    while(getline(data_file, data)) {
        response.append(data + "\n");
    }
}

static pair<bool, string> find_user(string &username) {
    string data;
    fstream data_file;
    data_file.open("./database/user_passwd.txt");
    while(getline(data_file, data)) {
        int sep = data.find(",");
        if(data.substr(0, sep) == username) {
            return {1, data.substr(sep + 1)};
        }
    }
    return {0, ""};
}

static void load_comments(string &response) {
    string data;
    fstream template_file, comments_file;
    template_file.open("./template/board.html");
    for(int i = 1; i <= BOARDLEN; ++i) {
        getline(template_file, data);
        response.append(data + "\n");
    }
    comments_file.open("./database/comments.txt");
    while(getline(comments_file, data)) {
        int sep = data.find(",");
        response.append("<p>[" + data.substr(0, sep) + "]: " + data.substr(sep + 1) + "\n");
    }
    while(getline(template_file, data)) {
        response.append(data + "\n");
    }
}

int main(int argc, char *argv[]) {
    if(argc != 2 && argc != 3) {
        fprintf(stderr, "usage: %s [SERVER_PORT] [OPTIONAL]--init\n", argv[0]);
        exit(1);
    }

    string arg1 = (string)argv[1];
    int port = stoi(arg1);
    init_server(port);

    if(argc == 3 && (string)argv[2] == "--init") {
        init_database();
    }

    struct pollfd fdarray[maxfd];
    fdarray[0].fd = backend_fd;
    fdarray[0].events = POLLIN;
    nfds_t nfds = 1;
    int ready_fds = 0, client_len;

    char frontend_host[256];

    while(1) {
        if((ready_fds = poll(fdarray, nfds, -1)) < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                continue;
            }
        }
        for(int i = 1; i < nfds; ++i) { // we can threading here
            check_connection(fdarray, i, nfds);
            if(fdarray[i].revents & POLLIN) {
                if(read_package(fdarray[i].fd) <= 0 || check_connection(fdarray, i, nfds) > 0) {
                    close(fdarray[i].fd);
                    swap(fdarray[i--], fdarray[--nfds]);
                    continue;
                }

                string response;
                if(request_array[fdarray[i].fd].pkg.type == 0) {
                    homepage_message(response, "", LOGINLEN);
                    sprintf(request_array[fdarray[i].fd].pkg.buf, "%s", response.c_str());
                }
                else {
                    // cout << "req: " << (string)request_array[fdarray[i].fd].pkg.reqpath <<'\n';
                    if((string)request_array[fdarray[i].fd].pkg.reqpath == "/register") {
                        string username = (string)request_array[fdarray[i].fd].pkg.sender;
                        string password = (string)request_array[fdarray[i].fd].pkg.password;
                        memset(&(request_array[fdarray[i].fd].pkg), 0, sizeof(package));

                        pair<int, string> search_res;
                        if(username.empty() || password.empty()) {
                            homepage_message(response, "Username and Password cannot be empty.", REGISTERLEN);
                        }
                        else if((search_res = find_user(username)).first == 1) {
                            homepage_message(response, "The user name already exists.", REGISTERLEN);
                        }
                        else {
                            ofstream file;
                            file.open("./database/user_passwd.txt", ios_base::app);
                            file << username << "," << password << '\n';
                            file.close();
                            homepage_message(response, "Success.", REGISTERLEN);
                        }

                        sprintf(request_array[fdarray[i].fd].pkg.buf, "%s", response.c_str());
                    }
                    else if((string)request_array[fdarray[i].fd].pkg.reqpath == "/login") {
                        string username = (string)request_array[fdarray[i].fd].pkg.sender;
                        string password = (string)request_array[fdarray[i].fd].pkg.password;
                        memset(&(request_array[fdarray[i].fd].pkg), 0, sizeof(package));

                        // cout << username << " " << password << "\n";
                        pair<int, string> search_res;
                        if(username.empty() || password.empty()) {
                            homepage_message(response, "Username and Password cannot be empty.", LOGINLEN);
                        }
                        else if((search_res = find_user(username)).first == 0) {
                            homepage_message(response, "The username does not exist.", LOGINLEN);
                        }
                        else if(search_res.second != password) {
                            homepage_message(response, "The password is not correct.", LOGINLEN);
                        }
                        else {
                            sprintf(request_array[fdarray[i].fd].pkg.sender, "%s", username.c_str());
                            load_comments(response);
                        }
                        sprintf(request_array[fdarray[i].fd].pkg.buf, "%s", response.c_str());
                    }
                    else if((string)request_array[fdarray[i].fd].pkg.reqpath == "/comment") {
                        string username = (string)request_array[fdarray[i].fd].pkg.sender;
                        string message = (string)request_array[fdarray[i].fd].pkg.message;
                        memset(&(request_array[fdarray[i].fd].pkg), 0, sizeof(package));

                        if(username.empty()) {
                            homepage_message(response, "", LOGINLEN);
                        }
                        else {
                            ofstream file;
                            file.open("./database/comments.txt", ios_base::app);
                            file << username << "," << message << '\n';
                            file.close();
                            load_comments(response);
                        }

                        sprintf(request_array[fdarray[i].fd].pkg.buf, "%s", response.c_str());
                        sprintf(request_array[fdarray[i].fd].pkg.sender, "%s", username.c_str());
                    }
                    else if((string)request_array[fdarray[i].fd].pkg.reqpath == "/logout") {
                        homepage_message(response, "", LOGINLEN);
                        sprintf(request_array[fdarray[i].fd].pkg.buf, "%s", response.c_str());
                    }
                }
                fdarray[i].events = POLLOUT;
            }
            else if(fdarray[i].revents & POLLOUT) {
                // cout << "BUF:" << (string)(request_array[fdarray[i].fd].pkg.buf) << '\n';
                if(write(fdarray[i].fd, &(request_array[fdarray[i].fd].pkg), sizeof(package)) <= 0 || \
                    check_connection(fdarray, i, nfds) > 0) {

                    close(fdarray[i].fd);
                    swap(fdarray[i--], fdarray[--nfds]);
                    continue;
                }
                fdarray[i].events = POLLIN;
            }
        }
        if(fdarray[0].revents & POLLIN) {
            client_len = sizeof(frontendAddr);
            int conn_fd = accept(backend_fd, (struct sockaddr *)&frontendAddr, (socklen_t *)&client_len);

            if(conn_fd < 0) {
                if(errno == EINTR || errno == EAGAIN) {
                    continue;
                }
                if(errno == ENFILE) {
                    fprintf(stderr, "out of file descriptor table... (maxconn = %d)\n", maxfd);
                    continue;
                }
                ERR_EXIT("accept");
            }

            memset(frontend_host, 0, sizeof(frontend_host));
            strcpy(frontend_host, inet_ntoa(frontendAddr.sin_addr));
            printf("getting a new request... fd %d from %s\n", conn_fd, frontend_host);

            init_request(&(request_array[conn_fd]));
            request_array[conn_fd].conn_fd = conn_fd;
            fdarray[nfds].fd = conn_fd;
            fdarray[nfds++].events = POLLIN;
        }
    }
    return 0;
}
