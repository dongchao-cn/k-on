#ifndef _Node
#define _Node

#include <string>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <boost/container/vector.hpp>
#include <boost/container/map.hpp>
#include <boost/format.hpp>
#include "Protocol.h"
#define RECV_BUF_SIZE 5*1024 // 5k

template<typename P>
class Node
{
protected:
    int proxy_id;
    int sock;
    int epollfd;
    bool epollsend;
    string recv_buf;
    string send_buf;
    vector<P> v_recv_pkg;
    vector<P> v_send_pkg;

    static map<int, Node<P>*> sockMap;

    void add_recv_buf(string buf)
    {
        recv_buf += buf;
    }
    void add_recv_pkg(P pkg)
    {
        v_recv_pkg.push_back(pkg);
    }
    void add_send_buf(string buf)
    {
        send_buf += buf;
    }
    void add_send_pkg(P pkg)
    {
        v_send_pkg.push_back(pkg);
    }

    virtual void recv_buf2pkg() = 0;

    void send_pkg2buf()
    {
        P::pkg2str(v_send_pkg, send_buf);
    }

    void disconnect()
    {
        if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, NULL) == -1) 
        {
            print(sock, "epoll_ctl del");
        }
        close(sock);
        sockMap.erase(sock);
        print(sock, "disconnect!");
    }

    virtual void print(int sock, string info)
    {
        string buf = str(boost::format("[Node %d] %s") % sock % info.c_str());
        perror(buf.c_str());
    }

public:

    Node(int proxy_id, int sock, int epollfd) : proxy_id(proxy_id), sock(sock), epollfd(epollfd)
    {
        epollsend = false;
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sock;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) 
        {
            print(sock, "epoll_ctl add");
            close(sock);
            throw EpollAddException;
        }
        sockMap[sock] = this;
    }

    void on_recv()
    {
        // call when sock is ready to read
        // get package [recv_buf -> recv_pkg]
        char message[RECV_BUF_SIZE];
        int recv_size = recv(sock, message, RECV_BUF_SIZE, 0);

        if (recv_size <= 0) 
        {
            // node close
            disconnect();
        }
        else
        {
            // receive data
            string buf(message, recv_size);
            add_recv_buf(buf);
            recv_buf2pkg();
        }
    }

    bool on_send()
    {
        // call when sock is ready to write
        // send package [send_pkg -> send_buf]
        send_pkg2buf();
        int send_size = send(sock, send_buf.c_str(), send_buf.size(), 0);
        send_buf = send_buf.substr(send_size);
        if (send_buf.empty())
        {
            // remove EPOLLOUT
            epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = sock;
            if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sock, &ev) == -1) {
                print(sock, "epoll_ctl remove EPOLLOUT error");
                disconnect();
            }
            epollsend = false;
        }
    }

    void send_pkg(P pkg)
    {
        // non-blocking send package to Node 
        add_send_pkg(pkg);

        if (!epollsend)
        {
            // add EPOLLOUT
            epoll_event ev;
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = sock;
            if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sock, &ev) == -1) {
                print(sock, "epoll_ctl add EPOLLOUT error");
                disconnect();
            }
        }
    }

    vector<P> recv_pkg()
    {
        vector<P> ret(v_recv_pkg);
        v_recv_pkg.clear();
        return ret;
    }

    static Node<P> *get_node(int sock)
    {
        typename map<int, Node<P>*>::iterator it = sockMap.find(sock);
        typename map<int, Node<P>*>::iterator end = sockMap.end();
        if (it == end)
            return NULL;
        else
            return it->second;
    }
};

// init the sockMap for Node
template<typename P> map<int, Node<P>*> Node<P>::sockMap;

template<class P>
class ClientTemplate : public Node<P>
{
    void print(int sock, string info)
    {
        string buf = str(boost::format("[Client %d] %s") % sock % info.c_str());
        perror(buf.c_str());
    }

    void recv_buf2pkg()
    {
        P::str2pkg(Node<P>::recv_buf, Node<P>::v_recv_pkg);
    }

public:
    ClientTemplate(int proxy_id, int sock, int epollfd) : Node<P>(proxy_id, sock, epollfd) { };
};

template<class P>
class ServerTemplate : public Node<P>
{
    void print(int sock, string info)
    {
        string buf = str(boost::format("[Server %d] %s") % sock % info.c_str());
        perror(buf.c_str());
    }

    void recv_buf2pkg()
    {
        P::str2pkg(Node<P>::proxy_id, Node<P>::sock, Node<P>::recv_buf, Node<P>::v_recv_pkg);
    }

public:
    ServerTemplate(int proxy_id, int sock, int epollfd) : Node<P>(proxy_id, sock, epollfd) { };
};

typedef ClientTemplate<Package> Client;
typedef ServerTemplate<ProxyPackage> Server;

#endif
