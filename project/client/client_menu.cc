#include "client_menu.h"

static int is_valid_email(const char *email) 
{
    regex_t regex;
    int reti;

    // 正则表达式，匹配常见的邮箱格式
    std::string pattern = "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";

    // 编译正则表达式
    reti = regcomp(&regex, pattern.c_str(), REG_EXTENDED);
    if (reti) 
    {
        fprintf(stderr, "无法编译reti\n");
        exit(1);
    }

    // 执行正则表达式匹配
    reti = regexec(&regex, email, 0, NULL, 0);
    regfree(&regex);

    if (!reti) 
    {
        return 1; // 匹配成功，字符串符合邮箱格式
    } else {
        return 0; // 匹配失败，字符串不符合邮箱格式
    }
}

static void ClienthandleRegister(int sfd)
{
    std::string email;
    std::string passwd;
    std::string confirmPasswd;

    std::cout << "输入邮箱: ";
    std::cin >> email;

    if (!is_valid_email(email.c_str()))
    {
        std::cout << "邮箱格式不对";
        return;
    }

    static char verify_number[5]; // 用于存储结果的字符数组，包括最后一个位置用于'\0'
    srand(time(NULL)); // 设置随机数种子为当前时间
    int random_number = rand() % 9000 + 1000; // 生成1000到9999之间的随机数
    sprintf(verify_number, "%d", random_number); // 将随机数转换为字符串形式

    if(verify(email.c_str(), verify_number))
    {
        std::cout << "\n\n\n\n\t\t验证码发送失败"; 
        return;
    }
    else
    {
        std::cout << "\n\n\n\n\t\t已向你的邮箱发送验证码";
    } 

    char temp_number[5];
    std::cout << "\n\t\t请输入验证码: ";
    std::cin >> temp_number;

    if(strcmp(temp_number, verify_number) != 0)
    {
        std::cout << "\t\t验证失败\n";
        return;
    }
    else 
    {
        std::cout << "\t\t验证成功\n";
    }

    std::cout << "输入密码: ";

    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO; // 关闭回显
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cin >> passwd;

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // 恢复回显

    std::cout << std::endl; // 输出换行

    std::cout << "确认密码: ";

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO; // 关闭回显
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cin >> confirmPasswd;

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // 恢复回显

    std::cout << std::endl; // 输出换行

    // 检查密码和确认密码是否一致
    if (passwd != confirmPasswd)
    {
        std::cout << "密码不一致\n";
        return;
    }

    std::cout << "设置密码成功，输入你的名称: ";
    std::string name;
    std::cin >> name;

    std::string msg = email + " " + passwd + " " + name;

    char* cstr = new char[msg.size() + 1];

    if (cstr != nullptr)
        std::strcpy(cstr, msg.c_str());

    sendMsg(sfd, msg.c_str(), msg.size(), REGISTER);

    Type flag;
    recvMsg(sfd, &cstr, &flag);

    if(strcmp(cstr, "注册成功"))
        std::cout << "注册成功\n";

    // 在这里可以继续处理注册逻辑
    delete [] cstr;
}


static int handleLogin(int sfd)
{

}

static void handleForgetPasswd(int sfd)
{

}

static void handleChangePasswd(int sfd)
{

}

void enterChatroom(int sfd)
{
    for(;;)
    {
        std::cout << "1.注册     2.登录\n";
        std::cout << "3.忘记密码 4.修改密码\n";
        std::cout << "输入选项：";

        int choice;
        std::cin >> choice;

        switch (choice)
        {
            case 1: ClienthandleRegister(sfd); break; 
            case 2: if(handleLogin(sfd) == 1)   
                        return;
                    break;
            case 3: handleForgetPasswd(sfd); break;
            case 4: handleChangePasswd(sfd); break;
            default: std::cout << "无效选项\n";
        }
    }
}
