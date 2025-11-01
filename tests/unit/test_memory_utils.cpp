#include <gtest/gtest.h>
#include "memory_utils.hpp"

#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

#include <filesystem>
#include <string>


TEST(MemoryUtils, GetAbsolutePath_Valid) {
    std::string cwd = std::filesystem::current_path();
    std::string result = getAbsolutePath(".");
    EXPECT_EQ(result, cwd);
}

TEST(MemoryUtils, GetAbsolutePath_InvalidPath) {
    EXPECT_THROW(getAbsolutePath("/this/does/not/exist/12345"), std::runtime_error);
}

TEST(MemoryUtils, GetBaseAddress_Self) {
    pid_t pid = getpid();
    std::string exePath = getAbsolutePath("/proc/self/exe");

    uintptr_t base = getBaseAddress(pid, exePath);

    // Should be non-zero and page-aligned
    EXPECT_NE(base, 0);
    EXPECT_EQ(base % 0x1000, 0);
}


TEST(MemoryUtils, ReadProcessMemory_Self) {
    pid_t pid = getpid();

    if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1) {
        GTEST_SKIP() << "Skipping ptrace test: insufficient permission or restricted by ptrace_scope";
    }

    waitpid(pid, nullptr, 0);

    int localVar = 1234;
    const auto address = reinterpret_cast<uintptr_t>(&localVar);
    const uint64_t value = readProcessMemory(pid, address, sizeof(localVar));
    EXPECT_EQ(value, static_cast<uint64_t>(localVar));

    ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
}