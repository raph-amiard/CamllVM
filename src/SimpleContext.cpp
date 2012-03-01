#include <Context.hpp>

#include "llvm/LLVMContext.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/IRReader.h"

void SimpleContext::generateMod() {
  std::cout << "SimpleContext::generateMod" << std::endl;
}

void SimpleContext::compile() {
    std::cout << "SimpleContext::compile: '" << Instructions.size()
              << "' instructions" << std::endl;
}

void SimpleContext::exec() {
    std::cout << "SimpleContext::exec" << std::endl;

    llvm::InitializeNativeTarget();

    llvm::SMDiagnostic Diag;
    llvm::Module *lib_module = llvm::ParseIRFile("bin/ZamSimpleInterpreter.ll", Diag, llvm::getGlobalContext());

    std::string some_msg_error;
    llvm::ExecutionEngine *main_engine = llvm::EngineBuilder(lib_module).setErrorStr(&some_msg_error).create();
    if (!main_engine) {
        std::cerr << "Could not create the EngineBuilder for the SimpleContext execution: " << some_msg_error << std::endl;
        std::exit(1);
    }

    llvm::Function *run_funtion = nullptr;
    for (llvm::Function& a_fun : lib_module->getFunctionList()) {
        //std::cerr << a_fun.getName().data() << std::endl;
        //a_fun.dump();
        //std::cerr << "----" << std::endl;

        /* BUG : finding the 'run' function must be better than this
           in particular see the following 'main_engine->FindFunctionNamed("exit");'
         */
        std::size_t found = a_fun.getName().find("zsi3run");
        if ((found != std::string::npos) && (run_funtion == nullptr)) {
            run_funtion = &a_fun;
        }
    }

    /* BUG : make this function work */
    /*
    llvm::Function *run_funtion = main_engine->FindFunctionNamed("run");
    */

    if (run_funtion == nullptr) {
        std::cerr << "The 'run' function was not found inside ZamSimpleInterpreter.ll" << std::endl;
        std::exit(1);
    }

    // JIT the function, returning a function pointer.
    void *FPtr = main_engine->getPointerToFunction(run_funtion);
    // Cast it to the right type
    void (*FP)( std::vector< ZInstruction* > * ) =
        (void (*) (std::vector< ZInstruction* > *)) (intptr_t)FPtr;
    FP(&Instructions);
    assert(false); // can not happend as long as the 'run' function call 'exit'


}

