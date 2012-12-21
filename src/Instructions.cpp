#include <Instructions.hpp>
#include <deque>

using namespace std;

void readInstructions(vector<ZInstruction*>& Instructions, int32_t* TabInst, uint32_t Size) {

    int i = 0;
    uint32_t Pos = 0;
    map <int, ZInstruction*> InstPositions;
    int32_t* TabInstB = TabInst;

    // Size in words
    Size /= 4;
    deque <ZInstruction*> InstsToAdjust;

    while (Pos < Size) {
        // Read instruction
        // And then fill arguments values
        ZInstruction* Inst = new ZInstruction();
        Inst->OrigIdx = (int32_t)(TabInst - TabInstB);
        Inst->OpNum = toBigEndian(*TabInst++);
        Pos++;
        InstPositions[Pos] = Inst;
        Inst->idx = i;
        Inst->Annotation = NOTHING;

        for (int j = 0; j < Inst->Arity(); j++) {

            int32_t ArgVal = toBigEndian(*TabInst);

            if (Inst->hasCodeOffset() && Inst->getCodeOffsetArgIdx() == j) {
                ArgVal = Pos + ArgVal + 1;
                InstsToAdjust.push_back(Inst);
            }

            Inst->Args[j] = ArgVal;
            Pos++;
            TabInst++;
        }

        if (Inst->OpNum == CLOSUREREC) {
            Pos++;
            for (int j = 0; j < Inst->Args[0]; j++) {
                auto ArgVal = toBigEndian(*TabInst++);
                Inst->ClosureRecFns[j] = Pos + ArgVal;
            }
            Pos += Inst->Args[0] -1;
            InstsToAdjust.push_back(Inst);
        }

        if (Inst->OpNum == SWITCH) {
            Pos++;
            auto NumSwitchEntries = (Inst->Args[0] >> 16) + (Inst->Args[0] & 0XFFFF);
            for (int j = 0; j < NumSwitchEntries; j++) {
                auto ArgVal = toBigEndian(*TabInst++);
                Inst->SwitchEntries.push_back(Pos + ArgVal);
            }
            Pos += NumSwitchEntries - 1;
            InstsToAdjust.push_back(Inst);
        }

        i++;
        Instructions.push_back(Inst);
    }

    for (auto Inst : InstsToAdjust) {
        if (Inst->OpNum == CLOSUREREC) {
            for (int j = 0; j < Inst->Args[0]; j++) {
                Inst->ClosureRecFns[j] = InstPositions[Inst->ClosureRecFns[j]]->idx;
            }
        } else if (Inst->OpNum == SWITCH) {
            int j = 0;
            for (auto Pos : Inst->SwitchEntries) {
                Inst->SwitchEntries[j] = InstPositions[Pos]->idx;
                j++;
            }
        } else {
            int ArgIdx = Inst->getCodeOffsetArgIdx();
            Inst->Args[ArgIdx] = InstPositions[Inst->Args[ArgIdx]]->idx;
        }
    }

}

void annotateNodes(vector<ZInstruction*>& Instructions) {

    for (auto Inst: Instructions) {
        if (Inst->isClosure()) {
            Instructions[Inst->getDestIdx()]->Annotation = FUNCTION_START;
        } 
        if (Inst->isClosureRec()) {
            for (int j = 0; j < Inst->Args[0]; j++)
                Instructions[Inst->ClosureRecFns[j]]->Annotation = FUNCTION_START;
        } 
        if (Inst->isSwitch()) {
            for (auto Dest : Inst->SwitchEntries)
                Instructions[Dest]->Annotation = BLOCK_START;
        }
        if (Inst->isPushTrap()) {
            Instructions[Inst->getDestIdx()]->Annotation = BLOCK_START;
            Instructions[Inst->idx + 1]->Annotation = BLOCK_START;
        } 
        if (Inst->isJumpInst()) {
            Instructions[Inst->getDestIdx()]->Annotation = BLOCK_START;
            Instructions[Inst->idx + 1]->Annotation = BLOCK_START;
        }
    }

}
