#include "../include/memory_utils.hpp"

#include <sys/ptrace.h>
#include <cerrno>
#include <climits>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>


uintptr_t getBaseAddress(pid_t pid, const std::string& programPath) {
    std::ifstream maps("/proc/" + std::to_string(pid) + "/maps");
    if (!maps.is_open())
        throw std::runtime_error("Failed to open /proc/<pid>/maps");

    std::string line;
    while (std::getline(maps, line)) {
        if (line.find(programPath) != std::string::npos) {
            std::istringstream iss(line);
            std::string addressRange;
            iss >> addressRange;

            std::string start_str = addressRange.substr(0, addressRange.find('-'));
            uintptr_t base = std::stoull(start_str, nullptr, 16);
            return base;
        }
    }

    throw std::runtime_error("Could not find base address for " + programPath);
}

uint64_t readProcessMemory(const pid_t& pid, const uintptr_t& addr, const size_t& size) {
    errno = 0;
    unsigned long data = ptrace(PTRACE_PEEKDATA, pid, (void*)addr, nullptr);

    if (data == static_cast<unsigned long>(-1) && errno != 0) {
        throw std::runtime_error(std::string("Failed to read memory: ") + std::strerror(errno));
    }
    uint64_t val = data;
    if (size == 4) val &= 0xFFFFFFFFULL;
    return val;
}

std::string getAbsolutePath(const std::string &path) {
    char resolved[PATH_MAX];
    if (!realpath(path.c_str(), resolved))
        throw std::runtime_error("Failed to resolve absolute path for " + path);
    return {resolved};
}