#ifndef _Node
#define _Node

#include <string>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <boost/container/vector.hpp>
#include <boost/format.hpp>
#include <errno.h>
#include "Protocol.h"
#define RECV_BUF_SIZE 5*1024 // 5k


template<typename P>
class Node
{
protected:
    int sock;
    bool valid;
    string recv_buf;
    string send_buf;
    vector<P> v_recv_pkg;
    vector<P> v_send_pkg;

    void setnonblocking()
    {
        int opts;
        opts=fcntl(sock,F_GETFL);
        if(opts<0)
        {
            print(sock, "fcntl(sock,GETFL)");
            valid = false;
            return;
        }
        opts = opts|O_NONBLOCK;
        if(fcntl(sock,F_SETFL,opts)<0)
        {
            print(sock, "fcntl(sock,SETFL,opts)");
            valid = false;
            return;
        }
    }

    virtual void recv_buf2pkg() = 0;

    void send_pkg2buf()
    {
        P::pkg2str(v_send_pkg, send_buf);
    }

    virtual void print(int sock, string info)
    {
        string buf = str(boost::format("[Node %d] %s") % sock % info.c_str());
        perror(buf.c_str());
    }

public:

    Node(int sock) : sock(sock)
    {
        print(sock, string("connect!"));
        valid = true;
        setnonblocking();
    }

    ~Node()
    {
        print(sock, string("disconnect!"));
        close(sock);
    }

    int get_sock()
    {
         return sock;
    }

    void on_recv()
    {
        if (!valid) throw NodeVaildException;

        // call when sock is ready to read
        // get package [recv_buf -> recv_pkg]
        char message[RECV_BUF_SIZE];
        int recv_size = recv(sock, message, RECV_BUF_SIZE, 0);

        while (1)
        {
            if (recv_size > 0)
            {
                // receive data
                recv_buf.append(message, recv_size);
                recv_size = recv(sock, message, RECV_BUF_SIZE, 0);
            }
            else if (recv_size == 0)
            {
                // node close
                valid = false;
                break;
            }
            else if (recv_size == -1)
            {
                if ((errno & EAGAIN) || (errno & EWOULDBLOCK))
                {
                    // would block, recv finish
                    break;
                }
                else
                {
                    // other error
                    valid = false;
                    break;
                }
            }
        }
        recv_buf2pkg();
    }

    void on_send()
    {
        if (!valid) throw NodeVaildException;

        // call when sock is ready to write
        // send package [send_pkg -> send_buf]
        send_pkg2buf();

        int send_size = send(sock, send_buf.c_str(), send_buf.size(), 0);
        while (1)
        {
            if (send_size > 0)
            {
                // send success
                send_buf = send_buf.substr(send_size);
                if (send_buf.empty())
                {
                    // send finish
                    break;
                }
                else
                {
                    send_size = send(sock, send_buf.c_str(), send_buf.size(), 0);
                }
            }
            else if (send_size == -1)
            {
                if ((errno & EAGAIN) || (errno & EWOULDBLOCK))
                {
                    // would block, send finish
                    break;
                }
                else
                {
                    // other error
                    valid = false;
                    break;
                }
            }
        }
    }

    void send_pkg(P pkg)
    {
        if (!valid) throw NodeVaildException;

        // add pkg to v_send_pkg
        v_send_pkg.push_back(pkg);
    }

    vector<P> recv_pkg()
    {
        if (!valid) throw NodeVaildException;

        // return copy of v_recv_pkg
        vector<P> ret(v_recv_pkg);
        v_recv_pkg.clear();
        return ret;
    }

    bool is_valid()
    {
        return valid;
    }
};

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
    ClientTemplate(int sock) : Node<P>(sock) { }
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
        P::str2pkg(Node<P>::sock, Node<P>::recv_buf, Node<P>::v_recv_pkg);
    }

public:
    ServerTemplate(int sock) : Node<P>(sock) { }
};

typedef ClientTemplate<Package> Client;
typedef ServerTemplate<ProxyPackage> Server;

#endif
