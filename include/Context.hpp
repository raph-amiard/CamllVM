#include <Instructions.hpp>
#include <CodeGen.hpp>
#include <string>

class Context {
    std::string FileName;
    std::vector<ZInstruction*> Instructions;
    GenModule* Mod;

public:
    void init(std::string FileName, int PrintFrom, int EraseFirst, int EraseLast);
    void generateMod();
    void compile();
    void exec();
};
