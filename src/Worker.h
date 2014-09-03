#ifndef _Worker
#define _Worker

#include <string>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <boost/container/map.hpp>
#include <boost/format.hpp>
#include <boost/container/vector.hpp>
#include "exception.h"
#include "SocketOp.h"
using namespace std;

typedef void*(*WorkerThread)(void*);

// init the sockMap for NodeManager
// template<typename P> map<int, Node<P>*> NodeManager<P>::sockMap;

template<class N>
class Worker
{
private:
    pthread_t thread;
    int epollfd;

    void print(int sock, string info)
    {
        string buf = str(boost::format("[Worker] %d %s") % sock % info.c_str());
        perror(buf.c_str());
    }

public:
    Worker(WorkerThread worker_thread)
    {
        epollfd = epoll_create(1); // Since Linux 2.6.8, the size argument is ignored, but must be greater than zero
        if (epollfd == -1)
        {
            perror("epoll create error");
            exit(1);
        }

        printf("epoll_create epollfd = %d\n", epollfd);

        if (pthread_create(&thread, NULL, worker_thread, this) != 0)
        {
            perror("create thread failed");
            exit(1);
        }
        printf("%s", "worker start success!\n");
    }

    ~Worker()
    {
        printf("%s", "worker finish!\n");
        close(epollfd);
    }

    int get_epollfd()
    {
        return epollfd;
    }

    void add_node(N* pnode, int events)
    {
        int sock = pnode->get_sock();
        epoll_event ev;
        ev.events = events;
        ev.data.ptr = pnode;
        printf("epoll_ctl add epollfd = %d, sock = %d\n", epollfd, sock);
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) 
        {
            print(sock, "epoll_ctl add");
            throw EpollException;
        }
    }

    void del_node(N* pnode)
    {
        int sock = pnode->get_sock();
        if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, NULL) == -1) 
        {
            print(sock, "epoll_ctl del");
            throw EpollException;
        }
        close(sock);
    }

    void mod_node(N* pnode, int events)
    {
        int sock = pnode->get_sock();
        epoll_event ev;
        ev.events = events;
        ev.data.ptr = pnode;
        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sock, &ev) == -1) 
        {
            print(sock, "epoll_ctl mod");
            throw EpollException;
        }
    }
};

template<class N>
class WorkerManager
{

    // sock, worker pos
    map<N*, int> sockMap;
    vector<Worker<N> > workers;
public:
    WorkerManager(int worker_num, WorkerThread worker_thread)
    {
        for (int i = 0; i < worker_num; ++i)
        {
            workers.push_back(Worker<N>(worker_thread));
        }
        printf("%s", "WorkerManager start success!\n");
    }

    // void add_node(N *node, int events);
    void add_node(N *pnode, int events)
    {
        static int turn = 0;
        workers[turn].add_node(pnode, events);
        sockMap[pnode] = turn;
        turn = (turn + 1) % workers.size();
    }

    void del_node(N *pnode)
    {
        typename map<N*, int>::iterator it = sockMap.find(pnode);
        if (it == sockMap.end())
        {
            printf("can't find %d in sockMap", pnode->get_sock());
        }
        else
        {
            workers[it->second].del_node(pnode);
        }
    }

    void mod_node(N *pnode, int events)
    {
        typename map<N*, int>::iterator it = sockMap.find(pnode);
        if (it == sockMap.end())
        {
            printf("can't find %d in sockMap", pnode->get_sock());
        }
        else
        {
            workers[it->second].mod_node(pnode, events);
        }
    }
};

#endif
