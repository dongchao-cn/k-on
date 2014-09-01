/*
    C++ ECHO socket client
*/
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <boost/container/vector.hpp>
#include <boost/lexical_cast.hpp>
#include "SocketOp.h"
#include "Node.h"

#define PROXY_ID 1

using namespace std;

const int BUF_SIZE = 1024;
const int BACKLOG = 3;
const int MAX_EVENT_NUM = 10;

int main(int argc, char *argv[])
{
    string server_ip = boost::lexical_cast<string>(argv[1]);
    int server_port = boost::lexical_cast<int>(argv[2]);
    int interval = boost::lexical_cast<int>(argv[3]);

    //Create socket
    int sock = get_socket();

    connect_socket(sock, server_ip, server_port);
    
    int epollfd = epoll_create(1); // Since Linux 2.6.8, the size argument is ignored, but must be greater than zero
    if (epollfd == -1)
    {
        perror("epoll create error");
        exit(1);
    }

    Client client(PROXY_ID, sock, epollfd);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) 
    {
        perror("epoll_ctl add STDIN_FILENO");
        exit(1);
    }

    ev.events = EPOLLOUT;
    ev.data.fd = STDOUT_FILENO;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, STDOUT_FILENO, &ev) == -1) 
    {
        perror("epoll_ctl add STDOUT_FILENO");
        exit(1);
    }

    //keep communicating with server
    epoll_event *events = new epoll_event[MAX_EVENT_NUM]; // alloc in heap
    while(1)
    {
        int nfds = epoll_wait(epollfd, events, MAX_EVENT_NUM, -1);
        if (nfds == -1) 
        {
            perror("epoll_pwait");
            exit(1);
        }

        for (int i = 0; i < nfds; ++i) 
        {
            if (sock == events[i].data.fd)
            {
                if (events[i].events & EPOLLIN)
                {
                    // ready to recv
                    client.on_recv();
                }
                else if (events[i].events & EPOLLOUT)
                {
                    // ready to send
                    client.on_send();
                }
                else
                {
                    throw;
                }
            }
            else if (STDIN_FILENO == events[i].data.fd)
            {
                char buf[BUF_SIZE];
                read(STDIN_FILENO, buf, BUF_SIZE);
                unsigned int buf_len = strlen(buf);
                Package pkg = Package(buf_len, string(buf));
                client.send_pkg(pkg);
            }
            else if (STDOUT_FILENO == events[i].data.fd)
            {
                vector<Package> pkgs = client.recv_pkg();
                for (int i = 0; i < pkgs.size(); ++i)
                {
                    write(STDOUT_FILENO, pkgs[i].serialize().c_str(), pkgs[i].serialize().size());
                }
            }
        }
    }
    exit(0);
}
