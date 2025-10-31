#pragma once

#include <string>

struct Arguments {
    std::string symbol;
    std::string execPath;
    char** execArgs;
};