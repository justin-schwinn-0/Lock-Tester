#include "OptionParser.h"
#include "Utils.h"


OptionParser::OptionParser()
{}

OptionParser::~OptionParser()
{}

void OptionParser::addOption(const std::string& name,
                             Handler handler,
                             bool expectsValue)
{
    mOptions[name] = Option{handler, expectsValue};
}

bool OptionParser::parse(int argc, char** argv)
{
    std::vector<std::string> positional;

    for (int i = 1; i < argc; ++i) 
    {
        std::string arg = argv[i];

        // Option?
        if (arg.starts_with("-")) 
        {
            auto it = mOptions.find(arg);
            if (it == mOptions.end()) 
            {
                printf("Unknown option: %s", arg.c_str());
                return false;
            }

            std::string value;
            if (it->second.expectsValue) 
            {
                if (i + 1 >= argc) 
                {
                    printf("Option %s requires value: %s",it->first.c_str(),arg.c_str());
                    return false;
                }
                value = argv[++i];
            }

            it->second.handler(value);
        }
        else 
        {
            positional.push_back(arg);
        }
    }

    return true;
}

