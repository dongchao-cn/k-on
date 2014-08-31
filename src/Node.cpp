#include <string>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <boost/container/vector.hpp>
#include <boost/container/map.hpp>
#include <boost/format.hpp>
#include "exception.h"
#include "Protocol.h"
#include "Node.h"
using namespace std;


// void Client::Client(int proxy_id, int sock, int epollfd)
// {
// }

// void Client::print(int sock, string info)
// {
//     string buf = str(boost::format("[Client %d] %s") % sock % info.c_str());
//     perror(buf.c_str());
// }

// void Client::recv_buf2pkg()
// {
//     Package::str2pkg(recv_buf, v_recv_pkg);
// }

// void Server::print(int sock, string info)
// {
//     string buf = str(boost::format("[Server %d] %s") % sock % info.c_str());
//     perror(buf.c_str());
// }

// void Server::recv_buf2pkg()
// {
//     ProxyPackage::str2pkg(proxy_id, sock, recv_buf, v_recv_pkg);
// }