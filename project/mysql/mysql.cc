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

    // 创建表格
    if (mysql_query(connect, "CREATE TABLE IF NOT EXISTS accounts (\
                    email VARCHAR(255) PRIMARY KEY,\
                    password VARCHAR(255) NOT NULL,\
                    name VARCHAR(255) NOT NULL,\
                    online_status BOOLEAN DEFAULT false)")) 
    {
        std::cerr << "Error creating table: " << mysql_error(connect) << '\n';
        mysql_close(connect);
    }
    
    std::cout << "database ready\n";

    return connect;
}

int sql_insert(MYSQL* connect, char* email, char* password, char* name)
{

    printf("6\n");
    // 构造插入 SQL 语句
    std::string sql_query = "INSERT INTO accounts (email, password, name) VALUES ('" +
                            std::string(email) + "', '" + std::string(password) + "', '" +
                            std::string(name) + "')";
    // 执行 SQL 语句

    printf("7\n");

    if (mysql_query(connect, sql_query.c_str())) 
    {
        std::cerr << "Error inserting data into table: " << mysql_error(connect) << '\n';

        return 0;
    }
    
    return 1;
}

