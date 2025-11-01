#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace fs = std::filesystem;

TEST(Integration, GWatchDetectsWrites) {
    const std::string build_dir = fs::current_path(); 
    const std::string gwatchPath = (fs::path(build_dir) / "gwatch").string();
    const std::string testProgramPath = (fs::path(build_dir) / "testprog").string();

    ASSERT_TRUE(fs::exists(gwatchPath)) << "gwatch binary not found";
    ASSERT_TRUE(fs::exists(testProgramPath)) << "testprog binary not found";

    const std::string output_file = (fs::path(build_dir) / "gwatch_output.txt").string();
    const std::string cmd = gwatchPath + " --var global_var --exec " + testProgramPath + " > " + output_file + " 2>&1";

    int ret = std::system(cmd.c_str());
    ASSERT_EQ(ret, 0) << "gwatch exited with nonzero code";

    std::ifstream output(output_file);
    ASSERT_TRUE(output.is_open()) << "Failed to open gwatch_output.txt";

    std::stringstream buffer;
    buffer << output.rdbuf();
    std::string content = buffer.str();

    EXPECT_NE(content.find("global_var"), std::string::npos);
    EXPECT_NE(content.find("write"), std::string::npos);
}