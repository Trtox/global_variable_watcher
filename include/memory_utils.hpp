#pragma once

#include <sys/types.h>
#include <cstdint>
#include <string>

uintptr_t getBaseAddress(pid_t pid, const std::string& programPath);
uint64_t readProcessMemory(const pid_t& pid, const uintptr_t& addr, const size_t& size);

std::string getAbsolutePath(const std::string &path);
