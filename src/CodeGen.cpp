#include <CodeGen.hpp>

#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/IRReader.h"

extern "C" {
    #include <ocaml_runtime/mlvalues.h>
    #include <ocaml_runtime/major_gc.h>
    #include <ocaml_runtime/memory.h>
}

#include <stdexcept>
#include <cstdio>

/* GC interface 
 * Those macros are empty for the moment 
 * But need to be defined if we want to use Alloc_small
 */

#define Setup_for_gc \
  {}
#define Restore_after_gc \
  {}
#define Setup_for_c_call \
  {}
#define Restore_after_c_call \
  {}

using namespace std;
using namespace llvm;

// ============================ HELPERS ============================== //

Type* getValType() {
    return Type::getIntNTy(getGlobalContext(), sizeof(value) * 8);
}

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
            generateFunction(&QInstructions);
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
    initFunction(Module->MainFunction, &MainBlockInsts);

    return Module;
}

void GenModuleCreator::generateFunction(deque<ZInstruction*>* Instructions) {

    deque<ZInstruction*> FuncInsts;
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
        FuncInsts.push_back(Inst);

        // Keep track of the labeled instruction with the max idx
        // that is part of the function
        if (Inst->isJumpInst()) MaxInstIdx = max(MaxInstIdx, Inst->getDestIdx());

        // If the instruction is a return instruction
        // And there is no jump to an instruction after this one
        // We have reached the end of the function
        if (Inst->isReturn() && Inst->idx >= MaxInstIdx) {
            initFunction(Func, &FuncInsts);
            return;
        }
    }

}

void GenModuleCreator::initFunction(GenFunction* Function, deque<ZInstruction*>* Instructions) {
    genBlocks(Function, Instructions);
}

void GenModuleCreator::genBlocks(GenFunction* Function, deque<ZInstruction*>* Instructions) {

    // The first instruction is the beginning of a block
    Instructions->at(0)->Annotation = BLOCK_START;

    // Make a block for every instruction that is a BLOCK_START
    // This is necessary so we can reference next and previous blocks
    for (ZInstruction* Inst: *Instructions)
        if (Inst->Annotation == BLOCK_START) {
            auto Block = new GenBlock(Inst->idx, Function);
            Function->Blocks[Inst->idx] = Block;
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
            if (Inst->isJumpInst()) {
                CBlock->setNext(Function->Blocks[Inst->getDestIdx()], true);
            }
        }
    }

}

// ================ GenBlock Implementation ================== //

GenBlock::GenBlock(int Id, GenFunction* Function) {
    // Basic init
    this->Id = Id;
    this->Function = Function;
    this->Builder = Function->Module->Builder;

    // Create the LlvmBlock
    LlvmBlock = BasicBlock::Create(getGlobalContext(), name());
}

void GenBlock::setNext(GenBlock* Block, bool IsBrBlock) {
    NextBlocks.push_back(Block);
    Block->PreviousBlocks.push_front(this);
    if (IsBrBlock) BrBlock = Block;
    else NoBrBlock = Block;
}

std::string GenBlock::name() {
    stringstream ss;
    ss << "Block_" << Id;
    return ss.str();
}

Value* GenBlock::getStackAt(size_t n) {
    if (n >= Stack.size()) {
        if (PreviousBlocks.size() == 0)
            throw std::logic_error("Bad stack access !");
        else if (PreviousBlocks.size() == 1) {
            return PreviousBlocks.front()->Stack[n - Stack.size()];
        } else {
            // TODO: Make the PHI
            printf("YOU SHOULDNT BE HERE MECREANT\n");
            return nullptr;
        }
    } else {
        return Stack[n];
    }
}

Value* GenBlock::stackPop() {
    auto Val = getStackAt(0);
    // TODO: Consider previous blocks stacks
    Stack.pop_front();
    return Val;
}

void GenBlock::push() { 
    this->Stack.push_front(Accu); 
}
void GenBlock::acc(int n) { this->Accu = this->getStackAt(n); }

void GenBlock::envAcc(int n) { 
    auto Env = Builder->CreateCall(getFunction("getEnv"));
    Accu = Builder->CreateCall2(getFunction("getField"), 
                               Env,
                               ConstInt(n));
}

void GenBlock::pushAcc(int n) { push(); acc(n); }

BasicBlock* GenBlock::CodeGen() {
    // Create the block and generate instruction's code
    Builder->SetInsertPoint(LlvmBlock);

    for (auto Inst : this->Instructions)
        GenCodeForInst(Inst);

    return LlvmBlock;
}

Value* GenBlock::castToInt(Value* Val) {
    auto IntType = Type::getInt32Ty(getGlobalContext());
    if (Val->getType() != IntType)
        return Builder->CreatePtrToInt(Val, IntType);
    else
        return Val;
}

Value* GenBlock::castToPtr(Value* Val) {
    if (Val->getType() != getValType())
        return Builder->CreateIntToPtr(Val, getValType());
    else
        return Val;
}

Value* GenBlock::intVal(Value* From) {
    return Builder->CreateLShr(From, 1);
}

Value* GenBlock::valInt(Value* From) {
    return Builder->CreateAdd(Builder->CreateShl(From, 1), ConstInt(1));
}

Value* ConstInt(uint64_t val) {
    return ConstantInt::get(
        getGlobalContext(), 
        APInt(sizeof(value)*8, val, /*signed=*/true)
    );
}

void GenBlock::makeApply(size_t n) {

    ClosureInfo CI = Function->ClosuresFunctions[Accu];


    if (CI.IsBare && CI.LlvmFunc->arg_size() == n) {

        vector<Value*> ArgsV;
        for (size_t i = 0; i < n; i++)
            ArgsV.push_back(castToPtr(getStackAt(i)));
        Accu = Builder->CreateCall(CI.LlvmFunc, ArgsV);

    } else {

        auto Array = Builder->CreateAlloca(ArrayType::get(getValType(), n));
        for (size_t i = 0; i < n; i++) {
            vector<Value*> GEPlist; 
            GEPlist.push_back(ConstInt(0));
            GEPlist.push_back(ConstInt(i));
            auto Ptr = Builder->CreateGEP(Array, GEPlist);
            auto Val = getStackAt(i);
            Builder->CreateStore(Val, Ptr);
        }

        auto ArrayPtr = Builder->CreatePointerCast(Array, getValType()->getPointerTo());
        ArrayPtr->dump();
        Accu = Builder->CreateCall3(getFunction("apply"), Accu, ConstInt(n), ArrayPtr);

    }
}

void GenBlock::GenCodeForInst(ZInstruction* Inst) {

    Value *TmpVal;

    cout << "GENERATING INSTRUCTION ";
    Inst->Print();
        printf("%p\n", Accu);

    switch (Inst->OpNum) {

        case CONST0: this->Accu = ConstInt(Val_int(0)); break;
        case CONST1: this->Accu = ConstInt(Val_int(1)); break;
        case CONST2: this->Accu = ConstInt(Val_int(2)); break;
        case CONST3: this->Accu = ConstInt(Val_int(3)); break;
        case CONSTINT:
            this->Accu = ConstInt(Val_int(Inst->Args[0]));
            break;

        case PUSH: push(); break;
        case PUSH_RETADDR:
            TmpVal = Accu;
            Accu = nullptr;
            push(); push(); push();
            Accu = TmpVal;
            break;

        case PUSHACC0: pushAcc(0); break;
        case PUSHACC1: pushAcc(1); break;
        case PUSHACC2: pushAcc(2); break;
        case PUSHACC3: pushAcc(3); break;
        case PUSHACC4: pushAcc(4); break;
        case PUSHACC5: pushAcc(5); break;
        case PUSHACC6: pushAcc(6); break;
        case PUSHACC7: pushAcc(7); break;


        case PUSHCONST0: push(); this->Accu = ConstInt(Val_int(0)); break;
        case PUSHCONST1: push(); this->Accu = ConstInt(Val_int(1)); break;
        case PUSHCONST2: push(); this->Accu = ConstInt(Val_int(2)); break;
        case PUSHCONST3: push(); this->Accu = ConstInt(Val_int(3)); break;
        case PUSHCONSTINT: 
            push(); 
            this->Accu = ConstInt(Val_int(Inst->Args[0])); 
            break;

        case ACC0: acc(0); break;
        case ACC1: acc(1); break;
        case ACC2: acc(2); break;
        case ACC3: acc(3); break;
        case ACC4: acc(4); break;
        case ACC5: acc(5); break;
        case ACC6: acc(6); break;
        case ACC7: acc(7); break;

        case ENVACC1: envAcc(1); break;
        case ENVACC2: envAcc(2); break;
        case ENVACC3: envAcc(3); break;
        case ENVACC4: envAcc(4); break;
        case ENVACC:  envAcc(Inst->Args[0]); break;

        case PUSHENVACC1: push(); envAcc(1); break;
        case PUSHENVACC2: push(); envAcc(2); break;
        case PUSHENVACC3: push(); envAcc(3); break;
        case PUSHENVACC4: push(); envAcc(4); break;
        case PUSHENVACC:  push(); envAcc(Inst->Args[0]); break;

        // Fall through return
        case STOP:
            cout << "IN STOP INSTRUCTION \n";
        case RETURN: 
            Builder->CreateRet(castToPtr(Accu)); 
            break;

        case ADDINT:
            TmpVal = castToInt(stackPop());
            Accu = Builder->CreateAdd(castToInt(Accu), Builder->CreateSub(TmpVal, ConstInt(1)));
            break;
        case NEGINT:
            Accu = Builder->CreateSub(ConstInt(2), Accu);
            break;
        case SUBINT:
            TmpVal = stackPop();
            Accu = Builder->CreateSub(Accu, Builder->CreateAdd(TmpVal, ConstInt(1)));
            break;
        case MULINT:
            TmpVal = stackPop();
            TmpVal = Builder->CreateMul(intVal(Accu), intVal(TmpVal));
            Accu = valInt(TmpVal);
            break;
        case OFFSETINT:
            Accu = Builder->CreateAdd(Accu, ConstInt(Inst->Args[0] << 1));
            break;

        case CLOSUREREC:
            // Simple recursive function with no trampoline and no closure fields
            if (Inst->Args[0] == 1 && Inst->Args[1] == 0) {
                cout << "INTO MAKE CLOSUREREC" << endl;
                Inst->Args[0] = 0;
                Inst->Args[1] = Inst->Args[2];
                goto closure;
            } else {
                // TODO: Understand how this CLOSUREREC shit works properly
            }
            break;

        case CLOSURE:{
            closure:
            auto MakeClos = getFunction("makeClosure");
            auto ClosSetVar = getFunction("closureSetVar");

            // Create pointer to dest function
            auto DestGenFunc = Function->Module->Functions[Inst->Args[1]];
            if (DestGenFunc->LlvmFunc == NULL) {
                DestGenFunc->CodeGen();
            }
            Builder->SetInsertPoint(LlvmBlock);
            auto ClosureFunc = DestGenFunc->ApplierFunction;
            auto CastPtr = Builder->CreatePtrToInt(ClosureFunc, getValType());


            int FuncNbArgs = ClosureFunc->arg_size();
            int NbFields = Inst->Args[0];

            Accu = Builder->CreateCall3(MakeClos, 
                                        ConstInt(NbFields), 
                                        CastPtr, 
                                        ConstInt(FuncNbArgs));

            // Set Closure fields
            for (int i = 0; i < NbFields; i++)
                Builder->CreateCall3(ClosSetVar, Accu, ConstInt(i), getStackAt(i));

            ClosureInfo CI = {DestGenFunc->LlvmFunc, NbFields == 0 ? true : false};
            this->Function->ClosuresFunctions[Accu] = CI;
            break;
        }

        case OFFSETCLOSURE0: {
            Accu = Builder->CreateCall(getFunction("getEnv"));
            ClosureInfo CI = {Function->LlvmFunc, true};
            this->Function->ClosuresFunctions[Accu] = CI;
            break;
        }

        case APPLY1: makeApply(1); break;
        case APPLY2: makeApply(2); break;
        case APPLY3: makeApply(3); break;
        case APPLY:  makeApply(Inst->Args[0]); break;
        case APPTERM1: makeApply(1); break;
        case APPTERM2: makeApply(2); break;
        case APPTERM3: makeApply(3); break;
        case APPTERM: makeApply(Inst->Args[0]); break;


        case BRANCH:{
            BasicBlock* LBrBlock = BrBlock->LlvmBlock;
            Builder->CreateBr(LBrBlock);
            break;
        }

        case BEQ:
            TmpVal = Builder->CreateICmpEQ(ConstInt(Inst->Args[0]), intVal(Accu));
            goto makebr;
        case BNEQ:
           TmpVal = Builder->CreateICmpNE(ConstInt(Inst->Args[0]), intVal(Accu));
           goto makebr;
        case BLTINT:
           TmpVal = Builder->CreateICmpSLT(ConstInt(Inst->Args[0]), intVal(Accu));
           goto makebr;
        case BLEINT:
           TmpVal = Builder->CreateICmpSLE(ConstInt(Inst->Args[0]), intVal(castToInt(Accu)));
           goto makebr;
        case BGTINT:
           TmpVal = Builder->CreateICmpSGT(ConstInt(Inst->Args[0]), intVal(Accu));
           goto makebr;
        case BGEINT:
           TmpVal = Builder->CreateICmpSGE(ConstInt(Inst->Args[0]), intVal(Accu));
           goto makebr;
        case BULTINT:
           TmpVal = Builder->CreateICmpULT(ConstInt(Inst->Args[0]), intVal(Accu));
           goto makebr;
        case BUGEINT:
           TmpVal = Builder->CreateICmpUGE(ConstInt(Inst->Args[0]), intVal(Accu));
           goto makebr;

        makebr: {
            BasicBlock* LBrBlock = BrBlock->LlvmBlock;
            BasicBlock* LNoBrBlock = NoBrBlock->LlvmBlock;
            Builder->CreateCondBr(TmpVal, LBrBlock, LNoBrBlock);
            break;
        }


    }
}

void GenBlock::Print() {
    for (ZInstruction* Inst : Instructions) {
        printTab(2);
        Inst->Print(true);
    }
}

void GenBlock::PrintAdjBlocks() {
    cout << "PREDECESSORS : ";

    for (auto Block : PreviousBlocks)
        cout << Block->Id << " ";

    cout << "\nSUCCESSORS : ";

    for (auto Block : NextBlocks)
        cout << Block->Id << " ";

    cout << "\n";
}


// ================ GenFunction Implementation ================== //

GenFunction::GenFunction(int Id, GenModule* Module) {
    this->Id = Id;
    this->Module = Module;
}

void GenFunction::Print() {
    for (auto BlockP : Blocks) {
        printTab(1);
        cout << "BLOCK " << BlockP.first << ":" << endl;
        BlockP.second->Print();
    }
}

void GenFunction::PrintBlocks() {
    for (auto BlockP : Blocks) {
        printTab(1);
        cout << "BLOCK " << BlockP.first;
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
        LlvmFunc->getBasicBlockList().push_back(BlockP.second->LlvmBlock);
    }

    // Generate Applier
    if (this->Id != MAIN_FUNCTION_ID)
        this->generateApplierFunction();

    // Verify if the function is well formed
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

// ================ GenModule Implementation ================== //

GenModule::GenModule() {

    InitializeNativeTarget();
    SMDiagnostic Diag;
    TheModule = ParseIRFile("bin/StdLib.ll", Diag, getGlobalContext()); //new Module("testmodule", getGlobalContext());
    for (Function& Func : TheModule->getFunctionList()) {
        if (Func.getName() == "closureSetVar") {
            llvm::Attributes Attrs;
            Attrs |= Attribute::AlwaysInline;
            Func.addFnAttr(Attrs);
        }
    }
    Builder = new IRBuilder<>(getGlobalContext());
    string ErrStr;
    ExecEngine = EngineBuilder(TheModule).setErrorStr(&ErrStr).create();
    if (!ExecEngine) {
        cerr << "Could not create ExecutionEngine: " << ErrStr << endl;
        exit(1);
    }

    PM = new PassManager();
    PM->add(createFunctionInliningPass());
    PM->add(createAlwaysInlinerPass());

    FPM = new FunctionPassManager(TheModule);
    FPM->add(new TargetData(*ExecEngine->getTargetData()));
    FPM->add(createBasicAliasAnalysisPass());
    FPM->add(createInstructionCombiningPass());
    FPM->add(createReassociatePass());
    FPM->add(createGVNPass());
    FPM->add(createCFGSimplificationPass());
    FPM->add(createSCCPPass());

}

void GenModule::Print() {
    cout << " ============= FUNCTIONS ============ " << endl << endl;
    for (auto FuncP : Functions) {
        cout << "FUNCTION " << FuncP.first << " (";
        for (int i = 0; i < FuncP.second->Arity; i++) {
            cout << "arg" << i;
            if (i < FuncP.second->Arity - 1) cout << ", ";
        }
        cout << ") { " << endl;
        FuncP.second->Print();
        cout << "}" << endl;
    }
    cout << "MAIN FUNCTION { " << endl;
    MainFunction->Print();
    cout << "}\n";
}

Function* GenBlock::getFunction(string Name) {
  auto F = Function->Module->TheModule->getFunction(Name);
  return F;
}
