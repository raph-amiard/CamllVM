#include <Instructions.hpp>
#include <deque>

using namespace std;

static int InstCodeOffsetArgs[][2] = {
    {PUSH_RETADDR, 0},
    {CLOSURE, 1},
    {BRANCH, 0},
    {BRANCHIF, 0},
    {BRANCHIFNOT, 0},
    {PUSHTRAP, 0},
    {BEQ, 1},
    {BNEQ, 1},
    {BLTINT, 1},
    {BLEINT, 1},
    {BGTINT, 1},
    {BGEINT, 1},
    {BULTINT, 1},
    {BUGEINT, 1},
};

map<int, int>& getCodeOffsetArgs() {

    static map<int, int> JumpInsts;
    static bool JumpInstsInit = false;

    if (!JumpInstsInit) {
        JumpInstsInit = true;
        int NbInsts = sizeof(InstCodeOffsetArgs) / (sizeof(int) *2);
        for (int i = 0; i < NbInsts; i++) 
            JumpInsts[InstCodeOffsetArgs[i][0]] = InstCodeOffsetArgs[i][1];
    }

    return JumpInsts;
}


void readInstructions(vector<ZInstruction*>& Instructions, int32_t* TabInst, uint32_t Size) {

    int i = 0;
    uint32_t Pos = 0;
    map <int, ZInstruction*> InstPositions;

    // Size in words
    Size /= 4;
    deque <ZInstruction*> InstsToAdjust;

    while (Pos < Size) {
        // Read instruction
        // And then fill arguments values
        ZInstruction* Inst = new ZInstruction();
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
            for (int j = 0; j < Inst->Args[0]; j++) {
                auto ArgVal = toBigEndian(*TabInst++);
                Pos++;
                Inst->ClosureRecFns[j] = Pos + ArgVal;
            }
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
        } else {
            int ArgIdx = Inst->getCodeOffsetArgIdx();
            Inst->Args[ArgIdx] = InstPositions[Inst->Args[ArgIdx]]->idx;
        }
    }

}

void annotateNodes(vector<ZInstruction*>& Instructions) {

    for (auto Inst: Instructions)
        if (Inst->isClosure()) {
            Instructions[Inst->getDestIdx()]->Annotation = FUNCTION_START;
        } else if (Inst->isClosureRec()) {
            for (int j = 0; j < Inst->Args[0]; j++)
                Instructions[Inst->ClosureRecFns[j]]->Annotation = FUNCTION_START;
        } else if (Inst->isReturn()) {
            Inst->Annotation = FUNCTION_RETURN;
        } else if (Inst->OpNum == PUSHTRAP) {
            Instructions[Inst->getDestIdx()]->Annotation = BLOCK_START;
        } else if (Inst->isJumpInst()) {
            Instructions[Inst->getDestIdx()]->Annotation = BLOCK_START;
            Instructions[Inst->idx + 1]->Annotation = BLOCK_START;
        }

}
