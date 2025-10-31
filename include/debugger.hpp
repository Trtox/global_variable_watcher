#pragma once

#include <cstdint>
#include <string>
#include <sys/types.h>

// The Debugger class runs a traced child process and sets a hardware watchpoint
// on a given global variable. When the watched variable is read or written,
// corresponding events are logged in the same format as GDB watchpoints:
//   <symbol>    write    <old> -> <new>
//   <symbol>    read     <value>
class Debugger {
public:
    // programPath: path to the target executable
    // varName: name of the watched symbol (for printing)
    // symbolOffset: offset of the symbol from ELF symbol table (translated at runtime)
    // varSize: size of the variable in bytes (typically 4 or 8)
    // execArgs: optional argv array for execv, may be nullptr
    Debugger(std::string programPath,
             std::string varName,
             uintptr_t symbolOffset,
             size_t varSize,
             char** execArgs);

    // Forks, execs, and starts watching the variable in the child process.
    void run() const;

private:
    // Core function that applies the watchpoint and monitors variable accesses.
    void watchVariable(pid_t pid, uintptr_t runtimeAddress) const;

    // Installs a hardware watchpoint (DR0–DR7) on the given runtime address.
    void setHardwareWatchpoint(pid_t pid, uintptr_t runtimeAddress) const;

    std::string programPath;
    std::string varName;
    uintptr_t symbolOffset;
    size_t varSize;
    char** execArgs;
};