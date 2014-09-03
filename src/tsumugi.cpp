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
#include "Worker.h"

#define MAX_EVENT_NUM 10
#define BUF_SIZE 1024
using namespace std;


void* client_thread(void* ptr)
{
    Worker<Client> *worker = (Worker<Client> *)ptr;
    int epollfd = worker->get_epollfd();

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
            Client *pclient = (Client *)events[i].data.ptr;
            if (events[i].events & EPOLLIN)
            {
                // ready to recv
                pclient->on_recv();
                vector<Package> pkgs = pclient->recv_pkg();
                for (int j = 0; j < pkgs.size(); ++j)
                {
                    pkgs[i].print();
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                // ready to send
                char buf[BUF_SIZE];
                scanf("%s", buf);
                pclient->send_pkg(Package(sizeof(buf), buf));
                pclient->on_send();
            }
            else
            {
                throw;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    string server_ip = boost::lexical_cast<string>(argv[1]);
    int server_port = boost::lexical_cast<int>(argv[2]);

    //Create socket
    int sock = get_socket();

    connect_socket(sock, server_ip, server_port);
    
    Client client = Client(sock);

    printf("client success, sock = %d\n", sock);

    WorkerManager<Client> workerMgr = WorkerManager<Client>(1, client_thread);

    printf("workerMgr success\n");

    workerMgr.add_node(&client, EPOLLOUT);

    while (1)
    {
        sleep(1);
    }

    exit(0);
}
