#include <mysql/mysql.h>
#include <iostream>


MYSQL* sql_init(MYSQL* connect);

int sql_insert(MYSQL* connect, char* email, char* password, char* name);

int sql_select(MYSQL* connect, const char* email, const char* password);

int sql_online(MYSQL* connect, const char* email);

int update_online_status(MYSQL* connect, const char* email, int status);
