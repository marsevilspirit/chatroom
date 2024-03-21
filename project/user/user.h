#include <string>

struct User{
    std::string name;
    std::string email;
    std::string passwd;
    bool        online;//在不在线
    int         status;//0为私聊，status为gid聊天
};
