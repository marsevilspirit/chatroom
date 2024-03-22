#include "server.h"
#include "../threadpool/threadpool.h"
#include "../message/message.h"

extern std::unordered_map<int, std::string> hashTable;

static std::string getIpAddress() 
{
    struct ifaddrs *ifAddrStruct = nullptr;
    struct ifaddrs *ifa = nullptr;
    void *tmpAddrPtr = nullptr;
    std::string ipAddress;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) 
    {
        if (!ifa->ifa_addr) 
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET) 
        {// IPv4地址
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

            // 检查是否是回环地址
            if (strcmp(addressBuffer, "127.0.0.1") != 0) 
            {
                ipAddress = std::string(addressBuffer);
                break;
            }
        }
    }

    if (ifAddrStruct != nullptr)
    {
        freeifaddrs(ifAddrStruct);
    }

    return ipAddress;
}

static void setFdNoblock(int fd)
{
    fcntl(fd, F_SETFL, fcntl(fd, F_SETFL) | O_NONBLOCK);
}

server::server(int port)
{
    epoll_fd = epoll_create1(0);
    if(epoll_fd == -1)
        throw std::runtime_error("Error in epoll_create1");

    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket == -1)
        throw std::runtime_error("Error in socket");

    int reuse = 1;
    if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1)
        throw std::runtime_error("Error in setsocketopt");

    if(bind(server_socket, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1)
        throw std::runtime_error("Error in bind");

    if(listen(server_socket, SOMAXCONN) == -1)
        throw std::runtime_error("Error in listen");

    setFdNoblock(server_socket);//使server_socket不堵塞

    std::cout << "server start on " << getIpAddress() << ':' << port << '\n';

    connect = sql_init(connect);
}

server::~server()
{
    close(server_socket);
    close(epoll_fd);
    mysql_close(connect);
}

void server::handleClientConnect(int client_socket)//没写
{
    printf("success!!!\n");
}

void server::handleReceivedMessage(int client_socket)
{
    Type flag;
    char* msg = nullptr;
    int ret = recvMsg(client_socket, &msg, &flag);
    if(ret == -1)
    {
        std::cerr << "Failed to receive message from client_socket: " << client_socket << '\n';
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
        auto it = std::find(client_sockets.begin(), client_sockets.end(), client_socket);
        if (it != client_sockets.end()) 
        {
            client_sockets.erase(it); // 从客户端套接字集合中移除该套接字
        }

        update_online_status(connect, hashTable[client_socket].c_str(), 0);

        close(client_socket);
    }
    else if (ret == 0)
    {
        // 客户端断开连接
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
        std::cout << "Client disconnected: " << client_socket << '\n';
        auto it = std::find(client_sockets.begin(), client_sockets.end(), client_socket);
        if (it != client_sockets.end()) 
        {
            client_sockets.erase(it); // 从客户端套接字集合中移除该套接字
        }

        update_online_status(connect, hashTable[client_socket].c_str(), 0);

        close(client_socket);
    }
    else
    {
        switch (flag)
        {
            case GROUP_MESSAGE: handleGroupMessage(msg, client_socket, client_sockets); break;
            case REGISTER:      ServerhandleRegister(msg, client_socket, connect);      break;
            case LOGIN:         ServerhandleLogin(msg, client_socket, connect);         break;
            case FORGET_PASSWD: ServerhandleForgetPasswd(msg, client_socket, connect);  break;
            case DELETE_ACCOUNT:ServerhandleDeleteAccount(msg, client_socket, connect); break;
            default:            std::cerr << "Unknown message type\n";                  break;
        }
    }
}

void server::run()
{
    struct epoll_event event{};
    event.events = EPOLLIN;
    event.data.fd = server_socket;

    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1)
        throw std::runtime_error("Error in epoll_ctl");

    threadpool threadPool(THREAD_NUM);

    epoll_event events[MAX_EVENTS];

    while(true)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if(num_events == -1)
            throw std::runtime_error("Error in epoll_wait");

        for(int i = 0; i < num_events; i++)
        {
            if(events[i].data.fd == server_socket)//server_socket有事，说明有人要连咱们
            {
                std::cout << "accepting new client...\n";
                int client_socket = accept(server_socket, nullptr, nullptr);
                if(client_socket == -1)
                {
                    std::cerr << "Failed to accept client connection\n";
                    continue;
                }
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_socket;
                setFdNoblock(client_socket);//不让客户端堵塞

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);
                  
                std::cout << "client on line client_socket: " << client_socket << '\n';
                client_sockets.push_back(client_socket);
                threadPool.enqueue(&server::handleClientConnect, this, client_socket);//---------------to do
            }
            else//不是server_socket有事
            {
                if (events[i].events & EPOLLERR || events[i].events & EPOLLHUP)//处理错误，但是我一次都没有看到它触发过,ok,后来触发了一次，我很满意
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, nullptr);
                    auto it = std::find(client_sockets.begin(), client_sockets.end(), events[i].data.fd);
                    if (it != client_sockets.end()) 
                    {
                        client_sockets.erase(it); // 从客户端套接字集合中移除该套接字
                    }
                    std::cout << "Client disconnected by EPOLLERR | EPOLLHUP: " << events[i].data.fd << '\n';

                    update_online_status(connect, hashTable[events[i].data.fd].c_str(), 0);

                    close(events[i].data.fd);
                }
                else if(events[i].events & EPOLLIN)//接收信息
                {
                    int fd = events[i].data.fd;
                    threadPool.enqueue(&server::handleReceivedMessage, this, fd);
                }
                else if(events[i].events & EPOLLOUT)//发送信息
                {
                    //todo-------------------------
                    std::cout << "EPOLLOUT\n";
                }
            }
        }
    }
}
