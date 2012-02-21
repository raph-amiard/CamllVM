extern "C" {
    #include <ocaml_runtime/mlvalues.h>
    #include <ocaml_runtime/major_gc.h>
    #include <ocaml_runtime/memory.h>
    #include <stdio.h>
}

#define NUM_LIST(code) \
    code(1) code(2) code(3) code(4) code(5) code(6) code(7) code(8) code(9) code(10) code(11) code(12) code(13) code(14) code(15) code(16) code(17) code(18) code(19) code(20) code(21) code(22) code(23) code(24) code(25) code(26) code(27) code(28) code(29) code(30) code(31) code(32) code(33) code(34) code(35) code(36) code(37) code(38) code(39) code(40) code(41) code(42) code(43) code(44) code(45) code(46)

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

    value Env = 0;
    void setEnv(value E) {
        //printf("IN SETENVVV\n\n");
        Env = E;
    }
    value getEnv() {
        //printf("IN GETENVVV, ENV = %p\n\n", (void*)Env);
        return Env;
    }

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
    // -----------------------------
    // |CodePtr|Vars|Args|NbRemArgs|
    // -----------------------------
    //
    // It is compatible with the regular ocaml runtime's closure layout 

    value makeClosure(value NVars, value FPtr, value NbArgs) {
        //printf("INTO MAKECLOSURE\n");
        //printf("NB ARGS = %ld\n", NbArgs);
        value Closure;
        int BlockSize = 3 + NVars + NbArgs;
        Alloc_small(Closure, BlockSize, Closure_tag);
        // Set the code pointer
        Code_val(Closure) = (code_t)FPtr;
        // Set the NbRemArgs to the total nb of args
        Field(Closure, BlockSize - 1) = NbArgs;
        return Closure;
    }

    void closureSetVar(value Closure, value VarIdx, value Value) {
        //printf("INTO CLOSURESETVAR\n\n");
        Field(Closure, VarIdx + 1) = Value;
    }

    value apply(value Closure, int NbArgs, value* Args) {

        int ArgsSize = NbArgs;
        value CClosure = Closure;

        //printf("INTO APPLY\n\n");

        int i = 1;
        while (NbArgs > 0) {
            int Size = Wosize_val(CClosure);
            //printf("LOOP NUMBER %d, NBargs = %d\n\n", i, NbArgs);
            int NbRemArgs = Field(CClosure, (Size - 1));
            //printf("NBREMARGS FOR THIS CLOSURE : %d\n", NbRemArgs);

            while (NbRemArgs > 0 && NbArgs > 0) {
                Field(CClosure, (Size - 1 - NbRemArgs)) = 
                    (value)Args[ArgsSize - NbArgs];
                NbRemArgs--; NbArgs--;
            }

            if (NbRemArgs > 0) {
                Field(CClosure, (Size - 1)) = NbRemArgs;
                return CClosure;
            }

            value (*FPtr)(value) = (value(*)(value)) Code_val(CClosure);
            value OldEnv = Env;
            Env = CClosure;
            CClosure = FPtr(CClosure);
            Env = OldEnv;
            i++;
        }

        return CClosure;
    }

    value getBlockSize(value Block) { return Wosize_val(Block); }
    value getField(value Block, value Idx) { return Field(Block, Idx); }

    void setField(value Block, int Idx, value Val) {
        Field(Block, Idx) = (value)Val;
    }

}
