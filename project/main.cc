#include "server/server.h"

#define PORT 8000

int main(void)
{
    server tcp_server(PORT);

    tcp_server.run();

    return 0;
}
