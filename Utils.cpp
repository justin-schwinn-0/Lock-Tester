#include "Utils.h"

#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <sstream>

std::vector<std::string> Utils::split(std::string str, std::string delim)
{
    std::vector<std::string> splits;
    uint32_t prevPos = 0;

    size_t pos = 0;
    while ((pos = str.find(delim)) != std::string::npos) 
    {
        std::string token = str.substr(0, pos);
        if(!token.empty())
        {
            splits.push_back(token);
        }
        str.erase(0, pos + delim.length());
    }
    //add remainder of split string
    if(!str.empty())
    {
        splits.push_back(str);
    }

    return splits;
}

int Utils::strToInt(std::string s)
{
    std::istringstream intss(s);
    int ret;
    intss >> ret;
    return ret;
}

uint64_t Utils::strToULong(std::string s)
{
    std::istringstream intss(s);
    uint64_t ret;
    intss >> ret;
    return ret;
}

std::string Utils::getAddressFromHost(std::string host)
{
    struct addrinfo *result;

    int err = getaddrinfo(host.c_str(),NULL,NULL,&result); 

    if(err != 0)
    {
        Utils::log( "getaddrinfo failed" , err , gai_strerror(errno) );

        return "";
    }

    const int addrLen = 1024;
    char addr[addrLen];

    void* p = &((struct sockaddr_in*) result->ai_addr)->sin_addr;

    inet_ntop(result->ai_family, p, addr, addrLen);
    return addr;
}

void Utils::error(std::string s)
{
    log(s+":",strerror(errno));
}

int Utils::pollForFd(int fd, int time, int flag )
{
    pollfd pfds[1];

    pfds[0].fd= fd;
    pfds[0].events = flag;

    
    return poll(pfds,1,time);
}
