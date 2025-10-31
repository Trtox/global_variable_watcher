#include <iostream>
#include <string>

#include "../include/args.hpp"     
#include "../include/elf_utils.hpp"   
#include "../include/debugger.hpp"    

int main(int argc, char** argv) {
    Arguments args;
    if (!parseArguments(argc, argv, args)) {
        printUsage(argv[0]);
        return 1;
    }

    std::cout << "Symbol to watch: " << args.symbol << "\n";
    std::cout << "Executable path: " << args.execPath << "\n";
    if (args.execArgs != nullptr) {
        std::cout << "Executable arguments:\n";
        for (char** a = args.execArgs; *a != nullptr; ++a) {
            std::cout << "  - " << *a << "\n";
        }
    }

    uintptr_t symbolOffset = 0;
    size_t symbolSize = 0;
    if (!findSymbolAddress(args.execPath, args.symbol, symbolOffset, symbolSize)) {
        std::cerr << "Error: symbol '" << args.symbol << "' not found in " << args.execPath << "\n";
        return 2;
    }

    std::cout << "Symbol " << args.symbol << " found at 0x"
              << std::hex << symbolOffset << std::dec
              << " (size=" << symbolSize << " bytes)\n";


    Debugger dbg(args.execPath, args.symbol, symbolOffset, symbolSize, args.execArgs);
    try {
        dbg.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 3;
    }

    return 0;
}
