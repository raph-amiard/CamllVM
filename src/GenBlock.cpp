#include <CodeGen.hpp>
#include <Utils.hpp>

#include "llvm/DerivedTypes.h"
#include "llvm/LLVMContext.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
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

#define Setup_for_gc {}
#define Restore_after_gc {}
#define Setup_for_c_call {}
#define Restore_after_c_call {}

using namespace std;
using namespace llvm;

// ============================ HELPERS ============================== //

Type* getValType() {
    return Type::getIntNTy(getGlobalContext(), sizeof(value) * 8);
}

llvm::Type* getBoolType() {
    return Type::getInt1Ty(getGlobalContext());
}


// ================ GenBlock Implementation ================== //

GenBlock::GenBlock(int Id, GenFunction* Function) {
    // Basic init
    this->Id = Id;
    this->Function = Function;
    this->Builder = Function->Module->Builder;
    this->Sp = Function->Module->TheModule->getGlobalVariable("StackPointer");
    this->Accu = Function->Module->TheModule->getGlobalVariable("Accu");

    addBlock();
}

pair<BasicBlock*, BasicBlock*> GenBlock::addBlock() {
    auto OldBlock = LlvmBlock;
    LlvmBlock = BasicBlock::Create(getGlobalContext(), name());
    LlvmBlocks.push_back(LlvmBlock);
    return make_pair(OldBlock, LlvmBlock);
}

void GenBlock::setNext(GenBlock* Block, bool IsBrBlock) {
    NextBlocks.push_back(Block);
    Block->PreviousBlocks.push_front(this);
    if (IsBrBlock) BrBlock = Block;
    else NoBrBlock = Block;
}

void GenBlock::addCallInfo() {
    Builder->SetInsertPoint(LlvmBlock);
    makeCall1("addCall", ConstInt(Function->Id));
}

std::string GenBlock::name() {
    stringstream ss;
    ss << "Block_" << Id;
    return ss.str();
}

Value* GenBlock::getStackAt(size_t n) {
    auto Ptr = Builder->CreateGEP(Sp, ConstInt(n));
    return Builder->CreateLoad(Ptr);
}

Value* GenBlock::getAccu(bool CreatePhi) {
    return Builder->CreateLoad(Accu);
}

void GenBlock::push(bool CreatePhi) { makeCall0("push"); }

void GenBlock::acc(int n) {makeCall1("acc", ConstInt(n));}

void GenBlock::envAcc(int n) { 
    makeCall1("envAcc", ConstInt(n));
}

void GenBlock::pushAcc(int n) { push(); acc(n); }

BasicBlock* GenBlock::CodeGen() {
    // Create the block and generate instruction's code
    Builder->SetInsertPoint(LlvmBlock);

    DEBUG(debug(ConstInt(this->Id));)

    if (Function->Id == MAIN_FUNCTION_ID && this->PreviousBlocks.size() == 0)
        makeCall0("init");

    for (auto Inst : this->Instructions)
        GenCodeForInst(Inst);

    return LlvmBlock;
}

void GenBlock::genTermInst() {

    Builder->SetInsertPoint(LlvmBlock);
    auto Inst = Instructions.back();

    if (!(Inst->isJumpInst() || Inst->isReturn() || Inst->isSwitch()))
        Builder->CreateBr(this->NextBlocks.front()->LlvmBlock);
}

Value* GenBlock::castToInt(Value* Val) {
    if (Val->getType() != getValType())
        return Builder->CreatePtrToInt(Val, getValType());
    else
        return Val;
}

Value* GenBlock::castToVal(Value* Val) {
    if (Val->getType() != getValType()) {
        if (Function->BoolsAsVals.find(Val) != Function->BoolsAsVals.end()) {
            return Function->BoolsAsVals[Val];
        } else {
            return Builder->CreateIntCast(Val, getValType(), true);
        }
    } else
        return Val;
}

Value* GenBlock::castToBool(llvm::Value* Val) {
    return Builder->CreateIntCast( Val,
        Type::getInt1Ty(getGlobalContext()),
        true
    );
}

Value* GenBlock::castToPtr(Value* Val) {
    if (Val->getType() != getValType())
        return Builder->CreateIntToPtr(Val, getValType()->getPointerTo());
    else
        return Val;
}

Value* GenBlock::intVal(Value* From) {
    return Builder->CreateLShr(From, 1);
}

Value* GenBlock::valInt(Value* From) {
    return Builder->CreateAdd(Builder->CreateShl(From, 1), ConstInt(1));
}

ConstantInt* ConstInt(uint64_t val) {
    return ConstantInt::get(
        getGlobalContext(), 
        APInt(sizeof(value)*8, val, /*signed=*/true)
    );
}

void GenBlock::makeCheckedCall(Value* Callee, ArrayRef<Value*> Args) {
    Accu = Builder->CreateCall(Callee, Args);
}

Value* GenBlock::createArrayFromStack(size_t Size) {
    return Builder->CreateGEP(Sp, ConstInt(1));
}

void GenBlock::makeOffsetClosure(int32_t n) {
    Accu = Builder->CreateCall(getFunction("shiftClosure"), ConstInt(n));
}

void GenBlock::makeSetField(size_t n) {
    makeCall1("setField", ConstInt(n));
}

void GenBlock::makeGetField(size_t n) {
    makeCall1("getField", ConstInt(n));
}

Value* GenBlock::makeCall0(std::string FuncName) {
    return Builder->CreateCall(getFunction(FuncName));
}

Value* GenBlock::makeCall1(std::string FuncName, llvm::Value* arg1) {
    return Builder->CreateCall(getFunction(FuncName), arg1);
}

Value* GenBlock::makeCall2(std::string FuncName, llvm::Value* arg1, llvm::Value* arg2) {
    return Builder->CreateCall2(getFunction(FuncName), arg1, arg2);
}

Value* GenBlock::makeCall3(std::string FuncName, llvm::Value* arg1, llvm::Value* arg2, llvm::Value* arg3) {
    return Builder->CreateCall3(getFunction(FuncName), arg1, arg2, arg3);
}

Value* GenBlock::makeCall4(std::string FuncName, llvm::Value* arg1, llvm::Value* arg2, llvm::Value* arg3, llvm::Value* arg4) {
    return Builder->CreateCall4(getFunction(FuncName), arg1, arg2, arg3, arg4);
}

Value* GenBlock::makeCall5(std::string FuncName, llvm::Value* arg1, llvm::Value* arg2, llvm::Value* arg3, llvm::Value* arg4, llvm::Value* arg5) {
    return Builder->CreateCall5(getFunction(FuncName), arg1, arg2, arg3, arg4, arg5);
}

void GenBlock::makeBoolToIntCast() {
    auto AsVal = valInt(Builder->CreateIntCast(Accu, getValType(), true));
    Function->BoolsAsVals[Accu] = AsVal;
}

void GenBlock::debug(Value* DbgVal) {
    if (DbgVal->getType() != getValType()) {
        DbgVal = Builder->CreateIntCast(DbgVal, getValType(), true);
    }
    Builder->CreateCall(getFunction("debug"), DbgVal);
}

void GenBlock::GenCodeForInst(ZInstruction* Inst) {

    Value *TmpVal;

    DEBUG(
        cout << "Generating Instruction "; Inst->Print(true);
        printTab(2);
        printf("Accu pointer before: {%p}\n", Accu);
        if (Accu) Accu->dump();
    )

    //debug(ConstInt(Inst->OrigIdx));

    switch (Inst->OpNum) {

        case CONST0: makeCall1("constInt", ConstInt(Val_int(0))); break;
        case CONST1: makeCall1("constInt", ConstInt(Val_int(1))); break;
        case CONST2: makeCall1("constInt", ConstInt(Val_int(2))); break;
        case CONST3: makeCall1("constInt", ConstInt(Val_int(3))); break;
        case CONSTINT: makeCall1("constInt", ConstInt(Val_int(Inst->Args[0]))); break;

        case PUSHCONST0: push(); makeCall1("constInt", ConstInt(Val_int(0))); break;
        case PUSHCONST1: push(); makeCall1("constInt", ConstInt(Val_int(1))); break;
        case PUSHCONST2: push(); makeCall1("constInt", ConstInt(Val_int(2))); break;
        case PUSHCONST3: push(); makeCall1("constInt", ConstInt(Val_int(3))); break;
        case PUSHCONSTINT: push(); makeCall1("constInt", ConstInt(Val_int(Inst->Args[0]))); break;

        case POP: makeCall1("pop", ConstInt(Inst->Args[0])); break;

        case PUSH: push(); break;
        case PUSH_RETADDR: makeCall0("pushRetAddr"); break; 

        // TODO
        case PUSHTRAP: {
            auto Buf = Builder->CreateCall(getFunction("getNewBuffer")); 
            auto SetJmpFunc = getFunction("__sigsetjmp"); 
            auto JmpBufType = Function->Module->TheModule->getTypeByName("struct.__jmp_buf_tag")->getPointerTo();
            auto JmpBuf = Builder->CreateBitCast(Buf, JmpBufType);
            auto Const0 = ConstantInt::get(getGlobalContext(), APInt(32, 0, true));
            auto SetJmpRes = Builder->CreateCall2(SetJmpFunc, JmpBuf, Const0);
            auto BoolVal = Builder->CreateIntCast(SetJmpRes, Type::getInt1Ty(getGlobalContext()), getValType());
            auto Blocks = addBlock();
            auto TrapBlock = Function->Blocks[Inst->Args[0]];
            Builder->CreateCondBr(BoolVal, TrapBlock->LlvmBlocks.front(), Blocks.second);

            Builder->SetInsertPoint(TrapBlock->LlvmBlock);
            makeCall0("getExceptionValue");
            makeCall0("removeExceptionContext");

            Builder->SetInsertPoint(Blocks.second);

            makeCall0("pushTrap");
            break;

        }

        case POPTRAP:
            makeCall0("removeExceptionContext");
            makeCall1("pop", ConstInt(4));
            break;

        case RAISE: 
            makeCall1("throwException", getAccu());
            Builder->CreateRetVoid();
            break;

        case ACC0:  acc(0); break;
        case ACC1:  acc(1); break;
        case ACC2:  acc(2); break;
        case ACC3:  acc(3); break;
        case ACC4:  acc(4); break;
        case ACC5:  acc(5); break;
        case ACC6:  acc(6); break;
        case ACC7:  acc(7); break;
        case ACC:   acc(Inst->Args[0]); break;

        case PUSHACC0: pushAcc(0); break;
        case PUSHACC1: pushAcc(1); break;
        case PUSHACC2: pushAcc(2); break;
        case PUSHACC3: pushAcc(3); break;
        case PUSHACC4: pushAcc(4); break;
        case PUSHACC5: pushAcc(5); break;
        case PUSHACC6: pushAcc(6); break;
        case PUSHACC7: pushAcc(7); break;
        case PUSHACC:  pushAcc(Inst->Args[0]); break;


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

        case ADDINT: makeCall0("addInt"); break;
        case NEGINT: makeCall0("negInt"); break;
        case SUBINT: makeCall0("subInt"); break;
        case MULINT: makeCall0("mulInt"); break;
        case DIVINT: makeCall0("divInt"); break;
        case MODINT: makeCall0("modInt"); break;
        case OFFSETINT: makeCall1("offsetInt", ConstInt(Inst->Args[0] << 1)); break;
        case ANDINT: makeCall0("andInt"); break;
        case ORINT: makeCall0("orInt"); break;
        case XORINT: makeCall0("xorInt"); break;
        case LSLINT: makeCall0("lslInt"); break;
        case LSRINT: makeCall0("lsrInt"); break;
        case ASRINT: makeCall0("asrInt"); break;
        case BOOLNOT: makeCall0("boolNot"); break;
        case ISINT: makeCall0("isInt"); break;

        case OFFSETREF: makeCall1("offsetRef",ConstInt(Inst->Args[0])); break;

        case GEINT: makeCall0("geInt"); break;
        case LTINT: makeCall0("ltInt"); break;
        case LEINT: makeCall0("leInt"); break;
        case ULTINT: makeCall0("ultInt"); break;
        case UGEINT: makeCall0("ugeInt"); break;
        case GTINT: makeCall0("gtInt"); break;
        case NEQ: makeCall0("cmpNeq"); break;
        case EQ: makeCall0("cmpEq"); break;

        case ASSIGN: makeCall1("assign", ConstInt(Inst->Args[0])); break;

        case PUSHGETGLOBAL: push();
        case GETGLOBAL: makeCall1("getGlobal", ConstInt(Inst->Args[0])); break;
        case SETGLOBAL: makeCall1("setGlobal", ConstInt(Inst->Args[0])); break;

        case PUSHGETGLOBALFIELD: push();
        case GETGLOBALFIELD: 
            makeCall1("getGlobal", ConstInt(Inst->Args[0]));
            makeGetField(Inst->Args[1]);
            break;

        case PUSHATOM0: push();
        case ATOM0: makeCall1("getAtom", ConstInt(0)); break;

        case PUSHATOM: push();
        case ATOM: makeCall1("getAtom", ConstInt(Inst->Args[0])); break;

        case MAKEBLOCK1: makeCall1("makeBlock1", ConstInt(Inst->Args[0])); break;
        case MAKEBLOCK2: makeCall1("makeBlock2", ConstInt(Inst->Args[0])); break;
        case MAKEBLOCK3: makeCall1("makeBlock3", ConstInt(Inst->Args[0])); break;
        case MAKEBLOCK: makeCall2("makeBlock", ConstInt(Inst->Args[1]), ConstInt(Inst->Args[0])); break;
        case MAKEFLOATBLOCK: makeCall1("makeFloatBlock", ConstInt(Inst->Args[0])); break;

        case SETFIELD0: makeSetField(0); break;
        case SETFIELD1: makeSetField(1); break;
        case SETFIELD2: makeSetField(2); break;
        case SETFIELD3: makeSetField(3); break;
        case SETFIELD:  makeSetField(Inst->Args[0]); break;
        case SETFLOATFIELD: makeCall1("storeDoubleField", ConstInt(Inst->Args[0])); break;

        case GETFIELD0: makeGetField(0); break;
        case GETFIELD1: makeGetField(1); break;
        case GETFIELD2: makeGetField(2); break;
        case GETFIELD3: makeGetField(3); break;
        case GETFIELD:  makeGetField(Inst->Args[0]); break;
        case GETFLOATFIELD: makeCall1("getDoubleField", ConstInt(Inst->Args[0])); break;

        case VECTLENGTH: makeCall0("vectLength"); break;
        case GETVECTITEM: makeCall0("getVectItem"); break;
        case SETVECTITEM: makeCall0("setVectItem"); break; 

        case GETSTRINGCHAR: makeCall0("getStringChar"); break; 
        case SETSTRINGCHAR: makeCall0("setStringChar"); break; 

        // Closure related Instructions
        case CLOSUREREC:
            makeCall3("closureRec", ConstInt(Inst->Args[0]), ConstInt(Inst->Args[1]), getPtrToFunc(Inst->ClosureRecFns[0]));
            for (int i = 1; i < Inst->Args[0]; i++)
                makeCall2("setClosureRecNestedClos", ConstInt(i), getPtrToFunc(Inst->ClosureRecFns[i]));
            break;

        case CLOSURE:
            makeCall2("closure", ConstInt(Inst->Args[0]), getPtrToFunc(Inst->Args[1]));
            break;

        case PUSHOFFSETCLOSURE: push();
        case OFFSETCLOSURE: makeCall1("offsetClosure", ConstInt(Inst->Args[0])); break;

        case PUSHOFFSETCLOSUREM2: push();
        case OFFSETCLOSUREM2: makeCall1("offsetClosure", ConstInt(-2)); break;

        case PUSHOFFSETCLOSURE0: push();
        case OFFSETCLOSURE0: makeCall1("offsetClosure", ConstInt(0)); break;

        case PUSHOFFSETCLOSURE2: push();
        case OFFSETCLOSURE2: makeCall1("offsetClosure", ConstInt(2)); break;

        case GRAB: {
            auto BoolVal = Builder->CreateIntCast(makeCall1("checkGrab", ConstInt(Inst->Args[0])),
                                                  Type::getInt1Ty(getGlobalContext()), getValType());
            auto Blocks = addBlock();
            auto BlockReturn = Blocks.second;
            Blocks = addBlock();
            auto BlockContinue = Blocks.second;
            Builder->CreateCondBr(BoolVal, BlockContinue, BlockReturn);

            // Code for the creation of a partial closure
            Builder->SetInsertPoint(BlockReturn);

            auto ResFuncPtr = Builder->CreatePtrToInt(Function->RestartFunction, getValType());
            makeCall1("createRestartClosure", ResFuncPtr);
            Builder->CreateRetVoid();

            // Code for continue
            Builder->SetInsertPoint(BlockContinue);
            makeCall1("substractExtraArgs", ConstInt(Inst->Args[0]));

            break;
        }

        // Object oriented Instructions
        case GETMETHOD: makeCall0("getMethod"); break;
        case GETPUBMET: makeCall1("getPubMet", ConstInt(Inst->Args[0])); break;
        case GETDYNMET: makeCall0("getDynMet"); break;


        // C Calls Instructions
        case C_CALL1: makeCall1("c_call1", ConstInt(Inst->Args[0])); break;
        case C_CALL2: makeCall1("c_call2", ConstInt(Inst->Args[0])); break;
        case C_CALL3: makeCall1("c_call3", ConstInt(Inst->Args[0])); break;
        case C_CALL4: makeCall1("c_call4", ConstInt(Inst->Args[0])); break;
        case C_CALL5: makeCall1("c_call5", ConstInt(Inst->Args[0])); break;
        case C_CALLN: makeCall2("c_calln", ConstInt(Inst->Args[0]), ConstInt(Inst->Args[1])); break;

        case APPLY1: {
            Builder->CreateCall(makeCall0("apply1"))->setCallingConv(CallingConv::Fast);
            break;
        }
        case APPLY2: {
            Builder->CreateCall(makeCall0("apply2"))->setCallingConv(CallingConv::Fast);
            break;
        }
        case APPLY3: {
            Builder->CreateCall(makeCall0("apply3"))->setCallingConv(CallingConv::Fast);
            break;
        }
        case APPLY: {
            Builder->CreateCall(makeCall1("apply", ConstInt(Inst->Args[0])))->setCallingConv(CallingConv::Fast);
            break;
        }

        case APPTERM1: {
            auto Func = makeCall1("appterm1", ConstInt(Inst->Args[0])); 
            auto Call = Builder->CreateCall(Func);
            Call->setCallingConv(CallingConv::Fast);
            Call->setTailCall();
            Builder->CreateRetVoid();
            break;
        }
        case APPTERM2: {
            auto Func = makeCall1("appterm2", ConstInt(Inst->Args[0])); 
            auto Call = Builder->CreateCall(Func);
            Call->setCallingConv(CallingConv::Fast);
            Call->setTailCall();
            Builder->CreateRetVoid();
            break;
        }
        case APPTERM3: {
            auto Func = makeCall1("appterm3", ConstInt(Inst->Args[0])); 
            auto Call = Builder->CreateCall(Func);
            Call->setCallingConv(CallingConv::Fast);
            Call->setTailCall();
            Builder->CreateRetVoid();
            break;
        }
        case APPTERM: {
            auto Func = makeCall2("appterm", ConstInt(Inst->Args[0]), ConstInt(Inst->Args[1])); 
            auto Call = Builder->CreateCall(Func);
            Call->setCallingConv(CallingConv::Fast);
            Call->setTailCall();
            Builder->CreateRetVoid();
            break;
        }

        // Fall through return
        // TODO
        case STOP:
            Builder->CreateRetVoid();
            break;
        case RETURN: {
            auto Func = makeCall1("handleReturn", ConstInt(Inst->Args[0]));
            auto IntPtr = Builder->CreatePtrToInt(Func, getValType());
            auto BoolVal = Builder->CreateICmpNE(IntPtr, ConstInt(0), "BranchRet");
            auto Blocks = addBlock();
            auto BlockInvoke = Blocks.second;
            Blocks = addBlock();
            auto BlockReturn = Blocks.second;
            Builder->CreateCondBr(BoolVal, BlockInvoke, BlockReturn);

            Builder->SetInsertPoint(BlockInvoke);
            auto Call = Builder->CreateCall(Func);
            Call->setCallingConv(CallingConv::Fast);
            Call->setTailCall();
            Builder->CreateRetVoid();

            Builder->SetInsertPoint(BlockReturn);
            Builder->CreateRetVoid();

            break;
        }

        case BRANCH:{
            BasicBlock* LBrBlock = BrBlock->LlvmBlock;
            Builder->CreateBr(LBrBlock);
            break;
        }
        case BRANCHIF: {
            auto BoolVal = Builder->CreateICmpNE(getAccu(), ConstInt(Val_false), "BranchCmp");
            Builder->CreateCondBr(BoolVal, BrBlock->LlvmBlocks.front(), NoBrBlock->LlvmBlocks.front());
            break;
        }
        case BRANCHIFNOT: {
            auto BoolVal = Builder->CreateICmpNE(getAccu(), ConstInt(Val_false), "BranchCmp");
            Builder->CreateCondBr(BoolVal, NoBrBlock->LlvmBlocks.front(), BrBlock->LlvmBlocks.front());
            break;
        }
        case SWITCH: {
            auto SwitchVal = Builder->CreateCall2(getFunction("getSwitchOffset"),
                                                  ConstInt(Inst->Args[0]),
                                                  getAccu());
            auto DefaultBlock = Function->Blocks[Inst->SwitchEntries[0]];
            this->setNext(DefaultBlock, false);
            auto Switch = Builder->CreateSwitch(SwitchVal, DefaultBlock->LlvmBlock);
            for (size_t i = 0; i < Inst->SwitchEntries.size(); i++)
                Switch->addCase(ConstInt(i), Function->Blocks[Inst->SwitchEntries[i]]->LlvmBlock);

            break;
        }


        case BEQ: TmpVal = Builder->CreateICmpEQ(ConstInt(Val_int(Inst->Args[0])), getAccu()); goto makebr;
        case BNEQ: TmpVal = Builder->CreateICmpNE(ConstInt(Val_int(Inst->Args[0])), getAccu()); goto makebr;
        case BLTINT: TmpVal = Builder->CreateICmpSLT(ConstInt(Val_int(Inst->Args[0])), getAccu()); goto makebr;
        case BLEINT: TmpVal = Builder->CreateICmpSLE(ConstInt(Val_int(Inst->Args[0])), getAccu()); goto makebr;
        case BGTINT: TmpVal = Builder->CreateICmpSGT(ConstInt(Val_int(Inst->Args[0])), getAccu()); goto makebr;
        case BGEINT: TmpVal = Builder->CreateICmpSGE(ConstInt(Val_int(Inst->Args[0])), getAccu()); goto makebr;
        case BULTINT: TmpVal = Builder->CreateICmpULT(ConstInt(Val_int(Inst->Args[0])), getAccu()); goto makebr;
        case BUGEINT: TmpVal = Builder->CreateICmpUGE(ConstInt(Val_int(Inst->Args[0])), getAccu()); goto makebr;

        makebr: {
            BasicBlock* LBrBlock = BrBlock->LlvmBlocks.front();
            BasicBlock* LNoBrBlock = NoBrBlock->LlvmBlocks.front();
            Builder->CreateCondBr(TmpVal, LBrBlock, LNoBrBlock);
            break;
        }

        case CHECK_SIGNALS:
            // TODO: Understand and implement ocaml's event system
            break;

        default:
            printTab(2);
            cout << "INSTRUCTION NOT HANDLED" << endl;
            exit(42);

    }

    //DEBUG(cout << "Stack DIFF : " << StackSize - Stack.size() << endl;)
    DEBUG(cout << "Instruction generated ===  \n";)
}

void GenBlock::Print() {
    PrintAdjBlocks();
    for (ZInstruction* Inst : Instructions) {
        printTab(2);
        Inst->Print(true);
    }
}

void GenBlock::PrintAdjBlocks() {
    printTab(2);
    cout << "Predecessors : ";

    for (auto Block : PreviousBlocks)
        cout << Block->Id << " ";

    cout << " | Successors : ";

    for (auto Block : NextBlocks)
        cout << Block->Id << " ";

    cout << "\n";
}

Function* GenBlock::getFunction(string Name) {
  return Function->Module->getFunction(Name);
}


llvm::Value* GenBlock::getPtrToFunc(int32_t FnId) {
    auto DestGenFunc = Function->Module->Functions[FnId];

    if (DestGenFunc->LlvmFunc == NULL)
        DestGenFunc->CodeGen();

    Builder->SetInsertPoint(LlvmBlock);
    auto ClosureFunc = DestGenFunc->LlvmFunc;
    
    return Builder->CreatePtrToInt(ClosureFunc, getValType());
}
