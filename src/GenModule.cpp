#include <CodeGen.hpp>
#include <Utils.hpp>

#include "llvm/Support/IRReader.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/LLVMContext.h"

using namespace std;
using namespace llvm;

// ================ GenModule Implementation ================== //

GenModule::GenModule() {

    InitializeNativeTarget();
    SMDiagnostic Diag;
    auto StdLibPath = getExecutablePath();
    StdLibPath.append("StdLib.ll");
    cout << StdLibPath << endl;
    TheModule = ParseIRFile(StdLibPath, Diag, getGlobalContext()); //new Module("testmodule", getGlobalContext());
    for (Function& Func : TheModule->getFunctionList()) {
        if (Func.getName() == "makeClosure") {
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
    cout << " ============= Functions ============ " << endl << endl;
    for (auto FuncP : Functions) {
        cout << "Function " << FuncP.first << " (";
        for (int i = 0; i < FuncP.second->Arity; i++) {
            cout << "arg" << i;
            if (i < FuncP.second->Arity - 1) cout << ", ";
        }
        cout << ") { " << endl;
        FuncP.second->Print();
        cout << "}" << endl;
    }
    cout << "Main Function { " << endl;
    MainFunction->Print();
    cout << "}\n";
}
