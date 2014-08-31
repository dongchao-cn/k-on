#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "SocketOp.h"

using namespace std;

int get_socket()
{
    int sock;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("get_socket error");
        exit(1);
    }
    return sock;
}

void set_reuse_addr(int listen_sock)
{
    int ret;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &ret, sizeof(ret)) == -1) 
    {
        perror("set_reuse_addr error");
        exit(1);
    }
}

void listen_socket_bind(int listen_sock, string ip, int port)
{
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    server_addr.sin_port = htons(port);

    if( bind(listen_sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("listen_socket_bind error");
        exit(1);
    }
}

void listen_socket_listen(int listen_sock)
{
    if (listen(listen_sock, 128) == -1)
    {
        perror("listen_socket_listen error");
        exit(1);
    }
}

int accept_socket(int listen_sock)
{
    sockaddr_in client;
    socklen_t addr_size = sizeof(sockaddr_in);
    return accept(listen_sock, (sockaddr *)&client, &addr_size);
}
