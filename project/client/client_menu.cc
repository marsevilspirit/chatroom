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
        exit(EXIT_FAILURE);
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
        std::cout << "邮箱格式不对\n";
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

    std::cout << '\n'; // 输出换行

    std::cout << "确认密码: ";

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO; // 关闭回显
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cin >> confirmPasswd;

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // 恢复回显

    std::cout << '\n'; // 输出换行

    // 检查密码和确认密码是否一致
    if (passwd != confirmPasswd)
    {
        std::cout << "密码不一致\n";
        return;
    }

    // 使用SHA-256哈希函数对密码进行加密
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        std::cerr << "创建EVP_MD_CTX失败\n";
        return;
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr))
    {
        std::cerr << "初始化哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return;
    }

    if (!EVP_DigestUpdate(ctx, passwd.c_str(), passwd.size()))
    {
        std::cerr << "更新哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (!EVP_DigestFinal_ex(ctx, hash, &hashLen))
    {
        std::cerr << "完成哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return;
    }

    EVP_MD_CTX_free(ctx);

    // 将哈希值转换为十六进制字符串
    std::string hashedPasswd;
    char hexHash[hashLen * 2 + 1];
    for (unsigned int i = 0; i < hashLen; i++)
    {
        sprintf(hexHash + (i * 2), "%02x", hash[i]);
    }
    hexHash[hashLen * 2] = '\0';
    hashedPasswd = hexHash;

    std::cout << "设置密码成功，输入你的名称: ";
    std::string name;
    std::cin >> name;

    std::string msg = "{\"email\": \"" + email + "\", \"passwd\": \"" + hashedPasswd + "\", \"name\": \"" + name + "\"}";

    sendMsg(sfd, msg.c_str(), msg.size(), REGISTER);

    char* cstr; 
    Type flag;
    recvMsg(sfd, &cstr, &flag);

    std::cout << cstr << '\n';
}


static int ClienthandleLogin(int sfd)
{
    std::string email;
    std::string passwd;

    std::cout << "输入账号: ";
    std::cin >> email;

    /*
    if (!is_valid_email(email.c_str()))
    {
        std::cout << "邮箱格式不对\n";
        return 0;
    }
    */

    std::cout << "输入密码: ";

    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO; // 关闭回显
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cin >> passwd;

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // 恢复回显

    std::cout << '\n'; // 输出换行

        // 使用SHA-256哈希函数对密码进行加密
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        std::cerr << "创建EVP_MD_CTX失败\n";
        return 0;
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr))
    {
        std::cerr << "初始化哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return 0;
    }

    if (!EVP_DigestUpdate(ctx, passwd.c_str(), passwd.size()))
    {
        std::cerr << "更新哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return 0;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (!EVP_DigestFinal_ex(ctx, hash, &hashLen))
    {
        std::cerr << "完成哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return 0;
    }

    EVP_MD_CTX_free(ctx);

    // 将哈希值转换为十六进制字符串
    std::string hashedPasswd;
    char hexHash[hashLen * 2 + 1];
    for (unsigned int i = 0; i < hashLen; i++)
    {
        sprintf(hexHash + (i * 2), "%02x", hash[i]);
    }
    hexHash[hashLen * 2] = '\0';
    hashedPasswd = hexHash;


    std::string msg = "{\"email\": \"" + email + "\", \"passwd\": \"" + hashedPasswd + "\"}";

    sendMsg(sfd, msg.c_str(), msg.size(), LOGIN);

    char* cstr; 
    Type flag;
    recvMsg(sfd, &cstr, &flag);

    std::cout << cstr << '\n';

    if(strcmp(cstr, "登录成功") == 0)
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

static void ClienthandleForgetPasswd(int sfd)
{
    std::string email;
    std::string passwd;
    std::string confirmPasswd;
     
    std::cout << "输入账号: ";
    std::cin >> email;

    if (!is_valid_email(email.c_str()))
    {
        std::cout << "邮箱格式不对\n";
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

    std::cout << '\n'; // 输出换行

    std::cout << "确认密码: ";

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO; // 关闭回显
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cin >> confirmPasswd;

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // 恢复回显

    std::cout << '\n'; // 输出换行

            // 使用SHA-256哈希函数对密码进行加密
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        std::cerr << "创建EVP_MD_CTX失败\n";
        return;
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr))
    {
        std::cerr << "初始化哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return;
    }

    if (!EVP_DigestUpdate(ctx, passwd.c_str(), passwd.size()))
    {
        std::cerr << "更新哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (!EVP_DigestFinal_ex(ctx, hash, &hashLen))
    {
        std::cerr << "完成哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return;
    }

    EVP_MD_CTX_free(ctx);

    // 将哈希值转换为十六进制字符串
    std::string hashedPasswd;
    char hexHash[hashLen * 2 + 1];
    for (unsigned int i = 0; i < hashLen; i++)
    {
        sprintf(hexHash + (i * 2), "%02x", hash[i]);
    }
    hexHash[hashLen * 2] = '\0';
    hashedPasswd = hexHash;


    // 检查密码和确认密码是否一致
    if (passwd != confirmPasswd)
    {
        std::cout << "密码不一致\n";
        return;
    }

    std::string msg = "{\"email\": \"" + email + "\", \"passwd\": \"" + hashedPasswd + "\"}";

    sendMsg(sfd, msg.c_str(), msg.size(), FORGET_PASSWD); 

    char* cstr; 
    Type flag;
    recvMsg(sfd, &cstr, &flag);

    std::cout << cstr << '\n';
}

void ClienthandleDeleteAccount(int sfd)
{
    std::string email;
    std::string passwd;

    std::cout << "输入邮箱: "; 
    std::cin >> email;

    if (!is_valid_email(email.c_str()))
    {
        std::cout << "邮箱格式不对\n";
        return;
    }

    std::cout << "输入密码: ";

    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO; // 关闭回显
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    std::cin >> passwd;

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // 恢复回显

    std::cout << '\n'; // 输出换行

            // 使用SHA-256哈希函数对密码进行加密
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
    {
        std::cerr << "创建EVP_MD_CTX失败\n";
        return;
    }

    if (!EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr))
    {
        std::cerr << "初始化哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return;
    }

    if (!EVP_DigestUpdate(ctx, passwd.c_str(), passwd.size()))
    {
        std::cerr << "更新哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return;
    }

    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen;
    if (!EVP_DigestFinal_ex(ctx, hash, &hashLen))
    {
        std::cerr << "完成哈希函数失败\n";
        EVP_MD_CTX_free(ctx);
        return;
    }

    EVP_MD_CTX_free(ctx);

    // 将哈希值转换为十六进制字符串
    std::string hashedPasswd;
    char hexHash[hashLen * 2 + 1];
    for (unsigned int i = 0; i < hashLen; i++)
    {
        sprintf(hexHash + (i * 2), "%02x", hash[i]);
    }
    hexHash[hashLen * 2] = '\0';
    hashedPasswd = hexHash;


    std::string choice;
    std::cout << "你确定要删除账号吗？(y/n): ";
    std::cin >> choice;

    if(choice != "y")
    {
        return;
    }

    std::string msg = "{\"email\": \"" + email + "\", \"passwd\": \"" + hashedPasswd + "\"}";
    
    sendMsg(sfd, msg.c_str(), msg.size(), DELETE_ACCOUNT);

    char* cstr;

    Type flag;
    recvMsg(sfd, &cstr, &flag);

    std::cout << cstr << '\n';
}

static void clearInputBuffer() 
{
    std::cin.clear(); // 清除输入状态标志
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // 忽略剩余输入
}

void enterChatroom(int sfd)
{
    for(;;)
    {
        std::cout << "1.注册     2.登录\n";
        std::cout << "3.忘记密码 4.删除账号\n";
        std::cout << "输入选项：";

        int choice;
        std::cin >> choice;
        clearInputBuffer(); // 清空输入缓冲区

        switch (choice)
        {
            case 1: ClienthandleRegister(sfd); break; 
            case 2: if(ClienthandleLogin(sfd) == 1)   
                        return;
                    break;
            case 3: ClienthandleForgetPasswd(sfd); break;
            case 4: ClienthandleDeleteAccount(sfd); break;
            default: std::cout << "无效选项\n";
        }
    }
}
