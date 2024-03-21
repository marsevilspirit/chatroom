#include <string>
#include <vector>
#include <iostream>

class Group{
public:
    Group(const std::string& name);
    ~Group();

    void addUser(const std::string& username);
    void removeUser(const std::string& username);
    void sendMessage(const std::string& sender, const std::string& message);

private:
    int gid;
    std::string groupName;
    std::vector<std::string> users;
};
