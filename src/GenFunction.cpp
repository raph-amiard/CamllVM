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
    auto FT = FunctionType::get(Type::getVoidTy(getGlobalContext()), false);

    // Create the llvm Function object
    LlvmFunc = Function::Create(FT, Function::ExternalLinkage, name(), Module->TheModule);
    
    if (Id != 0) // is not main function
        LlvmFunc->setCallingConv(CallingConv::Fast);

    // If not main function, initialize restart helper func
    if (Id != MAIN_FUNCTION_ID) {
        auto FT = FunctionType::get(Type::getVoidTy(getGlobalContext()), false);
        RestartFunction = Function::Create(FT, Function::ExternalLinkage, name() + "_Restart", Module->TheModule);
        this->generateRestartFunction();
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



    // Verify if the function is well formed
    verifyFunction(*LlvmFunc);

    return LlvmFunc;
}

void GenFunction::generateRestartFunction() {

    auto Builder = Module->Builder;
    auto Block1 = BasicBlock::Create(getGlobalContext());
    Builder->SetInsertPoint(Block1);
    Builder->CreateCall(Module->getFunction("restart"));
    auto Call = Builder->CreateCall(LlvmFunc);
    Call->setCallingConv(CallingConv::Fast);
    Call->setTailCall();
    Builder->CreateRetVoid();

    RestartFunction->getBasicBlockList().push_back(Block1);
    RestartFunction->setCallingConv(CallingConv::Fast);

    verifyFunction(*RestartFunction);
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
