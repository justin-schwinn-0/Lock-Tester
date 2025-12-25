#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <stdexcept>

class OptionParser {
public:
    using Handler = std::function<void(const std::string&)>;

    OptionParser();
    ~OptionParser();

    void addOption(const std::string& name, Handler handler, bool expectsValue);

    bool parse(int argc, char** argv);

private:
    struct Option {
        Handler handler;
        bool expectsValue;
    };

    std::unordered_map<std::string, Option> mOptions;
};

