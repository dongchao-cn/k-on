/*
    C++ ECHO socket client
*/
#include <cstdio>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <boost/lexical_cast.hpp>
using namespace std;

#ifdef DEBUG
#define print(fmt, arg...) printf(fmt, ##arg)
#else
#define print(fmt, arg...)
#endif

const int BUF_SIZE = 1024;
const int BACKLOG = 3;

int main(int argc, char *argv[])
{
    string server_ip = boost::lexical_cast<string>(argv[1]);
    int server_port = boost::lexical_cast<int>(argv[2]);
    int interval = boost::lexical_cast<int>(argv[3]);

    //Create socket
    int sock;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        perror("could not create socket");
        exit(1);
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
    server_addr.sin_port = htons(server_port);
 
    //Connect to remote server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect failed");
        exit(1);
    }
    
    //keep communicating with server
    int count = 0;
    while(1)
    {
        char message[BUF_SIZE];
        
        sprintf(message, "%d", count++);
        int msg_len = strlen(message);

        string send_buf;
        send_buf.append((char*)msg_len, sizeof(int));
        send_buf.append(message, msg_len);
        
        //Send count
        print("[Send]: %s\n", message);
        if( send(sock, send_buf.c_str(), buf.size(), 0) < 0)
        {
            perror("send failed");
            exit(1);
        }

        //Receive a reply from the server
        if( recv(sock, message, BUF_SIZE, 0) < 0)
        {
            perror("recv failed");
            exit(1);
        }

        int recv_size = *(int *)message;
        string recv(message+sizeof(int), recv_size);
        

        print("[Recv]: %s\n", recv.c_str());
        sleep(interval);
    }
    
    close(sock);
    exit(0);
}
