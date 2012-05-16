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
  { StackPointer -= 2; StackPointer[0] = Accu; StackPointer[1] = Env; caml_extern_sp = StackPointer; }
#define Restore_after_gc \
  { Accu = StackPointer[0]; Env = StackPointer[1]; StackPointer += 2; }
#define Setup_for_c_call \
  { *--StackPointer = Env; caml_extern_sp = StackPointer; }
#define Restore_after_c_call \
  { StackPointer = caml_extern_sp; Env = *StackPointer++; }


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


int callnb = 0;

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

value getBlockSize(value Block) { return Wosize_val(Block); }

void getField(value Idx) { 
    IFDBG(printf("GETFIELD %ld: %p\n", Idx, (void*)Field(Accu, Idx));)
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

void makeBlock(value tag, value vwosize) {
    mlsize_t i;
    mlsize_t wosize = (mlsize_t)vwosize;
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
    Accu = block;
}

void makeFloatBlock(value VSize) {
   
    value Block;
    mlsize_t i;
    mlsize_t Size = (mlsize_t)VSize;

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
    StackPointer++;
}

void getDoubleField(value Idx) {
    double d = Double_field(Accu, Idx);
    Alloc_small(Accu, Double_wosize, Double_tag);
    Store_double_val(Accu, d);
}

// ================================= GLOBAL DATA ============================ //

void getGlobal(value Idx) {
    IFDBG(printf("IN GETGLOBAL %ld\n", Idx);)
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

void getExceptionValue() {
    IFDBG(printf("IN GETEXCEPTIONVALUE : %p\n", (void*)Accu);)
    StackPointer = caml_trapsp;
    Env = StackPointer[2];
    caml_trapsp = Trap_link(StackPointer);
    extra_args = Long_val(StackPointer[3]);
    StackPointer += 4;
    Accu = caml_exn_bucket;
}

value addExceptionContext() {
    IFDBG(printf("IN ADD EXCEPTION CONTEXT\n");)
    JmpBufList* NewContext = malloc(sizeof(JmpBufList));
    NewContext->Next = NextExceptionContext;
    NextExceptionContext = NewContext;

    if (sigsetjmp(NewContext->JmpBuf.buf, 0) == 0) {
        IFDBG(printf("RETURNING FROM ADD EXCEPTION CONTEXT NORMALLY\n");)
        return 0;
    } else {
        IFDBG(printf("RETURNING FROM A THROW\n");)
        JmpBufList* Ctx = NextExceptionContext;
        NextExceptionContext = Ctx->Next;
        free(Ctx);
        IFDBG(printf("WE OUTTA THERE\n");)
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
    if (!NextExceptionContext) exit(0);
    caml_exn_bucket = ExcVal;
    siglongjmp(NextExceptionContext->JmpBuf.buf, 1);
}

void vectLength() {
    mlsize_t size = Wosize_val(Accu);
    if (Tag_val(Accu) == Double_array_tag) size = size / Double_wosize;
    Accu = Val_long(size);
}

void getVectItem() {
    Accu = Field(Accu, Long_val(StackPointer[0]));
    StackPointer += 1;
}

void setVectItem() {
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

void setStringChar() {
    Byte_u(Accu, Long_val(StackPointer[0])) = Int_val(StackPointer[1]);
    StackPointer += 2;
    Accu = Val_unit;
}

// ============================= OBJECTS ============================== //

void getMethod() {
    Accu = Lookup(StackPointer[0], Accu);
}

void getDynMet() {
      value meths = Field (StackPointer[0], 0);
      int li = 3, hi = Field(meths,0), mi;
      while (li < hi) {
        mi = ((li+hi) >> 1) | 1;
        if (Accu < Field(meths,mi)) hi = mi-2;
        else li = mi;
      }
      Accu = Field (meths, li-1);

}

void getPubMet(value Met) {
    *--StackPointer = Accu;
    Accu = Val_int(Met);
    getDynMet();
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

void offsetRef(value Offset) {
    Field(Accu, 0) += Offset << 1;
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
    IFDBG(printf("%ld", CCall->CallFnId);)
    if (CCall->NbSubCalls > 0) {
        IFDBG(printf("[");)
        for (i = 0; i < CCall->NbSubCalls; i++) {
            printCallChain(CCall->SubCalls[i], depth+1);
            if (i < CCall->NbSubCalls - 1)
                IFDBG(printf(", ");)
        }
        IFDBG(printf ("]");)
    }
}

void cmpDebug(value A, value B) {
    IFDBG(printf("ULTINT : A = %ld, B = %ld\n", A, B);)
}


/* =========== EXPLICIT STACK VERSION FUNCTIONS =========== */

void init() {
    StackPointer = caml_extern_sp;
}

void constInt(value CI) {
    IFDBG(printf("CONSTINT\n");)
    Accu = CI;
}

void push() {
    IFDBG(printf("PUSH\n");)
    *--StackPointer = Accu;
}

void pop(value N) {
    IFDBG(printf("POP\n");)
    StackPointer += N;
}

void acc(value N) {
    IFDBG(printf("ACC %ld\n", N);)
    Accu = StackPointer[N];
}

void envAcc(value N) {
    Accu = Field(Env, N);
    IFDBG(printf("ENVACC %ld: %p\n", N, (void*)Accu);)
}

void addInt() { 
    IFDBG(printf("ADDINT\n");)
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
void isInt() { Accu = Val_long(Accu & 1); }
void boolNot() { Accu = Val_not(Accu); }

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
    p = &Field(Accu, (Idx * 2) - 1);
    *p = Make_header(Idx * 2, Infix_tag, Caml_white);  /* color irrelevant. */
    p++;
    *p = CodePtr;
    *--StackPointer = (value) p;
}

void closure(value nvars, value CodePtr) {
    int i;
    if (nvars > 0) *--StackPointer = Accu;
    Alloc_small(Accu, 1 + nvars, Closure_tag);
    Code_val(Accu) = (code_t)CodePtr;
    for (i = 0; i < nvars; i++) {
        Field(Accu, i + 1) = StackPointer[i];
        IFDBG(printf("FIELD %d = %p\n", i + 1, (void*)StackPointer[i]);)
    }
    IFDBG(printf("CLOSURE %ld %p = %p\n", nvars, (void*)CodePtr, (void*)Accu);)
    StackPointer += nvars;
}

void offsetClosure(value offset) {
    Accu = Env + (offset * sizeof(value));
}

value checkGrab(value required) {
    IFDBG(printf("IN CHECKGRAB %ld %ld\n", extra_args, required);)
    value ret = extra_args >= required;
    return ret;
}

void createRestartClosure(value CodePtr) {
    mlsize_t num_args, i;
    num_args = 1 + extra_args; /* arg1 + extra args */
    Alloc_small(Accu, num_args + 2, Closure_tag);
    Field(Accu, 1) = Env;
    for (i = 0; i < num_args; i++) Field(Accu, i + 2) = StackPointer[i];
    Code_val(Accu) = (code_t)CodePtr; /* Point to the preceding RESTART instr. */
    StackPointer += num_args;
    Env = StackPointer[1];
    extra_args = Long_val(StackPointer[2]);
    StackPointer += 3;
}

void substractExtraArgs(value required) {
    extra_args -= required;
    IFDBG(printf("IN SUBSTRACT EXTRA ARGS\n");)
    IFDBG(printf("{{ EXTRA ARGS = %ld\n", extra_args);)
}

void restart() {
    int num_args = Wosize_val(Env) - 2;
    int i;
    StackPointer -= num_args;
    for (i = 0; i < num_args; i++) StackPointer[i] = Field(Env, i + 2);
    Env = Field(Env, 1);
    extra_args += num_args;
    IFDBG(printf("IN RESTART\n");)
    IFDBG(printf("{{ EXTRA ARGS = %ld\n", extra_args);)
}

#define CHECK_STACKS() \
    if (StackPointer < caml_stack_threshold) { \
        caml_extern_sp = StackPointer; \
        caml_realloc_stack(Stack_threshold / sizeof(value)); \
        StackPointer = caml_extern_sp; \
    } 

void apply(value N) {
    IFDBG(printf("APPLY\n");)
    extra_args = N -1;
    IFDBG(printf("IN APPLY\n");)
    IFDBG(printf("{{ EXTRA ARGS = %ld\n", extra_args);)
    Env = Accu;
    void (*CodePtr)() = (void(*)())Code_val(Accu);
    CodePtr();
}

void apply1() {
    int cnb = callnb++;
    IFDBG(printf("IN APPLY 1 %d\n", cnb);)
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
    IFDBG(printf("OUT APPLY 1 %d , result : %p\n", cnb, (void*)Accu);)
}

void apply2() {
    IFDBG(printf("APPLY2\n");)
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
    StackPointer += stsz;
    if (extra_args > 0) {
        extra_args--;
        Env = Accu;
        void (*CodePtr)() = (void(*)())Code_val(Accu);
        CodePtr();
    } else {
        // No partial application
        Env = StackPointer[1];
        extra_args = Long_val(StackPointer[2]);
        StackPointer += 3;
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
    IFDBG(printf("C_CALL1\n");)
    Setup_for_c_call;
    Accu = Primitive(prim)(Accu);
    Restore_after_c_call;
    IFDBG(printf("AFTER C_CALL1\n");)
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

void pushRetAddr() {
    StackPointer -= 3;
    StackPointer[0] = Val_unit;
    StackPointer[1] = Env;
    StackPointer[2] = Val_long(extra_args);
}

void printAccu() {
    printf("%ld\n", Accu);
}
