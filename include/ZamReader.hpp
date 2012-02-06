#ifndef ZAM_READER_HPP
#define ZAM_READER_HPP

#include <string>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <tuple>
#include <list>
#include <boost/cstdint.hpp>
#include <iostream>
#include <fstream>
#include <OpCodes.hpp>

#define MAGIC_STRING "Caml1999X008"
#define MAGIC_STRING_SIZE 12

class ZamFile {
public:
    std::vector<Instruction> Instructions;

    ZamFile(const char* Filename);
    inline void printInstructions() {
        for (auto inst: Instructions) inst.Print();
    }

private:
    void readInstructions(std::ifstream& FileStream, uint32_t Size);

};

template <typename T> inline void read(std::ifstream& FileStream, T& buffer) {
    FileStream.read((char*)&buffer, sizeof(T));
}

#endif // ZAM_READER_HPP
