#include "message.h"
#include <vector>

static int writen(int fd, const char* msg, int size)
{
    const char* buf = msg;
    int count = size;
    while(count > 0)
    {
        int len = send(fd, buf, count, 0);
        if(len == -1)
        {
            if(errno == EPIPE || errno == ECONNRESET)
            {
                printf("Network connection reset or broken.\n");
                return -1; // Return -1 to indicate network error
            }
            else
            {
                perror("send");
                close(fd);
                return -1;
            }
        }
        else if(len == 0)
        {
            continue;
        }

        buf += len;
        count -= len;
    }

    return size;
}

int sendMsg(int cfd, const char* msg, int len, Type flag)
{
    if (msg == nullptr || len <= 0 || cfd <= 0)
    {
        return -1;
    }

    char* data = (char*)malloc(len + 8); // 4字节用于存放长度，4字节用于存放标志
    if (data == nullptr)
    {
        perror("malloc");
        return -1;
    }

    int bigLen = htonl(len);
    int bigFlag = htonl(flag);

    memcpy(data, &bigLen, 4);
    memcpy(data + 4, &bigFlag, 4);
    memcpy(data + 8, msg, len);

    int ret = writen(cfd, data, len + 8);

    free(data);

    return ret;
}

static int readn(int fd, char* buf, int size)
{
    char* pt = buf;
    int count = size;

    while (count > 0)
    {
        int len = recv(fd, pt, count, 0);
        if (len == -1)
            return -1;
        else if (len == 0)
            return size - count;

        pt += len;
        count -= len;
    }

    return size;
}

int recvMsg(int cfd, char** msg, Type* flag)
{
    int len = 0;
    if (readn(cfd, (char*)&len, 4) == -1) {
        return -1; // Error reading length
    }

    len = ntohl(len);

    int recvFlag = 0;
    if (readn(cfd, (char*)&recvFlag, 4) == -1) {
        return -1; // Error reading flag
    }

    recvFlag = ntohl(recvFlag);

    char* buf = (char*)malloc(len + 1);
    int ret = readn(cfd, buf, len);
    if (ret != len)
    {
        close(cfd);
        free(buf);
        return -1;
    }

    buf[len] = '\0';
    *msg = buf;
    *flag = (Type)recvFlag;

    return ret;
}

void handleGroupMessage(char* msg, int client_socket, std::vector<int>& client_sockets)
{
    std::cout << "Received message from client_socket: " << client_socket << '\n';
    std::cout << msg << '\n';
    int len = strlen(msg);
    for(const auto& socket : client_sockets) 
    {
        if(socket != client_socket)
            sendMsg(socket, msg, len, GROUP_MESSAGE);
    }
    free(msg);   
}

void ServerhandleRegister(char* msg, int client_socket, MYSQL* connect)
{
    std::cout << "New user want to register: " << client_socket << '\n';
    std::cout << msg <<'\n';
    
    char* email = msg;  

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* password = msg;

    while(*msg != ' ' && *msg!= '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* name = msg;

    std::string send;

    if(sql_insert(connect, email, password, name) == 0)
    {
        send = "该账号已注册"; 
    }
    else 
    {
        std::cout << email << " " << password << " " << name << '\n';

        send = "注册成功";
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleLogin(char* msg, int client_socket, MYSQL* connect)
{
    std::cout << "new user want to login: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* email = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* password = msg;

    bool find = sql_select(connect, email, password);

    std::string send;

    if(find)
        send = "登录成功";
    else 
        send = "登录失败，账号或密码错误";

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);

}
