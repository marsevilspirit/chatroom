#include <string>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>
#include <vector>
#include <unordered_map>
#include "../mysql/mysql.h"

enum Type{
    LOGIN = 1,
    REGISTER,
    FORGET_PASSWD,
    DELETE_ACCOUNT,
    GROUP_MESSAGE,
    SERVER_MESSAGE,
};

int sendMsg(int cfd, const char* msg, int len, Type flag);

int recvMsg(int cfd, char** msg, Type* flag);

void handleGroupMessage(char* msg, int client_socket, std::vector<int>& client_sockets);

void ServerhandleRegister(char* msg, int client_socket, MYSQL* connect);

void ServerhandleLogin(char* msg, int client_socket, MYSQL* connect);

void ServerhandleForgetPasswd(char* msg, int client_socket, MYSQL* connect);

void ServerhandleDeleteAccount(char* msg, int client_socket, MYSQL* connect);
