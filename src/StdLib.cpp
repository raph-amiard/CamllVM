extern "C" {
    #include <ocaml_runtime/mlvalues.h>
    #include <ocaml_runtime/major_gc.h>
    #include <ocaml_runtime/memory.h>
}

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

extern "C" {

    // Closure handling is a little complex. 
    // We need to store :
    // - The code pointer
    // - The closure captured elements
    // - The total number of args the function 
    //      takes because we don't have it anywhere
    // - The yet supplied number of args
    // - The yet supplied args if any
    // In any case we are gonna need 1 + NVars + 2 + NbArgs more fields 
    // in the closure to store all this extra information
    // The layout of the block will thus be :
    //
    // -----------------------------------------
    // |CodePtr|Vars|Args|NbSuppliedArgs|NbArgs|
    // -----------------------------------------
    //
    // It is compatible with the regular ocaml runtime's closure layout 

    inline char* makeClosure(int32_t NVars, int32_t* FPtr, int32_t NbArgs, int32_t NbSuppliedArgs) {
        value Closure;
        int BlockSize = 3 + NVars + NbArgs;
        Alloc_small(Closure, BlockSize, Closure_tag);
        // Set the code pointer
        Code_val(Closure) = FPtr;
        // Set the NbArgs
        Field(Closure, BlockSize - 1) = NbArgs;
        Field(Closure, BlockSize - 2) = NbSuppliedArgs;
        return (char*)Closure;
    }

    void closureSetVar(char* Closure, int VarIdx, char* Value) {
        Field(Closure, VarIdx + 1) = (value)Value;
    }

    void closureSetArg(char* Closure, int ArgIdx, char* Value) {
        int Size = Wosize_val(Closure);
        Field(Closure, (Size - 3 - ArgIdx)) = (value)Value;
        Field(Closure, (Size - 2)) -= 1;
    }

    char* closureGetNbSuppliedArgs(char* Closure, char* Value) {
        return (char*)Field(Closure, (Wosize_val(Closure) - 2));
    }

    char* closureGetNbArgs(char* Closure, char* Value) {
        return (char*)Field(Closure, (Wosize_val(Closure) - 1));
    }

    void setField(char* Block, int Idx, char* Val) {
        Field(Block, Idx) = (value)Val;
    }
}
