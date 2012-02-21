#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include <list>
#include <vector>
#include <map>
#include <deque>
#include <sstream>

#include "llvm/Value.h"
#include "llvm/Module.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/PassManager.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"

#include <Instructions.hpp>

#define MAIN_FUNCTION_ID 0

class GenModule;
class GenFunction;

class CodeGen {
public:
    virtual void Print() = 0;
};

inline void printTab(int nbTabs) {
    for (int i = 0; i < nbTabs; i++)
        std::cout << "  ";
}

// ================ GenBlock Declaration ================== //

struct ClosureInfo {
    llvm::Function* LlvmFunc;
    bool IsBare;
};

class GenBlock : public CodeGen {
    friend class GenModuleCreator;
    friend class GenFunction;

private:
    int Id;
    GenFunction* Function;
    llvm::IRBuilder<> * Builder;

    // Siblings blocks handling
    std::list<GenBlock*> PreviousBlocks;
    std::list<GenBlock*> NextBlocks;
    GenBlock* BrBlock;
    GenBlock* NoBrBlock;

    // Instructions to generate
    std::vector<ZInstruction*> Instructions;
    
    // Stack handling
    std::deque<llvm::Value*> Stack;
    llvm::Value* Accu;

    // CodeGen result
    llvm::BasicBlock* LlvmBlock;

    // This pointer is set only if the block last instruction 
    // is a conditional branch. It is equal to the llvm Val 
    // containing the result of the test
    llvm::Value* CondVal;

public:
    GenBlock(int Id, GenFunction* Function);
    void setNext(GenBlock* Block, bool IsBrBlock);
    void Print(); 
    void PrintAdjBlocks(); 
    void GenCodeForInst(ZInstruction* Inst);
    llvm::BasicBlock* CodeGen();
    std::string name();

    // Codegen helper funcs
    void pushAcc(int n);
    void acc(int n);
    void envAcc(int n);
    void push();
    void makeApply(size_t n);
    void makeClosure(int32_t NbFields, int32_t FnId);
    void debug(llvm::Value* DbgVal);

    llvm::Value* getStackAt(size_t n);
    llvm::Value* stackPop();

    llvm::Value* intVal(llvm::Value* From);
    llvm::Value* valInt(llvm::Value* From);
    llvm::Value* castToInt(llvm::Value* Val);
    llvm::Value* castToPtr(llvm::Value* Val);

    // Function getters
    llvm::Function* getFunction(std::string FuncName);

};

llvm::Value* ConstInt(uint64_t val);


// ================ GenFunction Declaration ================== //

class GenFunction : public CodeGen {
    friend class GenModuleCreator;
    friend class GenModule;
    friend class GenBlock;

private:
    int Id;
    int Arity;

    std::map<int, GenBlock*> Blocks;
    GenBlock* FirstBlock;
    GenModule* Module;

    // Map closures to llvm Functions to keep track of signatures
    std::map<llvm::Value*, ClosureInfo> ClosuresFunctions;

    void generateApplierFunction();

public:
    llvm::Function* ApplierFunction;
    llvm::Function* LlvmFunc;
    GenFunction(int Id, GenModule* Module);
    std::string name();
    void Print(); 
    void PrintBlocks(); 
    llvm::Function* CodeGen();
};


// ================ GenModule Declaration ================== //

class GenModule : public CodeGen {

    friend class GenModuleCreator;
    friend class GenFunction;
    friend class GenBlock;

public:

    GenFunction* MainFunction;
    std::map<int, GenFunction*> Functions;

    llvm::FunctionPassManager* FPM;
    llvm::PassManager* PM;
    llvm::Module *TheModule;
    llvm::IRBuilder<> * Builder;
    llvm::ExecutionEngine* ExecEngine;

    GenModule();
    void Print(); 
};


// ================ GenModuleCreator Declaration ================== //

class GenModuleCreator {

private:
    std::vector<ZInstruction*>* OriginalInstructions;
    GenModule* Module;

public:

    GenModuleCreator(std::vector<ZInstruction*>* Instructions) { 
        this->OriginalInstructions = Instructions; 
        Module = new GenModule();
    }

    GenModule* generate(int FirstInst=0, int LastInst=0);
    std::deque<ZInstruction*>* initFunction(std::deque<ZInstruction*>* Instructions);
    void generateFunction(GenFunction* Function, std::deque<ZInstruction*>* Instructions);
};

// ================ STDLIB Declaration ================== //

extern "C" {
    char* makeClosure(int NVars, int32_t* FPtr, int NbArgs, int NbSuppliedArgs);
    void setField(char* Block, int Idx, char* Val); 
    void closureSetVar(char* Closure, int VarIdx, char* Value); 
    void closureSetArg(char* Closure, int ArgIdx, char* Value); 
    char* closureGetNbSuppliedArgs(char* Closure, char* Value);
    char* closureGetNbArgs(char* Closure, char* Value);
}
#endif // CODEGEN_HPP
