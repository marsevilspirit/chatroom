#include "command_menu.h"
#include "../message/message.h"

static void clearInputBuffer() 
{
    std::cin.clear(); // 清除输入状态标志
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略剩余输入
}

static void add_friend(int sfd)
{
    std::cout << "请输入添加好友的邮箱: ";
    std::string email;
    std::cin >> email;

    sendMsg(sfd, email.c_str(), email.size(), ADD_FRIEND);
}

static void delete_friend(int sfd)
{
    std::cout << "请输入删除好友的邮箱: ";
    std::string email;
    std::cin >> email;

    sendMsg(sfd, email.c_str(), email.size(), DELETE_FRIEND);
}

static void block_friend(int sfd)
{
    std::cout << "请输入屏蔽好友的邮箱: ";
    std::string email;
    std::cin >> email;

    sendMsg(sfd, email.c_str(), email.size(), BLOCK_FRIEND);
}

static void unblock_friend(int sfd)
{
    std::cout << "请输入解除屏蔽好友的邮箱: ";
    std::string email;
    std::cin >> email;

    sendMsg(sfd, email.c_str(), email.size(), UNBLOCK_FRIEND);
}

static void display_friend(int sfd)
{
    std::string msg = "display_friend";
    std::cout << msg << '\n';
    sendMsg(sfd, msg.c_str(), msg.size(), DISPLAY_FRIEND);
}

static void friend_menu(int sfd) 
{
    std::cout << "1.添加好友 2.删除好友\n";
    std::cout << "3.屏蔽好友 4.解除屏蔽\n";
    std::cout << "5.好友列表 6.返回\n";

    char command;
    std::cin >> command;
    clearInputBuffer();

    switch (command)
    {
        case '1':     add_friend(sfd);                          break; 
        case '2':     delete_friend(sfd);                       break;
        case '3':     block_friend(sfd);                        break;
        case '4':     unblock_friend(sfd);                      break;
        case '5':     display_friend(sfd);                      break;
        case '6':     std::cout << "退出好友操作界面\n";        return;
        case 'h': 
                      std::cout << "1.添加好友 2.删除好友\n";
                      std::cout << "3.屏蔽好友 4.解除屏蔽\n";
                      std::cout << "5.好友列表 6.返回\n";
                      break;
    }
}

static void private_chat(int sfd)
{
    std::string friend_email;
    std::cout << "请输入对方的邮箱: ";
    std::cin >> friend_email;

    std::string msg;
    std::cout << "进入私聊: \n";

    clearInputBuffer();

    while (msg != "exit")
    {
        std::getline(std::cin, msg); // 读取整行输入

        std::cout << "msg: " << msg << '\n';

        std::string final_msg = friend_email + " " + msg;

        usleep(1000); // 暂停一毫秒

        if (sendMsg(sfd, final_msg.c_str(), final_msg.size(), PRIVATE_MESSAGE) == -1) 
        {
            std::cout << "无效信息，发送失败\n";
            continue;
        }

        std::cout << '\n';

        usleep(10000);
    }

    std::cout << "退出私聊\n";
}

static void create_group(int sfd)
{
    std::string group_name;
    std::cout << "请输入群聊名称: ";
    std::cin >> group_name;

    std::string send = group_name;
    sendMsg(sfd, send.c_str(), send.size(), CREATE_GROUP);
}

static void delete_group(int sfd)
{
    std::string group_name;
    std::cout << "请输入解散群聊的名称: ";
    std::cin >> group_name;

    std::string send = group_name;
    sendMsg(sfd, send.c_str(), send.size(), DELETE_GROUP);
}

static void request_join_group(int sfd)
{
    std::string group_name;
    std::cout << "请输入加入群聊的名称: ";
    std::cin >> group_name;

    std::string send = group_name;
    sendMsg(sfd, send.c_str(), send.size(), JOIN_GROUP);
}

static void exit_group(int sfd)
{
    std::string group_name;
    std::cout << "请输入退出群聊的名称: ";
    std::cin >> group_name;

    std::string send = group_name;
    sendMsg(sfd, send.c_str(), send.size(), EXIT_GROUP);
}

static void display_list_group(int sfd)
{
    std::string msg = "display_group";
    sendMsg(sfd, msg.c_str(), msg.size(), DISPLAY_GROUP);
}

static void display_request_list(int sfd)
{
    std::string send;
    std::cout << "请输入群聊名称：";
    std::cin >> send;

    sendMsg(sfd, send.c_str(), send.size(), DISPLAY_GROUP_REQUEST);
}

static void set_manager(int sfd)
{
    std::string group_name;
    std::string email;
    std::cout << "请输入群聊名称: ";
    std::cin >> group_name;
    std::cout << "请输入要设置为管理员的邮箱: ";
    std::cin >> email;

    std::string send = group_name + " " + email;
    sendMsg(sfd, send.c_str(), send.size(), SET_MANAGER);
}

static void add_people_in_group(int sfd)
{
    std::string group_name;
    std::string email;
    std::cout << "请输入群聊名称: ";
    std::cin >> group_name;
    std::cout << "请输入要拉进群的邮箱: ";
    std::cin >> email;

    std::string send = group_name + " " + email;
    sendMsg(sfd, send.c_str(), send.size(), ADD_GROUP);
}

static void cancel_manager(int sfd)
{
    std::string group_name;
    std::string email;
    std::cout << "请输入群聊名称: ";
    std::cin >> group_name;
    std::cout << "请输入要取消管理员的邮箱: ";
    std::cin >> email;

    std::string send = group_name + " " + email;
    sendMsg(sfd, send.c_str(), send.size(), CANCEL_MANAGER);
}

static void kick_somebody(int sfd)
{
    std::string group_name;
    std::string email;
    std::cout << "请输入群聊名称: ";
    std::cin >> group_name;
    std::cout << "请输入要踢出群聊的邮箱: ";
    std::cin >> email;

    std::string send = group_name + " " + email;
    sendMsg(sfd, send.c_str(), send.size(), KICK_SOMEBODY);
}

static void display_group_member(int sfd)
{
    std::string group_name;
    std::cout << "请输入群聊名称: ";
    std::cin >> group_name;

    std::string send = group_name;
    sendMsg(sfd, send.c_str(), send.size(), DISPLAY_GROUP_MEMBER);
}

static void group_menu(int sfd)
{
    std::cout << "群操作界面\n";
    std::cout << "a.创建群聊    b.申请加入群聊\n";
    std::cout << "c.退出群聊    d.解散群聊\n";
    std::cout << "e.群聊列表    f.设置管理员\n";
    std::cout << "g.取消管理员  i.踢人\n";
    std::cout << "j.群聊成员    k.申请列表\n";
    std::cout << "l.拉人进群    m.退出\n";

    char command;
    std::cin >> command;
    clearInputBuffer();

    switch (command)
    {
        case 'a':    create_group(sfd);         break;
        case 'b':    request_join_group(sfd);   break;
        case 'c':    exit_group(sfd);           break;
        case 'd':    delete_group(sfd);         break;
        case 'e':    display_list_group(sfd);   break;
        case 'f':    set_manager(sfd);          break;
        case 'g':    cancel_manager(sfd);       break;
        case 'i':    kick_somebody(sfd);        break;
        case 'j':    display_group_member(sfd); break;
        case 'k':    display_request_list(sfd); break;
        case 'l':    add_people_in_group(sfd);  break;
        case 'm':    std::cout << "退出群操作界面\n";         return;
        case 'h': 
                     std::cout << "群操作界面\n";
                     std::cout << "a.创建群聊    b.加入群聊\n";
                     std::cout << "c.退出群聊    d.解散群聊\n";
                     std::cout << "e.群聊列表    f.设置管理员\n";
                     std::cout << "g.删除管理员  i.踢人\n";
                     std::cout << "j.群聊成员    k.申请列表\n";
                     std::cout << "l.拉人进群    m.退出\n";
                     break;
    }
}

void group_chat(int sfd)
{
    std::string group_name;
    std::cout << "请输入群聊名称: ";
    std::cin >> group_name;

    std::string msg;
    std::cout << "进入群聊: \n";

    clearInputBuffer();

    while (msg != "exit")
    {
        std::getline(std::cin, msg); // 读取整行输入

        std::cout << "msg: " << msg << '\n';

        std::string final_msg = group_name + " " + msg;

        usleep(1000); // 暂停一毫秒

        if (sendMsg(sfd, final_msg.c_str(), final_msg.size(), GROUP_MESSAGE) == -1) 
        {
            std::cout << "无效信息，发送失败\n";
            continue;
        }

        std::cout << '\n';

        usleep(10000);
    }

    std::cout << "退出群聊\n";
}

void send_file(int sfd)
{
    std::string resver;
    std::cout << "请输入对方的邮箱: ";
    std::cin >> resver;

    std::string file_path;
    std::cout << "请输入文件路径: ";
    std::cin >> file_path;

    std::string file_name;
    std::cout << "请输入文件名: ";
    std::cin >> file_name;

    sendFile(sfd, file_name.c_str(), file_path.c_str(), resver.c_str());
}

void receive_file(int sfd)
{
    std::string friend_email;
    std::cout << "请输入对方的邮箱: ";
    std::cin >> friend_email;

    std::string file_name;
    std::cout << "请输入文件名: ";
    std::cin >> file_name;

    std::string file_path;
    std::cout << "请输入文件路径:";
    std::cin  >> file_path;

    std::string send = friend_email + " " + file_name;

    char* buffer = nullptr;
    recvFile(sfd, buffer, send.size(), file_path.c_str());
}

void commandMenu(int sfd)
{
    std::cout << "command界面\n";
    std::cout << "1.私聊      2.群聊\n";
    std::cout << "3.发送文件  4.接受文件\n";
    std::cout << "5.好友操作  6.群操作\n";
    std::cout << "7.退出\n";

    while(true)
    {
        char command;
        std::cin >> command;
        clearInputBuffer(); // 清空输入缓冲区

        switch (command)
        {
            case '1':     private_chat(sfd);                 break;
            case '2':     group_chat(sfd);                   break;
            case '3':     send_file(sfd);                    break;
            case '4':     receive_file(sfd);                 break;
            case '5':     friend_menu(sfd);                  break;
            case '6':     group_menu(sfd);                   break;
            case '7':     std::cout << "退出command界面\n";  return;
            case 'h': 
                          std::cout << "command界面\n";
                          std::cout << "1.私聊      2.群聊\n";
                          std::cout << "3.发送文件  4.查看文件\n";
                          std::cout << "5.好友操作  6.群操作\n";
                          std::cout << "7.退出\n";
                          break;
        }
    }
}

