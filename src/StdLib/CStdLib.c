#include <ocaml_runtime/mlvalues.h>
#include <ocaml_runtime/major_gc.h>
#include <ocaml_runtime/memory.h>
#include <ocaml_runtime/prims.h>
#include <stdio.h>
#include <setjmp.h>

/* GC interface */

#define Setup_for_gc \
  {}
#define Restore_after_gc \
  {}
#define Setup_for_c_call \
  {}
#define Restore_after_c_call \
  {}

// ================ STDLIB Declaration ================== //

value Env = 0;
void setEnv(value E) {
    printf("setEnv %p\n", (void*)E);
    ////printf("IN SETENVVV\n\n");
    Env = E;
}
value getEnv() {
    //printf("IN GETENVVV, ENV = %p\n\n", (void*)Env);
    return Env;
}

void debug(value Arg) {
    printf("DEBUG : %ld\n", (long) Arg);
}

// Closure handling is a little complex. 
// We need to store :
// - The code pointer
// - The closure captured elements
// - The yet supplied number of args
// - The yet supplied args if any
// In any case we are gonna need 1 + NVars + 1 + NbArgs more fields 
// in the closure to store all this extra information
// The layout of the block will thus be :
//
// -----------------------------
// |CodePtr|Vars|Args|NbRemArgs|
// -----------------------------
//
// It is compatible with the regular ocaml runtime's closure layout 

value makeClosure(value NVars, value FPtr, value NbArgs) {
    //printf("IN MAKE CLOSURE\n");
    value Closure;
    int BlockSize = 2 + NVars + NbArgs;
    Alloc_small(Closure, BlockSize, Closure_tag);
    // Set the code pointer
    Code_val(Closure) = (code_t)FPtr;
    // Set the NbRemArgs to the total nb of args
    Field(Closure, BlockSize - 1) = NbArgs;
    //printf("CLOSURE ADDR: %p\n", (void*)Closure);
    //printf("CLOSURE FN ADDR: %p\n", (void*)FPtr);
    return Closure;
}

void shiftClosure(value Shift) {
    printf("===================\nshiftClosure\n===============\n");
    printf("Shift = %ld\n", Shift);
    printf("Env = %p\n", (void*)Env);
    value* Envv = (value*)Env;
    while (Shift) {
        if (Shift < 0) {
            // We check the field right after the code pointer to see the
            // closure index. If this is the first, it's a special case
            // (we want to go back to the 'main' closure) 
            if (*(Envv+2) == 1)
                Envv -= 2;
            else
                // We find the size of the previous closure from its NbArgs field
                Envv -= (*(Envv-1) + 4);

            Shift++;
        } else {
            printf("accessing header\n");
            value Header = *(Envv);
            printf("Header = %x\n", Header);
            // Check if we are in the first closure because its header wosize
            // includes all others, so we check the tag instead
            if (Tag_hd(Header) == Closure_tag) {
                printf("main closure shift to first nested\n");
                Envv += 2;
            } else {
                printf("general case\n");
                // if in an 'infix' closure we find the size of the current
                // closure from the header wosize
                value WoSize = (*(Envv))>>10;
                Envv += (WoSize + 4);
            }
            Shift--;
        }
    }
    printf("-------------------\nEnv = %p\n", Envv);
    Env = (value) Envv;
    printf("===================\nEND shiftClosure\n===============\n");
}

void closureSetNestedClos(value Closure, value CurrentIdx, value ClosureIdx,
                          value FPtr,    value NbArgs) {
    printf("================================\nIN closureSetNestedClos\n=======================================\n");
    printf("@Closure : %p\n", (void*)Closure);
    printf("@NestedClosure : %p\n", (void*)&Field(Closure, CurrentIdx));
    printf("Field(%ld) = %x\n", CurrentIdx,Make_header(3 + NbArgs, Infix_tag, Caml_white));
    Field(Closure, CurrentIdx) = Make_header(3 + NbArgs, Infix_tag, Caml_white);
    printf("Field(%ld) = %p\n", CurrentIdx+1, FPtr);
    Field(Closure, CurrentIdx + 1) = FPtr;
    printf("Field(%ld) = %ld\n", CurrentIdx+2, ClosureIdx);
    Field(Closure, CurrentIdx + 2) = ClosureIdx;
    printf("Field(%ld) = %ld\n", CurrentIdx + 3 + NbArgs, NbArgs);
    Field(Closure, CurrentIdx + 3 + NbArgs) = NbArgs;
    printf("=============================\n OUT closureSetNestedClos\n=================================\n");
}

void closureSetVar(value Closure, value VarIdx, value Value) {
    //printf("INTO CLOSURESETVAR\n\n");
    Field(Closure, VarIdx + 1) = Value;
}

value blockShift(value Block, value Shift) {
    return Block + Shift*sizeof(value);
}

value apply(value Closure, value NbArgs, value* Args) {

    printf("IN APPLYZE, CLOSURE = %p, NBARGS = %ld \n", (void*)Closure, NbArgs);
    printf("ARG 1 = %d\n", Args[0]);

    int ArgsSize = NbArgs;
    value CClosure = Closure;

    // While we still have arg to apply 
    while (NbArgs > 0) {

        printf("APPLYING ARGS\n");
        // Get the number of args the current closure needs
        int Size = Wosize_val(CClosure);
        //printf("SIZE = %d\n", Size);
        int NbRemArgs = Field(CClosure, (Size - 1));

        // Fill the closure with args until it's full 
        // or we don't have any args left
        while (NbRemArgs > 0 && NbArgs > 0) {
            Field(CClosure, (Size - 1 - NbRemArgs)) = 
                (value)Args[ArgsSize - NbArgs];
            NbRemArgs--; NbArgs--;
        }

        // If the closure is not full
        // and we don't have any args left, return the closure
        if (NbRemArgs > 0) {
            Field(CClosure, (Size - 1)) = NbRemArgs;
            return CClosure;
        }

        printf("APPLYING THE CLOZRS\n");
        // If the closure is full, apply it
        value (*FPtr)(value) = (value(*)(value)) Code_val(CClosure);
        value OldEnv = Env;
        Env = CClosure;
        CClosure = FPtr(CClosure);
        Env = OldEnv;

        // If NbArgs = 0, 
        // we get out of the loop and return the application result
    }

    return CClosure;
}

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

value getField(value Block, value Idx) { 
    printf("Getfield : %p\n", (void*)Field(Block, Idx)); 
    return Field(Block, Idx); 
}

void setField(value Field, value Idx, value NewVal) {
    Modify(&Field(Field, Idx), NewVal);
}

value getAtom(value Idx) {
    return Atom(Idx);
}

value makeBlock1(value tag, value Val1) {
      value block;
      Alloc_small(block, 1, (tag_t)tag);
      Field(block, 0) = Val1;
      return block;
}

value makeBlock2(value tag, value Val1, value Val2) {
      value block;
      Alloc_small(block, 2, (tag_t)tag);
      Field(block, 0) = Val1;
      Field(block, 1) = Val2;
      return block;
}

value makeBlock3(value tag, value Val1, value Val2, value Val3) {
      value block;
      Alloc_small(block, 3, (tag_t)tag);
      Field(block, 0) = Val1;
      Field(block, 1) = Val2;
      Field(block, 2) = Val3;
      return block;
}

value makeBlock(value tag, value NbVals) {
      value block;
      Alloc_small(block, NbVals, (tag_t)tag);
      return block;
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

// ================================= GLOBAL DATA ============================ //

value getGlobal(value Idx) {
    //printf("In get global number %ld\n", Idx);
    value Glob = Field(caml_global_data, Idx);
    //printf("Global = %p\n", (void*)Glob);
    return Glob;
}

void setGlobal(value Idx, value Val) {
    //printf("In set global number %ld\n", Idx);
    //printf("Global = %p\n", (void*)Val);
    Modify(&Field(caml_global_data, Idx), Val);
}


// ============================ EXCEPTION HANDLING ========================= //

typedef struct _JmpBufList {
    struct _JmpBufList* Next;
    jmp_buf JmpBuf;
    value Env;
} JmpBufList;

JmpBufList* NextExceptionContext = NULL;
value CurrentExceptionVal;

char* getNewBuffer() {
    JmpBufList* NewContext = malloc(sizeof(JmpBufList));
    NewContext->Next = NextExceptionContext;
    NewContext->Env = Env;
    NextExceptionContext = NewContext;
    return (char*)NewContext->JmpBuf;
}

value getExceptionValue() {
    return CurrentExceptionVal;
}

value addExceptionContext() {
    printf("IN ADD EXCEPTION CONTEXT\n");
    JmpBufList* NewContext = malloc(sizeof(JmpBufList));
    NewContext->Next = NextExceptionContext;
    NextExceptionContext = NewContext;

    if (setjmp(NewContext->JmpBuf) == 0) {
        printf("RETURNING FROM ADD EXCEPTION CONTEXT NORMALLY\n");
        return 0;
    } else {
        printf("RETURNING FROM A THROW\n");
        JmpBufList* Ctx = NextExceptionContext;
        NextExceptionContext = Ctx->Next;
        free(Ctx);
        printf("WE OUTTA THERE\n");
        return CurrentExceptionVal;
    }
}

void removeExceptionContext() {
    JmpBufList* Context = NextExceptionContext;
    NextExceptionContext = Context->Next;
    free(Context);
}

void throwException(value ExcVal) {
    if (!NextExceptionContext) {
        printf("Uncatched Exception\n");
        exit(0);
    }
    CurrentExceptionVal = ExcVal;
    Env = NextExceptionContext->Env;
    longjmp(NextExceptionContext->JmpBuf, 1);
}

value vectLength(value Vect) {
    mlsize_t Size = Wosize_val(Vect);
    if (Tag_val(Vect) == Double_array_tag) Size = Size / Double_wosize;
    return Val_long(Size);
}

value getVectItem(value Vect, value Idx) {
    return Field(Vect, Long_val(Idx));
}

void setVectItem(value Vect, value Idx, value NewVal) {
      Modify(&Field(Vect, Long_val(Idx)), NewVal);
}

