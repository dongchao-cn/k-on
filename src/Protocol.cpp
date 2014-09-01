#include <string>
#include <boost/container/vector.hpp>
#include "exception.h"
#include "Protocol.h"
using namespace std;

Package::Package(string &stream)
{
    // init from stream, 
    size = *(unsigned int *)stream.c_str();
    if (stream.size() < sizeof(size) + size)
        throw StreamEOFException;
    content = stream.substr(sizeof(size), size);
    stream = stream.substr(sizeof(int)+size);
}

Package::Package(unsigned int size, string content) : size(size), content(content){ }

string Package::serialize()
{
    // return the package to buffer
    string ret;
    ret.append((char*)&size, sizeof(int));
    ret.append(content);
    return ret;
}

void Package::print()
{
    printf("%s\n", content.c_str());
}

void Package::str2pkg(string &stream, vector<Package> &pkgs)
{
    while (1)
    {
        try
        {
            pkgs.push_back(Package(stream));
        }
        catch (int e)
        {
            if (e == StreamEOFException)
                break;
            else
                throw;
        }
    }
}

void Package::pkg2str(vector<Package> &pkgs, string &stream)
{
    for (int i = 0; i < pkgs.size(); ++i)
    {
        stream.append(pkgs[i].serialize());
    }
    pkgs.clear();
}



ProxyPackage::ProxyPackage(int proxy_id, int client_sock, string &stream) : \
    proxy_id(proxy_id), client_sock(client_sock), pkg(Package(stream))
{ }

ProxyPackage::ProxyPackage(int proxy_id, int client_sock, Package &pkg) : \
    proxy_id(proxy_id), client_sock(client_sock), pkg(pkg)
{ }

string ProxyPackage::serialize()
{
    // return the package to buffer
    string ret;
    ret.append((char*)&proxy_id, sizeof(int));
    ret.append((char*)&client_sock, sizeof(int));
    ret.append(pkg.serialize());
    return ret;
}

int const ProxyPackage::get_client_sock()
{
    return client_sock;
}

int const ProxyPackage::get_proxy_id()
{
    return proxy_id;
}

Package const &ProxyPackage::get_pkg()
{
    return pkg;
}

void ProxyPackage::print()
{
    printf("[%d %d] ", proxy_id, client_sock);
    pkg.print();
}

void ProxyPackage::str2pkg(int proxy_id, int client_sock, string &stream, vector<ProxyPackage> &pkgs)
{
    while (1)
    {
        try
        {
            pkgs.push_back(ProxyPackage(proxy_id, client_sock, stream));
        }
        catch (int e)
        {
            if (e == StreamEOFException)
                break;
            else
                throw;
        }
    }
}

void ProxyPackage::pkg2str(vector<ProxyPackage> &pkgs, string &stream)
{
    for (int i = 0; i < pkgs.size(); ++i)
    {
        stream.append(pkgs[i].serialize());
    }
    pkgs.clear();
}

