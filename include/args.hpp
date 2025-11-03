#pragma once

#include "types.hpp"

/**
 * @brief Parses command-line arguments into a structured format.
 *
 * @param argc Number of args.
 * @param argv Array of args.
 * @param args Structure that will hold parsed args.
 * @return true if parsing was successful, false otherwise.
 */
bool parseArguments(const int& argc, char** argv, Arguments& args);

/**
 * @brief Prints required usage to user.
 *
 * @param programName Name of current program.
 */
void printUsage(const char* programName);
