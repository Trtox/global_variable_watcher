#include <gtest/gtest.h>
#include "debugger.hpp"

#include <string>
#include <cerrno>

class DebuggerTest : public ::testing::Test {
protected:
    std::unique_ptr<Debugger> dbg;

    void SetUp() override {
        dbg = std::make_unique<Debugger>(
            "/bin/ls",      // program path
            "global_var",   // variable name
            0x1234,         // symbol offset
            8,              // variable size
            nullptr         // exec args
        );
    }
};


TEST_F(DebuggerTest, ConstructorInitializesMembers) {
    EXPECT_EQ(dbg->getProgramPath(), "/bin/ls");
    EXPECT_EQ(dbg->getVarName(), "global_var");
    EXPECT_EQ(dbg->getSymbolOffset(), 0x1234);
    EXPECT_EQ(dbg->getVarSize(), 8);
}
