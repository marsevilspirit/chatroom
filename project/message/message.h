#include <string>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <cerrno>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include "../mysql/mysql.h"
#include "../json.hpp"

enum Type{
    LOGIN = 1,
    REGISTER,
    FORGET_PASSWD,
    DELETE_ACCOUNT,
    GROUP_MESSAGE,
    ADD_FRIEND,
    DELETE_FRIEND,
    SERVER_MESSAGE,
    BLOCK_FRIEND,
    DISPLAY_FRIEND,
    UNBLOCK_FRIEND,
    PRIVATE_MESSAGE,
    CREATE_GROUP,
    DELETE_GROUP,
    JOIN_GROUP,
    EXIT_GROUP,
    DISPLAY_GROUP,
    DISPLAY_GROUP_REQUEST,
    SET_MANAGER,
    ADD_GROUP,
    CANCEL_MANAGER,
    KICK_SOMEBODY,
    DISPLAY_GROUP_MEMBER,
    WORLD_MESSAGE,
    SEND_FILE,
    SEND_FILE_LONG,
    RECEIVE_FILE,
    CHECK_FILE,
    FRIEND_HISTORY,
    GROUP_HISTORY,
};

int sendMsg(int cfd, const char* msg, int len, Type flag);

int recvMsg(int cfd, char** msg, Type* flag);

int sendFile(int cfd, const char* file_name, const char* file_path, const char* resver);

int recvFile(int cfd, char* buffer, int ret, const char* filename);

void resvFile(const char* msg, const char* file_path);

int recvFile_long(int cfd, char* buffer, int ret, const char* file_path);

void handleWorldMessage(MYSQL* connect, char* msg, int client_socket, std::vector<int>& client_sockets);

void ServerhandleRegister(char* msg, int client_socket, MYSQL* connect);

void ServerhandleLogin(char* msg, int client_socket, MYSQL* connect);

void ServerhandleForgetPasswd(char* msg, int client_socket, MYSQL* connect);

void ServerhandleDeleteAccount(char* msg, int client_socket, MYSQL* connect);

void ServerhandleAddFriend(char* msg, int client_socket, MYSQL* connect);

void ServerhandleDeleteFriend(char* msg, int client_socket, MYSQL* connect);

void ServerhandleBlockFriend(char* msg, int client_socket, MYSQL* connect);

void ServerhandleUnblockFriend(char* msg, int client_socket, MYSQL* connect);

void ServerhandleDisplayFriend(char* msg, int client_socket, MYSQL* connect);

void ServerhandlePrivateMessage(char* msg, int client_socket, MYSQL* connect);

void ServerhandleCreateGroup(char* msg, int client_socket, MYSQL* connect);

void ServerhandleDeleteGroup(char* msg, int client_socket, MYSQL* connect);

void ServerhandleRequestJoinGroup(char* msg, int client_socket, MYSQL* connect);

void ServerhandleExitGroup(char* msg, int client_socket, MYSQL* connect);

void ServerhandleDisplayGroupList(int client_socket, MYSQL* connect);

void ServerhandleDisplayRequestList(char* msg, int client_socket, MYSQL* connect);

void ServerhandleSetManager(char* msg, int client_socket, MYSQL* connect);

void ServerhandleAddGroup(char* msg, int client_socket, MYSQL* connect);

void ServerhandleCancelManager(char* msg, int client_socket, MYSQL* connect);

void ServerhandleKickSomebody(char* msg, int client_socket, MYSQL* connect);

void ServerhandleDisplayGroupMember(char* msg, int client_socket, MYSQL* connect);

void ServerhandleGroupMessage(char* msg, int client_socket, MYSQL* connect);

void ServerhandleSendFile(char* msg, int len, int client_socket, MYSQL* connect);

void ServerhandleSendFile_long(char* msg, int len, int client_socket, MYSQL* connect);

void ServerhandleCheckFile(char* msg, int client_socket, MYSQL* connect);

void ServerhandleReceiveFile(char* msg, int client_socket, MYSQL* connect);

void ServerhandleFriendHistory(char* msg, int client_socket, MYSQL* connect);

void ServerhandleGroupHistory(char* msg, int client_socket, MYSQL* connect);
