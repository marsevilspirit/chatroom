#include "message.h"

std::unordered_map<int, std::string> hashTable;

int find_key_by_value(const std::unordered_map<int, std::string>& map, const std::string& value)
{
    for(const auto& pair : map)
    {
        if(pair.second == value)
            return pair.first;
    }

    return -1;
}

static int writen(int fd, const char* msg, int size)
{
    const char* buf = msg;
    int count = size;
    while(count > 0)
    {
        usleep(10000);

        int len = send(fd, buf, count, 0);
        if(len == -1)
        {
            if(errno == EPIPE || errno == ECONNRESET)
            {
                LogError("网络连接重置或中断");
                close(fd);
                return -1;
            }
            else
            {
                LogError("send func error");
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
        LogError("malloc func error");
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
        usleep(10000);

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
    if (readn(cfd, (char*)&len, 4) == -1) 
    {
        LogTrace("len: {}", len);
        return -1;
    }

    len = ntohl(len);

    int recvFlag = 0;
    if (readn(cfd, (char*)&recvFlag, 4) == -1) 
    {
        return -1;
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

// 发送文件函数
int sendFile(int cfd, const char* file_name, const char* file_path, const char* resver)
{
    FILE* file = fopen(file_path, "rb");
    if (!file)
    {
        LogError("没有找到文件{} 或文件打开失败", file_path);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 计算消息总大小
    size_t msgSize;
    const size_t block_size = 409600;

    if (fileSize <= block_size)
        msgSize = strlen(file_name) + strlen(resver) + fileSize + 2;
    else
        msgSize = strlen(file_name) + strlen(resver) + block_size + 2;

    char* buffer = (char*)malloc(msgSize);
    if (!buffer)
    {
        LogError("malloc func error");
        fclose(file);
        return -1;
    }

    // 将文件名、文件路径和接收方用户名写入 buffer，中间用空格分隔
    char* ptr = buffer;
    strcpy(ptr, file_name);
    ptr += strlen(file_name);
    *ptr = ' ';
    ptr++;
    strcpy(ptr, resver);
    ptr += strlen(resver);
    *ptr = ' ';
    ptr++;

    // 读取文件内容并写入 buffer
    if (fileSize <= block_size)
        fread(ptr, 1, fileSize, file);
    else
        fread(ptr, 1, block_size, file);

    int ret = sendMsg(cfd, buffer, msgSize, SEND_FILE);

    fileSize -= block_size;

    if (fileSize <= block_size)
    {
        fclose(file);
        free(buffer);
        return ret;
    }

    LogInfo("文件上传中");

    while (fileSize > 0)
    {
        usleep(25000);

        memset(buffer, 0, msgSize);

        char* ptr = buffer;
        strcpy(ptr, file_name);
        ptr += strlen(file_name);
        *ptr = ' ';
        ptr++;
        strcpy(ptr, resver);
        ptr += strlen(resver);
        *ptr = ' ';
        ptr++;

        if (fileSize < block_size)
            fread(ptr, 1, fileSize, file);
        else
            fread(ptr, 1, block_size, file);

        if (fileSize > block_size)
            sendMsg(cfd, buffer, strlen(file_name) + strlen(resver) + block_size + 2, SEND_FILE_LONG);
        else
            sendMsg(cfd, buffer, strlen(file_name) + strlen(resver) + fileSize + 2, SEND_FILE_LONG);

        fileSize -= block_size;
    }

    LogInfo("文件上传成功");

    fclose(file);
    free(buffer);

    return 0;
}
// 发送文件函数
int sendFile(int cfd, const char* file_path, const char* to_file_path)
{
    FILE* file = fopen(file_path, "rb");
    if (!file)
    {
        LogError("没有找到文件{}或文件打开失败", file_path);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 计算消息总大小
    size_t msgSize;
    const size_t block_size = 409600;

    if(fileSize <= block_size)
        msgSize = strlen(to_file_path) + fileSize + 1; // 加上空格的大小
    else
        msgSize = strlen(to_file_path) + block_size + 1; // 加上空格的大小

    char* buffer = (char*)malloc(msgSize);
    if (!buffer)
    {
        LogError("malloc func error");
        fclose(file);
        return -1;
    }

    // 将文件名、文件路径和接收方用户名写入 buffer，中间用空格分隔
    char* ptr = buffer;
    strcpy(ptr, to_file_path);
    ptr += strlen(to_file_path);
    *ptr = ' ';
    ptr++;

    // 读取文件内容并写入 buffer
    if(fileSize <= block_size)
        fread(ptr, 1, fileSize, file);
    else
        fread(ptr, 1, block_size, file);

    int ret = sendMsg(cfd, buffer, msgSize, SEND_FILE);

    fileSize -= block_size;

    if(fileSize <= block_size)
    {
        fclose(file);
        free(buffer);
        return ret;
    }

    LogInfo("文件传输中");

    while(fileSize > 0)
    {
        usleep(25000);

        memset(buffer, 0, msgSize);

        char* ptr = buffer;
        strcpy(ptr, to_file_path);
        ptr += strlen(to_file_path);
        *ptr = ' ';
        ptr++;

        if(fileSize < block_size)
            fread(ptr, 1, fileSize, file);
        else
            fread(ptr, 1, block_size, file);

        if(fileSize > block_size)
            sendMsg(cfd, buffer, strlen(to_file_path) + block_size + 1, SEND_FILE_LONG);
        else
            sendMsg(cfd, buffer, strlen(to_file_path) + fileSize + 1, SEND_FILE_LONG);

        fileSize -= block_size;

        //std::cout << "(while)fileSize: " << fileSize << '\n';
    }

    LogInfo("文件传输完成");

    fclose(file);
    free(buffer);

    return ret;
}


int sendFile(int cfd, const char* file_path)
{

    FILE* file = fopen(file_path, "rb");
    if (!file)
    {
        LogError("没有找到文件{}或文件打开失败", file_path);
        return -1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(fileSize);
    if (!buffer)
    {
        LogError("malloc func error");
        fclose(file);
        return -1;
    }

    fread(buffer, 1, fileSize, file);
    fclose(file);

    LogTrace("buffer: \n{}", buffer);

    int ret = sendMsg(cfd, buffer, fileSize, SEND_FILE);

    free(buffer);

    return ret;
}

// 接收文件函数
int recvFile(int cfd, char* buffer, int ret, const char* file_path)
{
    FILE* file = fopen(file_path, "wb");
    if (!file)
    {
        LogError("没有找到文件{}或文件打开失败", file_path);
        free(buffer);
        return -1;
    }

    fwrite(buffer, 1, ret, file);

    fclose(file);
    return 0;
}

// 接收文件函数
int recvFile_long(int cfd, char* buffer, int ret, const char* file_path)
{
    FILE* file = fopen(file_path, "ab");
    if (!file)
    {
        LogError("没有找到文件{}或文件打开失败", file_path);
        free(buffer);
        return -1;
    }

    fwrite(buffer, 1, ret, file);

    fclose(file);
    return 0;
}

//下线通知好友函数
void handleOffline(MYSQL* connect, int client_socket)
{
    std::string email = hashTable[client_socket];

    std::string name = sql_getname(connect, email.c_str());
    std::string offline = "你的朋友" + name + "下线了";

    std::string friend_list;

    if(sql_display_friend(connect, email.c_str(), friend_list) == 0)
    {
        LogError("获取好友列表失败");
        return;
    }

    for(const auto& socket : hashTable)
    {
        if(friend_list.find(socket.second) != std::string::npos)
        {
            if(socket.first != client_socket)
                sendMsg(socket.first, offline.c_str(), offline.size(), SERVER_MESSAGE);
        }
    }
}

void handleWorldMessage(MYSQL* connect, char* msg, int client_socket, std::vector<int>& client_sockets)
{
    LogInfo("收到了{}的向全服发送消息", hashTable[client_socket]);

    std::string send = sql_getname(connect, hashTable[client_socket].c_str()) + ": \n" + msg;

    for(const auto& socket : client_sockets) 
    {
        if(sql_if_online(connect, hashTable[socket].c_str()) == 0)
            continue;

        if(socket != client_socket)
            sendMsg(socket, send.c_str(), send.size(), WORLD_MESSAGE);
    }
    free(msg);   
}

void ServerhandleRegister(char* msg, int client_socket, MYSQL* connect)
{
    nlohmann::json j = nlohmann::json::parse(std::string(msg));

    std::string email = j["email"];
    std::string password = j["passwd"];
    std::string name = j["name"];

    LogInfo("{}用户想要注册", email);

    std::string send;

    if(sql_insert(connect, email.c_str(), password.c_str(), name.c_str()) == 0)
    {
        send = "该账号已注册"; 
        LogInfo("注册失败, 该账号已注册");
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else 
    {
        LogInfo("{}用户注册成功", email);
        send = "注册成功";
    }

    //为注册的用户创建列表
    if(sql_create_list(connect, email.c_str()) == 0)
    {
        LogError("创建好友列表失败");
        send = "创建好友列表失败";
    }

    if(sql_create_group_list(connect, email.c_str()) == 0)
    {
        LogError("创建群组列表失败");
        send = "创建群组列表失败";
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleLogin(char* msg, int client_socket, MYSQL* connect)
{
    nlohmann::json j = nlohmann::json::parse(std::string(msg));

    std::string send;

    std::string email = j["email"];
    std::string password = j["passwd"];

    LogInfo("{}用户想要登陆", email);

    int ret = sql_online(connect, email.c_str());

    if(ret == 3)
    {
        LogInfo("由于{}已经登录, 登录失败", email);
        send = "账号已登录, 请先退出登录";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 2)
    {
        LogInfo("{}账号不存在", email);
        send = "账号不存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        LogError("数据库错误");
        send = "数据库错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    bool find = sql_select(connect, email.c_str(), password.c_str());

    if(find)
    {
        LogInfo("{}用户登录成功", email);
        send = "登录成功";
    }
    else 
    {
        LogInfo("{}用户登录失败, 密码错误", email);
        send = "登录失败，密码错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(update_online_status(connect, email.c_str(), 1) == 0)
    {
        LogError("修改在线状态失败");
        send = "修改在线状态失败";
    }

    hashTable[client_socket] = email;

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);

    //上线通知好友
    std::string name = sql_getname(connect, email.c_str());
    std::string online = name + "上线了";

    std::string friend_list;
    if(sql_display_friend(connect, email.c_str(), friend_list) == 0)
    {
        LogError("获取好友列表失败");
        return;
    }

    for(const auto& socket : hashTable)
    {
        if(friend_list.find(socket.second) != std::string::npos)
        {
            if(socket.first != client_socket)
                sendMsg(socket.first, online.c_str(), online.size(), SERVER_MESSAGE);
        }
    }
}

void ServerhandleForgetPasswd(char* msg, int client_socket, MYSQL* connect)
{
    nlohmann::json j = nlohmann::json::parse(std::string(msg));
    std::string email = j["email"];
    std::string password = j["passwd"];

    LogInfo("{}用户想要修改密码", email);

    std::string send;

    if(sql_online(connect, email.c_str()) == 0)
    {
        LogInfo("{}用户正在登录, 无法修改密码", email);
        send = "用户正在登录, 无法修改密码";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(update_passwd(connect, email.c_str(), password.c_str()) == 0)
    {
        LogError("{}用户密码更新失败", email);
        send = "密码更新失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    LogInfo("{}用户密码更新成功", email);
    send = "密码更新成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);

}

void ServerhandleDeleteAccount(char* msg, int client_socket, MYSQL* connect)
{
    std::cout << "new user want to delete account: " << client_socket << '\n'; 

    nlohmann::json j = nlohmann::json::parse(std::string(msg));
    std::string email = j["email"];
    std::string password = j["passwd"];

    LogInfo( "用户{}想要删除账号", email);

    std::string send;

    int ret = sql_online(connect, email.c_str());

    if(ret == 3)
    {
        send = "账号已登录, 请先退出登录";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 2)
    {
        send = "账号不存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        send = "数据库错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(delete_account(connect, email.c_str(), password.c_str()) == 0)
    {
        send = "删除账号失败, 密码错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    handle_delete_friend(connect, email.c_str());

    if(sql_delete_list(connect, email.c_str()) == 0)
    {
        LogError("删除列表失败");
        send = "删除列表失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(sql_delete_group_list(connect, email.c_str()) == 0)
    {
        LogInfo("删除群组列表失败");
        send = "删除群组列表失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "删除账号成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleAddFriend(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    char* friend_email = msg;

    LogInfo("用户{}想要添加用户{}好友", email, friend_email);

    int friend_fd = find_key_by_value(hashTable, friend_email);

    std::string send;

    int ret = sql_add_friend(connect, email.c_str(), friend_email);

    if(ret == 2)
    {
        send = "添加好友失败, 该用户不存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        send = "添加好友失败, 数据库错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 3)
    {
        send = "添加好友失败, 不能添加自己为好友";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(sql_if_online(connect, friend_email))
    {
        if(is_your_friend_or_request(connect, email.c_str(), friend_email))
        {
            send = email + "接受了你的好友邀请";
            sendMsg(friend_fd, send.c_str(), send.size(), SERVER_MESSAGE);
        }
        else
        {
            send = "您有新的好友请求from " + email;
            sendMsg(friend_fd, send.c_str(), send.size(), SERVER_MESSAGE);
        }
    }

    if(!is_your_friend_or_request(connect, email.c_str(), friend_email))
    {
        sql_request(connect, email.c_str(), friend_email);//好友申请
    }

    send = "添加好友成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleDeleteFriend(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    char* friend_email = msg;

    LogInfo( "用户{}想要删除用户{}好友", email, friend_email);

    std::string send;

    int ret = sql_delete_friend(connect, email.c_str(), friend_email);

    if(ret == 2)
    {
        send = "删除好友失败, 该用户不是好友";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        send = "删除好友失败, 数据库错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "删除好友成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleBlockFriend(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    char* friend_email = msg;

    LogInfo("用户{}想要屏蔽用户{}", email, friend_email);

    std::string send;

    if(sql_block_friend(connect, email.c_str(), friend_email) == 2)
    {
        send = "屏蔽好友失败, 该用户不是好友";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "屏蔽好友成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleUnblockFriend(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    char* friend_email = msg;

    LogInfo("用户{}想要解除屏蔽用户{}", email, friend_email);

    std::string send;

    int ret = sql_unblock_friend(connect, email.c_str(), friend_email);

    if(ret == 2)
    {
        send = "解除屏蔽好友失败, 该用户未被屏蔽";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        send = "解除屏蔽好友失败, 数据库错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "解除屏蔽好友成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleDisplayFriend(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    LogInfo("用户{}想要查看好友列表", email);

    std::string send;

    if(sql_display_friend(connect, email.c_str(), send) == 0)
    {
        send = "获取好友列表失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleDisplayFriendRequest(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    LogInfo("用户{}想要查看好友请求列表", email);

    std::string send;

    if(sql_display_friend_request(connect, email.c_str(), send) == 0)
    {
        send = "获取好友请求列表失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandlePrivateMessage(char* msg, int client_socket, MYSQL* connect)
{
    std::string from_email = hashTable[client_socket];

    char* to_email = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* message = msg;

    LogInfo("用户{}想要发送私聊消息给用户{}", from_email, to_email);

    //在message后加上发送时间
    time_t now = time(0);
    char* dt = ctime(&now);
    std::string time = dt;
    time.pop_back();

    std::string send = sql_getname(connect, from_email.c_str()) + ": ("+ time + ")\n" + message + "\n";

    /*

    if(!is_your_friend_or_block(connect, email.c_str(), friend_email))
    {
        send = "您不是" + std::string(friend_email) + "好友";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(sql_if_block(connect, email.c_str(), friend_email))
    {
        send = "您已被" + std::string(friend_email) + "屏蔽";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    */

    std::string to_emailStr = std::string(to_email);
    std::replace(to_emailStr.begin(), to_emailStr.end(), '@', '0');

    std::string::size_type pos = to_emailStr.find('.');
    if (pos != std::string::npos) 
    {
        to_emailStr = to_emailStr.substr(0, pos) + "_list";
    }

    std::string from_emailStr = std::string(from_email);
    std::replace(from_emailStr.begin(), from_emailStr.end(), '@', '0');

    std::string::size_type pos2 = from_emailStr.find('.');
    if (pos2 != std::string::npos) 
    {
        from_emailStr = from_emailStr.substr(0, pos2) + "_list";
    }

    std::cout << "emailStr: " << to_emailStr << '\n';

    std::string type = getTypeByEmail(connect, to_emailStr, from_email);

    if(type == "request")
    { 
        send = "您不是" + std::string(to_email) + "好友";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(type == "block")
    {
        send = "您已被" + std::string(to_email) + "屏蔽";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    std::string type2 = getTypeByEmail(connect, from_emailStr, to_email);

    if(type2 == "block")
    {
        send = "您已屏蔽" + std::string(to_email);
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    add_friend_message_list(connect, from_email.c_str(), to_email, message);

    if(sql_if_online(connect, to_email) == 0)
    {
        send = std::string(to_email) + "不在线";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    for(const auto& socket : hashTable)
    {
        if(socket.second == to_email)
        {
            sendMsg(socket.first, send.c_str(), send.size(), PRIVATE_MESSAGE);
            break;
        }
    }
}

void ServerhandleCreateGroup(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    char* group_name = msg;

    LogInfo("用户{}想要创建群组{}", email, group_name);

    std::string send;

    if(sql_create_group(connect, email.c_str(), group_name) == 0)
    {
        send = "创建群组失败, 该群组已存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "创建群组成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleDeleteGroup(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    char* group_name = msg;

    LogInfo("用户{}想要删除群组{}", email, group_name);

    std::string send;

    int ret = sql_delete_group(connect, email.c_str(), group_name);

    if(ret == 2)
    {
        send = "删除群组失败, 该群组不存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 3)
    {
        send = "删除群组失败, 你不是群主";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "删除群组成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleRequestJoinGroup(char* msg, int client_socket, MYSQL* connect)
{
    std::string my_email = hashTable[client_socket];

    char* group_name = msg;

    LogInfo("用户{}想要加入群组{}", my_email, group_name);

    std::string send;

    int ret = sql_add_group(connect, my_email.c_str(), group_name);

    if(ret == 2)
    {
        send = "加入群组失败, 该群组不存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        send = "加入群组失败, 数据库错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 3)
    {
        send = "加入群组失败, 你已经申请或加入";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "申请加入群组成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleExitGroup(char* msg, int client_socket, MYSQL* connect)
{
    std::string my_email = hashTable[client_socket];

    char* group_name = msg;

    LogInfo("用户{}想要退出群组{}", my_email, group_name);

    std::string send;

    int ret = sql_exit_group(connect, my_email.c_str(), group_name);

    if(ret == 2)
    {
        send = "退出群组失败, 该群组不存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 3)
    {
        send = "退出群组失败, 你是群主，不能退出";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "退出群组成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleDisplayGroupList(int client_socket, MYSQL* connect)
{
    LogInfo("用户{}想要查看群组列表", hashTable[client_socket]);

    std::string send;

    if(sql_display_group(connect, send) == 0)
    {
        send = "获取群组列表失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleDisplayRequestList(char* msg, int client_socket, MYSQL* connect)
{
    std::string my_email = hashTable[client_socket];

    char* group_name = msg;

    LogInfo("用户{}想要查看群组{}请求列表", my_email, group_name);

    std::string send;

    int ret = sql_display_request_list(connect, my_email.c_str(), group_name, send);

    if(ret == 2)
    {
        send = "获取请求列表失败, 你不是群主或管理员";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        send = "获取请求列表失败, 数据库错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleSetManager(char* msg, int  client_socket, MYSQL* connect)
{
    std::string my_email = hashTable[client_socket];

    nlohmann::json j = nlohmann::json::parse(std::string(msg));
    std::string email = j["email"];
    std::string group_name = j["group_name"];

    LogInfo("用户{}想要设置用户{}为群组{}管理员", my_email, email, group_name);

    std::string send;

    int ret = sql_set_manager(connect, my_email.c_str(), email.c_str(), group_name.c_str());

    if(ret == 2)
    {
        send = "设置管理员失败, 该用户不是群成员";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        send = "设置管理员失败, 你不是群主";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "设置管理员成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleAddGroup(char* msg, int client_socket, MYSQL* connect)
{
    std::string my_email = hashTable[client_socket];

    nlohmann::json j = nlohmann::json::parse(std::string(msg));
    std::string group_name = j["group_name"];
    std::string email = j["email"];

    LogInfo("用户{}将用户{}加入群组{}", my_email, email, group_name);

    std::string send;

    int ret = sql_real_add_group(connect, my_email.c_str(), email.c_str(), group_name.c_str());

    if(ret == 2)
    {
        send = "加入群组失败, 不是群主或管理员";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        send = "加入群组失败, 数据库错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 3)
    {
        send = "加入群组失败, 该用户没申请";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "申请加入群组成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleCancelManager(char* msg, int client_socket, MYSQL* connect)
{
    std::string my_email = hashTable[client_socket];

    nlohmann::json j = nlohmann::json::parse(std::string(msg));
    std::string group_name = j["group_name"];
    std::string email = j["email"];

    LogInfo("用户{}想要取消用户{}群组{}管理员", my_email, email, group_name);

    std::string send;

    int ret = sql_cancel_manager(connect, my_email.c_str(), email.c_str(), group_name.c_str());

    if(ret == 3)
    {
        send = "取消管理员失败, 该用户不是管理员";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 2)
    {
        send = "你不是群主";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 0)
    {
        send = "取消管理员失败, 数据库错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "取消管理员成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleKickSomebody(char* msg, int client_socket, MYSQL* connect)
{
    //群主可以踢管理员和普通成员, 管理员只能踢普通成员
    std::string my_email = hashTable[client_socket];
    int ret;

    nlohmann::json j = nlohmann::json::parse(std::string(msg));
    std::string group_name = j["group_name"];
    std::string email = j["email"];

    LogInfo("用户{}想要踢用户{}出群组{}", my_email, email, group_name);

    std::string send;

    if(my_email == email)
    {
        send = "你不能踢自己";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(if_group_exist(connect, group_name.c_str()) == 1)
    {
        send = "群组不存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return; 
    }

    if(if_master(connect, my_email.c_str(), group_name.c_str())) 
    {
        ret = sql_kick_anybody(connect, my_email.c_str(), email.c_str(), group_name.c_str());
        if(ret == 0)
        {
            send = "踢人失败, 该用户不存在";
            sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
            return;
        }
        else if(ret == 2)
        {
            send = "你不能踢自己";
            sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
            return;
        }
    }

    if(if_manager(connect, my_email.c_str(), group_name.c_str()))
    {
        if(if_master(connect, email.c_str(), group_name.c_str()))
        {
            send = "你没权力踢群主";
            sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
            return;
        }

        ret = sql_kick_normal(connect, my_email.c_str(), email.c_str(), group_name.c_str());
        if(ret == 0)
        {
            send = "踢人失败,  该用户不存在";
            sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
            return;
        }
    }
    else
    {
        send = "你不是群主或管理员";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return; 
    }

    send = "踢" + std::string(email) + "成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleDisplayGroupMember(char* msg, int client_socket, MYSQL* connect)
{
    std::string my_email = hashTable[client_socket];

    char* group_name = msg;

    LogInfo("用户{}想要查看群组{}成员", my_email, group_name);

    std::string send;

    int ret = sql_display_group_member_without_request(connect, my_email.c_str(), group_name, send);

    if(ret == 0)
    {
        send = "获取群成员失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 2)
    {
        send = "你不是群成员";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleGroupMessage(char* msg, int client_socket, MYSQL* connect)
{
    std::string my_email = hashTable[client_socket];

    char* group_name = msg;

    LogInfo("用户{}想要发送群组{}消息", my_email, group_name);

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';

    msg++;

    char* message = msg;

    LogTrace("group_name: {}, message: {}", group_name, message);

    //在message后加上发送时间
    time_t now = time(0);
    char* dt = ctime(&now);
    std::string time = dt;
    time.pop_back();

    std::string send = sql_getname(connect, my_email.c_str()) + ": ("+ time + ") from " + std::string(group_name) + "\n" + message + "\n";

    if(if_group_exist(connect, group_name)== 1)
    {
        send = "群组不存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    std::string group_member;

    int ret = sql_display_group_member_without_request(connect, my_email.c_str(), group_name, group_member);

    if(ret == 0)
    {
        send = "获取群成员失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }
    else if(ret == 2)
    {
        send = "你不是群成员";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    for (const auto& socket : hashTable)
    {
        if (socket.first != client_socket && group_member.find(socket.second) != std::string::npos)
        {
            sendMsg(socket.first, send.c_str(), send.size(), GROUP_MESSAGE);
        }
    }

    add_group_message_list(connect, my_email.c_str(), group_name, message);
}

void ServerhandleSendFile(char* msg, int len, int client_socket, MYSQL* connect)
{
    std::string sender = hashTable[client_socket];

    char* file_name = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;

    char* resver = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;

    LogInfo("用户{}想要发送文件{}给{}", sender, file_name, resver);

    size_t msgSize = len - strlen(file_name) - strlen(resver) - 2; // 加上空格的大小

    std::string savePath = "./server_file/" + sender + "_" + std::string(file_name);

    int ret = sql_file_list(connect, file_name, savePath.c_str(), sender.c_str(), resver);

    if(ret == 0)
    {
        std::string send = "发送文件失败，确定对方是你的朋友";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    recvFile(client_socket, msg, msgSize, savePath.c_str());

}

void ServerhandleSendFile_long(char* msg, int len, int client_socket, MYSQL* connect)
{
    std::string sender = hashTable[client_socket];

    char* file_name = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;

    char* resver = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;

    LogInfo("用户{}想要发送文件{}给{}", sender, file_name, resver);


    size_t msgSize = len - strlen(file_name) - strlen(resver) - 2; // 加上空格的大小

    std::string savePath = "./server_file/" + sender + "_" + std::string(file_name);

    recvFile_long(client_socket, msg, msgSize, savePath.c_str());
}


void ServerhandleCheckFile(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    LogInfo("用户{}想要查看文件列表", email);

    std::string send;

    if(sql_check_file(connect, email.c_str(), send) == 0)
    {
        send = "获取文件列表失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleReceiveFile(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    std::cout << "user want to receive file: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* friend_email = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* file_name = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* to_file_path = msg;


    std::string send;

    if(sql_receive_file(connect, email.c_str(), friend_email, file_name, send) == 0)
    {
        send = "接收文件失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    sendFile(client_socket, send.c_str(), to_file_path);

    send = "接收文件成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}


void ServerhandleFriendHistory(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    char* friend_email = msg;

    LogInfo("用户{}想要查看与{}的聊天记录", email, friend_email);

    std::string send;

    if(sql_friend_history(connect, email.c_str(), friend_email, send) == 0)
    {
        send = "获取聊天记录失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleGroupHistory(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    char* group_name = msg;

    LogInfo("用户{}想要查看群组{}的聊天记录", email, group_name);

    std::string send;

    int ret = sql_group_history(connect, email.c_str(), group_name, send);

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleHeartBeat(char* msg, int client_socket)
{
    std::string email = hashTable[client_socket];

    std::cout << "user want to send heart beat: " << client_socket << '\n'; 

    LogTrace("用户{}发送心跳包", email);

    std::string send = "心跳包";

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}
