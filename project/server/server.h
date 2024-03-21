#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <ifaddrs.h>
#include <netinet/in.h>
#include "../mysql/mysql.h"

#define MAX_EVENTS 1024

class server{
public:
    server(int port);
    ~server();

    void run();

    void handleClientConnect(int client_socket);

    void handleReceivedMessage(int client_socket);

private:
    int epoll_fd;
    int server_socket;
    std::vector<int> client_sockets;
    MYSQL* connect{};
};
