#include "server.h"
//#include "../threadpool/threadpool.h"
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

server::server(int port): threadPool(THREAD_NUM)
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

/*
void server::handleClientConnect(int client_socket)//没写
{
}
*/

void server::handleReceivedMessage(int client_socket)
{
    Type flag;
    char* msg = nullptr;

    while(1)
    {
        int ret = recvMsg(client_socket, &msg, &flag);
        LogTrace("flag: {}, ret: {}", int(flag), ret);
        if(ret == -1)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // 已经读取完所有可用数据
                LogInfo("客户端{}读取完所有可用数据", client_socket);
                return;
            }

            LogInfo("(ret == -1)客户端{}断开连接", client_socket);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
            auto it = std::find(client_sockets.begin(), client_sockets.end(), client_socket);
            if (it != client_sockets.end()) 
            {
                client_sockets.erase(it); // 从客户端套接字集合中移除该套接字
            }

            update_online_status(connect, hashTable[client_socket].c_str(), 0);

            handleOffline(connect, client_socket);

            close(client_socket);

            return;
        }
        else if (ret == 0)
        {
            // 客户端断开连接
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, nullptr);
            LogInfo("(ret == 0)客户端{}断开连接", client_socket);
            auto it = std::find(client_sockets.begin(), client_sockets.end(), client_socket);
            if (it != client_sockets.end()) 
            {
                client_sockets.erase(it); // 从客户端套接字集合中移除该套接字
            }

            if (!hashTable[client_socket].empty())
            {
                update_online_status(connect, hashTable[client_socket].c_str(), 0);

                handleOffline(connect, client_socket);
            }

            close(client_socket);

            return;
        }
        else
        {
            switch (flag)
            {
                case WORLD_MESSAGE:         handleWorldMessage(connect, msg, client_socket, client_sockets);                     break;
                case REGISTER:              threadPool.enqueue(ServerhandleRegister, msg, client_socket, connect);               break;
                case LOGIN:                 ServerhandleLogin(msg, client_socket, connect);                                      break;
                case FORGET_PASSWD:         threadPool.enqueue(ServerhandleForgetPasswd, msg, client_socket, connect);           break;
                case DELETE_ACCOUNT:        threadPool.enqueue(ServerhandleDeleteAccount, msg, client_socket, connect);          break;
                case ADD_FRIEND:            threadPool.enqueue(ServerhandleAddFriend, msg, client_socket, connect);              break;
                case DELETE_FRIEND:         threadPool.enqueue(ServerhandleDeleteFriend, msg, client_socket, connect);           break;
                case BLOCK_FRIEND:          threadPool.enqueue(ServerhandleBlockFriend, msg, client_socket, connect);            break;
                case UNBLOCK_FRIEND:        threadPool.enqueue(ServerhandleUnblockFriend, msg, client_socket, connect);          break;
                case DISPLAY_FRIEND:        threadPool.enqueue(ServerhandleDisplayFriend, msg, client_socket, connect);          break;
                case PRIVATE_MESSAGE:       ServerhandlePrivateMessage(msg, client_socket, connect);                             break;
                case CREATE_GROUP:          threadPool.enqueue(ServerhandleCreateGroup, msg, client_socket, connect);            break;
                case DELETE_GROUP:          threadPool.enqueue(ServerhandleDeleteGroup, msg, client_socket, connect);            break;
                case JOIN_GROUP:            threadPool.enqueue(ServerhandleRequestJoinGroup, msg, client_socket, connect);       break;
                case EXIT_GROUP:            threadPool.enqueue(ServerhandleExitGroup, msg, client_socket, connect);              break;
                case DISPLAY_GROUP:         threadPool.enqueue(ServerhandleDisplayGroupList, client_socket, connect);            break;
                case DISPLAY_GROUP_REQUEST: threadPool.enqueue(ServerhandleDisplayRequestList, msg, client_socket, connect);     break;
                case SET_MANAGER:           threadPool.enqueue(ServerhandleSetManager, msg, client_socket, connect);             break;
                case ADD_GROUP:             threadPool.enqueue(ServerhandleAddGroup, msg, client_socket, connect);               break;
                case CANCEL_MANAGER:        threadPool.enqueue(ServerhandleCancelManager, msg, client_socket, connect);          break;
                case KICK_SOMEBODY:         threadPool.enqueue(ServerhandleKickSomebody, msg, client_socket, connect);           break;
                case DISPLAY_GROUP_MEMBER:  threadPool.enqueue(ServerhandleDisplayGroupMember, msg, client_socket, connect);     break;
                case GROUP_MESSAGE:         ServerhandleGroupMessage(msg, client_socket, connect);                               break;
                case SEND_FILE:             threadPool.enqueue(ServerhandleSendFile, msg, ret, client_socket, connect);          break;
                case SEND_FILE_LONG:        threadPool.enqueue(ServerhandleSendFile_long, msg, ret, client_socket, connect);     break;
                case CHECK_FILE:            threadPool.enqueue(ServerhandleCheckFile, msg, client_socket, connect);              break;
                case RECEIVE_FILE:          threadPool.enqueue(ServerhandleReceiveFile, msg, client_socket, connect);            break;
                case FRIEND_HISTORY:        threadPool.enqueue(ServerhandleFriendHistory, msg, client_socket, connect);          break;
                case GROUP_HISTORY:         threadPool.enqueue(ServerhandleGroupHistory, msg, client_socket, connect);           break;
                case HEART_BEAT:            threadPool.enqueue(ServerhandleHeartBeat, msg,  client_socket);                      break;
                default:                    LogError("Unknown message type");                                                    break;
            }
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
                LogInfo("接受新的客户端{}连接", server_socket);
                int client_socket = accept(server_socket, nullptr, nullptr);
                if(client_socket == -1)
                {
                    LogError("Failed to accept client connection");
                    continue;
                }
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = client_socket;
                setFdNoblock(client_socket);//不让客户端堵塞

                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event);

                client_sockets.push_back(client_socket);
                //threadPool.enqueue(&server::handleClientConnect, this, client_socket);//---------------to do
                //server::handleClientConnect(client_socket);
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

                    LogInfo("Client disconnected by EPOLLERR | EPOLLHUP: {}", events[i].data.fd);

                    update_online_status(connect, hashTable[events[i].data.fd].c_str(), 0);

                    handleOffline(connect, events[i].data.fd);

                    close(events[i].data.fd);
                }
                else if(events[i].events & EPOLLIN)//接收信息
                {
                    int fd = events[i].data.fd;
                    //threadPool.enqueue(&server::handleReceivedMessage, this, fd);
                    server::handleReceivedMessage(fd);
                }
                else if(events[i].events & EPOLLOUT)//发送信息
                {
                    //todo-------------------------
                    LogInfo("EPOLLOUT");
                }
            }
        }
    }
}
