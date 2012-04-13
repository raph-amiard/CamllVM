#include <Utils.hpp>
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
    auto FT = FunctionType::get(getValType(), ArgTypes, false);

    // Create the llvm Function object
    LlvmFunc = Function::Create(FT, Function::ExternalLinkage, name(), Module->TheModule);
    if (Id != 0) // is not main function
        LlvmFunc->setCallingConv(CallingConv::Fast);

    // Put the function arguments on the stack of the first block
    size_t i = 0;
    for (auto AI = LlvmFunc->arg_begin(); AI != LlvmFunc->arg_end(); ++AI) {
        stringstream s; s << "arg" << i;
        FirstBlock->Stack.push_front(new StackValue(AI));
        AI->setName(s.str());
        i++;
    }

    //FirstBlock->addCallInfo();

    // Generate each block and put it in the function's list of blocks
    for (auto BlockP : Blocks) {
        BlockP.second->CodeGen();
        BlockP.second->genTermInst();
        //DEBUG(BlockP.second->dumpStack();)
        for (auto BBlock : BlockP.second->LlvmBlocks)
            LlvmFunc->getBasicBlockList().push_back(BBlock);
    }


    // Handle phi nodes in reverse order, so that if phi are generated 
    // in previous blocks they will be handled
    bool StillHasPhiNodes = true;
    while (StillHasPhiNodes) {
        for (auto BIt = Blocks.rbegin(); BIt != Blocks.rend(); ++BIt)
            BIt->second->handlePHINodes();

        StillHasPhiNodes = false;
        for (auto BIt = Blocks.rbegin(); BIt != Blocks.rend(); ++BIt) 
            if (BIt->second->PHINodes.size()) StillHasPhiNodes = true;
    }

    // Generate Applier
    if (this->Id != MAIN_FUNCTION_ID)
        this->generateApplierFunction();

    // Verify if the function is well formed
    //DEBUG(LlvmFunc->dump();)
    verifyFunction(*LlvmFunc);

    LlvmFunc->setGC("shadow-stack");

    return LlvmFunc;
}

void GenFunction::generateApplierFunction() {
    auto Builder = Module->Builder;

    vector<Type*> ArgTypes(2, getValType());
    auto FT = FunctionType::get(getValType(), ArgTypes, false);
    ApplierFunction = Function::Create(FT, Function::ExternalLinkage, name() + "_Applier", Module->TheModule);

    auto Block1 = BasicBlock::Create(getGlobalContext());
    Builder->SetInsertPoint(Block1);

    auto ArgsTab = ApplierFunction->arg_begin();

    vector<Value*> Args;
    for (int i = 0; i < Arity; i++) {
        Args.push_back(Builder->CreateCall2(
            Module->TheModule->getFunction("getField"),
            ArgsTab,
            ConstInt(Arity - i - 1)
        ));
    }
    auto Ret = Builder->CreateCall(this->LlvmFunc, ArrayRef<Value*>(Args));
    Ret->setCallingConv(CallingConv::Fast);
    Builder->CreateRet(Ret);

    ApplierFunction->getBasicBlockList().push_back(Block1);

    verifyFunction(*ApplierFunction);
}

/*
 * Remove blocks which are not the first block
 * And have no predecessors.
 * IE : Dead code blocks
 */
void GenFunction::removeUnusedBlocks() {
    int i = 0;
    list<int> ToRemoveBlocks;
    bool restart = false;
    for (auto BlockP : Blocks) {
        if (i > 0 && BlockP.second->PreviousBlocks.size() == 0) {
            ToRemoveBlocks.push_front(BlockP.first);
        }
        i++;
    }
    for (auto BlockNum : ToRemoveBlocks) {
        auto Block = Blocks[BlockNum];
        for (auto NBlock : Block->NextBlocks) {
            NBlock->PreviousBlocks.remove(Block);
            if (NBlock->PreviousBlocks.size() == 0) restart = true;
        }
        Blocks.erase(BlockNum);
    }
    if (restart) removeUnusedBlocks();
}
