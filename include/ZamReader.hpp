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

class ZamFile {
public:
    std::vector<Instruction> Instructions;

    ZamFile(const char* Filename);
    inline void PrintInstructions() {
        for (auto inst: Instructions) inst.Print();
    }

private:
    void ReadInstructions(std::ifstream& FileStream, uint32_t Size);

};

template <typename T> void Read(std::ifstream& FileStream, T& buffer) {
    FileStream.read((char*)&buffer, sizeof(T));
}

#endif // ZAM_READER_HPP
