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
#include "Worker.h"
#include "SocketOp.h"

using namespace std;


// void Worker::print(int sock, string info)
// {
//     string buf = str(boost::format("[Worker %d] %s") % sock % info.c_str());
//     perror(buf.c_str());
// }


// Worker::Worker(WorkerThread worker_thread)
// {
//     epollfd = epoll_create(1); // Since Linux 2.6.8, the size argument is ignored, but must be greater than zero
//     if (epollfd == -1)
//     {
//         perror("epoll create error");
//         exit(1);
//     }

//     if (pthread_create(&thread, NULL, worker_thread, this) != 0)
//     {
//         perror("create thread failed");
//         exit(1);
//     }
//     printf("%s", "worker start success!\n");
// }

// Worker::~Worker()
// {
//     printf("%s", "worker finish!\n");
//     close(epollfd);
// }

// int Worker::get_epollfd()
// {
//     return epollfd;
// }

// void Worker::add_node(int sock, int events)
// {
//     epoll_event ev;
//     ev.events = events;
//     ev.data.fd = sock;
//     if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) 
//     {
//         print(sock, "epoll_ctl add");
//         throw EpollException;
//     }
// }

// void Worker::del_node(int sock)
// {
//     if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, NULL) == -1) 
//     {
//         print(sock, "epoll_ctl del");
//         throw EpollException;
//     }
//     close(sock);
// }

// void Worker::mod_node(int sock, int events)
// {
//     epoll_event ev;
//     ev.events = events;
//     ev.data.fd = sock;
//     if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sock, &ev) == -1) 
//     {
//         print(sock, "epoll_ctl mod");
//         throw EpollException;
//     }
// }

// WorkerManager::WorkerManager(int worker_num, WorkerThread worker_thread)
// {
//     for (int i = 0; i < worker_num; ++i)
//     {
//         workers.push_back(Worker(worker_thread));
//     }

//     printf("%s", "WorkerManager start success!\n");
// }

// void WorkerManager::add_node(int sock, int events)
// {
//     static int turn = 0;
//     workers[turn].add_node(sock, events);
//     sockMap[sock] = turn;
//     turn = (turn + 1) % workers.size();
// }

// void WorkerManager::del_node(int sock)
// {
//     map<int, int>::iterator it = sockMap.find(sock);
//     if (it == sockMap.end())
//     {
//         printf("can't find %d in sockMap", sock);
//     }
//     else
//     {
//         workers[it->second].del_node(sock);
//     }
// }

// void WorkerManager::mod_node(int sock, int events)
// {
//     map<int, int>::iterator it = sockMap.find(sock);
//     if (it == sockMap.end())
//     {
//         printf("can't find %d in sockMap", sock);
//     }
//     else
//     {
//         workers[it->second].mod_node(sock, events);
//     }
// }
