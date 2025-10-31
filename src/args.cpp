#include "../include/args.hpp"

#include <cstring>
#include <iostream>

bool parseArguments(const int& argc, char** argv, Arguments& args) {
    if (argc < 5) {
        return false;
    }

    if (std::strcmp(argv[1], "--var") != 0) {
        std::cerr << "Error: Expected '--var' as first argument\n";
        return false;
    }
    args.symbol = argv[2];

    if (args.symbol.empty()) {
        std::cerr << "Error: Symbol name cannot be empty\n";
        return false;
    }

    if (std::strcmp(argv[3], "--exec") != 0) {
        std::cerr << "Error: Expected '--exec' as third argument\n";
        return false;
    }
    args.execPath = argv[4];

    if (args.execPath.empty()) {
        std::cerr << "Error: Executable path cannot be empty\n";
        return false;
    }

    if (argc > 5) {
        if (std::strcmp(argv[5], "--") != 0) {
            std::cerr << "Error: Expected '--' before program arguments\n";
            return false;
        }
        args.execArgs = argv + 6;
    } else {
        args.execArgs = nullptr;
    }

    return true;
}

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " --var <symbol> --exec <path> [-- arg1 ... argN]\n";
    std::cerr << "\nOptions:\n";
    std::cerr << "  --var <symbol>    Symbol/variable to watch\n";
    std::cerr << "  --exec <path>     Path to executable to run\n";
    std::cerr << "  -- arg1 ... argN  Optional arguments to pass to the executable\n";
}