#include <mysql/mysql.h>
#include <iostream>


MYSQL* sql_init(MYSQL* connect);

void sql_insert(MYSQL* connect, char* email, char* password, char* name);


