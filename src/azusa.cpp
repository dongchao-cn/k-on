#include <iostream>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <boost/lexical_cast.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/map.hpp>

#include "SocketOp.h"
#include "Protocol.h"
#include "Node.h"
#include "exception.h"

#define PROXY_ID 1

using namespace std;

const int MAX_EVENT_NUM = 1024;


class ServerManager
{
    vector<Server> servers;

    int connect_server(string ip, int port)
    {
        int sock;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
            perror("get_socket error");
            exit(1);
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
        server_addr.sin_port = htons(port);

        if( connect(sock, (sockaddr *)&server_addr, sizeof(server_addr)) )
        {
            perror("connect_server error");
            exit(1);
        }

        return sock;
    }

public:
    void add_server(string ip, int port, int epollfd)
    {
        int sock = connect_server(ip, port);
        servers.push_back(Server(PROXY_ID, sock, epollfd));
    }

    Server *get_server()
    {
        static int turn = 0;
        turn++;
        turn %= servers.size();
        return &servers[turn];
    }
};


ServerManager serverMgr;

void* worker_thread(void *ptr);

class Worker
{
    pthread_t thread;
    int* epollfd;
public:
    Worker()
    {
        epollfd = new int;
        *epollfd = epoll_create(1); // Since Linux 2.6.8, the size argument is ignored, but must be greater than zero
        if (*epollfd == -1) 
        {
            perror("epoll create error");
            exit(1);
        }

        if (pthread_create(&thread, NULL, worker_thread, epollfd) != 0)
        {
            perror("create thread failed");
            exit(1);
        }
    }

    ~Worker()
    {
        delete epollfd;
    }

    int get_epollfd()
    {
        return *epollfd;
    }
};

void* worker_thread(void *ptr)
{
    int epollfd = *(int*)ptr;
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
            int sock = events[i].data.fd;
            if (events[i].events & EPOLLIN)
            {
                // ready to recv
                Client* client = (Client*)Client::get_node(sock);
                if (client)
                {
                    // recv from client, post it to server
                    client->on_recv();
                    vector<Package> pkgs = client->recv_pkg();
                    for (int i = 0; i < pkgs.size(); ++i)
                    {
                        serverMgr.get_server()->send_pkg(ProxyPackage(PROXY_ID, sock, pkgs[i]));
                    }
                }
                else
                {
                    // recv from server, post to client
                    Server* server = (Server*)Server::get_node(sock);
                    server->on_recv();
                    vector<ProxyPackage> pkgs = server->recv_pkg();
                    for (int i = 0; i < pkgs.size(); ++i)
                    {
                        // TODO
                        // check proxy_id
                        Client* client = (Client*)Client::get_node(pkgs[i].get_client_sock());
                        client->send_pkg(pkgs[i].get_pkg());
                    }
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                // ready to send
                Client* client = (Client*)Client::get_node(sock);
                if (client)
                {
                    // client ready to send
                    client->on_send();
                }
                else
                {
                    // server ready to send
                    Server* server = (Server*)Server::get_node(sock);
                    server->on_send();
                }
            }
            else
            {
                throw;
            }
        }
    }
}

class WorkerManager
{
    vector<Worker> workers;
public:
    WorkerManager(int worker_num)
    {
        for (int i = 0; i < worker_num; ++i)
        {
            workers.push_back(Worker());
        }
    }

    int get_epollfd()
    {
        static int turn = 0;
        int epollfd = workers[turn++].get_epollfd();
        turn %= workers.size();
        return epollfd;
    }
};


int main(int argc, char *argv[])
{
    // TODO
    // need argv check
    string ip = boost::lexical_cast<string>(argv[1]);
    int port = boost::lexical_cast<int>(argv[2]);
    int worker_num = boost::lexical_cast<int>(argv[3]);


    int listen_sock = get_socket();

    set_reuse_addr(listen_sock);

    listen_socket_bind(listen_sock, ip, port);

    listen_socket_listen(listen_sock);

    WorkerManager workerMgr(worker_num);

    for (int i = 4; i + 1 < argc; i += 2)
    {
        string upip = boost::lexical_cast<string>(argv[i]);
        int upport = boost::lexical_cast<int>(argv[i+1]);
        serverMgr.add_server(upip, upport, workerMgr.get_epollfd());
    }

    while (1)
    {
        int client_sock = accept_socket(listen_sock);
        if (client_sock == -1)
        {
            perror("accept failed");
            continue;
        }

        try
        {
            Client(PROXY_ID, client_sock, workerMgr.get_epollfd());
        }
        catch (int e)
        {
            if (e == EpollAddException)
                continue;
        }
    }
}

