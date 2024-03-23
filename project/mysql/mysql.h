#include <mysql/mysql.h>
#include <iostream>
#include <algorithm>


MYSQL* sql_init(MYSQL* connect);

int sql_insert(MYSQL* connect, char* email, char* password, char* name);

int sql_select(MYSQL* connect, const char* email, const char* password);

int sql_online(MYSQL* connect, const char* email);

int sql_if_online(MYSQL* connect, const char* email);

int update_online_status(MYSQL* connect, const char* email, int status);

int update_passwd(MYSQL* connect, const char* email, const char* password);

int delete_account(MYSQL* connect, const char* email);

std::string sql_getname(MYSQL* connect, const char* email);

int sql_create_list(MYSQL* connect, const char* email);

int sql_delete_list(MYSQL* connect, const char* email);
