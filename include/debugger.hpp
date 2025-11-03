#pragma once

#include <cstdint>
#include <string>
#include <sys/types.h>

/**
 * @brief The Debugger class runs a child process under ptrace and sets a hardware watchpoint
 * on a specified global variable. It logs reads and writes to the variable in the following format:
 *
 * Example output:
 *   <symbol>    write    <old> -> <new>
 *   <symbol>    read     <value>
 */
class Debugger {
public:
    /**
     * @brief Constructs a Debugger for the given target program and variable.
     *
     * @param programPath Path to the target executable.
     * @param varName Name of the global variable to watch (for logging purposes).
     * @param symbolOffset Offset of the symbol from the ELF symbol table (resolved at runtime).
     * @param varSize Size of the variable in bytes (4 or 8).
     * @param execArgs Optional argv array to pass to execv in the child process.
     */
    Debugger(std::string programPath,
             std::string varName,
             uintptr_t symbolOffset,
             size_t varSize,
             char** execArgs);

    /** @brief Returns the path of the target program. */
    [[nodiscard]] std::string getProgramPath() const { return programPath; }

    /** @brief Returns the name of the watched variable. */
    [[nodiscard]] std::string getVarName() const { return varName; }

    /** @brief Returns the ELF symbol offset of the watched variable. */
    [[nodiscard]] uintptr_t getSymbolOffset() const { return symbolOffset; }

    /** @brief Returns the size in bytes of the watched variable. */
    [[nodiscard]] size_t getVarSize() const { return varSize; }

    /**
     * @brief Forks and execs the target process, sets the hardware watchpoint, and
     * monitors the variable in the child process.
     */
    void run() const;

private:
    /**
     * @brief Main loop that applies the hardware watchpoint and logs variable accesses.
     *
     * @param pid PID of the traced child process.
     * @param runtimeAddress Runtime memory address of the watched variable.
     */
    void watchVariable(pid_t pid, uintptr_t runtimeAddress) const;

    /**
     * @brief Installs a hardware watchpoint using debug registers DR0–DR7.
     *
     * @param pid PID of the traced child process.
     * @param runtimeAddress Runtime memory address of the variable to watch.
     */
    void setHardwareWatchpoint(pid_t pid, uintptr_t runtimeAddress) const;

    std::string programPath;   ///< Path to the target executable
    std::string varName;       ///< Name of the variable to watch
    uintptr_t symbolOffset;    ///< ELF symbol offset
    size_t varSize;            ///< Variable size in bytes
    char** execArgs;           ///< Optional exec arguments
};