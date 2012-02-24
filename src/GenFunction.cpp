#include <CodeGen.hpp>
#include "llvm/Analysis/Verifier.h"
#include "llvm/LLVMContext.h"

using namespace std;
using namespace llvm;

// ================ GenFunction Implementation ================== //

GenFunction::GenFunction(int Id, GenModule* Module) {
    this->Id = Id;
    this->Module = Module;
    this->ApplierFunction = nullptr;
    this->LlvmFunc = nullptr;
}

void GenFunction::Print() {
    for (auto BlockP : Blocks) {
        printTab(1);
        cout << "Block " << BlockP.first << ":" << endl;
        BlockP.second->Print();
    }
}

void GenFunction::PrintBlocks() {
    for (auto BlockP : Blocks) {
        printTab(1);
        cout << "Block " << BlockP.first;
    }
}

string GenFunction::name() {
    stringstream ss;
    ss << "Function_" << Id;
    return ss.str();
}


Function* GenFunction::CodeGen() {

    // Make function type
    vector<Type*> ArgTypes(this->Arity, getValType());
    //auto RetType = this->Id == MAIN_FUNCTION_ID ? Type::getVoidTy(getGlobalContext()) : getValType();
    auto FT = FunctionType::get(getValType(), ArgTypes, false);

    // Create the llvm Function object
    LlvmFunc = Function::Create(FT, Function::ExternalLinkage, name(), Module->TheModule);

    // Put the function arguments on the stack of the first block
    for (auto AI = LlvmFunc->arg_begin(); AI != LlvmFunc->arg_end(); ++AI)
        FirstBlock->Stack.push_front(AI);

    // Generate each block and put it in the function's list of blocks
    for (auto BlockP : Blocks) {
        BlockP.second->CodeGen();
        BlockP.second->genTermInst();
        BlockP.second->dumpStack();
        LlvmFunc->getBasicBlockList().push_back(BlockP.second->LlvmBlock);
    }

    for (auto BlockP : Blocks) {
        BlockP.second->handlePHINodes();
    }

    // Generate Applier
    if (this->Id != MAIN_FUNCTION_ID)
        this->generateApplierFunction();

    // Verify if the function is well formed
    LlvmFunc->dump();
    verifyFunction(*LlvmFunc);

    return LlvmFunc;
}

void GenFunction::generateApplierFunction() {
    auto Builder = Module->Builder;

    vector<Type*> ArgTypes(2, getValType());
    auto FT = FunctionType::get(getValType(), ArgTypes, false);
    ApplierFunction = Function::Create(FT, Function::ExternalLinkage, name() + "_Applier", Module->TheModule);

    auto Block1 = BasicBlock::Create(getGlobalContext());
    Builder->SetInsertPoint(Block1);

    auto Closure = ApplierFunction->arg_begin();
    auto BlockSize = Builder->CreateCall(Module->TheModule->getFunction("getBlockSize"), Closure);

    vector<Value*> Args;
    for (int i = 0; i < Arity; i++) {
        Args.push_back(Builder->CreateCall2(
            Module->TheModule->getFunction("getField"),
            Closure,
            Builder->CreateSub(BlockSize, ConstInt(1 + Arity - i))
        ));
    }
    auto Ret = Builder->CreateCall(this->LlvmFunc, ArrayRef<Value*>(Args));
    Builder->CreateRet(Ret);

    ApplierFunction->getBasicBlockList().push_back(Block1);

    verifyFunction(*ApplierFunction);
}


Function* GenBlock::getFunction(string Name) {
  auto F = Function->Module->TheModule->getFunction(Name);
  return F;
}
