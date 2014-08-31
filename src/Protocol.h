#ifndef _Protocol
#define _Protocol

#include <string>
#include <boost/container/vector.hpp>
#include "exception.h"
#include "Protocol.h"
using namespace std;


class Package
{
    unsigned int size;
    string content;  // size bytes

public:
    Package(string &stream);

    string serialize();

    static void str2pkg(string &stream, vector<Package> &pkgs);

    static void pkg2str(vector<Package> &pkgs, string &stream);
};

class ProxyPackage
{
    int proxy_id;
    int client_sock;
    Package pkg;

public:
    ProxyPackage(int proxy_id, int client_sock, string &stream);

    ProxyPackage(int proxy_id, int client_sock, Package &pkg);
    
    string serialize();

    int const get_client_sock();

    int const get_proxy_id();

    Package const &get_pkg();

    static void str2pkg(int proxy_id, int client_sock, string &stream, vector<ProxyPackage> &pkgs);

    static void pkg2str(vector<ProxyPackage> &pkgs, string &stream);
};


#endif
