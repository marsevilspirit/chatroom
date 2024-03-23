#include "mysql.h"
#include <mysql/mysql.h>

MYSQL* sql_init(MYSQL* connect)
{
    connect = mysql_init(nullptr);  // 初始化 MySQL 连接
    if (!connect) {
        std::cerr << "Error initializing MySQL connection\n";
    }

    // 连接 MySQL 数据库
    if (!mysql_real_connect(connect, "localhost", "mars", "661188", "chatroom", 0, nullptr, 0)) 
    {
        std::cerr << "Error connecting to MySQL database: " << mysql_error(connect) << '\n';
        mysql_close(connect);
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

    std::cout << "database ready\n";

    return connect;
}

int sql_insert(MYSQL* connect, char* email, char* password, char* name)
{
    // 构造插入 SQL 语句
    std::string sql_query = "INSERT INTO accounts (email, password, name) VALUES ('" +
        std::string(email) + "', '" + std::string(password) + "', '" +
        std::string(name) + "')";
    // 执行 SQL 语句
    if (mysql_query(connect, sql_query.c_str())) 
    {
        std::cerr << "Error inserting data into table: " << mysql_error(connect) << '\n';

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
        std::cerr << "Error querying database: " << mysql_error(connect) << '\n';
        return 0; // 查询失败
    }

    // 获取查询结果
    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr) 
    {
        std::cerr << "Error storing result: " << mysql_error(connect) << '\n';
        return 0; // 获取结果失败
    }

    // 判断是否有匹配的记录
    int count = mysql_num_rows(result);

    // 释放查询结果
    mysql_free_result(result);

    // 返回查询结果，1 表示匹配，0 表示不匹配
    return (count > 0) ? 1 : 0;
}

int sql_online(MYSQL* connect, const char* email)
{
    std::string query = "SELECT 1 FROM accounts WHERE email ='" + std::string(email) + "' AND online_status='0' LIMIT 1";

    if (mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error querying database: " << mysql_error(connect) << '\n';
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr)
    {
        std::cerr << "Error storing result: " << mysql_error(connect) << '\n';
        return 0;
    }

    int count = mysql_num_rows(result);

    return (count > 0) ? 1 : 0;
}

int sql_if_online(MYSQL* connect, const char* email)
{
    std::string query = "SELECT 1 FROM accounts WHERE email ='" + std::string(email) + "' AND online_status='1' LIMIT 1";

    if (mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error querying database: " << mysql_error(connect) << '\n';
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if (result == nullptr)
    {
        std::cerr << "Error storing result: " << mysql_error(connect) << '\n';
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
        std::cerr << "Error updating online status: " << mysql_error(connect) << '\n';
        return 0;
    }

    return 1;
}

int update_passwd(MYSQL* connect, const char* email, const char* password)
{
    std::string query = "UPDATE accounts SET password = " + std::string(password) + " WHERE email = '" + std::string(email) + "'";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error updating password: " << mysql_error(connect) << '\n';
        return 0;
    }

    return 1;
}

int delete_account(MYSQL* connect, const char* email)
{
    std::string query = "DELETE FROM accounts WHERE email = '" + std::string(email) + "'";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error deleting account: " << mysql_error(connect) << '\n';
        return 0;
    }

    return 1;
}

std::string sql_getname(MYSQL* connect, const char* email)
{
    std::string query = "SELECT name FROM accounts WHERE email = '" + std::string(email) + "'";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error querying database: " << mysql_error(connect) << '\n';
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if(result == nullptr)
    {
        std::cerr << "Error storing result: " << mysql_error(connect) << '\n';
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

    std::cout << "create list " << email << '\n';

    if (mysql_query(connect, query.c_str())) 
    {
        std::cerr << "Error creating list: " << mysql_error(connect) << '\n';
        return 0;
    }

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
        std::cerr << "Error deleting list: " << mysql_error(connect) << '\n';
        return 0;
    }

    return 1;
}
