#ifndef MYSQL_H
#define MYSQL_H

#include <mysql/mysql.h>
#include <iostream>
#include <algorithm>
#include <cstring>


MYSQL* sql_init(MYSQL* connect);

int sql_insert(MYSQL* connect, const char* email, const char* password, const char* name);

int sql_select(MYSQL* connect, const char* email, const char* password);

int sql_online(MYSQL* connect, const char* email);

int sql_if_online(MYSQL* connect, const char* email);

int update_online_status(MYSQL* connect, const char* email, int status);

int update_passwd(MYSQL* connect, const char* email, const char* password);

int delete_account(MYSQL* connect, const char* email, const char* password);

std::string sql_getname(MYSQL* connect, const char* email);

int sql_create_list(MYSQL* connect, const char* email);

int handle_delete_friend(MYSQL* connect, const char* email);

int sql_delete_list(MYSQL* connect, const char* email);

int sql_add_friend(MYSQL* connect, const char* email, const char* friend_email);

bool is_your_friend_or_request(MYSQL* connect, const char* email, const char* friend_email);

bool is_your_friend(MYSQL* connect, const char* email, const char* friend_email);

bool is_your_friend_or_block(MYSQL* connect, const char* email, const char* friend_email);

bool is_my_friend_or_request(MYSQL* connect, const char* email, const char* friend_email);

bool is_my_friend(MYSQL* connect, const char* email, const char* friend_email);

int sql_request(MYSQL* connect,const char* email,const char* friend_email);//好友申请

int sql_delete_friend(MYSQL* connect, const char* email, const char* friend_email);

int sql_block_friend(MYSQL* connect, const char* email, const char* friend_email);

int sql_if_block(MYSQL* connect, const char* email, const char* friend_email);

int sql_unblock_friend(MYSQL* connect, const char* email, const char* friend_email);

int sql_display_friend(MYSQL* connect, const char* email, std::string& send);

std::string getTypeByEmail(MYSQL* connect, const std::string& list, const std::string& email);

//--------------------------------------------------------------------------------
int if_group_exist(MYSQL* connect, const char* group_name);

int sql_create_group_list(MYSQL* connect, const char* email);

int sql_delete_group_list(MYSQL* connect, const char* email);

int sql_create_group(MYSQL* connect, const char* email, const char* group_name);

int sql_delete_group(MYSQL* connect, const char* email, const char* group_name);

int sql_add_group(MYSQL* connect, const char* my_email, const char* group_name);

int sql_exit_group(MYSQL* connect, const char* my_email, const char* group_name);

int sql_display_group(MYSQL* connect, std::string& send);

int sql_display_request_list(MYSQL* connect, const char* my_email, const char* group_name, std::string& send);

int sql_set_manager(MYSQL* connect, const char* my_email, const char* email, const char* group_name);

int sql_real_add_group(MYSQL* connect, const char* my_email, const char* email, const char* group_name);

int sql_cancel_manager(MYSQL* connect, const char* my_email, const char* email, const char* group_name);

int if_master(MYSQL* connect, const char* email, const char* group_name);

int if_member(MYSQL* connect, const char* email, const char* group_name);

int if_manager(MYSQL* connect, const char* email, const char* group_name);

int sql_kick_anybody(MYSQL* connect, const char* my_email, const char* email, const char* group_name);

int sql_kick_normal(MYSQL* connect, const char* my_email, const char* email, const char* group_name);

int sql_display_group_member(MYSQL* connect, const char* my_email, const char* group_name, std::string& send);

int sql_display_group_member_without_request(MYSQL* connect, const char* my_email, const char* group_name, std::string& send);//1成功，2不存在, 0数据库错误

//------------------------------------------------------------------------------file
int sql_file_list(MYSQL* connect, const char* file_name, const char* savePath, const char* sender, const char* resver);

int sql_check_file(MYSQL* connect, const char* email, std::string& send);

int sql_receive_file(MYSQL* connect, const char* email, const char* sender, const char* file_name, std::string& send);

//---------------------------------------------------------------------------------history
int sql_friend_history(MYSQL* connect, const char* email, const char* friend_email, std::string& send);

int add_friend_message_list(MYSQL* connect, const char* email, const char* friend_email, const char* message);

int add_group_message_list(MYSQL* connect, const char* email, const char* group_name, const char* message);

int sql_group_history(MYSQL* connect, const char* email, const char* group_name, std::string& send);

#endif // MYSQL_H
