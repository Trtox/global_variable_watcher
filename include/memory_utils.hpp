#pragma once

#include <sys/types.h>
#include <cstdint>
#include <string>

/**
 * @brief Retrieves the base address of a loaded executable in a process.
 *
 * Reads the memory mapping of the process to find the starting address
 * of the given executable in the process's address space.
 *
 * @param pid Process ID of the target process.
 * @param programPath Absolute or relative path to the executable.
 * @return The base address where the executable is mapped in memory.
 * @throws std::runtime_error If the maps file cannot be opened or the
 *         executable mapping is not found.
 */
uintptr_t getBaseAddress(pid_t pid, const std::string& programPath);

/**
 * @brief Reads memory from a target process at a specific address.
 *
 * Uses `ptrace` to read data from another process's memory.
 *
 * @param pid Process ID of the target process.
 * @param addr Address in the target process's memory to read from.
 * @param size Number of bytes to read.
 * @return The value read from memory as a 64-bit integer.
 * @throws std::runtime_error If the read fails.
 */
uint64_t readProcessMemory(const pid_t& pid, const uintptr_t& addr, const size_t& size);

/**
 * @brief Resolves an absolute path for a given file or directory.
 *
 *
 * @param path Input path (absolute or relative).
 * @return Absolute path as a string.
 * @throws std::runtime_error If the path cannot be resolved.
 */
std::string getAbsolutePath(const std::string &path);