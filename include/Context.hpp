#include <Instructions.hpp>
#include <CodeGen.hpp>
#include <string>

class Context {
    std::string FileName;
    GenModule* Mod;

protected:
    std::vector<ZInstruction*> Instructions;

public:
    virtual ~Context() {};
    void init(std::string FileName, int EraseFrom, int EraseFirst, int EraseLast);
    virtual void generateMod();
    virtual void compile();
    void exec(bool PrintTime);

};

class SimpleContext : public Context {
public:
    void generateMod();
    void compile();
    void exec();
};
