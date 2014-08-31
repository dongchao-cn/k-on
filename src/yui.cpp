/*
    C++ ECHO socket server
*/
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "base.h"
using namespace std;

const int BUF_SIZE = 1024;
const int BACKLOG = 3;

int get_listen_socket()
{
    int listen_sock;
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1)
    {
        perror("get_listen_socket error");
        exit(1);
    }
    return listen_sock;
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
    if (listen(listen_sock, BACKLOG) == -1)
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

sockaddr_in get_client_addr(int sock)
{
    sockaddr_in client;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    if (getpeername(sock, (struct sockaddr*)&client, &addr_size) != 0)
    {
        perror("get_client_addr error");
        exit(1);
    }
    return client;
}

void print_start()
{
    print("Server started success, listen at %s:%d\n", SERVER_IP, SERVER_PORT);
}

void print_conn(int client_sock)
{
    #ifdef DEBUG
    sockaddr_in client = get_client_addr(client_sock);
    #endif

    print("[Conn %d] %s:%d\n", client_sock, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
}

void print_recv(int client_sock, char* message)
{
    #ifdef DEBUG
    sockaddr_in client = get_client_addr(client_sock);
    #endif

    print("[Recv %d] %s:%d %s\n", client_sock, inet_ntoa(client.sin_addr), ntohs(client.sin_port), message);
}

void print_send(int client_sock, char* message)
{
    #ifdef DEBUG
    sockaddr_in client = get_client_addr(client_sock);
    #endif

    print("[Send %d] %s:%d %s\n", client_sock, inet_ntoa(client.sin_addr), ntohs(client.sin_port), message);
}

void print_dconn(int client_sock)
{
    #ifdef DEBUG
    sockaddr_in client = get_client_addr(client_sock);
    #endif

    print("[DConn %d] %s:%d\n", client_sock, inet_ntoa(client.sin_addr), ntohs(client.sin_port));
}

class ProxyPackage
{
    int proxy_id;
    int client_sock;
    Package pkg;
public:
    ProxyPackage(int proxy_id, int client_sock, Package pkg): proxy_id(proxy_id), client_sock(client_sock), pkg(pkg) { };
    string to_buf()
    {
        // return the package to buffer
        string ret;
        ret.append((char*)&proxy_id, sizeof(int));
        ret.append((char*)&client_sock, sizeof(int));
        ret += pkg.to_buf();
        return ret;
    }

    static void str2pkg(string &str, vector<ProxyPackage> &pkgs)
    {
        int proxy_id = *(int *)str.c_str();
        int client_sock = *((int *)str.c_str() + 1);
        int size = *((int *)str.c_str() + 2);
        while (str.size() >= 3*sizeof(int) + size)
        {
            pkgs.push_back(ProxyPackage(proxy_id, client_sock, 
                Package(size, str.substr(3*sizeof(int), size))));
            str = str.substr(3*sizeof(int)+size);
            proxy_id = *(int *)str.c_str();
            client_sock = *((int *)str.c_str() + 1);
            size = *((int *)str.c_str() + 2);
        }
    }

    int get_proxy_id()
    {
        return proxy_id;
    }

    int get_client_sock()
    {
        return client_sock;
    }

    Package &get_pkg()
    {
        return pkg;
    }
};


template<typename P>
class Node
{
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

    void recv_buf2pkg()
    {
        P::str2pkg(recv_buf, v_recv_pkg);
    }

    void send_pkg2buf()
    {
        for (int i = 0; i < v_send_pkg.size(); ++i)
        {
            add_send_buf(v_send_pkg[i].to_buf());
        }
        v_send_pkg.clear();
    }

    void disconnect()
    {
        if (epoll_ctl(epollfd, EPOLL_CTL_DEL, sock, NULL) == -1) 
        {
            printf("[Node %d] epoll_ctl del error", sock);
        }
        close(sock);
        sockMap.erase(sock);
        printf("[Node %d] disconnect!\n", sock);
    }

public:

    Node(int sock, int epollfd) : sock(sock), epollfd(epollfd)
    {
        epollsend = false;
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sock;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sock, &ev) == -1) 
        {
            printf("[Node %d] epoll_ctl add error", sock);
            close(sock);
            throw 1;
        }
        sockMap[sock] = this;
    }

    bool on_recv()
    {
        // call when sock is ready to read
        // get package [recv_buf -> recv_pkg]
        char message[RECV_BUF_SIZE];
        int recv_size = recv(sock, message, RECV_BUF_SIZE, 0);

        if (recv_size <= 0) 
        {
            // node close
            disconnect();
            return false;
        }
        else
        {
            // receive data
            string buf(message, recv_size);
            add_recv_buf(buf);
            recv_buf2pkg();
            return true;
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
                printf("[Client %d] epoll_ctl remove EPOLLOUT error\n", sock);
                disconnect();
            }
            epollsend = false;
        }
    }

    void send_pkg(P pkg)
    {
        // non-blocking send package to Node 
        add_send_pkg(pkg);

        if (!epollsend && !v_send_pkg.empty())
        {
            // add EPOLLOUT
            epoll_event ev;
            ev.events = EPOLLIN | EPOLLOUT;
            ev.data.fd = sock;
            if (epoll_ctl(epollfd, EPOLL_CTL_MOD, sock, &ev) == -1) {
                printf("[Client %d] epoll_ctl add EPOLLOUT error\n", sock);
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

    static Node<P>* get_node(int sock)
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


typedef Node<ProxyPackage> Server;

Server server;

int main(int argc, char *argv[])
{
    // create socket
    int listen_sock = get_listen_socket();
    
    // set reuse
    set_reuse_addr(listen_sock);

    // bind
    listen_socket_bind(listen_sock);
    
    // listen
    listen_socket_listen(listen_sock);

    // start success
    print_start();

    while (1)
    {
        // accept connection from an incoming client
        int client_sock = accept_socket(listen_sock);
        if (client_sock == -1)
        {
            perror("accept failed");
            continue;
        }
        print_conn(client_sock);

        epollfd = epoll_create(1);
        if (epollfd == -1) 
        {
            perror("epoll_create");
            exit(1);
        }

        epoll_event ev;
        ev.events = EPOLLIN | EPOLLOUT;
        ev.data.fd = listen_sock;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, client_sock, &ev) == -1) 
        {
            perror("epoll_ctl: client_sock");
            exit(1);
        }

        bool discon = false;
        while(discon)
        {
            epoll_event events[1];
            int nfds = epoll_wait(epollfd, events, 1, -1);
            if (nfds == -1) {
                perror("epoll_pwait");
                exit(1);
            }

            for (n = 0; n < nfds; ++n) {
                if (events[i].events & EPOLLIN)
                {
                    // server recv request
                    if (!server.on_recv())
                    {
                        discon = true; 
                        break;
                    }
                    vector<ProxyPackage> pkgs server.recv_pkg();
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














