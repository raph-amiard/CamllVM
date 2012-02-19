#ifndef INSTRUCTIONS_HPP
#define INSTRUCTIONS_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>

#include <OpCodes.hpp>
#include <EndianUtils.hpp>

void readInstructions(std::vector<ZInstruction>& Instructions, int32_t* TabInst, uint32_t Size);

inline void printInstructions(std::vector<ZInstruction>& Instructions, bool LineNums=true, int StartAt=0) {
    for (int i = StartAt; i < Instructions.size(); i++) {

        switch (Instructions[i].Annotation) {
            case FUNCTION_START:
                std::cout << " ===== FUNCTION ===== " << std::endl;
                break;
            case BLOCK_START:
                std::cout << " ===== BLOCK ===== " << std::endl;
                break;
        }

        Instructions[i].Print(LineNums);

        if (Instructions[i].Annotation == FUNCTION_RETURN) 
            std::cout << std::endl;
    }
}

void convertCodeOffsets(std::vector<ZInstruction>& Instructions);
void annotateNodes(std::vector<ZInstruction>& Instructions);


#endif
