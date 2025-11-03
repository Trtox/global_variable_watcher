#pragma once

#include <string>

/**
 * Helper structure to hold parsed command-line arguments.
 */
struct Arguments {
    std::string symbol;
    std::string execPath;
    char** execArgs;
};