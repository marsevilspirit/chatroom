#include "mysql.h"

int sql_create_group_list(MYSQL* connect, const char* email)
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

    std::string query = "CREATE TABLE IF NOT EXISTS " + emailStr + "_group (group_name VARCHAR(255) NOT NULL, status VARCHAR(255) NOT NULL);";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }
    return 1;
}

int sql_delete_group_list(MYSQL* connect, const char* email)
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

    std::string query = "DROP TABLE " + emailStr + "_group;";
    
    std::cout << query << '\n';

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }
    return 1;
}

int if_group_exist(MYSQL* connect, const char* group_name)//0代表存在，1代表不存在
{
    std::string query = "SELECT 1 FROM group_list WHERE group_name = '" + std::string(group_name) + "' LIMIT 1;";
    
    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error1: " << mysql_error(connect) << std::endl;
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if(result && mysql_num_rows(result) > 0)
    {
        std::cerr << "Group name  exists." << std::endl;
        mysql_free_result(result);
        return 0;
    }
    mysql_free_result(result);

    return 1;
}

int sql_create_group(MYSQL* connect, const char* email, const char* group_name)
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

    if(if_group_exist(connect, group_name) == 0)
    {
        std::cout << "群组已存在\n";
        return 0;
    }

    std::string query = "INSERT INTO group_list (group_name) VALUES ('" + std::string(group_name) + "');";

    std::cout << query << '\n';

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }
    
    query = "INSERT INTO " + emailStr + "_group (group_name, status) VALUES ('" + std::string(group_name) + "', 'master');";

    std::cout << query << '\n';

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    query = "CREATE TABLE IF NOT EXISTS " + std::string(group_name) + "_group (email VARCHAR(255) NOT NULL, status VARCHAR(255) NOT NULL);";

    std::cout << query << '\n';

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    query = "INSERT INTO " + std::string(group_name) + "_group (email, status) VALUES ('" + std::string(email) + "', 'master');";

    std::cout << query << '\n';

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    return 1;
}

int if_master(MYSQL* connect, const char* email, const char* group_name)//0代表不是master，1代表是
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

    std::string query = "SELECT 1 FROM " + emailStr + "_group WHERE group_name = '" + std::string(group_name) + "' AND status = 'master' LIMIT 1;";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if(result && mysql_num_rows(result) > 0)
    {
        mysql_free_result(result);
        return 1;
    }
    mysql_free_result(result);

    return 0;
}

int sql_delete_group(MYSQL* connect, const char* email, const char* group_name)//1成功，2不存在，3不是群主
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

    if(if_group_exist(connect, group_name) == 1)
    {
        std::cout << "群组不存在(DELETE)\n";
        return 0;
    }

    if(if_master(connect, email, group_name) == 0)
    {
        std::cout << "您不是群主\n";
        return 0;
    }

    std::string query = "DELETE FROM " + emailStr + "_group WHERE group_name = '" + std::string(group_name) + "' AND status = 'master';";
    
    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    query = "DELETE FROM group_list WHERE group_name = '" + std::string(group_name) + "';";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    //从std::string(group_name) + "_group"中取出用户
    query = "SELECT email FROM " + std::string(group_name) + "_group;";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    MYSQL_ROW row;

    while((row = mysql_fetch_row(result)))
    {
        //替换@符号
        std::string emailStr = std::string(row[0]);
        std::replace(emailStr.begin(), emailStr.end(), '@', '0');

        //去除.之后的内容
        std::string::size_type pos = emailStr.find('.');
        if (pos != std::string::npos) 
        {
            emailStr = emailStr.substr(0, pos);
        } 

        query = "DELETE FROM " + std::string(emailStr) + "_group WHERE group_name = '" + std::string(group_name) + "';";
        if(mysql_query(connect, query.c_str()))
        {
            std::cerr << "Error: " << mysql_error(connect) << std::endl;
            return 0;
        }
    }

    mysql_free_result(result);

    query = "DROP TABLE " + std::string(group_name) + "_group;";
    
    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    return 1;
}

int sql_add_group(MYSQL* connect, const char* my_email, const char* group_name)//1成功，2不存在, 0数据库错误
{
    //替换@符号
    std::string my_emailStr = std::string(my_email);
    std::replace(my_emailStr.begin(), my_emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = my_emailStr.find('.');
    if (pos != std::string::npos) 
    {
        my_emailStr = my_emailStr.substr(0, pos);
    }

    if(if_group_exist(connect, group_name) == 1)
    {
        std::cout << "群组不存在\n";
        return 2;
    }

    std::string query = "INSERT INTO " + my_emailStr + "_group (group_name, status) VALUES ('" + std::string(group_name) + "', 'request');";
    
    std::cout << query << '\n';

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error1: " << mysql_error(connect) << std::endl;
        return 0;
    }

    query = "INSERT INTO " + std::string(group_name) + "_group (email, status) VALUES ('" + std::string(my_email) + "', 'request');";

    std::cout << query << '\n';

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error2: " << mysql_error(connect) << std::endl;
        return 0;
    }

    return 1;
}

int sql_exit_group(MYSQL* connect, const char* my_email, const char* group_name)// 1成功，2不存在, 0数据库错误, 3为群主
{
    //替换@符号
    std::string my_emailStr = std::string(my_email);
    std::replace(my_emailStr.begin(), my_emailStr.end(), '@', '0');

    //去除.之后的内容
    std::string::size_type pos = my_emailStr.find('.');
    if (pos != std::string::npos) 
    {
        my_emailStr = my_emailStr.substr(0, pos);
    }

    //检测是否为该群的成员
    std::string query = "SELECT 1 FROM " + my_emailStr + "_group WHERE group_name = '" + std::string(group_name) + "'LIMIT 1;";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error1: " << mysql_error(connect) << std::endl;
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);

    if(result && mysql_num_rows(result) > 0)
    {
        mysql_free_result(result);
    }
    else
    {
        std::cerr << "You are not a member of this group." << std::endl;
        mysql_free_result(result);
        return 2;
    }

    //检测是否为群主
    query = "SELECT 1 FROM " + my_emailStr + "_group WHERE group_name = '" + std::string(group_name) + "' AND status = 'master' LIMIT 1;";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error2: " << mysql_error(connect) << std::endl;
        return 0;
    }

    result = mysql_store_result(connect);

    if(result && mysql_num_rows(result) > 0)
    {
        std::cerr << "You are master of this group." << std::endl;
        mysql_free_result(result);
        return 3;
    }
    else
    {
        mysql_free_result(result);
    }

    query = "DELETE FROM " + my_emailStr + "_group WHERE group_name = '" + std::string(group_name) + "';";
    
    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error1: " << mysql_error(connect) << std::endl;
        return 0;
    }

    query = "DELETE FROM " + std::string(group_name) + "_group WHERE email = '" + std::string(my_email) + "';";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error2: " << mysql_error(connect) << std::endl;
        return 0;
    }

    return 1;
}

int sql_display_group(MYSQL* connect, std::string& send) //1成功，0数据库错误
{
    std::string query = "SELECT group_name FROM group_list;";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    MYSQL_ROW row;

    while((row = mysql_fetch_row(result)))
    {
        send += row[0];
        send += '\n';
    }

    mysql_free_result(result);

    return 1;
}

int if_master_or_manager(MYSQL* connect, const char* email, const char* group_name)//0代表不是master或manager，1代表是
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

    std::string query = "SELECT 1 FROM " + emailStr + "_group WHERE group_name = '" + std::string(group_name) + "' AND (status = 'master' OR status = 'manager') LIMIT 1;";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    if(result && mysql_num_rows(result) > 0)
    {
        mysql_free_result(result);
        return 1;
    }
    mysql_free_result(result);

    return 0;
}

int sql_display_request_list(MYSQL* connect, const char* my_email, const char* group_name, std::string& send)//1成功，0数据库错误，2不是群主或管理员
{
    if(if_master_or_manager(connect, my_email, group_name) == 0)
    {
        std::cout << "您不是群主或管理员\n";
        return 2;
    }

    std::string query = "SELECT email FROM " + std::string(group_name) + "_group WHERE status = 'request';";

    if(mysql_query(connect, query.c_str()))
    {
        std::cerr << "Error: " << mysql_error(connect) << std::endl;
        return 0;
    }

    MYSQL_RES* result = mysql_store_result(connect);
    MYSQL_ROW row;

    while((row = mysql_fetch_row(result)))
    {
        send += row[0];
        send += '\n';
    }

    mysql_free_result(result);

    return 1;
}
