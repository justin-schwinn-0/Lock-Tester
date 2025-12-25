#include "Utils.h"

#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <cstring>
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
