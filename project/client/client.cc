#include "client_menu.h"
#include "command_menu.h"
#include <unistd.h>

#define PORT_NUM 8000
#define IP "10.30.1.139"

std::mutex mtx; // 互斥量，用于保护标准输出

void errExit(const char* error)
{
    perror(error);
    exit(EXIT_FAILURE);
}

static void clearInputBuffer() 
{
    std::cin.clear(); // 清除输入状态标志
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略剩余输入
}

void sendMessageThread(int sfd) 
{
    std::string msg;

    clearInputBuffer();

    while (true)
    {
        std::getline(std::cin, msg); // 读取整行输入

        usleep(1000); // 暂停一毫秒

        if (msg == "exit")
        {
            std::cout << "Exiting...\n";
            exit(EXIT_SUCCESS);
            break;
        }

        if(msg == "command")
        {
            commandMenu(sfd);
            continue;
        }

        sendMsg(sfd, msg.c_str(), msg.size(), WORLD_MESSAGE);

    }
}

void real_file_resv(char* msg, int sfd, int len)
{
    char* to_file_path = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;


    size_t msgSize = len - strlen(to_file_path) - 1; // 加上空格的大小

    recvFile(sfd, msg, msgSize, to_file_path);
}

void real_file_resv_long(char* msg, int sfd, int len)
{
    char* to_file_path = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;


    size_t msgSize = len - strlen(to_file_path) - 1; // 加上空格的大小

    recvFile_long(sfd, msg, msgSize, to_file_path);
}


void receiveMessageThread(int sfd) 
{
    char* msg;
    Type flag;
    while (true) 
    {
        int len = recvMsg(sfd, &msg, &flag);
        switch (flag)
        {
            case GROUP_MESSAGE:     std::cout << "群聊\n";                  break;
            case PRIVATE_MESSAGE:   std::cout << "私聊\n";                  break;
            case SERVER_MESSAGE:    std::cout << "服务器信息\n";            break;
            case WORLD_MESSAGE:     std::cout << "全服\n";                  break;
            case SEND_FILE:         real_file_resv(msg, sfd, len);          break;
            case SEND_FILE_LONG:    real_file_resv_long(msg, sfd, len);     break;
            default:                std::cout << "未知错误\n";              break;
        }
        if (len == -1) 
        {
            perror("recv");
            break;
        } else if (len == 0) 
        {
            printf("Server closed the connection.\n");
            break;
        }

        if (flag == SEND_FILE || flag == SEND_FILE_LONG)
        {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(mtx); // 使用互斥量保护标准输出
            std::cout << msg << '\n';
        }

        std::cout << '\n';
    }
}

void sendHeartbeatThread(int sfd) 
{
    while (true) 
    {
        std::string msg = "heartbeat";
        usleep(1000000); // 暂停1秒
        sendMsg(sfd, msg.c_str(), msg.length(), HEART_BEAT);
    }
}

int main(void) 
{
    int sfd;
    struct sockaddr_in svaddr;

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) 
    {
        errExit("socket");
    }

    memset(&svaddr, 0, sizeof(struct sockaddr_in)); 
    svaddr.sin_family = AF_INET;
    svaddr.sin_port = htons(PORT_NUM);
    inet_pton(AF_INET, IP, &svaddr.sin_addr);

    if (connect(sfd, (const struct sockaddr*) &svaddr, sizeof(struct sockaddr_in)) == -1) 
    {
        errExit("connect");
    }

    enterChatroom(sfd);

    // 创建发送消息线程
    std::thread sendThread(sendMessageThread, sfd);
    // 创建接收消息线程
    std::thread receiveThread(receiveMessageThread, sfd);
    // 创建发送心跳包线程
    //std::thread heartbeatThread(sendHeartbeatThread, sfd);

    // 等待发送消息线程和接收消息线程结束
    sendThread.join();
    receiveThread.join();
    //heartbeatThread.join();

    if (close(sfd) == 0) 
    {
        printf("close success\n");
    }

    return 0;
}
