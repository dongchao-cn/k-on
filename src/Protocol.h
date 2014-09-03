#ifndef _Protocol
#define _Protocol

#include <string>
#include <boost/container/vector.hpp>
#include "exception.h"
using namespace std;


class Package
{
    unsigned int size;
    string content;  // size bytes

public:
    Package(string &stream);

    Package(unsigned int size, string content);

    string serialize();

    void print();

    static void str2pkg(string &stream, vector<Package> &pkgs);

    static void pkg2str(vector<Package> &pkgs, string &stream);
};

class ProxyPackage
{
    int client_sock;
    Package pkg;

public:
    ProxyPackage(int client_sock, string &stream);

    ProxyPackage(int client_sock, Package &pkg);
    
    string serialize();

    int const get_client_sock();

    Package const &get_pkg();

    void print();

    static void str2pkg(int client_sock, string &stream, vector<ProxyPackage> &pkgs);

    static void pkg2str(vector<ProxyPackage> &pkgs, string &stream);
};


#endif
