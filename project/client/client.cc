#include "client_menu.h"

#define PORT_NUM 8000
#define IP "127.0.0.1"
#define BUF_SIZE 2048

std::mutex mtx; // 互斥量，用于保护标准输出

void errExit(const char* error)
{
    perror(error);
    exit(EXIT_FAILURE);
}

void sendMessageThread(int sfd) {
    char buf[BUF_SIZE];
    while (true) {
        memset(buf, 0, BUF_SIZE);
        if (fgets(buf, BUF_SIZE, stdin) == nullptr) {
            printf("Exiting...\n");
            break;
        }

        int len = strlen(buf);
        
        // 去除输入内容末尾的换行符
        if (len > 0 && buf[len-1] == '\n') {
            buf[len-1] = '\0';
            len--;
        }

        if (buf[0] == '\0') {
            continue;
        }

        if (strcmp(buf, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }

        if (sendMsg(sfd, buf, len, GROUP_MESSAGE) == -1) {
            printf("Failed to send message.\n");
            break;
        }
    }
}

void receiveMessageThread(int sfd) {
    char* buf;
    Type flag;
    while (true) {
        int len = recvMsg(sfd, &buf, &flag);
        switch (flag)
        {
            case GROUP_MESSAGE: std::cout << "GROUP_MESSAGE\n"; break;
            case LOGIN: std::cout << "LOGIN\n"; break;
            case REGISTER: std::cout << "REGISTER\n"; break;
            case SERVER_MESSAGE: std::cout << "SERVER_MESSAGE"; break;
            default: std::cout << "UNKNOWN\n"; break;
        }
        if (len == -1) {
            perror("recv");
            break;
        } else if (len == 0) {
            printf("Server closed the connection.\n");
            break;
        }

        std::lock_guard<std::mutex> lock(mtx); // 使用互斥量保护标准输出
        printf("Received message from server: %s\n", buf);
    }
}

int main(void) {

    int sfd;
    struct sockaddr_in svaddr;

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd == -1) {
        errExit("socket");
    }

    memset(&svaddr, 0, sizeof(struct sockaddr_in)); 
    svaddr.sin_family = AF_INET;
    svaddr.sin_port = htons(PORT_NUM);
    inet_pton(AF_INET, IP, &svaddr.sin_addr);

    if (connect(sfd, (const struct sockaddr*) &svaddr, sizeof(struct sockaddr_in)) == -1) {
        errExit("connect");
    }

    enterChatroom(sfd);

    // 创建发送消息线程
    std::thread sendThread(sendMessageThread, sfd);
    // 创建接收消息线程
    std::thread receiveThread(receiveMessageThread, sfd);

    // 等待发送消息线程和接收消息线程结束
    sendThread.join();
    receiveThread.join();

    if (close(sfd) == 0) {
        printf("close success\n");
    }

    return 0;
}

