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

// ================ GenBlock Implementation ================== //

GenBlock::GenBlock(int Id, GenFunction* Function) {
    // Basic init
    this->Id = Id;
    this->Function = Function;
    this->Builder = Function->Module->Builder;
    this->Accu = nullptr;
    this->StackOffset = 0;

    addBlock();
}

pair<BasicBlock*, BasicBlock*> GenBlock::addBlock() {
    auto OldBlock = LlvmBlock;
    LlvmBlock = BasicBlock::Create(getGlobalContext(), name());
    LlvmBlocks.push_back(LlvmBlock);
    return make_pair(OldBlock, LlvmBlock);
}

void GenBlock::setNext(GenBlock* Block, bool IsBrBlock) {

    /*
    for (auto Bl : NextBlocks)
        if (Bl == Block) return;
        */

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
    return _getStackAt(n, nullptr)->Val;
}

StackValue* GenBlock::_getStackAt(size_t n, GenBlock* StartBlock) {

    std::list<GenBlock*> PrBlocks = PreviousBlocks;

    if (StartBlock == nullptr) StartBlock = this;
    else PrBlocks.remove(StartBlock);

    StackValue* Ret = nullptr;

    //DEBUG(cout << "STACK ACCESS IN BLOCK " << Id << " WITH N = " << n << " AND STACK SIZE = " << Stack.size() << endl;)
    if (n >= Stack.size()) {
        auto NbPrevBlocks = PrBlocks.size();
        int PrevStackPos = n - Stack.size() + StackOffset;

        auto It = PrevStackCache.find(PrevStackPos);
        if (It != PrevStackCache.end()) {
            Ret = It->second;
        } else {

            if (NbPrevBlocks == 0) {
                dumpStack();
                Function->LlvmFunc->dump();
                cout << "BAD STACK ACCESS IN BLOCK " << Id << " WITH N = " << n << " AND STACK SIZE = " << Stack.size() << endl;
                throw std::logic_error("Bad stack access !");
            } else if (NbPrevBlocks == 1) {
                Ret = PrBlocks.front()->_getStackAt(PrevStackPos, StartBlock);
            } else {
                auto B = Builder->GetInsertBlock();
                Builder->SetInsertPoint(LlvmBlocks.front());
                PHINode* PHI = Builder->CreatePHI(getValType(), NbPrevBlocks, "phi");
                PHINodes.push_back(make_pair(PHI, PrevStackPos));
                Builder->SetInsertPoint(B);
                Ret = new StackValue(PHI);
            }
            PrevStackCache[PrevStackPos] = Ret;
        }

    } else {
        Ret = Stack[n];
    }

    Ret = getMutatedValue(Ret);

    return Ret;
}

StackValue* GenBlock::getMutatedValue(StackValue* Val) {
    auto It = MutatedVals.find(Val);
    if (It != MutatedVals.end())
        return getMutatedValue(It->second);

    return Val;
}

void GenBlock::handlePHINodes() {
    //DEBUG(cout << "IN HANDLEPHI NODES FOR BLOCK " << this->Id << "\n";)
    for (auto Pair : PHINodes)  {
        auto PhiNode = Pair.first;
        for (auto Block : PreviousBlocks) {
            if (Pair.second == -1) {
                auto Val = Block->getAccu();
                if (Val->getType() != getValType()) 
                    Val = Function->BoolsAsVals[Val];
                PhiNode->addIncoming(Val, Block->LlvmBlocks.front());
            } else {
                // We take the corresponding value in the stack of the previous block
                // And link it in the phi to the last llvm block of the previous block
                // Because we are necessarily coming from there if we come from this block
                auto Val = Block->getStackAt(Pair.second);
                if (Val->getType() != getValType()) 
                    Val = Function->BoolsAsVals[Val];
                PhiNode->addIncoming(Val, Block->LlvmBlocks.back());
            }
        }
    }


    // Remove all phi nodes we handled
    while (PHINodes.size()) PHINodes.pop_front();

    auto& IList = LlvmBlocks.front()->getInstList();
    deque<decltype(IList.begin())> Phis;
    for (auto It=IList.begin(); It != IList.end(); ++It) {
        if (isa<PHINode>(*It)) {
            Phis.push_back(It);
        }
    }
    for (auto Phi : Phis) IList.remove(Phi);
    for (auto Phi : Phis) IList.push_front(Phi);
}

void GenBlock::dumpStack() {
    cout << "============== Stack For Block " << name() << " ==================" << endl;
    //for (auto Val : Stack) printf("%p\n", Val);
    for (auto Val : Stack) Val->Val->dump();
    cout << "==========================================================" << endl;
}

Value* GenBlock::stackPop() {
    auto Val = getStackAt(0);
    if (Stack.size() == 0) {
        StackOffset++;
    } else {
        Stack.pop_front();
    }
    return Val;
}

Value* GenBlock::getAccu(bool CreatePhi) {
    if (Accu == nullptr) {
        if (PreviousBlocks.size() > 1 && CreatePhi) {
            Builder->SetInsertPoint(this->LlvmBlock);
            auto PHI = Builder->CreatePHI(getValType(), PreviousBlocks.size());
            // TODO: This is hackish
            // Using -1 to distinguish this Phi node from the other and treat it right in handle phi nodes
            PHINodes.push_back(make_pair(PHI, -1));
            Accu = PHI;
        } else if (PreviousBlocks.size() == 1) {
            Accu = PreviousBlocks.front()->Accu;
        }
    } 
    return Accu;
}

void GenBlock::push(bool CreatePhi) { 
    this->Stack.push_front(new StackValue(getAccu(CreatePhi)));
}

void GenBlock::acc(int n) { 
    this->Accu = this->getStackAt(n); 
}

void GenBlock::envAcc(int n) { 
    auto Env = Builder->CreateCall(getFunction("getEnv"), "Env");
    Accu = Builder->CreateCall2(getFunction("getField"), 
                               Env,
                               ConstInt(n), "Field");
}

void GenBlock::pushAcc(int n) { push(); acc(n); }

BasicBlock* GenBlock::CodeGen() {
    // Create the block and generate instruction's code
    Builder->SetInsertPoint(LlvmBlock);

    DEBUG(debug(ConstInt(this->Id));)

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
    /*
    if (UnwindBlocks.size() > 0) {
        auto Blocks = addBlock();
        Accu = Builder->CreateInvoke(Callee, Blocks.second, UnwindBlocks.front(), Args);
        Builder->SetInsertPoint(Blocks.second);
    } else {
    */
        Accu = Builder->CreateCall(Callee, Args);
    //}
}

void GenBlock::makeApply(size_t n, bool isTerminal) {

    auto Ci = Function->ClosuresFunctions.find(getAccu());
    vector<Value*> ArgsV;


    if (Ci != Function->ClosuresFunctions.end() 
            && Ci->second.LlvmFunc->arg_size() == n) {

        auto FuncToCall = Ci->second.LlvmFunc;
        bool SameFunc = FuncToCall != this->Function->LlvmFunc;
        Value* TmpEnv;

        if (SameFunc) {
            TmpEnv = Builder->CreateCall(getFunction("getEnv"), "SavedEnv");
            Builder->CreateCall(getFunction("setEnv"), getAccu());
        }

        for (size_t i = 1; i <= n; i++)
            ArgsV.push_back(getStackAt(n-i));

        auto Call = Builder->CreateCall(FuncToCall, ArgsV);
        Call->setCallingConv(CallingConv::Fast);
        if (isTerminal) Call->setTailCall();
        Accu = Call;

        if (SameFunc)
            Builder->CreateCall(getFunction("setEnv"), TmpEnv);

    } else {

        auto Array = Builder->CreateAlloca(ArrayType::get(getValType(), n));
        Array->setName("ApplyArray");
        for (size_t i = 1; i <= n; i++) {
            vector<Value*> GEPlist; 
            GEPlist.push_back(ConstInt(0));
            GEPlist.push_back(ConstInt(i-1));
            stringstream ss; ss << "ApplyArrayEl" << i;
            auto Ptr = Builder->CreateGEP(Array, GEPlist, ss.str());
            auto Val = getStackAt(n-i);
            Builder->CreateStore(Val, Ptr);
        }

        auto ArrayPtr = Builder->CreatePointerCast(Array, getValType()->getPointerTo(), "ApplyArrayPtr");
        getAccu()->setName("ApplyClosure");
        ArgsV.push_back(getAccu());
        ArgsV.push_back(ConstInt(n));
        ArgsV.push_back(ArrayPtr);
        makeCheckedCall(getFunction("apply"), ArgsV);
        getAccu()->setName("ApplyRes");

    }

    for (size_t i = 0; i < n; i++)
        stackPop();
}

void GenBlock::makePrimCall(size_t n, int32_t NumPrim) {
    vector<Value*> Args;
    stringstream ss;
    ss << "primCall";

    Args.push_back(ConstInt(NumPrim));
    if (n < 6) {
        Args.push_back(getAccu());
        for (size_t i = 1; i < n; i++)
            Args.push_back(stackPop());
        ss << n;
    } else {
        Args.push_back(ConstInt(n));
        ss << "n";
    }
    makeCheckedCall(getFunction(ss.str()), Args);
    getAccu()->setName("PrimCallRes");
}

void GenBlock::makeClosure(int32_t NbFields, int32_t FnId) {
    auto MakeClos = getFunction("makeClosure");
    auto ClosSetVar = getFunction("closureSetVar");

    // Create pointer to dest function
    auto DestGenFunc = Function->Module->Functions[FnId];
    if (DestGenFunc->LlvmFunc == NULL) {
        DestGenFunc->CodeGen();
    }
    Builder->SetInsertPoint(LlvmBlock);
    auto ClosureFunc = DestGenFunc->ApplierFunction;
    auto CastPtr = Builder->CreatePtrToInt(ClosureFunc, getValType());


    int FuncNbArgs = DestGenFunc->LlvmFunc->arg_size();

    auto Closure = Builder->CreateCall3(MakeClos, 
                                        ConstInt(NbFields), 
                                        CastPtr, 
                                        ConstInt(FuncNbArgs));

    // If there are fields, push the Accu on the stack
    if (NbFields > 0) push();
    // Set Closure fields
    for (int i = 0; i < NbFields; i++) {
        auto FieldVal = stackPop();
        Builder->CreateCall3(ClosSetVar, Closure, ConstInt(i), FieldVal);
    }

    Accu = Closure;
    Accu->setName("Closure");

    ClosureInfo CI = {DestGenFunc->LlvmFunc, NbFields == 0 ? true : false};
    this->Function->ClosuresFunctions[Accu] = CI;
}

void GenBlock::makeClosureRec(int32_t NbFuncs, int32_t NbFields, int32_t* FnIds) {

    auto MakeClos = getFunction("makeClosure");
    auto ClosSetNest = getFunction("closureSetNestedClos");

    vector<GenFunction*> DestGenFuncs;
    vector<llvm::Value*> CastPtrs;
    int32_t AllNbArgs = 0;


    // Create pointer to all dest functions
    for (int i = 0; i < NbFuncs; i++) {
        auto DestGenFunc = Function->Module->Functions[FnIds[i]];
        if (DestGenFunc->LlvmFunc == NULL) {
            DestGenFunc->CodeGen();
        }
        Builder->SetInsertPoint(LlvmBlock);
        DestGenFuncs.push_back(DestGenFunc);

        auto ClosureFunc = DestGenFunc->ApplierFunction;
        CastPtrs.push_back(Builder->CreatePtrToInt(ClosureFunc, getValType()));
        // Accumulate the number of all arguments 
        // except thoses of the main function
        if (i > 0)
            AllNbArgs += DestGenFunc->LlvmFunc->arg_size() + 1;
    }

    /* ------------------------------------------------------------------------------------------------------------
     * |MainClCodePtr|Header1|NestedClos1|...|Field1|...|NestClosArgsn|NestClosNbArgsn|...|MainClArgs|MainClNbArgs|
     * ------------------------------------------------------------------------------------------------------------
     * Example with 3 functions, fn1(a), fn2(a, b), fn3(a,b)
     *
     *                     <--------fn3 closure size : 8 ------------------------->                                      
     * --------------------------------------------------------------------------------------------------------------------------------------------
     * |fn1|HdFn2|fn2|hdfn3|fn3|Fld1|Fld2|Fld3|fn3a1|fn3a2|fn3nbremargs|fn3nbtargs|fn2a1|fn2a2|fn2nbremargs|fn2nbargs|fn1a1|fn1nbremargs|fn1nbargs|
     * --------------------------------------------------------------------------------------------------------------------------------------------
     *           <------------------------------fn2 closure size : 14 -----------------------------------------------> 
     * <----------------------------------------Main closure size : 19 --------------------------------------------------------------------------->
     *  fn1|hdfn2|fn2|fn2a1|fn2nbargs|fn2nbtargs|fn1a1|fn1nbargs|fn1nbtargs|
     */
    
    // Get the main closure function number of args
    int FuncNbArgs = DestGenFuncs[0]->LlvmFunc->arg_size();

    // Calculate the number of fields necessary to store everything
    // except : - The main code ptr
    //          - The main nb args
    //          - The main args slots
    //  That are given separately to the makeClosure function
    size_t NbReqFields = (NbFuncs-1)*4 + NbFields + AllNbArgs;
    auto Closure = Builder->CreateCall3(MakeClos, 
                                        ConstInt(NbReqFields),
                                        CastPtrs[0], 
                                        ConstInt(FuncNbArgs));


    // If there are fields, push the Accu on the stack
    if (NbFields > 0) push();

    // Set Closure fields
    auto FieldOffset = 1 + (2 * (NbFuncs-1));
    for (int i = 0; i < NbFields; i++) {
        auto FieldVal = stackPop();
        Builder->CreateCall3(getFunction("setField"), Closure, ConstInt(i+FieldOffset), FieldVal);
    }

    // Set Closure's nested closures
    // Offset where the current offset closure starts relatively to the first
    int32_t NestClosOfs = 2;
    // Nested closure relative size
    size_t NClosSize = (NbReqFields + 3 + FuncNbArgs) - NestClosOfs - FuncNbArgs - 2;

    Accu = Closure; push();

    for (int i = 1; i < NbFuncs; i++) {
        auto ArgSize = DestGenFuncs[i]->LlvmFunc->arg_size();

        // Usage : 
        // ClosureSetNext(MainClosure, NestedClosureOffset,
        //                NestedClosureSize, NestedFunctionPtr,
        //                NestedClosureNbArgs)
        Accu = Builder->CreateCall5(ClosSetNest, Closure,
                                    ConstInt(NestClosOfs),
                                    ConstInt(NClosSize),
                                    CastPtrs[i],
                                    ConstInt(ArgSize));
        push();
        NestClosOfs += 2;
        NClosSize -= 2 + ArgSize;
    }

    Accu = Closure;
    Accu->setName("Closure");

    ClosureInfo CI = {DestGenFuncs[0]->LlvmFunc, NbFields == 0 ? true : false};
    this->Function->ClosuresFunctions[Accu] = CI;
}

void GenBlock::makeOffsetClosure(int32_t n) {
    Accu = Builder->CreateCall(getFunction("shiftClosure"), ConstInt(n));
}

void GenBlock::makeSetField(size_t n) {
    Builder->CreateCall3(getFunction("setField"), getAccu(), ConstInt(n), stackPop());
    Accu = ConstInt(Val_unit);
}

void GenBlock::makeGetField(size_t n) {
    Accu = Builder->CreateCall2(getFunction("getField"), getAccu(), ConstInt(n));
}

void GenBlock::makeBoolToIntCast() {
    auto AsVal = Builder->CreateIntCast(Accu, getValType(), true);
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

    int StackSize = Stack.size();

    switch (Inst->OpNum) {

        case CONST0: Accu = ConstInt(Val_int(0)); break;
        case CONST1: Accu = ConstInt(Val_int(1)); break;
        case CONST2: Accu = ConstInt(Val_int(2)); break;
        case CONST3: Accu = ConstInt(Val_int(3)); break;
        case CONSTINT:
            Accu = ConstInt(Val_int(Inst->Args[0]));
            break;

        case PUSHCONST0: push(); Accu = ConstInt(Val_int(0)); break;
        case PUSHCONST1: push(); Accu = ConstInt(Val_int(1)); break;
        case PUSHCONST2: push(); Accu = ConstInt(Val_int(2)); break;
        case PUSHCONST3: push(); Accu = ConstInt(Val_int(3)); break;
        case PUSHCONSTINT: 
            push(); 
            Accu = ConstInt(Val_int(Inst->Args[0])); 
            break;

        case POP: 
            for (int i = 0; i < Inst->Args[0]; i++) stackPop(); 
            break;
        case PUSH: push(); break;
        case PUSH_RETADDR: {
            auto AccuSv = Accu;
            Accu = ConstInt(Val_unit);
            push(); 
            Accu = Builder->CreateCall(getFunction("getEnv"));
            push(); 
            Accu = ConstInt(Val_unit);
            push();
            Accu = AccuSv;            
            break;
        }

        case PUSHTRAP: {
            auto Buf = Builder->CreateCall(getFunction("getNewBuffer"));
            auto SetJmpFunc = getFunction("_setjmp");
            auto JmpBufType = Function->Module->TheModule->getTypeByName("struct.__jmp_buf_tag")->getPointerTo();
            auto JmpBuf = Builder->CreateBitCast(Buf, JmpBufType);
            auto SetJmpRes = Builder->CreateCall(SetJmpFunc, JmpBuf);
            auto BoolVal = Builder->CreateIntCast(SetJmpRes, Type::getInt1Ty(getGlobalContext()), getValType());
            auto Blocks = addBlock();
            auto TrapBlock = Function->Blocks[Inst->Args[0]];
            Builder->CreateCondBr(BoolVal, TrapBlock->LlvmBlocks.front(), Blocks.second);

            Builder->SetInsertPoint(TrapBlock->LlvmBlock);
            TrapBlock->Accu = Builder->CreateCall(getFunction("getExceptionValue"));
            Builder->CreateCall(getFunction("removeExceptionContext"));

            Builder->SetInsertPoint(Blocks.second);
            auto AccuSv = Accu;
            Accu = ConstInt(Val_unit);
            for (int i=0;i<4;i++) push(false);
            Accu = AccuSv;
            break;

        }
        case POPTRAP: {
            //UnwindBlocks.pop_front();
            Builder->CreateCall(getFunction("removeExceptionContext"));
            stackPop();stackPop();stackPop();stackPop();
            break;
        }
        case RAISE: {
            Builder->CreateCall(getFunction("throwException"), getAccu());
            Builder->CreateRet(getAccu());
            break;
        }


        case ACC0: acc(0); break;
        case ACC1: acc(1); break;
        case ACC2: acc(2); break;
        case ACC3: acc(3); break;
        case ACC4: acc(4); break;
        case ACC5: acc(5); break;
        case ACC6: acc(6); break;
        case ACC7: acc(7); break;
        case ACC: acc(Inst->Args[0]); break;

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

        case ADDINT:
            TmpVal = castToInt(stackPop());
            Accu = Builder->CreateAdd(castToInt(getAccu()), Builder->CreateSub(TmpVal, ConstInt(1)));
            getAccu()->setName("AddInt");
            break;
        case NEGINT:
            Accu = Builder->CreateSub(ConstInt(2), getAccu());
            getAccu()->setName("NegInt");
            break;
        case SUBINT:
            TmpVal = stackPop();
            Accu = Builder->CreateAdd(Builder->CreateSub(getAccu(), TmpVal), ConstInt(1));
            getAccu()->setName("SubInt");
            break;
        case MULINT:
            TmpVal = stackPop();
            TmpVal = Builder->CreateMul(intVal(getAccu()), intVal(TmpVal));
            Accu = valInt(TmpVal);
            getAccu()->setName("Mulint");
            break;
        case DIVINT:
            TmpVal = stackPop();
            TmpVal = Builder->CreateSDiv(intVal(getAccu()), intVal(TmpVal));
            Accu = valInt(TmpVal);
            getAccu()->setName("Divint");
            break;
        case MODINT:
            TmpVal = stackPop();
            TmpVal = Builder->CreateSRem(intVal(getAccu()), intVal(TmpVal));
            Accu = valInt(TmpVal);
            getAccu()->setName("ModInt");
            break;
        case OFFSETINT:
            Accu = Builder->CreateAdd(getAccu(), ConstInt(Inst->Args[0] << 1));
            getAccu()->setName("Offsetint");
            break;
        case ANDINT: // Untested
            Accu  = Builder->CreateAnd(getAccu(), stackPop());
            break;
        case ORINT: // Untested
            Accu  = Builder->CreateOr(getAccu(), stackPop());
            break;
        case XORINT: // Untested
            Accu  = Builder->CreateOr(Builder->CreateXor(getAccu(), stackPop()), 1);
            break;
        case LSLINT: // Untested
            Accu = Builder->CreateAdd(
                Builder->CreateShl(Builder->CreateSub(getAccu(), ConstInt(1)), 
                                    intVal(stackPop())),
                ConstInt(1)
            );
            break;
        case LSRINT: // Untested
            Accu = Builder->CreateOr(
                Builder->CreateLShr(Builder->CreateSub(getAccu(), ConstInt(1)), intVal(stackPop())),
                ConstInt(1)
            );
            break;
        case ASRINT: // Untested
            Accu = Builder->CreateOr(
                Builder->CreateAShr(Builder->CreateSub(getAccu(), ConstInt(1)), intVal(stackPop())),
                ConstInt(1)
            );
            break;

        case BOOLNOT: // Untested
            Accu = Builder->CreateNot(getAccu());
            break;

        case ISINT: // Untested
            Accu = Builder->CreateAnd(getAccu(), ConstInt(1));
            break;

        case OFFSETREF:
            Builder->CreateCall2(getFunction("offsetRef"), Accu, ConstInt(Inst->Args[0]));
            Accu = ConstInt(Val_unit);
            break;


        case GEINT:
            TmpVal = stackPop();
            Accu = Builder->CreateICmpSGE(getAccu(), TmpVal);
            makeBoolToIntCast();
            break;
        case LTINT:
            TmpVal = stackPop();
            Accu = Builder->CreateICmpSLT(getAccu(), TmpVal);
            makeBoolToIntCast();
            break;
        case LEINT:
            TmpVal = stackPop();
            Accu = Builder->CreateICmpSLE(getAccu(), TmpVal);
            makeBoolToIntCast();
            break;
        case ULTINT:
            TmpVal = stackPop();
            Accu = Builder->CreateICmpULT(getAccu(), TmpVal);
            makeBoolToIntCast();
            break;
        case UGEINT:
            TmpVal = stackPop();
            Accu = Builder->CreateICmpUGE(getAccu(), TmpVal);
            makeBoolToIntCast();
            break;            
        case GTINT:
            TmpVal = stackPop();
            Accu = Builder->CreateICmpSGT(getAccu(), TmpVal);
            makeBoolToIntCast();
            break;
        case NEQ:
            Accu = Builder->CreateICmpNE(getAccu(), stackPop());
            makeBoolToIntCast();
            break;
        case EQ: {
            Accu =  Builder->CreateICmpEQ(getAccu(), stackPop());
            makeBoolToIntCast();
            break;
        }

        case ASSIGN:
            MutatedVals[_getStackAt(Inst->Args[0])] = new StackValue(getAccu());
            break;

        case PUSHGETGLOBAL: push();
        case GETGLOBAL:
            Accu = Builder->CreateCall(getFunction("getGlobal"), ConstInt(Inst->Args[0]), "Global");
            break;

        case SETGLOBAL:
            Builder->CreateCall2(getFunction("setGlobal"), ConstInt(Inst->Args[0]), getAccu());
            break;

        case PUSHGETGLOBALFIELD: push();
        case GETGLOBALFIELD: // Untested
            TmpVal = Builder->CreateCall(getFunction("getGlobal"), ConstInt(Inst->Args[0]), "Global");
            Accu = Builder->CreateCall2(getFunction("getField"), TmpVal, ConstInt(Inst->Args[1]), "Field");
            break;

        case PUSHATOM0: push();
        case ATOM0:
            Accu = Builder->CreateCall(getFunction("getAtom"), ConstInt(0), "Atom");
            break;

        case PUSHATOM: push();
        case ATOM:
            Accu = Builder->CreateCall(getFunction("getAtom"),
                                       ConstInt(Inst->Args[0]), "Atom");
            break;


        case MAKEBLOCK1:
            Accu = Builder->CreateCall2(getFunction("makeBlock1"), 
                                        ConstInt(Inst->Args[0]), 
                                        getAccu(), "Block");
            break;
        case MAKEBLOCK2:
            Accu = Builder->CreateCall3(getFunction("makeBlock2"), 
                                        ConstInt(Inst->Args[0]), 
                                        getAccu(), 
                                        getStackAt(0), "Block");
            stackPop();
            break;
        case MAKEBLOCK3:
            Accu = Builder->CreateCall4(getFunction("makeBlock3"), 
                                        ConstInt(Inst->Args[0]), 
                                        getAccu(), 
                                        getStackAt(0), 
                                        getStackAt(1), "Block");
            stackPop(); stackPop();
            break;

        case MAKEBLOCK: {
            size_t BSize = Inst->Args[0];
            auto Block = Builder->CreateCall2(getFunction("makeBlock"), 
                                              ConstInt(Inst->Args[1]),
                                              ConstInt(BSize));
            Builder->CreateCall3(getFunction("setField"), Block, ConstInt(0) , getAccu());

            for (size_t i = 1; i < BSize; i++) {
                Builder->CreateCall3(getFunction("setField"), Block, ConstInt(i), stackPop());
            }
            Accu = Block;
            break;
        }

        case MAKEFLOATBLOCK: { // Untested
            size_t BSize = Inst->Args[0];
            auto FloatBlock = Builder->CreateCall(getFunction("makeFloatBlock"), 
                                                  ConstInt(BSize));
            Builder->CreateCall3(getFunction("storeDoubleField"), FloatBlock, ConstInt(0), Accu);
            for (size_t i = 1; i < BSize; i++) {
                Builder->CreateCall3(getFunction("storeDoubleField"), FloatBlock, ConstInt(i), stackPop());
            }
        }

        case SETFIELD0: makeSetField(0); break;
        case SETFIELD1: makeSetField(1); break;
        case SETFIELD2: makeSetField(2); break;
        case SETFIELD3: makeSetField(3); break;
        case SETFIELD:  makeSetField(Inst->Args[0]); break;
        case SETFLOATFIELD: { // Untested
            Builder->CreateCall3(getFunction("storeDoubleField"), 
                                Accu, 
                                ConstInt(Inst->Args[0]), 
                                stackPop());
            Accu = ConstInt(Val_unit);
            break;
        }

        case GETFIELD0: makeGetField(0); break;
        case GETFIELD1: makeGetField(1); break;
        case GETFIELD2: makeGetField(2); break;
        case GETFIELD3: makeGetField(3); break;
        case GETFIELD:  makeGetField(Inst->Args[0]); break;
        case GETFLOATFIELD:
            Accu = Builder->CreateCall2(getFunction("getDoubleField"), Accu, ConstInt(Inst->Args[0]));
            break;

        case VECTLENGTH:
            Accu = Builder->CreateCall(getFunction("vectLength"), Accu);
            break;
        case GETVECTITEM:
            Accu = Builder->CreateCall2(getFunction("getVectItem"), Accu, stackPop());
            break;
        case SETVECTITEM: {
            auto Idx = stackPop();
            auto NewVal = stackPop();
            Builder->CreateCall3(getFunction("setVectItem"), Accu, Idx, NewVal);
            break;
        }

        case GETSTRINGCHAR: { // Untested
            auto CharIdx = stackPop();
            Accu = Builder->CreateCall2(getFunction("getStringChar"), Accu, CharIdx);
            break;
        }

        case SETSTRINGCHAR: { // Untested
            auto CharIdx = stackPop();
            auto CharVal = stackPop();
            Builder->CreateCall3(getFunction("setStringChar"), Accu, CharIdx, CharVal);
            Accu = ConstInt(Val_unit);
            break;
        }

        // Closure related Instructions
        case CLOSUREREC: {
            // Simple recursive function with no trampoline
            if (Inst->Args[0] == 1) {
                makeClosure(Inst->Args[1], Inst->ClosureRecFns[0]);
                push();
            } else {
                makeClosureRec(Inst->Args[0], Inst->Args[1], Inst->ClosureRecFns);
                // TODO: Handle mutually recursive functions and rec fun with environnements
            }
            break;
        }

        case CLOSURE:
            makeClosure(Inst->Args[0], Inst->Args[1]);
            break;

        case PUSHOFFSETCLOSURE: push();
        case OFFSETCLOSURE: makeOffsetClosure(Inst->Args[0]); break;

        case PUSHOFFSETCLOSUREM2: push();
        case OFFSETCLOSUREM2: makeOffsetClosure(-2); break;

        case PUSHOFFSETCLOSURE0:
            push();
        case OFFSETCLOSURE0: {
            Accu = Builder->CreateCall(getFunction("getEnv"));
            ClosureInfo CI = {Function->LlvmFunc, true};
            this->Function->ClosuresFunctions[getAccu()] = CI;
            break;
        }

        case PUSHOFFSETCLOSURE2: push();
        case OFFSETCLOSURE2: makeOffsetClosure(2); break;


        // Object oriented Instructions
        case GETMETHOD: // Untested
            Accu = Builder->CreateCall2(getFunction("getMethod"),
                                        getStackAt(0),
                                        getAccu());
            break;
        case GETPUBMET: // Untested
            push();
            Accu = ConstInt(Inst->Args[0]);
            // FALLTHROUGH
        case GETDYNMET: // Untested
            Accu = Builder->CreateCall2(getFunction("getDynMethod"), getStackAt(0), getAccu());
            break;


        // C Calls Instructions
        case C_CALL1: makePrimCall(1, Inst->Args[0]); break;
        case C_CALL2: makePrimCall(2, Inst->Args[0]); break;
        case C_CALL3: makePrimCall(3, Inst->Args[0]); break;
        case C_CALL4: makePrimCall(4, Inst->Args[0]); break;
        case C_CALL5: makePrimCall(5, Inst->Args[0]); break;


        // Apply Instructions
        case APPLY1: makeApply(1); break;
        case APPLY2: makeApply(2); break;
        case APPLY3: makeApply(3); break;
        case APPLY:  makeApply(Inst->Args[0]); break;
        case APPTERM1: makeApply(1, true); Builder->CreateRet(getAccu()); break;
        case APPTERM2: makeApply(2, true); Builder->CreateRet(getAccu()); break;
        case APPTERM3: makeApply(3, true); Builder->CreateRet(getAccu()); break;
        case APPTERM: makeApply(Inst->Args[0], true); Builder->CreateRet(getAccu()); break;

        // Fall through return
        case STOP:
        case RETURN: {
            auto RetVal = getAccu();
            if (Accu->getType() != getValType()) {
                RetVal = Builder->CreateIntCast(Accu, getValType(), true);
            }
            Builder->CreateRet(RetVal); 
            break;
        }
        case BRANCH:{
            BasicBlock* LBrBlock = BrBlock->LlvmBlock;
            Builder->CreateBr(LBrBlock);
            break;
        }
        case BRANCHIF: {
            auto BoolVal = getAccu();
            if (getAccu()->getType() == getValType()) {
               // Set boolval to true if Accu != Val_False
               BoolVal = Builder->CreateICmpNE(getAccu(), ConstInt(Val_false), "BranchCmp");
            }
            Builder->CreateCondBr(BoolVal, BrBlock->LlvmBlocks.front(), NoBrBlock->LlvmBlocks.front());
            break;
        }
        case BRANCHIFNOT: {
            auto BoolVal = getAccu();
            if (getAccu()->getType() == getValType()) {
                BoolVal = Builder->CreateICmpNE(getAccu(), ConstInt(Val_false), "BranchCmp");
            }
            Builder->CreateCondBr(BoolVal, NoBrBlock->LlvmBlocks.front(), BrBlock->LlvmBlocks.front());
            break;
        }
        case SWITCH: {
            auto SwitchVal = Builder->CreateCall2(getFunction("getSwitchOffset"),
                                                  ConstInt(Inst->Args[0]),
                                                  Accu);
            auto DefaultBlock = Function->Blocks[Inst->SwitchEntries[0]];
            this->setNext(DefaultBlock, false);
            auto Switch = Builder->CreateSwitch(SwitchVal, DefaultBlock->LlvmBlock);
            for (size_t i = 0; i < Inst->SwitchEntries.size(); i++)
                Switch->addCase(ConstInt(i), Function->Blocks[Inst->SwitchEntries[i]]->LlvmBlock);

            break;
        }


        case BEQ:
            TmpVal = Builder->CreateICmpEQ(ConstInt(Val_int(Inst->Args[0])), getAccu());
            goto makebr;
        case BNEQ:
            TmpVal = Builder->CreateICmpNE(ConstInt(Val_int(Inst->Args[0])), getAccu());
            goto makebr;
        case BLTINT:
            TmpVal = Builder->CreateICmpSLT(ConstInt(Val_int(Inst->Args[0])), getAccu());
            goto makebr;
        case BLEINT:
            TmpVal = Builder->CreateICmpSLE(ConstInt(Val_int(Inst->Args[0])), getAccu());
            goto makebr;
        case BGTINT:
            TmpVal = Builder->CreateICmpSGT(ConstInt(Val_int(Inst->Args[0])), getAccu());
            goto makebr;
        case BGEINT:
            TmpVal = Builder->CreateICmpSGE(ConstInt(Val_int(Inst->Args[0])), getAccu());
            goto makebr;
        case BULTINT:
            TmpVal = Builder->CreateICmpULT(ConstInt(Val_int(Inst->Args[0])), getAccu());
            goto makebr;
        case BUGEINT:
            TmpVal = Builder->CreateICmpUGE(ConstInt(Val_int(Inst->Args[0])), getAccu());
            goto makebr;


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


