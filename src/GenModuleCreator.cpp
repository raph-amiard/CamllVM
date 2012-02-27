#include <CodeGen.hpp>
#include <stdexcept>

using namespace std;
using namespace llvm;

// ================ GenModuleCreator Implementation ================== //

GenModule* GenModuleCreator::generate(int FirstInst, int LastInst) {

    deque<ZInstruction*> QInstructions;
    deque<ZInstruction*> MainBlockInsts;

    // If last inst is negative, it will trim the last X instructions
    if (LastInst == 0) LastInst = OriginalInstructions->size() - 1;
    else if (LastInst < 0) LastInst = OriginalInstructions->size() + LastInst;

    // Create a queue of all instructions
    for (int i = FirstInst; i <= LastInst; i++) {
        QInstructions.push_back(OriginalInstructions->at(i));
    }

    // Create functions. This will also remove the function's instructions
    // from the queue, so that only the remaining instructions are added 
    // to MainBlockInsts
    while (QInstructions.size()) {
        ZInstruction* Inst = QInstructions[0];
        if (Inst->Annotation == FUNCTION_START) {
            auto FuncInsts = initFunction(&QInstructions);
            generateFunction(Module->Functions[Inst->idx], FuncInsts);
        } else {
            if (Inst->OpNum != RESTART) {
                MainBlockInsts.push_back(Inst);
            }
            QInstructions.pop_front();
        }
    }

    // Create the main function, based on the remaining instructions
    Module->MainFunction = new GenFunction(MAIN_FUNCTION_ID, Module);
    Module->MainFunction->Arity = 0;
    generateFunction(Module->MainFunction, &MainBlockInsts);

    return Module;
}

deque<ZInstruction*>* GenModuleCreator::initFunction(deque<ZInstruction*>* Instructions) {

    auto FuncInsts = new deque<ZInstruction*>();
    int MaxInstIdx = 0;

    // Create the Function and add it to the module functions
    int InstIdx = Instructions->at(0)->idx;
    GenFunction* Func = new GenFunction(InstIdx, this->Module);
    this->Module->Functions[InstIdx] = Func;

    // Remove first instruction if it is a GRAB
    ZInstruction* FirstInst = Instructions->at(0);
    if (FirstInst->OpNum == GRAB) {
        Func->Arity = FirstInst->Args[0] + 1;
        Instructions->pop_front();
    } else {
        Func->Arity = 1;
    }

    ZInstruction* Inst;
    while (true) {
        Inst = Instructions->at(0);
        Instructions->pop_front();
        FuncInsts->push_back(Inst);

        // Keep track of the labeled instruction with the max idx
        // that is part of the function
        if (Inst->isJumpInst()) MaxInstIdx = max(MaxInstIdx, Inst->getDestIdx());

        // If the instruction is a return instruction
        // And there is no jump to an instruction after this one
        // We have reached the end of the function
        if (Inst->isReturn() && Inst->idx >= MaxInstIdx) {
            return FuncInsts;
        }
    }

}

void GenModuleCreator::generateFunction(GenFunction* Function, deque<ZInstruction*>* Instructions) {

    // The first instruction is the beginning of a block
    Instructions->at(0)->Annotation = BLOCK_START;

    // Make a block for every instruction that is a BLOCK_START
    // This is necessary so we can reference next and previous blocks
    for (ZInstruction* Inst: *Instructions) {
        if (Inst->Annotation == BLOCK_START) {
            auto Block = new GenBlock(Inst->idx, Function);
            Function->Blocks[Inst->idx] = Block;
        }
    }

    // Get the first block
    Function->FirstBlock = Function->Blocks[Instructions->at(0)->idx];

    while (Instructions->size()) {

        // Get the first remaining instruction
        ZInstruction* Inst = Instructions->at(0);
        Instructions->pop_front();
        

        if (Inst->Annotation != BLOCK_START)
            throw std::logic_error("Malformed blocks ! ");

        // Get the corresponding block
        GenBlock* CBlock = Function->Blocks[Inst->idx];
        CBlock->Instructions.push_back(Inst);

        if (Inst->isJumpInst())
            CBlock->setNext(Function->Blocks[Inst->getDestIdx()], true);

        if (Inst->OpNum == PUSHTRAP) {
            CBlock->setNext(Function->Blocks[Inst->Args[0]], true);
        }

        // Put all the instructions into the block
        // Until we reach the next BLOCK_START
        while (Instructions->size()) {
            Inst = Instructions->at(0);

            // If the next inst is a block start, get out
            if (Inst->Annotation == BLOCK_START) {

                // But before, make the block connections if necessary
                ZInstruction* LastInst = CBlock->Instructions.back();
                if (!(LastInst->isUncondJump() || LastInst->isReturn())) {
                    CBlock->setNext(Function->Blocks[Inst->idx], false);
                }
                break;
            }

            Instructions->pop_front();
            CBlock->Instructions.push_back(Inst);

            // If we stumble on a jump instruction
            // Reference the destination block as a next block
            // And reference the current block as a previous block
            // of the destination
            if (Inst->isJumpInst() || Inst->OpNum == PUSHTRAP) {
                CBlock->setNext(Function->Blocks[Inst->getDestIdx()], true);
            }
        }
    }

}

