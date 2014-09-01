#ifndef _SocketOp
#define _SocketOp

#include <string>
using namespace std;

int get_socket();
void set_reuse_addr(int listen_sock);
void listen_socket_bind(int listen_sock, string ip, int port);
void listen_socket_listen(int listen_sock);
int accept_socket(int listen_sock);
void connect_socket(int sock, string ip, int port);

#endif
