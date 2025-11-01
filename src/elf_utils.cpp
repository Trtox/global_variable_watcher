#include "elf_utils.hpp"

#include <elf.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <iostream>
#include <cstring>
#include <stdexcept>

bool findSymbolAddress(const std::string& path, const std::string& symbol, uintptr_t& address, size_t& size) {
    // Open ELF file
    const int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        perror("open");
        return false;
    }

    struct stat st{};
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return false;
    }

    // Map file into memory
    void* const data = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return false;
    }

    auto* const ehdr = reinterpret_cast<const Elf64_Ehdr*>(data);
    auto* const shdrs = reinterpret_cast<const Elf64_Shdr*>(static_cast<const char*>(data) + ehdr->e_shoff);

    const char* const shstrtab = static_cast<const char*>(data) + shdrs[ehdr->e_shstrndx].sh_offset;

    // Iterate sections
    for (int i = 0; i < ehdr->e_shnum; ++i) {
        const char* const sectionName = shstrtab + shdrs[i].sh_name;

        // We care only about symbol tables
        if (std::strcmp(sectionName, ".symtab") != 0 && std::strcmp(sectionName, ".dynsym") != 0)
            continue;

        const auto* symtab = reinterpret_cast<const Elf64_Sym*>(
            static_cast<const char*>(data) + shdrs[i].sh_offset
        );
        const int symCount = shdrs[i].sh_size / sizeof(Elf64_Sym);

        const char* const strtab = static_cast<const char*>(data) + shdrs[shdrs[i].sh_link].sh_offset;

        for (int j = 0; j < symCount; ++j) {
            const char* const name = strtab + symtab[j].st_name;
            if (name && symbol == name) {
                address = symtab[j].st_value;
                size = symtab[j].st_size;

                munmap(data, st.st_size);
                close(fd);
                return true;
            }
        }
    }

    munmap(data, st.st_size);
    close(fd);

    std::cerr << "Error: symbol '" << symbol << "' not found in ELF '" << path << "'\n";
    return false;
}