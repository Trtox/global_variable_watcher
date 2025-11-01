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
        // Stop ourselves so parent can prepare (parent will wait)
        raise(SIGSTOP);

        // Execute target
        if (execArgs) {
            execvp(programPath.c_str(), execArgs);
        } else {
            // default argv: { programPath, nullptr }
            char* defaultArgs[] = { const_cast<char*>(programPath.c_str()), nullptr };
            execvp(programPath.c_str(), defaultArgs);
        }

        // exec failed
        perror("exec failed");
        _exit(1);
    }

    // Parent
    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        throw std::runtime_error(std::string("waitpid failed: ") + std::strerror(errno));
    }

    if (!WIFSTOPPED(status)) {
        throw std::runtime_error("Child did not stop as expected after exec/raise(SIGSTOP)");
    }

    // Resolve absolute path to match /proc/<pid>/maps entries reliably
    std::string absPath = getAbsolutePath(programPath);

    // Let the child continue so the loader maps segments; small delay can help
    // (child is stopped on SIGSTOP from raise; continue it now)
    ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed after initial stop");

    // Wait briefly for loader to map the binary. We could also poll /proc/<pid>/maps.
    // Here we block until the child stops again or sleep briefly and then read maps.
    usleep(10000); // 10ms

    // Get base mapping for the executable in the child's address space
    uintptr_t base = getBaseAddress(pid, absPath);
    uintptr_t runtimeAddr = base + symbolOffset;

    // Print runtime mapping info to stderr (keeps stdout purely for access events)
    std::cerr << "Runtime variable address: 0x" << std::hex << runtimeAddr << std::dec
              << " (process " << pid << ")\n";

    // Now call the watch loop (which sets DR registers and prints events to stdout)
    watchVariable(pid, runtimeAddr);
}


void Debugger::setHardwareWatchpoint(pid_t pid, uintptr_t runtimeAddr) const {
    // Set DR0 to the watched address
    ptraceChecked(PTRACE_POKEUSER, pid,
                  reinterpret_cast<void*>(offsetof(user, u_debugreg[0])),
                  reinterpret_cast<void*>(runtimeAddr),
                  "Failed to set DR0");

    // Build DR7:
    // enable local DR0 (bit 0)
    constexpr long ENABLE_LOCAL_DR0 = 1 << 0;
    // RW (bits 16-17) = 3 (read/write)
    constexpr long RW_READWRITE = 0b11;
    long len_code = dr_len_code(varSize);
    if (len_code < 0) throw std::runtime_error("Unsupported variable size for watchpoint");

    // place RW and LEN for breakpoint 0 at bits 16..19
    long dr7 = ENABLE_LOCAL_DR0 | (RW_READWRITE << 16) | (len_code << 18);

    ptraceChecked(PTRACE_POKEUSER, pid,
                  reinterpret_cast<void*>(offsetof(user, u_debugreg[7])),
                  reinterpret_cast<void*>(dr7),
                  "Failed to write DR7");
}

void clearDebugStatus(pid_t pid) {
    // Clear DR6 (status) by writing zero
    ptraceChecked(PTRACE_POKEUSER, pid,
                  reinterpret_cast<void*>(offsetof(user, u_debugreg[6])),
                  0,
                  "Failed to clear DR6");
}

void Debugger::watchVariable(pid_t pid, uintptr_t runtimeAddress) const  {
    // Read initial value
    uint64_t lastValue = 0;
    try {
        lastValue = readProcessMemory(pid, runtimeAddress, varSize);
    } catch (const std::exception &e) {
        std::cerr << "Initial read failed: " << e.what() << "\n";
        return;
    }

    // Print initial value to stderr so stdout is only access events (you requested stdout events,
    // but initial value can be helpful — put to stderr to avoid test capture confusion).
    std::cerr << varName << " initial=" << lastValue << "\n";

    // Install hardware watchpoint
    setHardwareWatchpoint(pid, runtimeAddress);

    // Ensure DR6 is clear
    clearDebugStatus(pid);

    // Continue the child under tracing
    ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed to start watch loop");

    // Event loop
    while (true) {
        int status = 0;
        if (waitpid(pid, &status, 0) == -1) {
            throw std::runtime_error(std::string("waitpid failed: ") + std::strerror(errno));
        }

        if (WIFEXITED(status)) {
            // child exited normally
            // we don't print access events here, only final status to stderr
            std::cerr << "Child exited (" << WEXITSTATUS(status) << ")\n";
            break;
        }

        if (WIFSTOPPED(status)) {
            const int sig = WSTOPSIG(status);

            // If it's SIGTRAP, likely our watchpoint fired.
            if (sig == SIGTRAP) {
                // Read DR6 to see which breakpoint triggered (bit 0 -> DR0)
                errno = 0;
                long dr6 = ptrace(PTRACE_PEEKUSER, pid, reinterpret_cast<void*>(offsetof(user, u_debugreg[6])), nullptr);
                if (dr6 == -1 && errno != 0) {
                    // If we can't read DR6, attempt to still check memory; but warn.
                    std::cerr << "Warning: failed to read DR6: " << std::strerror(errno) << "\n";
                }

                // On DR0 hit, compare value to detect read vs write
                if ( (dr6 & 0x1) != 0 ) {
                    uint64_t currentValue = 0;
                    try {
                        currentValue = readProcessMemory(pid, runtimeAddress, varSize);
                    } catch (const std::exception &e) {
                        std::cerr << "Read during trap failed: " << e.what() << "\n";
                        // Clear DR6 and resume, continue loop
                        clearDebugStatus(pid);
                        ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed after failed read");
                        continue;
                    }

                    if (currentValue != lastValue) {
                        // write
                        // print to stdout in required format: "<symbol>    write    42 -> 50"
                        std::cout << varName << "    write    " << lastValue << " -> " << currentValue << "\n";
                        lastValue = currentValue;
                    } else {
                        // read
                        std::cout << varName << "    read     " << currentValue << "\n";
                    }

                    // Clear DR6 status bits so the watchpoint can trigger again
                    clearDebugStatus(pid);

                    // Resume child
                    ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed after trap handling");
                    continue;
                } // end dr0_hit

                // Not DR0: could be single-step or other trap; forward as default
                // For non-watchpoint SIGTRAP, we resume (no output).
                ptraceChecked(PTRACE_CONT, pid, nullptr, nullptr, "ptrace(PTRACE_CONT) failed for non-DR0 SIGTRAP");
                continue;
            } // end if SIGTRAP

            // Other signals: forward them to the child (so it behaves naturally)
            // For these we don't treat as watch events — but check memory optionally
            // We'll try to detect writes on arbitrary stops as well (best-effort)
            try {
                uint64_t currentValue = readProcessMemory(pid, runtimeAddress, varSize);
                if (currentValue != lastValue) {
                    std::cout << varName << "    write    " << lastValue << " -> " << currentValue << "\n";
                    lastValue = currentValue;
                }
            } catch (...) {
                // ignore read errors on generic signals
            }

            // Resume, forwarding the same signal to child so it keeps semantics
            ptraceChecked(PTRACE_CONT, pid, nullptr, reinterpret_cast<void*>(static_cast<long>(sig)),
                          "ptrace(PTRACE_CONT) failed when forwarding signal");
        } // end WIFSTOPPED
    } // end while
}