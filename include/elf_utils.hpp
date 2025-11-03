#pragma once

#include <string>
#include <cstdint>

/**
 * @brief Finds the address and size of a symbol in a binary(ELF).
 * This function parses the symbol table of the given binary to locate the specified symbol.
 *
 * @param path Path to the executable.
 * @param symbol Name of the symbol to search for.
 * @param address Output parameter that will hold the symbol's address if found.
 * @param size Output parameter that will hold the size (in bytes) of the symbol if found.
 * @return true if the symbol was found successfully, false otherwise.
 */
bool findSymbolAddress(const std::string& path, const std::string& symbol, uintptr_t& address, size_t& size);
