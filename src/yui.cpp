/*
    C++ ECHO socket server
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <boost/container/vector.hpp>
#include <boost/lexical_cast.hpp>
#include "SocketOp.h"
#include "Node.h"
using namespace std;

const int BUF_SIZE = 1024;

#define PROXY_ID 1

int main(int argc, char *argv[])
{
    string server_ip = boost::lexical_cast<string>(argv[1]);
    int server_port = boost::lexical_cast<int>(argv[2]);

    // create socket
    int listen_sock = get_socket();
    
    // set reuse
    set_reuse_addr(listen_sock);

    // bind
    listen_socket_bind(listen_sock, server_ip, server_port);
    
    // listen
    listen_socket_listen(listen_sock);

    int epollfd = epoll_create(1); // Since Linux 2.6.8, the size argument is ignored, but must be greater than zero
    if (epollfd == -1)
    {
        perror("epoll create error");
        exit(1);
    }

    while (1)
    {
        // accept
        int sock = accept_socket(listen_sock);
        Server server(PROXY_ID, sock, epollfd);
        while(1)
        {
            epoll_event events[1];
            int nfds = epoll_wait(epollfd, events, 1, -1);
            if (nfds == -1)
            {
                perror("epoll_pwait");
                exit(1);
            }

            for (int i = 0; i < nfds; ++i) 
            {
                if (events[i].events & EPOLLIN)
                {
                    // server recv request
                    server.on_recv();
                    vector<ProxyPackage> pkgs = server.recv_pkg();
                    for (int i = 0; i < pkgs.size(); ++i)
                    {
                        // echo back
                        server.send_pkg(pkgs[i]);
                    }
                }
                else if (events[i].events & EPOLLOUT)
                {
                    server.on_send();
                }
            }
        }
    }

}














