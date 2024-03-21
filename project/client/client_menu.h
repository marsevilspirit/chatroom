#include <sys/socket.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <thread> 
#include <mutex> 
#include <iostream>
#include <regex.h>
#include <auth-client.h>
#include <libesmtp.h>
#include <openssl/ssl.h>
#include <termios.h>
#include "../message/message.h"

void enterChatroom(int sfd);

int verify(const char* usermail, char* verify_number);

