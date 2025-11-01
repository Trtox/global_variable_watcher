#include <gtest/gtest.h>
#include <string>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#include "elf_utils.hpp"
#include "memory_utils.hpp"

using namespace std;

// Helper: build a minimal C file and compile it to an ELF binary for testing
static string buildTestBinary(const string& name, const string& sourceCode) {
    string srcFile = "/tmp/" + name + ".c";
    string exeFile = "/tmp/" + name;

    ofstream out(srcFile);
    out << sourceCode;
    out.close();

    string cmd = "gcc -g -O0 -no-pie -o " + exeFile + " " + srcFile;
    int ret = system(cmd.c_str());
    if (ret != 0)
        throw runtime_error("Failed to compile test ELF binary");

    return exeFile;
}

// ---------------------------------------------------------------------------
// Test suite for findSymbolAddress()
// ---------------------------------------------------------------------------

TEST(ELFUtils, FindsGlobalVariable) {
    string exe = buildTestBinary("elf_test_global", R"(
        #include <stdio.h>
        int global_var = 4;
        int main() { return global_var; }
    )");

    uintptr_t addr = 0;
    size_t size = 0;

    ASSERT_TRUE(findSymbolAddress(exe, "global_var", addr, size));
    EXPECT_GT(addr, 0);
    EXPECT_GT(size, 0);
}

TEST(ELFUtils, FailsOnNonexistentSymbol) {
    string exe = buildTestBinary("elf_test_missing", R"(
        int main() { return 0; }
    )");

    uintptr_t addr = 0;
    size_t size = 0;

    EXPECT_FALSE(findSymbolAddress(exe, "does_not_exist", addr, size));
}

TEST(ELFUtils, FindsFunctionSymbol) {
    string exe = buildTestBinary("elf_test_func", R"(
        #include <stdio.h>
        void hello() {}
        int main() { hello(); return 0; }
    )");

    uintptr_t addr = 0;
    size_t size = 0;

    ASSERT_TRUE(findSymbolAddress(exe, "hello", addr, size));
    EXPECT_GT(addr, 0);
    EXPECT_GT(size, 0);
}

