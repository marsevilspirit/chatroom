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

    int command;
    std::cin >> command;
    clearInputBuffer();

    switch (command)
    {
        case 1: add_friend(sfd);     break; 
        case 2: delete_friend(sfd);  break;
        case 3: block_friend(sfd);   break;
        case 4: unblock_friend(sfd); break;
        case 5: display_friend(sfd); break;
        case 6: return;
    }
}

void commandMenu(int sfd)
{
    std::cout << "1.私聊      2.群聊\n";
    std::cout << "3.发送文件  4.查看文件\n";
    std::cout << "5.好友操作  6.群操作\n";
    std::cout << "7.黑名单    8.退出\n";

    while(true)
    {
        int command;
        std::cin >> command;
        clearInputBuffer(); // 清空输入缓冲区

        switch (command)
        {
            case 1:  break;
            case 2:  break;
            case 3:  break;
            case 4:  break;
            case 5: friend_menu(sfd); break;
            case 6:  break;
            case 7:  break;
            case 8:  return;
        }
    }
}

