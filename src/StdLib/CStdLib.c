#include <ocaml_runtime/mlvalues.h>
#include <ocaml_runtime/printexc.h>
#include <ocaml_runtime/major_gc.h>
#include <ocaml_runtime/memory.h>
#include <ocaml_runtime/prims.h>
#include <ocaml_runtime/fail.h>
#include <ocaml_runtime/stacks.h>
#include <stdio.h>

#define Lookup(obj, lab) Field (Field (obj, 0), Int_val(lab))

#ifndef STDDBG
#define STDDBG 0
#endif

#define IFDBG(insts) if (STDDBG) { insts }

/* GC interface */

#define Setup_for_gc \
  {}
#define Restore_after_gc \
  {}
#define Setup_for_c_call \
  {}
#define Restore_after_c_call \
  {}


typedef struct _Call {
    value CallFnId;
    int NbSubCalls;
    struct _Call* SubCalls[128];
    struct _Call* ParentCall;
} Call;

Call* RootCall = NULL;
Call* CurrentCall = NULL;

value Accu;
value* StackPointer;

intnat extra_args;

// ================ STDLIB Declaration ================== //

value Env = 0;
void setEnv(value E) {
    Env = E;
}
value getEnv() {
    IFDBG(printf("IN GETENVVV, ENV = %p\n\n", (void*)Env);)
    return Env;
}

value isClosureHeader(value Header) {
    return Tag_hd(Header) == Closure_tag;
}

void debug(value Arg) {
    printf("DEBUG : %ld\n", (long) Arg);
    fflush(stdout);
}

// TODO: Recomment that shit

value makeClosure(value NVars, value FPtr, value NbArgs) {

    IFDBG(
        printf("IN MAKE CLOSURE, NVars = %ld, FPtr = %p, NbArgs = %ld\n",NVars, (void*)FPtr, NbArgs);
    )

    value Closure;
    int BlockSize = 2 + NVars + NbArgs;
    Alloc_small(Closure, BlockSize, Closure_tag);

    // Set the code pointer
    Code_val(Closure) = (code_t)FPtr;

    // Set the NbRemArgs to the total nb of args
    Field(Closure, BlockSize - 1) = NbArgs;

    IFDBG(
        printf("CLOSURE ADDR: %p\n", (void*)Closure);
        printf("CLOSURE FN ADDR: %p\n", (void*)FPtr);
    )

    return Closure;
}

value shiftClosure(value Shift) {
    return Env + (Shift*sizeof(value));
}

// Usage
// ClosureSetNext(MainClosure, NestedClosureOffset,
//                NestedClosureSize, NestedFunctionPtr,
//                NestedClosureNbArgs)
value closureSetNestedClos(value Closure, value NClOffset, value NClSize,
                          value FPtr,    value NbArgs) {
    Field(Closure, NClOffset - 1) = Make_header(NClSize, Infix_tag, Caml_white);
    value NestedClos = Closure+NClOffset*sizeof(value);
    Code_val(NestedClos) = (code_t)FPtr;
    // Set NbRemArgs and NbTotalArgs
    Field(NestedClos, NClSize - 1) = NbArgs;
    Field(NestedClos, NClSize - 2) = NbArgs;
    return NestedClos;
}

void closureSetVar(value Closure, value VarIdx, value Value) {
    IFDBG(printf("INTO CLOSURESETVAR, %p\n", (void*)Value);)
    Field(Closure, VarIdx + 1) = Value;
}

int ExtraArgs = 0;

#define Partial_closure_tag 245
#define Is_partial(Closure) Tag_val(Closure) == Partial_closure_tag

value createPartial(value Closure, value NbArgs, value* Args) {
    IFDBG(printf("create partial\n");)
    int i;
    int IsPartial = Is_partial(Closure);
    value Partial;
    value CSize = Wosize_val(Closure);
    value CNbArgs = IsPartial ? CSize - 3 : Field(Closure, CSize - 1);
    // Size = One field for each arg + CodeVal + Nbargs
    value PartialSize = 3+CNbArgs;
    value NbRemArgs = Field(Closure, CSize-1);

    Alloc_small(Partial, PartialSize, Partial_closure_tag);

    if (IsPartial) {
        // Copy the partial
        for (i = 0; i < PartialSize-1; i++)
            Field(Partial, i) = Field(Closure, i);
    } else {
        // Add the reference to original closure
        Field(Partial, 1) = Closure;
        Code_val(Partial) = Code_val(Closure);
    }

    // Fill up remaining args
    for (i = 0; i < NbArgs; i++)
        Field(Partial, PartialSize-NbRemArgs-1+i) = Args[NbArgs-1-i];

    Field(Partial, PartialSize-1) = NbRemArgs - NbArgs;

    return Partial;
}

void printBlock(value Block) {
    int i;
    value BSize = Wosize_val(Block);
    printf("BLOCK : [");
    for (i=0; i < BSize; i++) {
        value Fld = Field(Block, i);
        if (Is_block(Fld))
            printf("%p", (void*)Fld);
        else
            printf("%ld", Fld);
        if (i < BSize-1) printf(" ");
    }
    printf("]\n");
}

/*value apply(value Closure, value NbArgs, value* Args) {

    IFDBG(printf("IN APPLY, Nbargs = %ld\n", NbArgs);)

    value CClosure = Closure;
    int i;

    while (NbArgs > 0) {
        value CSize = Wosize_val(CClosure);
        int CNbArgs = Field(CClosure, (CSize - 1));

        IFDBG(printf("IN APPLY, closure size = %ld\n", CSize);)

        // If not enough args
        // We'll have to create a new closure
        if (NbArgs < CNbArgs) {
            return createPartial(CClosure, NbArgs, Args);
        }

        // Fill up remaining args
        for (i = 0; i < NbArgs && i < CNbArgs; i++) {
            IFDBG(printf("ARG : %ld\n", Args[NbArgs-1-i]);)
            Field(CClosure, CSize-CNbArgs-1+i) = Args[NbArgs-1-i];
        }

        // Get the function pointer
        value (*FPtr)(value) = (value(*)(value)) Code_val(CClosure);

        // Save the env
        value OldEnv = Env;
        value ArgsTabPtr;

        // Set the env, and get a ptr to the beginning of the args
        if (Is_partial(CClosure)) {
            IFDBG(printf("closure is a partial application !\n");)
            Env = Field(CClosure, 1);
            ArgsTabPtr = CClosure + 2*sizeof(value);
        } else {
            Env = CClosure;
            ArgsTabPtr = CClosure + (CSize-CNbArgs-1)*sizeof(value);
        }

        IFDBG(
            for (int i = 0; i < CNbArgs; i++)
                printf("ARGINTAB: %ld\n", ((value*)ArgsTabPtr)[i]);
        )

        // Apply the closure
        CClosure = FPtr(ArgsTabPtr);

        Env = OldEnv;
        NbArgs -= CNbArgs;

    }

    IFDBG(printf("APPLY RESULT : %ld\n", CClosure);)
    return CClosure;
}
*/
/*
#define MAPPLY(a) \
    { FPtr = (value(*)(value)) Code_val(CClosure); \
    value OldEnv = Env; \
    Env = CClosure; \
    CClosure = FPtr(CClosure); \
    Env = OldEnv };


value apply1(value Closure, value Arg1) {
    return 0;
}

value apply2(value Closure, value Arg1, value Arg2) {

    value CClosure = Closure;
    int Size = Wosize_val(CClosure);
    int NbRemArgs = Field(CClosure, (Size - 1));
    value (*FPtr)(value);
    value OldEnv;

    Field(CClosure, (Size - 1 - NbRemArgs)) = Arg1;
    NbRemArgs--; 
    if (--NbRemArgs == 0) {
    }
    Field(CClosure, (Size - 1 - NbRemArgs)) = Arg2;
    NbRemArgs--; if (--NbRemArgs == 0) {}

    ret_lbl: return CClosure;
}
*/

value getBlockSize(value Block) { return Wosize_val(Block); }

void getField(value Idx) { 
    IFDBG(printf("Getfield : %p\n", (void*)Field(Accu, Idx));)
    Accu = Field(Accu, Idx); 
}

void setField(value Idx) {
    value NewVal = *StackPointer++;
    Modify(&Field(Accu, Idx), NewVal);
    Accu = Val_unit;
}

void getAtom(value Idx) {
    Accu = Atom(Idx);
}

void makeBlock1(value tag) {
    value block;
    Alloc_small(block, 1, (tag_t)tag);
    Field(block, 0) = Accu;
    Accu = block;
}

void makeBlock2(value tag) {
    value block;
    Alloc_small(block, 2, (tag_t)tag);
    Field(block, 0) = Accu;
    Field(block, 1) = StackPointer[0];
    StackPointer += 1;
    Accu = block;
}

void makeBlock3(value tag) {
    value block;
    Alloc_small(block, 3, (tag_t)tag);
    Field(block, 0) = Accu;
    Field(block, 1) = StackPointer[0];
    Field(block, 2) = StackPointer[1];
    StackPointer += 2;
    Accu = block;
}

void makeBlock(value tag, value wosize) {
    mlsize_t i;
    value block;
    if (wosize <= Max_young_wosize) {
        Alloc_small(block, wosize, tag);
        Field(block, 0) = Accu;
        for (i = 1; i < wosize; i++) Field(block, i) = *StackPointer++;
    } else {
        block = caml_alloc_shr(wosize, tag);
        caml_initialize(&Field(block, 0), Accu);
        for (i = 1; i < wosize; i++) caml_initialize(&Field(block, i), *StackPointer++);
    }
}

void makeFloatBlock(value Size) {
   
    value Block;
    mlsize_t i;

    if ((mlsize_t)Size <= Max_young_wosize / Double_wosize)
        Alloc_small(Block, Size * Double_wosize, Double_array_tag);
    else
        Block = caml_alloc_shr(Size * Double_wosize, Double_array_tag);
    

    Store_double_field(Block, 0, Double_val(Accu));

    for (i = 1; i < Size; i++){
        Store_double_field(Block, i, Double_val(*StackPointer));
        ++ StackPointer;
    }

    Accu = Block;
}

void storeDoubleField(value Idx) {
    //printf("in storedoublefield, block = %p, idx = %ld\n", (void*)Block, Idx);
    Store_double_field(Accu, Idx, Double_val(*StackPointer));
    Accu = Val_unit;
}

void getDoubleField(value Block, value Idx) {
    //printf("in getdoublefield, block = %p, idx = %ld\n", (void*)Block, Idx);
    double d = Double_field(Block, Idx);
    value Double;
    Alloc_small(Double, Double_wosize, Double_tag);
    Store_double_val(Double, d);
    Accu = Double;
}


// ============================= C CALLS ============================== //

value primCall(value Prim) {
  Setup_for_c_call;
  value Ret = Primitive(Prim)();
  Restore_after_c_call;
  return Ret;
}

value primCall1(value Prim, value Val1) {
  //printf("In primCall1, Prim number = %ld, Val1 = %p\n", Prim, (void*)Val1);
  Setup_for_c_call;
  value Ret = Primitive(Prim)(Val1);
  Restore_after_c_call;
  //printf("PrimCall Result: %ld\n", Ret);
  return Ret;
}

value primCall2(value Prim, value Val1, value Val2) {
  //printf("In primCall2, Prim number = %ld, Val1 = %p, Val2 = %p\n", Prim, (void*)Val1, (void*)Val2);
  Setup_for_c_call;
  value Ret = Primitive(Prim)(Val1, Val2);
  Restore_after_c_call;
  //printf("PrimCall Result: %ld\n", Ret);
  return Ret;
}

value primCall3(value Prim, value Val1, value Val2, value Val3) {
  //printf("In primCall3, Prim number = %ld, Val1 = %p, Val2 = %p, Val3 = %p\n", Prim, (void*)Val1, (void*)Val2, (void*)Val3);
  Setup_for_c_call;
  value Ret = Primitive(Prim)(Val1, Val2, Val3);
  Restore_after_c_call;
  //printf("PrimCall Result: %ld\n", Ret);
  return Ret;
}

value primCall4(value Prim, value Val1, value Val2, value Val3, value Val4) {
  //printf("In primCall4, Prim number = %ld, Val1 = %p, Val2 = %p, Val3 = %p, Val4 = %p\n", Prim, (void*)Val1, (void*)Val2, (void*)Val3, (void*)Val4);
    Setup_for_c_call;
    value Ret = Primitive(Prim)(Val1, Val2, Val3, Val4);
    Restore_after_c_call;
  //printf("PrimCall Result: %ld\n", Ret);
    return Ret;
}

value primCall5(value Prim, value Val1, value Val2, value Val3, value Val4, value Val5) {
    Setup_for_c_call;
    value Ret = Primitive(Prim)(Val1, Val2, Val3, Val4, Val5);
    Restore_after_c_call;
    return Ret;
}

value primCalln(value Prim, value Argv, value Argc) {
    printf("INTO PRIMCALL N\n");
    Setup_for_c_call;
    value Ret = Primitive(Prim)(Argv, Argc);
    Restore_after_c_call;
    return Ret;
}

// ================================= GLOBAL DATA ============================ //

void getGlobal(value Idx) {
    Accu = Field(caml_global_data, Idx);
}

void setGlobal(value Idx) {
    //printf("In set global number %ld\n", Idx);
    //printf("Global = %p\n", (void*)Val);
    Modify(&Field(caml_global_data, Idx), Accu);
}


// ============================ EXCEPTION HANDLING ========================= //

typedef struct _JmpBufList {
    struct _JmpBufList* Next;
    struct longjmp_buffer JmpBuf;
    value Env;
} JmpBufList;

JmpBufList* NextExceptionContext = NULL;

char* getNewBuffer() {
    JmpBufList* NewContext = malloc(sizeof(JmpBufList));
    NewContext->Next = NextExceptionContext;
    NewContext->Env = Env;
    NextExceptionContext = NewContext;
    caml_external_raise = &NewContext->JmpBuf;
    return (char*)NewContext->JmpBuf.buf;
}

value getExceptionValue() {
    return caml_exn_bucket;
}

value addExceptionContext() {
    printf("IN ADD EXCEPTION CONTEXT\n");
    JmpBufList* NewContext = malloc(sizeof(JmpBufList));
    NewContext->Next = NextExceptionContext;
    NextExceptionContext = NewContext;

    if (sigsetjmp(NewContext->JmpBuf.buf, 0) == 0) {
        printf("RETURNING FROM ADD EXCEPTION CONTEXT NORMALLY\n");
        return 0;
    } else {
        printf("RETURNING FROM A THROW\n");
        JmpBufList* Ctx = NextExceptionContext;
        NextExceptionContext = Ctx->Next;
        free(Ctx);
        printf("WE OUTTA THERE\n");
        return caml_exn_bucket;
    }
}

void removeExceptionContext() {
    JmpBufList* Context = NextExceptionContext;
    NextExceptionContext = Context->Next;
    free(Context);
    caml_external_raise = &NextExceptionContext->JmpBuf;
}

void printCallChain(Call* CCall, int depth);

void throwException(value ExcVal) {
    if (!NextExceptionContext) {
        char* msg = caml_format_exception(ExcVal);
        printf("Uncatched Exception: %s\n", msg);
        printCallChain(RootCall, 0);
        exit(0);
    }
    caml_exn_bucket = ExcVal;
    Env = NextExceptionContext->Env;
    siglongjmp(NextExceptionContext->JmpBuf.buf, 1);
}

void vectLength(value Vect) {
    mlsize_t Size = Wosize_val(Vect);
    if (Tag_val(Vect) == Double_array_tag) Size = Size / Double_wosize;
    Accu = Val_long(Size);
}

void getVectItem() {
    Accu = Field(Accu, Long_val(StackPointer[0]));
    StackPointer += 1;
}

void setVectItem(value Vect, value Idx, value NewVal) {
    //printf("INSETVECTITEM, Vect = %p, Idx = %ld, NewVal = %ld\n", (void*)Vect, Idx, NewVal);
    value * modify_dest, modify_newval;
    modify_dest = &Field(Accu, Long_val(StackPointer[0]));
    modify_newval = StackPointer[1];
    StackPointer += 2;
    Modify(modify_dest, modify_newval);
    Accu = Val_unit;
}

void getStringChar() {
    Accu = Val_int(Byte_u(Accu, Long_val(StackPointer[0])));
    StackPointer += 1;
}

void setStringChar(value String, value CharIdx, value Char) {
    Byte_u(Accu, Long_val(StackPointer[0])) = Int_val(StackPointer[1]);
    StackPointer += 2;
    Accu = Val_unit;
}

// ============================= OBJECTS ============================== //

value getMethod(value Object, value Label) {
    IFDBG(printf("INTO GETMETHOD\n");)
    return Lookup(Object, Label);
}

value getDynMethod(value Object, value Tag) {

    IFDBG(printf("INTO GETDYNMETHOD, Tag = %ld\n", Tag);)

    value Methods = Field (Object, 0);
    int Li = 3, 
        Hi = Field(Methods,0), 
        Mi;
    while (Li < Hi) {
      Mi = ((Li+Hi) >> 1) | 1;
      if (Tag < Field(Methods,Mi)) Hi = Mi-2;
      else Li = Mi;
    }

    IFDBG(printf("MeTHOD OFFSeT : %d\n", Li-1);)

    return Field (Methods, Li-1);
}

value getSwitchOffset(value SwitchArg, value Dispatch) {
    if (Is_block(Dispatch)) {
        int index = Tag_val(Dispatch);
        return (SwitchArg & 0xFFFF) + index;
    } else {
        int index = Long_val(Dispatch);
        return index;
    }
}

void offsetRef(value Ref, value Offset) {
    Field(Ref, 0) += Offset << 1;
    Accu = Val_unit;
}

void addCall(value FnId) {

    Call* NewCall = malloc(sizeof(Call));
    NewCall->CallFnId = FnId;
    NewCall->NbSubCalls = 0;
    NewCall->ParentCall = CurrentCall;

    if (!RootCall)
        RootCall = NewCall;
    else
        CurrentCall->SubCalls[CurrentCall->NbSubCalls++] = NewCall;

    CurrentCall = NewCall;
}

void endCall() {
    CurrentCall = CurrentCall->ParentCall;
}

void printCallChain(Call* CCall, int depth) {
    int i;
    printf("%ld", CCall->CallFnId);
    if (CCall->NbSubCalls > 0) {
        printf("[");
        for (i = 0; i < CCall->NbSubCalls; i++) {
            printCallChain(CCall->SubCalls[i], depth+1);
            if (i < CCall->NbSubCalls - 1)
                printf(", ");
        }
        printf ("]");
    }
}

void cmpDebug(value A, value B) {
    printf("ULTINT : A = %ld, B = %ld\n", A, B);
}


/* =========== EXPLICIT STACK VERSION FUNCTIONS =========== */

void init() {
    StackPointer = caml_extern_sp;
}

void constInt(value CI) {
    printf("CONSTINT\n");
    Accu = CI;
}

void push() {
    printf("PUSH\n");
    *--StackPointer = Accu;
}

void pop(value N) {
    printf("POP\n");
    StackPointer += N;
}

void acc(value N) {
    printf("ACC %d\n", N);
    Accu = StackPointer[N];
}

void envAcc(value N) {
    Accu = Field(Env, N);
}

void addInt() { 
    printf("ADDINT\n");
    Accu = (value)((intnat) Accu + (intnat) *StackPointer++ - 1); 
}

void negInt() { Accu = (value)(2 - (intnat)Accu); }

void subInt() { Accu = (value)((intnat) Accu - (intnat) *StackPointer++ + 1); }

void mulInt() { Accu = Val_long(Long_val(Accu) * Long_val(*StackPointer++)); }

void divInt() { 
    intnat Divisor = Long_val(*StackPointer++);
    Accu = Val_long(Long_val(Accu) / Divisor);
}

void modInt() {
    intnat Divisor = Long_val(*StackPointer++);
    Accu = Val_long(Long_val(Accu) % Divisor);
}

void offsetInt(value N) {
    Accu += N;
}

void andInt() { Accu = (value)((intnat) Accu & (intnat) *StackPointer++); }
void orInt() { Accu = (value)((intnat) Accu | (intnat) *StackPointer++); }
void xorInt() { Accu = (value)(((intnat) Accu ^ (intnat) *StackPointer++) | 1); }
void lslInt() { Accu = (value)((((intnat) Accu - 1) << Long_val(*StackPointer++)) + 1); }
void lsrInt() { Accu = (value)((((uintnat) Accu - 1) >> Long_val(*StackPointer++)) | 1); }
void asrInt() { Accu = (value)((((intnat) Accu - 1) >> Long_val(*StackPointer++)) | 1); }

void cmpEq() { Accu = Val_int((intnat) Accu == (intnat) *StackPointer++); }
void cmpNeq() { Accu = Val_int((intnat) Accu != (intnat) *StackPointer++); }
void ltInt() { Accu = Val_int((intnat) Accu < (intnat) *StackPointer++); }
void leInt() { Accu = Val_int((intnat) Accu <= (intnat) *StackPointer++); }
void gtInt() { Accu = Val_int((intnat) Accu > (intnat) *StackPointer++); }
void geInt() { Accu = Val_int((intnat) Accu >= (intnat) *StackPointer++); }
void ultInt() { Accu = Val_int((uintnat) Accu < (uintnat) *StackPointer++); }
void ugeInt() { Accu = Val_int((uintnat) Accu >= (uintnat) *StackPointer++); }

void assign(value N) {
    StackPointer[N] = Accu;
    Accu = Val_unit;
}

void closureRec(value nfuncs, value nvars, value MainCodePtr) {
    int i;
    value * p;

    printf("CLOSUREREC, CODEPTR %p\n", MainCodePtr);

    if (nvars > 0) *--StackPointer = Accu;
    Alloc_small(Accu, nfuncs * 2 - 1 + nvars, Closure_tag);
    p = &Field(Accu, nfuncs * 2 - 1);

    for (i = 0; i < nvars; i++) {
        *p++ = StackPointer[i];
    }

    StackPointer += nvars;
    p = &Field(Accu, 0);
    *p = MainCodePtr;
    *--StackPointer = Accu;
    p++;

}

void setClosureRecNestedClos(value Idx, value CodePtr) {
    value* p;
    p = &Field(Accu, 1 + (Idx * 2));
    *p = Make_header(Idx * 2, Infix_tag, Caml_white);  /* color irrelevant. */
    p++;
    *p = CodePtr;
    *--StackPointer = (value) p;
}

void closure(value nvars, value CodePtr) {
    printf("CLOSURE %ld %p\n", nvars, CodePtr);
    int i;
    if (nvars > 0) *--StackPointer = Accu;
    Alloc_small(Accu, 1 + nvars, Closure_tag);
    Code_val(Accu) = (code_t)CodePtr;
    for (i = 0; i < nvars; i++) Field(Accu, i + 1) = StackPointer[i];
    StackPointer += nvars;
}

void offsetClosure(value offset) {
    printf("OFFSETCLOSURE, CODEPTR = %p\n", Code_val(Accu));
    printf("OFFSETCLOSURE : %ld\n", Env);
    Accu = Env + (offset * sizeof(value));
    printf("OFFSETCLOSURE : Accu %ld\n", Accu);
}

value checkGrab(value required) {
    printf("IN CHECKGRAB %ld %ld\n", extra_args, required);
    value ret = extra_args >= required;
    printf("%ld\n", ret);
    return ret;
}

void createRestartClosure(value CodePtr) {
    printf("IN CREATE RESTART CLOSURE\n");
    mlsize_t num_args, i;
    num_args = 1 + extra_args; /* arg1 + extra args */
    Alloc_small(Accu, num_args + 2, Closure_tag);
    Field(Accu, 1) = Env;
    for (i = 0; i < num_args; i++) Field(Accu, i + 2) = StackPointer[i];
    Code_val(Accu) = (code_t)CodePtr; /* Point to the preceding RESTART instr. */
    StackPointer += num_args;
    extra_args = Long_val(StackPointer[2]);
    StackPointer += 3;
}

void substractExtraArgs(value required) {
    extra_args -= required;
}

void restart() {
    int num_args = Wosize_val(Env) - 2;
    int i;
    StackPointer -= num_args;
    for (i = 0; i < num_args; i++) StackPointer[i] = Field(Env, i + 2);
    Env = Field(Env, 1);
    extra_args += num_args;
}

#define CHECK_STACKS() \
    if (StackPointer < caml_stack_threshold) { \
        caml_extern_sp = StackPointer; \
        caml_realloc_stack(Stack_threshold / sizeof(value)); \
        StackPointer = caml_extern_sp; \
    } 

void apply(value N) {
    printf("APPLY\n");
    extra_args = N -1;
    Env = Accu;
    void (*CodePtr)() = (void(*)())Code_val(Accu);
    CodePtr();
}

void apply1() {
    value arg1 = StackPointer[0];
    StackPointer -= 3;
    StackPointer[0] = arg1;
    StackPointer[2] = Env;
    StackPointer[3] = Val_long(extra_args);
    Env = Accu;
    extra_args = 0;
    CHECK_STACKS();
    void (*CodePtr)() = (void(*)())Code_val(Accu);
    CodePtr();
}

void apply2() {
    printf("APPLY2\n");
    value arg1 = StackPointer[0];
    value arg2 = StackPointer[1];
    StackPointer -= 3;
    StackPointer[0] = arg1;
    StackPointer[1] = arg2;
    StackPointer[3] = Env;
    StackPointer[4] = Val_long(extra_args);
    Env = Accu;
    extra_args = 1;
    CHECK_STACKS();
    void (*CodePtr)() = (void(*)())Code_val(Accu);
    CodePtr();
    printf("OUTTA APPLY2\n");
}

void apply3() {
    value arg1 = StackPointer[0];
    value arg2 = StackPointer[1];
    value arg3 = StackPointer[2];
    StackPointer -= 3;
    StackPointer[0] = arg1;
    StackPointer[1] = arg2;
    StackPointer[2] = arg3;
    StackPointer[4] = Env;
    StackPointer[5] = Val_long(extra_args);
    Env = Accu;
    extra_args = 2;
    CHECK_STACKS();
    void (*CodePtr)() = (void(*)())Code_val(Accu);
    CodePtr();
}

void appterm(value nargs, value slotsize) {
    value * newsp;
    int i;
    /* Slide the nargs bottom words of the current frame to the top
     of the frame, and discard the remainder of the frame */
    newsp = StackPointer + slotsize - nargs;
    for (i = nargs - 1; i >= 0; i--) newsp[i] = StackPointer[i];
    StackPointer = newsp;
    Env = Accu;
    extra_args += nargs - 1;
    CHECK_STACKS();
    void (*CodePtr)() = (void(*)())Code_val(Accu);
    CodePtr();
}

void appterm1(value slotsize) {
    value arg1 = StackPointer[0];
    StackPointer = StackPointer + slotsize - 1;
    StackPointer[0] = arg1;
    Env = Accu;
    CHECK_STACKS();
    void (*CodePtr)() = (void(*)())Code_val(Accu);
    printf("APPTERM, CODEPTR = %p\n", CodePtr);
    CodePtr();
}

void appterm2(value slotsize) {
    value arg1 = StackPointer[0];
    value arg2 = StackPointer[1];
    StackPointer = StackPointer + slotsize - 2;
    StackPointer[0] = arg1;
    StackPointer[1] = arg2;
    Env = Accu;
    extra_args += 1;
    CHECK_STACKS();
    void (*CodePtr)() = (void(*)())Code_val(Accu);
    CodePtr();
}

void appterm3(value slotsize) {
    value arg1 = StackPointer[0];
    value arg2 = StackPointer[1];
    value arg3 = StackPointer[2];
    StackPointer = StackPointer + slotsize - 3;
    StackPointer[0] = arg1;
    StackPointer[1] = arg2;
    StackPointer[2] = arg3;
    Env = Accu;
    extra_args += 2;
    CHECK_STACKS();
    void (*CodePtr)() = (void(*)())Code_val(Accu);
    CodePtr();
}

void handleReturn(value stsz) {
    printf("IN HANDLE RETURN\n");
    StackPointer += stsz;
    printf("EXTRA ARGS : %d\n", extra_args);
    if (extra_args > 0) {
        extra_args--;
        Env = Accu;
        void (*CodePtr)() = (void(*)())Code_val(StackPointer[0]);
        CodePtr();
    } else {
        printf("SECOND BRANCH\n");
        Env = StackPointer[1];
        extra_args = Long_val(StackPointer[2]);
        StackPointer += 3;
        printf("OUTTA HANDLERETU\n");
        return;
    }
}

void pushTrap() {
      StackPointer -= 4;
      Trap_link(StackPointer) = caml_trapsp;
      StackPointer[2] = Env;
      StackPointer[3] = Val_long(extra_args);
      caml_trapsp = StackPointer;
}

void c_call1(value prim) {
    printf("C_CALL1\n");
    Setup_for_c_call;
    Accu = Primitive(prim)(Accu);
    Restore_after_c_call;
    printf("AFTER C_CALL1\n");
}

void c_call2(value prim) {
    Setup_for_c_call;
    Accu = Primitive(prim)(Accu, StackPointer[1]);
    Restore_after_c_call;
    StackPointer += 1;
}

void c_call3(value prim) {
    Setup_for_c_call;
    Accu = Primitive(prim)(Accu, StackPointer[1], StackPointer[2]);
    Restore_after_c_call;
    StackPointer += 2;
}

void c_call4(value prim) {
    Setup_for_c_call;
    Accu = Primitive(prim)(Accu, StackPointer[1], StackPointer[2], StackPointer[3]);
    Restore_after_c_call;
    StackPointer += 3;
}

void c_call5(value prim) {
    Setup_for_c_call;
    Accu = Primitive(prim)(Accu, StackPointer[1], StackPointer[2], StackPointer[3], StackPointer[4]);
    Restore_after_c_call;
    StackPointer += 4;
}

void c_calln(value prim, value nargs) {
      *--StackPointer = Accu;
      Setup_for_c_call;
      Accu = Primitive(prim)(StackPointer + 1, nargs);
      Restore_after_c_call;
      StackPointer += nargs;
}
