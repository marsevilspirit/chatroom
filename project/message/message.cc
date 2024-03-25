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
        int len = send(fd, buf, count, 0);
        if(len == -1)
        {
            if(errno == EPIPE || errno == ECONNRESET)
            {
                printf("Network connection reset or broken.\n");
                return -1;
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
    if (readn(cfd, (char*)&len, 4) == -1) 
    {
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

void handleGroupMessage(MYSQL* connect, char* msg, int client_socket, std::vector<int>& client_sockets)
{
    std::cout << "Received message from client: " << hashTable[client_socket] << '\n';
    std::cout << msg << '\n';

    std::string send = sql_getname(connect, hashTable[client_socket].c_str()) + ": \n" + msg;

    for(const auto& socket : client_sockets) 
    {
        if(sql_if_online(connect, hashTable[socket].c_str()) == 0)
            continue;

        if(socket != client_socket)
            sendMsg(socket, send.c_str(), send.size(), GROUP_MESSAGE);
    }
    free(msg);   
}

void ServerhandleRegister(char* msg, int client_socket, MYSQL* connect)
{
    std::cout << "New user want to register: " << client_socket << '\n';
    
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

    //为注册的用户创建列表
    if(sql_create_list(connect, email) == 0)
    {
        std::cerr << "fail to create friend table\n";
        send = "创建好友列表失败";
    }

    if(sql_create_group_list(connect, email) == 0)
    {
        std::cerr << "fail to create group table\n";
        send = "创建群组列表失败";
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleLogin(char* msg, int client_socket, MYSQL* connect)
{
    std::cout << "new user want to login: " << client_socket << '\n'; 

    char* email = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* password = msg;
    std::string send;

    int ret = sql_online(connect, email);

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


    bool find = sql_select(connect, email, password);


    if(find)
        send = "登录成功";
    else 
    { 
        send = "登录失败，密码错误";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(update_online_status(connect, email, 1) == 0)
    {
        std::cerr << "修改在线状态失败\n";
        send = "修改在线状态失败";
    }

    hashTable[client_socket] = email;

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);

}

void ServerhandleForgetPasswd(char* msg, int client_socket, MYSQL* connect)
{
    std::cout << "new user want to change password: " << client_socket << '\n'; 

    char* email = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* password = msg;
    std::string send;

    if(sql_online(connect, email) == 0)
    {
        send = "账号已登录";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(update_passwd(connect, email, password) == 0)
    {
        send = "密码更新失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    send = "密码更新成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
    
}

void ServerhandleDeleteAccount(char* msg, int client_socket, MYSQL* connect)
{
    std::cout << "new user want to delete account: " << client_socket << '\n'; 

    char* email = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* password = msg;
    std::string send;

    int ret = sql_online(connect, email);

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

    if(delete_account(connect, email) == 0)
    {
        send = "删除账号失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    handle_delete_friend(connect, email);

    if(sql_delete_list(connect, email) == 0)
    {
        std::cerr << "fail to delete table\n";
        send = "删除列表失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(sql_delete_group_list(connect, email) == 0)
    {
        std::cerr << "fail to delete group table\n";
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

    std::cout << "user want to add friend: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* friend_email = msg;

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
        std::cout << "send request------------------------------------\n";
        sql_request(connect, email.c_str(), friend_email);//好友申请
    }

    send = "添加好友成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandleDeleteFriend(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    std::cout << "user want to delete friend: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* friend_email = msg;

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

    std::cout << "user want to block friend: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* friend_email = msg;

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

    std::cout << "user want to unblock friend: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* friend_email = msg;

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

    std::cout << "user want to display friend: " << client_socket << '\n'; 

    std::string send;

    if(sql_display_friend(connect, email.c_str(), send) == 0)
    {
        send = "获取好友列表失败";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}

void ServerhandlePrivateMessage(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    std::cout << "user want to send private message: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* friend_email = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* message = msg;

    //在message后加上发送时间
    time_t now = time(0);
    char* dt = ctime(&now);
    std::string time = dt;
    time.pop_back();

    std::string send = sql_getname(connect, email.c_str()) + ": ("+ time + ")\n" + message + "\n";

    if(!is_your_friend(connect, email.c_str(), friend_email))
    {
        send = "您不是对方好友";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(sql_if_online(connect, friend_email) == 0)
    {
        send = "对方不在线";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    if(sql_if_block(connect, email.c_str(), friend_email))
    {
        send = "您已被对方屏蔽";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return;
    }

    for(const auto& socket : hashTable)
    {
        if(socket.second == friend_email)
        {
            sendMsg(socket.first, send.c_str(), send.size(), PRIVATE_MESSAGE);
            break;
        }
    }
}

void ServerhandleCreateGroup(char* msg, int client_socket, MYSQL* connect)
{
    std::string email = hashTable[client_socket];

    std::cout << "user want to create group: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* group_name = msg;

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

    std::cout << "user want to delete group: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* group_name = msg;

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

    std::cout << "user want to join group: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* group_name = msg;

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

    std::cout << "user want to exit group: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* group_name = msg;

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

    std::cout << "user want to display group list: " << client_socket << '\n'; 

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

    std::cout << "user want to display request list: " << client_socket << '\n';
    std::cout << group_name << '\n';

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

    std::cout << "user want to set manager: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* group_name = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* email = msg;

    std::string send;

    int ret = sql_set_manager(connect, my_email.c_str(), email, group_name);

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

    std::cout << "user want to add people in group: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* group_name = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* email = msg;

    std::string send;

    int ret = sql_real_add_group(connect, my_email.c_str(), email, group_name);

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

    std::cout << "user want to cancel manager: " << client_socket << '\n'; 
    std::cout << msg << '\n';

    char* group_name = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* email = msg;

    std::string send;

    int ret = sql_cancel_manager(connect, my_email.c_str(), email, group_name);

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

    std::cout << "user want to kick somebody: " << client_socket << '\n';
    std::cout << msg << '\n';

    char* group_name = msg;

    while(*msg != ' ' && *msg != '\0')
        msg++;

    *msg = '\0';
    msg++;
    char* email = msg;

    std::string send;

    if(if_group_exist(connect, group_name) == 1)
    {
        send = "群组不存在";
        sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
        return; 
    }

    if(if_master(connect, my_email.c_str(), group_name)) 
    {
        ret = sql_kick_anybody(connect, my_email.c_str(), email, group_name);
        if(ret == 0)
        {
            send = "踢人失败, 数据库错误";
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

    if(if_manager(connect, my_email.c_str(), group_name))
    {
        if(if_master(connect, email, group_name))
        {
            send = "你没权力踢群主";
            sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
            return;
        }

        ret = sql_kick_normal(connect, my_email.c_str(), email, group_name);
        if(ret == 0)
        {
            send = "踢人失败, 数据库错误";
            sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
            return;
        }
    }

    send = "踢" + std::string(email) + "成功";
    sendMsg(client_socket, send.c_str(), send.size(), SERVER_MESSAGE);
}
