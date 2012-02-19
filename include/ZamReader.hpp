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
#include <memory>

#include <OpCodes.hpp>
#include <Value.hpp>

#define MAGIC_NUMBER 0x8495A6BE
#define MAGIC_STRING "Caml1999X008"
#define MAGIC_STRING_SIZE 12

class ZamFile {
public:
    std::vector<ZInstruction> Instructions;
    std::vector<Value*> GlobalData;

    ZamFile(const char* Filename);
    inline void printInstructions() {
        for (auto inst: Instructions) inst.Print();
    }

private:
    void readInstructions(std::ifstream& FileStream, uint32_t Size);
    void readData(std::ifstream& FileStream, uint32_t Size);
    void readStrings(std::ifstream& FileStream, uint32_t Size);

};

template <typename T> inline void read(std::ifstream& FileStream, T& buffer) {
    FileStream.read((char*)&buffer, sizeof(T));
}

#endif // ZAM_READER_HPP
