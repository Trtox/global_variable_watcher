#pragma once

#include <string>
#include <cstdint>

bool findSymbolAddress(const std::string& path, const std::string& symbol, uintptr_t& address, size_t& size);
