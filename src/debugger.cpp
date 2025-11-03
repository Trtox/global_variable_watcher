#include "debugger.hpp"
#include "memory_utils.hpp"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>

#include <unistd.h>
#include <cerrno>
#include <cstring>

#include <iostream>
#include <stdexcept>

static void ptraceChecked(int request, const pid_t& pid, void* addr, void* data, const char* errMsg) {
    errno = 0;
    if (ptrace(static_cast<__ptrace_request>(request), pid, addr, data) == -1 && errno != 0) {
        throw std::runtime_error(std::string(errMsg) + ": " + std::strerror(errno));
    }
}

static long dr_len_code(const size_t& size) {
    switch (size) {
        case 1: return 0b00;
        case 2: return 0b01;
        case 4: return 0b11;
        case 8: return 0b10;
        default: return -1;
    }
}

Debugger::Debugger(std::string programPath,
                   std::string varName,
                   uintptr_t varAddress,
                   size_t varSize,
                   char** execArgs)
    : programPath(std::move(programPath)),
      varName(std::move(varName)),
      symbolOffset(varAddress),
      varSize(varSize),
      execArgs(execArgs) {}

void Debugger::run() const {
    pid_t pid = fork();
    if (pid == -1) {
        throw std::runtime_error(std::string("fork() failed: ") + std::strerror(errno));
    }

    if (pid == 0) {
        if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) == -1) {
            perror("ptrace(TRACEME)");
            _exit(1);
        }
        raise(SIGSTOP);

        if (execArgs) {
            execvp(programPath.c_str(), execArgs);
        } else {
            char* defaultArgs[] = { const_cast<char*>(programPath.c_str()), nullptr };
            execvp(programPath.c_str(), defaultArgs);
        }

        perror("exec failed");
        _exit(1);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        throw std::runtime_error(std::string("waitpid failed: ") + std::strerror(errno));
    }

    if (!WIFSTOPPED(status)) {
        throw std::runtime_error("Child did not stop as expected after exec/raise(SIGSTOP)");
    }

    std::string absPath = getAbsolutePath(programPath);
    ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed after initial stop");
    usleep(10000);

    uintptr_t base = getBaseAddress(pid, absPath);
    uintptr_t runtimeAddr = base + symbolOffset;

    std::cerr << "Runtime variable address: 0x" << std::hex << runtimeAddr << std::dec
              << " (process " << pid << ")\n";

    watchVariable(pid, runtimeAddr);
}

void Debugger::setHardwareWatchpoint(pid_t pid, uintptr_t runtimeAddr) const {
    ptraceChecked(PTRACE_POKEUSER, pid,
                  reinterpret_cast<void*>(offsetof(user, u_debugreg[0])),
                  reinterpret_cast<void*>(runtimeAddr),
                  "Failed to set DR0");

    constexpr long ENABLE_LOCAL_DR0 = 1 << 0;
    constexpr long RW_READWRITE = 0b11;
    long len_code = dr_len_code(varSize);
    if (len_code < 0) throw std::runtime_error("Unsupported variable size for watchpoint");

    long dr7 = ENABLE_LOCAL_DR0 | (RW_READWRITE << 16) | (len_code << 18);

    ptraceChecked(PTRACE_POKEUSER, pid,
                  reinterpret_cast<void*>(offsetof(user, u_debugreg[7])),
                  reinterpret_cast<void*>(dr7),
                  "Failed to write DR7");
}

void clearDebugStatus(pid_t pid) {
    ptraceChecked(PTRACE_POKEUSER, pid,
                  reinterpret_cast<void*>(offsetof(user, u_debugreg[6])),
                  0,
                  "Failed to clear DR6");
}

void Debugger::watchVariable(pid_t pid, uintptr_t runtimeAddress) const  {
    uint64_t lastValue = 0;
    try {
        lastValue = readProcessMemory(pid, runtimeAddress, varSize);
    } catch (const std::exception &e) {
        std::cerr << "Initial read failed: " << e.what() << "\n";
        return;
    }

    std::cerr << varName << " initial=" << lastValue << "\n";

    setHardwareWatchpoint(pid, runtimeAddress);
    clearDebugStatus(pid);
    ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed to start watch loop");

    while (true) {
        int status = 0;
        if (waitpid(pid, &status, 0) == -1) {
            throw std::runtime_error(std::string("waitpid failed: ") + std::strerror(errno));
        }

        if (WIFEXITED(status)) {
            std::cerr << "Child exited (" << WEXITSTATUS(status) << ")\n";
            break;
        }

        if (WIFSTOPPED(status)) {
            const int sig = WSTOPSIG(status);

            if (sig == SIGTRAP) {
                errno = 0;
                long dr6 = ptrace(PTRACE_PEEKUSER, pid, reinterpret_cast<void*>(offsetof(user, u_debugreg[6])), nullptr);
                if (dr6 == -1 && errno != 0) {
                    std::cerr << "Warning: failed to read DR6: " << std::strerror(errno) << "\n";
                }

                if ( (dr6 & 0x1) != 0 ) {
                    uint64_t currentValue = 0;
                    try {
                        currentValue = readProcessMemory(pid, runtimeAddress, varSize);
                    } catch (const std::exception &e) {
                        std::cerr << "Read during trap failed: " << e.what() << "\n";
                        clearDebugStatus(pid);
                        ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed after failed read");
                        continue;
                    }

                    if (currentValue != lastValue) {
                        std::cout << varName << "    write    " << lastValue << " -> " << currentValue << "\n";
                        lastValue = currentValue;
                    } else {
                        std::cout << varName << "    read     " << currentValue << "\n";
                    }

                    clearDebugStatus(pid);
                    ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed after trap handling");
                    continue;
                }

                ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed for non-DR0 SIGTRAP");
                continue;
            }

            try {
                uint64_t currentValue = readProcessMemory(pid, runtimeAddress, varSize);
                if (currentValue != lastValue) {
                    std::cout << varName << "    write    " << lastValue << " -> " << currentValue << "\n";
                    lastValue = currentValue;
                }
            } catch (...) {}

            ptraceChecked(PTRACE_CONT, pid, nullptr, reinterpret_cast<void*>(static_cast<long>(sig)),
                          "ptrace(PTRACE_CONT) failed when forwarding signal");
        }
    }
}