#include <Instructions.hpp>

using namespace std;

void readInstructions(vector<ZInstruction>& Instructions, int32_t* TabInst, uint32_t Size) {
    ZInstruction Inst;
    int i = 0;

    while (Size > 0) {
        // Read instruction
        // And then fill arguments values
        Inst.OpNum = toBigEndian(*TabInst++);
        cout << "INST OPNUM : " << toBigEndian(Inst.OpNum) << endl;
        Inst.idx = i;
        Inst.Annotation = NOTHING;
        int32_t Arity = Inst.Arity();
        for (int j = 0; j < Inst.Arity(); j++) 
            Inst.Args[j] = toBigEndian(*TabInst++);

        if (Inst.OpNum == CLOSUREREC) {
            Arity+=Inst.Args[0];
            TabInst += Inst.Args[0];
        }

        Size -= (4 + (4*Arity));
        i++;
        cout << "READ ISNTRUCTION : ";
        Inst.Print(true);
        Instructions.push_back(Inst);
    }
}

static int JumpInstsArgs[][2] = {
    {PUSH_RETADDR, 0},
    {CLOSURE, 1},
    //{CLOSUREREC, 2},
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

map<int, int>& getJumpInsts() {

    static map<int, int> JumpInsts;
    static bool JumpInstsInit = false;

    if (!JumpInstsInit) {
        JumpInstsInit = true;
        int NbInsts = sizeof(JumpInstsArgs) / (sizeof(int) *2);
        for (int i = 0; i < NbInsts; i++) 
            JumpInsts[JumpInstsArgs[i][0]] = JumpInstsArgs[i][1];
    }

    return JumpInsts;
}

void convertCodeOffsets(vector<ZInstruction>& Instructions) {

    auto JumpInsts = getJumpInsts();
    map<int, ZInstruction*> InstOffsets;
    int Offset = 0;

    for (ZInstruction& Inst: Instructions) {
        InstOffsets[Offset] = &Inst;
        Offset += Inst.Arity() + 1;
    }

    for (auto OInst = InstOffsets.begin(); OInst != InstOffsets.end(); OInst++) {
        auto it = JumpInsts.find(OInst->second->OpNum); 
        if (it != JumpInsts.end()) {
            Offset = OInst->second->Args[it->second] + (1 + it->second);
            int TargetInstOffset = OInst->first + Offset;
            ZInstruction* TargetInst = InstOffsets[TargetInstOffset];
            OInst->second->Args[it->second] = TargetInst->idx;
        }
    }
}

void annotateNodes(vector<ZInstruction>& Instructions) {

    for (ZInstruction& Inst: Instructions)
        if (Inst.isClosure()) {
            Instructions[Inst.getDestIdx()].Annotation = FUNCTION_START;
        }
        else if (Inst.isReturn())
            Inst.Annotation = FUNCTION_RETURN;
        else if (Inst.isJumpInst()) {
            Instructions[Inst.getDestIdx()].Annotation = BLOCK_START;
            Instructions[Inst.idx + 1].Annotation = BLOCK_START;
        }

}
