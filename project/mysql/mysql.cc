#include "mysql.h"

MYSQL* sql_init(MYSQL* connect)
{
    connect = mysql_init(nullptr);  // 初始化 MySQL 连接
    if (!connect) {
        std::cerr << "Error initializing MySQL connection\n";
    }

    // 连接 MySQL 服务器（此处数据库名可以是任意存在的数据库，如 "mysql"）
    if (!mysql_real_connect(connect, "localhost", "root", "661188", nullptr, 0, nullptr, 0)) {
        std::cerr << "Error connecting to MySQL server: " << mysql_error(connect) << '\n';
        mysql_close(connect);
        return nullptr;
    }

    // 创建 chatroom 数据库
    if (mysql_query(connect, "CREATE DATABASE IF NOT EXISTS chatroom")) {
        std::cerr << "Error creating database: " << mysql_error(connect) << '\n';
        mysql_close(connect);
        return nullptr;
    }

    // 选择 chatroom 数据库
    if (mysql_select_db(connect, "chatroom")) {
        std::cerr << "Error selecting database: " << mysql_error(connect) << '\n';
        mysql_close(connect);
        return nullptr;
    }

    // 创建账户表格
    if (mysql_query(connect, "CREATE TABLE IF NOT EXISTS accounts (\
                email VARCHAR(255) PRIMARY KEY,\
                password VARCHAR(255) NOT NULL,\
                name VARCHAR(255) NOT NULL,\
                online_status BOOLEAN DEFAULT false)")) 
                {
                    std::cerr << "Error creating table: " << mysql_error(connect) << '\n';
                    mysql_close(connect);
                }

    std::string query = "UPDATE accounts SET online_status = 0";
    if (mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error querying database: " << mysql_error(connect) << '\n';
    }
     
    query = "CREATE TABLE IF NOT EXISTS group_list (group_name VARCHAR(255) PRIMARY KEY);";
    if (mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error querying database: " << mysql_error(connect) << '\n';
    }

    query = "CREATE TABLE IF NOT EXISTS file_list (file_name VARCHAR(255) NOT NULL, file_path VARCHAR(255) NOT NULL, sender VARCHAR(255) NOT NULL, resver VARCHAR(255) NOT NULL)";
    if (mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error querying database: " << mysql_error(connect) << '\n';
    }

    // 创建好友消息历史记录
    query = "CREATE TABLE IF NOT EXISTS friend_message_list (\
            sender VARCHAR(255) NOT NULL,\
            resver VARCHAR(255) NOT NULL,\
            message TEXT NOT NULL,\
            time DATETIME NOT NULL)";
    if (mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error querying database: " << mysql_error(connect) << '\n';
    }

    // 创建群组消息历史记录
    query = "CREATE TABLE IF NOT EXISTS group_message_list (\
            group_name VARCHAR(255) NOT NULL,\
            sender VARCHAR(255) NOT NULL,\
            message TEXT NOT NULL,\
            time DATETIME NOT NULL)";
    if (mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error querying database: " << mysql_error(connect) << '\n';
    }

    std::cout << "database ready\n";

    return connect;
}

int sql_insert(MYSQL* connect, const char* email, const char* password, const char* name)
{
    // 构造插入 SQL 语句
    std::string sql_query = "INSERT INTO accounts (email, password, name) VALUES ('" +
        std::string(email) + "', '" + std::string(password) + "', '" +
        std::string(name) + "')";
    // 执行 SQL 语句
    if (mysql_query(connect, sql_query.c_str())) 
    {
        LogError("向表中插入数据失败: {}", mysql_error(connect));

        return 0;
    }

    return 1;
}

int sql_select(MYSQL* connect, const char* email, const char* password) 
{
    // 构建 SQL 查询语句
    std::string query = "SELECT 1 FROM accounts WHERE email='" + std::string(email) + "' AND password='" + std::string(password) + "' LIMIT 1";

    // 执行 SQL 查询
    if (mysql_query(connect, query.c_str())) 
    {
        LogError("查询数据库失败: {}", mysql_error(connect));
        return 0; // 查询失败
    }

    // 获取查询结果
    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("获取查询结果失败: {}", mysql_error(connect));
        return 0; // 获取结果失败
    }

    // 判断是否有匹配的记录
    int count = mysql_num_rows(result);

    // 释放查询结果
    mysql_free_result(result);

    // 返回查询结果，1 表示匹配，0 表示不匹配
    return (count > 0) ? 1 : 0;
}

std::string getTypeByEmail(MYSQL* connect, const std::string& list, const std::string& email)
{
    // 创建查询字符串
    std::string query = "SELECT type FROM " + list + " WHERE email = '" + email + "';";

    // 执行查询
    if (mysql_query(connect, query.c_str())) {
        LogError("MySQL query error: {}", mysql_error(connect));
        return "";
    }

    // 获取查询结果
    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) {
        LogError("MySQL store result error: {}", mysql_error(connect));
        return "";
    }

    // 获取结果中的一行
    MYSQL_ROW row = mysql_fetch_row(result);
    std::string type;

    if (row) {
        type = row[0];
    } else {
        LogError( "No result found for email: {}", email);
        type = "";
    }

    // 释放结果集
    mysql_free_result(result);

    return type;
}

int if_exist(MYSQL* connect, const char* email)// 1存在，0不存在
{
    std::string query = "SELECT 1 FROM accounts WHERE email ='" + std::string(email) + "' LIMIT 1";

    if (mysql_query(connect, query.c_str()))
    {
        LogError("查询数据库失败: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr)
    {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    int count = mysql_num_rows(result);

    return (count > 0) ? 1 : 0;
}

int sql_online(MYSQL* connect, const char* email)// 1成功，0数据库错误, 2用户不存在, 3用户已登录
{
    if(if_exist(connect, email) == 0)//用户不存在
    {
        LogError("Error: {} does not exist", email);
        return 2;
    }

    std::string query = "SELECT 1 FROM accounts WHERE email ='" + std::string(email) + "' AND online_status='0' LIMIT 1";

    if (mysql_query(connect, query.c_str()))
    {
        LogError("Error querying database: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr)
    {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    int count = mysql_num_rows(result);

    return (count > 0) ? 1 : 3;
}

int sql_if_online(MYSQL* connect, const char* email)
{
    std::string query = "SELECT 1 FROM accounts WHERE email ='" + std::string(email) + "' AND online_status='1' LIMIT 1";

    if (mysql_query(connect, query.c_str()))
    {
        LogError("Error querying database: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr)
    {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    int count = mysql_num_rows(result);

    return (count > 0) ? 1 : 0;
}

int update_online_status(MYSQL* connect, const char* email, int status) 
{
    std::string query = "UPDATE accounts SET online_status = " + std::to_string(status) + " WHERE email = '" + std::string(email) + "'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error updating online status: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

int update_passwd(MYSQL* connect, const char* email, const char* password)
{
    std::string query = "UPDATE accounts SET password = '" + std::string(password) + "' WHERE email = '" + std::string(email) + "'";

    std::cout << query << '\n';

    if(mysql_query(connect, query.c_str()))
    {
        LogError("Error updating password: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

int delete_account(MYSQL* connect, const char* email, const char* password)
{
    //检查 email 和 password 是否匹配
    if(sql_select(connect, email, password) == 0)
    {
        LogError("Error: {} and {} does not match", email, password);
        return 0;
    }

    std::string query = "DELETE FROM accounts WHERE email = '" + std::string(email) + "';"; 

    if(mysql_query(connect, query.c_str()))
    {
        LogError("Error deleting account: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

std::string sql_getname(MYSQL* connect, const char* email)
{
    std::string query = "SELECT name FROM accounts WHERE email = '" + std::string(email) + "'";

    if(mysql_query(connect, query.c_str()))
    {
        LogError("Error querying database: {}", mysql_error(connect));
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if(result == nullptr)
    {
        LogError("Error storing result: {}", mysql_error(connect));
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    std::string name = row[0];

    return name;
}

int sql_create_list(MYSQL* connect, const char* email) 
{
    //替换@符号
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "CREATE TABLE IF NOT EXISTS " + emailStr + "_list (\
            email VARCHAR(255) PRIMARY KEY,\
            type VARCHAR(255) NOT NULL)";

    LogInfo("create list {}", email);

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error creating list: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

int handle_delete_friend(MYSQL* connect, const char* email)
{
    //替换@符号
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    //查询列表中所有好友
    std::string query = "SELECT email FROM " + emailStr + "_list WHERE type = 'friend' OR type = 'request'";
    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error querying database: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) 
    {
        std::string friend_email = row[0];
            //替换@符号
        std::replace(friend_email.begin(), friend_email.end(), '@', '0');

        //去除.之后的内容
        std::string::size_type pos = friend_email.find('.');
        if (pos != std::string::npos) 
        {
            friend_email = friend_email.substr(0, pos);
        }

        std::string query = "DELETE FROM " + friend_email + "_list WHERE email = '" + std::string(email) + "'";
        if (mysql_query(connect, query.c_str())) 
        {
            LogError("Error deleting friend: {}", mysql_error(connect));
            return 0;
        }
    }

    mysql_free_result(result);

    return 1;
}

int sql_delete_list(MYSQL* connect, const char* email) 
{
    //替换@符号
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "DROP TABLE " + emailStr + "_list";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error deleting list: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

bool is_your_friend_or_request(MYSQL* connect, const char* email, const char* friend_email)
{
    std::string emailStr = std::string(friend_email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "SELECT COUNT(*) FROM " + emailStr + "_list WHERE email = '" + std::string(email) + "' AND type = 'friend' OR email = '" + std::string(email) + "' AND type = 'request'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error checking friend: {}", mysql_error(connect));
        return false; // 默认不是好友
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("Error retrieving result: {}", mysql_error(connect));
        return false; // 默认不是好友
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr || std::stoi(row[0]) == 0) 
    {
        mysql_free_result(result);
        return false; // 不是好友
    }

    mysql_free_result(result);
    return true; // 是好友
}

bool is_your_friend(MYSQL* connect, const char* email, const char* friend_email)
{
    std::string emailStr = std::string(friend_email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    /*
    std::string query = "SELECT COUNT(*) FROM " + emailStr + "_list WHERE email = '" + std::string(email) + "' AND type = 'friend'";

    if (mysql_query(connect, query.c_str())) 
    {
        std::cerr << "Error checking friend: " << mysql_error(connect) << '\n';
        return false; // 默认不是好友
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        std::cerr << "Error retrieving result: " << mysql_error(connect) << '\n';
        return false; // 默认不是好友
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr || std::stoi(row[0]) == 0) 
    {
        mysql_free_result(result);
        return false; // 不是好友
    }

    mysql_free_result(result);
    return true; // 是好友
    */

    std::string type = getTypeByEmail(connect, emailStr, email);

    return type == "friend";
}

bool is_your_friend_or_block(MYSQL* connect, const char *email, const char *friend_email)
{
    std::string emailStr = std::string(friend_email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }
    
    std::string type = getTypeByEmail(connect, emailStr, email);

    return type == "friend" || type == "block";
}

bool is_my_friend_or_request(MYSQL* connect, const char* email, const char* friend_email)
{
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "SELECT COUNT(*) FROM " + emailStr + "_list WHERE email = '" + std::string(friend_email) + "' AND type = 'friend' OR email = '" + std::string(friend_email) + "' AND type = 'request'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error checking friend: {}", mysql_error(connect));
        return false; // 默认不是好友
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("Error retrieving result: {}", mysql_error(connect));
        return false; // 默认不是好友
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr || std::stoi(row[0]) == 0) 
    {
        mysql_free_result(result);
        return false; // 不是好友
    }

    mysql_free_result(result);
    return true; // 是好友
}

bool is_my_friend(MYSQL* connect, const char* email, const char* friend_email)
{
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "SELECT COUNT(*) FROM " + emailStr + "_list WHERE email = '" + std::string(friend_email) + "' AND type = 'friend'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error checking friend: {}", mysql_error(connect));
        return false; // 默认不是好友
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("Error retrieving result: {}", mysql_error(connect));
        return false; // 默认不是好友
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr || std::stoi(row[0]) == 0) 
    {
        mysql_free_result(result);
        return false; // 不是好友
    }

    mysql_free_result(result);
    return true; // 是好友
}

int sql_add_friend(MYSQL* connect, const char* email, const char* friend_email)//1成功，0数据库错误, 2用户不存在,3不能加自己
{
    if(email == friend_email)
    {
        LogError("Error: You can't add yourself as a friend\n");
        return 3;
    }

    //替换@符号
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    if(if_exist(connect, friend_email) == 0)
    {
        LogError("Error: {} does not exist", friend_email);
        return 2;
    }

    std::string query;
    if(!is_my_friend_or_request(connect, email, friend_email))
        query = "INSERT INTO " + emailStr + "_list (email, type) VALUES ('" + std::string(friend_email) + "', 'friend')";
    else 
        query = "UPDATE " + emailStr + "_list SET type = 'friend' WHERE email = '" + std::string(friend_email) + "'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error adding friend: {}", mysql_error(connect));
        return 0;
    }

    return 1; 
}

int sql_request(MYSQL* connect,const char* email,const char* friend_email)
{
    //替换@符号
    std::string emailStr = std::string(friend_email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "INSERT INTO " + emailStr + "_list (email, type) VALUES ('" + std::string(email) + "', 'request')";

    if (mysql_query(connect, query.c_str())) 
    {
        std::cerr << "Error requesting friend: " << mysql_error(connect) << '\n';
        LogError("Error requesting friend: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

int sql_delete_friend(MYSQL* connect, const char* email, const char* friend_email)//1成功，0数据库错误, 2不是好友
{
    if (!is_my_friend(connect, email, friend_email)) 
    {
        LogError("Error: {} is not your friend", friend_email);
        return 2;
    }

    //替换@符号
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "DELETE FROM " + emailStr + "_list WHERE email = '" + std::string(friend_email) + "'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error deleting friend: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

bool is_my_block(MYSQL* connect, const char* email, const char* friend_email)
{
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "SELECT COUNT(*) FROM " + emailStr + "_list WHERE email = '" + std::string(friend_email) + "' AND type = 'block'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error checking friend: {}", mysql_error(connect));
        return false; // 默认不是好友
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("Error retrieving result: {}", mysql_error(connect));
        return false; // 默认不是好友
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr || std::stoi(row[0]) == 0) 
    {
        mysql_free_result(result);
        return false; // 不是好友
    }

    mysql_free_result(result);
    return true; // 是好友
}

int sql_block_friend(MYSQL* connect, const char* email, const char* friend_email)// 1成功，0数据库错误, 2不是好友
{
    if (!is_my_friend(connect, email, friend_email)) 
    {
        LogError("Error: {} is not your friend", friend_email);
        return 2;
    }

    //替换@符号
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "UPDATE " + emailStr + "_list SET type = 'block' WHERE email = '" + std::string(friend_email) + "'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error blocking friend: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

int sql_if_block(MYSQL* connect, const char* email, const char* friend_email)
{
    std::string emailStr = std::string(friend_email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "SELECT type FROM " + emailStr + "_list WHERE email = '" + std::string(email) + "'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error executing SQL query: {}", mysql_error(connect));
        return 0; // 查询执行失败，返回 0
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("Error retrieving result: {}", mysql_error(connect));
        return 0; // 获取结果失败，返回 0
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    if (row == nullptr || std::string(row[0]) != "block") 
    {
        mysql_free_result(result);
        return 0; // 如果结果为空或者 type 字段不是 "block"，返回 0
    }

    mysql_free_result(result);
    return 1; // type 字段是 "block"，返回 1
}

int sql_unblock_friend(MYSQL* connect, const char* email, const char* friend_email)//1成功，2没被屏蔽，0数据库错误
{
    if (!is_my_block(connect, email, friend_email)) 
    {
        LogError("Error: {} is not your block", friend_email);
        return 2;
    }

    //替换@符号
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "UPDATE " + emailStr + "_list SET type = 'friend' WHERE email = '" + std::string(friend_email) + "'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error unblocking friend: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

int sql_display_friend(MYSQL* connect, const char* email, std::string& send)
{
    //替换@符号
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "SELECT email, type FROM " + emailStr + "_list WHERE type = 'friend' OR type = 'block' OR type = 'request'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error displaying friend: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) 
    {
        std::string friend_email = row[0];
        if(sql_if_online(connect, friend_email.c_str()))
        {
            send += "online ";
        }
        else 
        {
            send += "leave  ";
        } 
        send += std::string(row[0]) + ' ' + std::string(row[1]) + '\n';
    }

    mysql_free_result(result);
    return 1;
}

int sql_display_friend_request(MYSQL* connect, const char* email, std::string& send)
{
    //替换@符号
    std::string emailStr = std::string(email);
    std::replace(emailStr.begin(), emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = emailStr.find('.');
    if (pos != std::string::npos) 
    {
        emailStr = emailStr.substr(0, pos);
    }

    std::string query = "SELECT email FROM " + emailStr + "_list WHERE type = 'request'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error displaying friend request: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) 
    {
        send += std::string(row[0]) + '\n';
    }

    mysql_free_result(result);
    return 1;
}

//file
int sql_file_list(MYSQL* connect, const char* file_name, const char* savePath, const char* sender, const char* resver)//firend 1, not friend 2, database error 0
{
    if(is_my_friend(connect, sender, resver)) 
    {
        int flag = 0;;

        std::string query = "SELECT 1 FROM file_list WHERE file_name = '" + std::string(file_name) + "' AND sender = '" + std::string(sender) + "' AND resver = '" + std::string(resver) + "'";

        if (mysql_query(connect, query.c_str())) 
        {
            LogError("Error checking file: {}", mysql_error(connect));
            return 0;
        }

        MYSQL_RES* result = mysql_store_result(connect);

        if (result == nullptr) 
        {
            LogError("Error storing result: {}", mysql_error(connect));
            return 0;
        }

        MYSQL_ROW row = mysql_fetch_row(result);

        if (row != nullptr) 
        {
            flag = 1;
        }

        mysql_free_result(result);

        if(flag == 0)
        {
            query = "INSERT INTO file_list (file_name, file_path, sender, resver) VALUES ('" + std::string(file_name) + "', '" + std::string(savePath) + "', '" + std::string(sender) + "', '" + std::string(resver) + "')";

            if (mysql_query(connect, query.c_str())) 
            {
                LogError( "Error inserting file: {}", mysql_error(connect));
                return 0;
            }
        }
        else if(flag == 1)
        {
            query = "UPDATE file_list SET file_path = '" + std::string(savePath) + "' WHERE file_name = '" + std::string(file_name) + "' AND sender = '" + std::string(sender) + "' AND resver = '" + std::string(resver) + "'";

            if (mysql_query(connect, query.c_str())) 
            {
                LogError("Error updating file: {}", mysql_error(connect));
                return 0;
            }
        }
        return 1;
    }

    return 0;
}

int sql_check_file(MYSQL* connect, const char* email, std::string& send)
{
    std::string query = "SELECT file_name, sender FROM file_list WHERE resver = '" + std::string(email) + "'";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error checking file: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) 
    {
        send += std::string(row[1]) + " send you a file: " + std::string(row[0]) + '\n';
    }

    mysql_free_result(result);
    return 1;
}

int sql_receive_file(MYSQL* connect, const char* email, const char* sender, const char* file_name, std::string& file_path)
{
    std::string query = "SELECT file_path FROM file_list WHERE resver = '" + std::string(email) + "' AND sender = '" + std::string(sender) + "' AND file_name = '" + std::string(file_name) + "'";

    std::cout << query << '\n';

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error receiving file: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);

    if (result == nullptr) 
    {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_ROW row = mysql_fetch_row(result);

    if (row == nullptr) 
    {
        mysql_free_result(result);
        return 0;
    }

    file_path = row[0];

    LogTrace("path: {}", file_path);

    mysql_free_result(result);

    /*query = "DELETE FROM file_list WHERE resver = '" + std::string(email) + "' AND sender = '" + std::string(sender) + "' AND file_name = '" + std::string(file_name) + "'";

    std::cout << query << '\n';

    if (mysql_query(connect, query.c_str())) 
    {
        std::cerr << "Error deleting file: " << mysql_error(connect) << '\n';
        return 0;
    }
    */

    return 1;
}

int sql_friend_history(MYSQL* connect, const char* email, const char* friend_email, std::string& send) {
    if (!is_my_friend(connect, email, friend_email)) {
        LogError("Error: {} is not your friend", friend_email);
        return 2;
    }

    std::string query = "SELECT sender, message, time FROM friend_message_list WHERE (sender = '" + std::string(email) + "' AND resver = '" + std::string(friend_email) + "') OR (sender = '" + std::string(friend_email) + "' AND resver = '" + std::string(email) + "') ORDER BY time";
        

    if (mysql_query(connect, query.c_str())) {
        LogError("Error checking friend history: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        send += std::string(row[0]) + " " + std::string(row[1]) + " " + std::string(row[2]) + '\n';
    }

    mysql_free_result(result);
    return 1;
}

int add_friend_message_list(MYSQL* connect, const char* email, const char* friend_email, const char* message)
{
    std::string query = "INSERT INTO friend_message_list(sender, resver, message, time) VALUES('" + std::string(email) + "', '" + std::string(friend_email) + "', '" + std::string(message) + "', NOW());";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error inserting friend history: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

int add_group_message_list(MYSQL* connect, const char* email, const char* group_name, const char* message)
{
    std::string query = "INSERT INTO group_message_list(group_name, sender, message, time) VALUES('" + std::string(group_name) + "', '" + std::string(email) + "', '" + std::string(message) + "', NOW());";

    if (mysql_query(connect, query.c_str())) 
    {
        LogError("Error inserting group history: {}", mysql_error(connect));
        return 0;
    }

    return 1;
}

int sql_group_history(MYSQL* connect, const char* email, const char* group_name, std::string& send)
{
    if(if_group_exist( connect, group_name) == 1)
    {
        send = "该群聊不存在"; 
        return 2;
    }

    if(if_member(connect, email, group_name) == 0)
    {
        send = "你不是该群组的成员";
        return 3;
    }

    std::string query = "SELECT sender, message, time FROM group_message_list WHERE group_name = '" + std::string(group_name) + "' ORDER BY time";

    std::cout << query << '\n';

    if (mysql_query(connect, query.c_str())) {
        LogError("Error checking group history: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) {
        LogError("Error storing result: {}", mysql_error(connect));
        return 0;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        send += std::string(row[0]) + " " + std::string(row[1]) + " " + std::string(row[2]) + '\n';
    }

    mysql_free_result(result);
    return 1;
}
