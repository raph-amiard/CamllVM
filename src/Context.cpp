#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <string.h>
#include <iostream>

extern "C" {
    #include <ocaml_runtime/config.h>
    #include <ocaml_runtime/exec.h>
    #include <ocaml_runtime/startup.h>
    #include <ocaml_runtime/dynlink.h>
    #include <ocaml_runtime/memory.h>
    #include <ocaml_runtime/fix_code.h>
    #include <ocaml_runtime/io.h>
    #include <ocaml_runtime/intext.h>
    #include <ocaml_runtime/gc_ctrl.h>
    #include <ocaml_runtime/stacks.h>
    #include <ocaml_runtime/custom.h>
    #include <ocaml_runtime/misc.h>
    #include <ocaml_runtime/fail.h>
}


#include <Context.hpp>
#include <Instructions.hpp>
#include <CodeGen.hpp>

using namespace std;

static uintnat percent_free_init = Percent_free_def;
static uintnat max_percent_free_init = Max_percent_free_def;
static uintnat minor_heap_init = Minor_heap_def;
static uintnat heap_chunk_init = Heap_chunk_def;
static uintnat heap_size_init = Init_heap_def;
static uintnat max_stack_init = Max_stack_def;

void Context::init(string _FileName) {

    cout << "LOL?" << endl;

    char * shared_lib_path, * shared_libs, * req_prims;
    struct exec_trailer Trail;
    struct channel * chan;
    int Fd;

    caml_init_custom_operations();
    caml_ext_table_init(&caml_shared_libs_path, 8);
    caml_external_raise = NULL;

    // Open file
    FileName = _FileName;
    char* CStrFileName = new char[FileName.length() + 1];
    strcpy(CStrFileName, FileName.c_str());
    Fd = caml_attempt_open(&CStrFileName, &Trail, 1);

    // Read section descriptors
    caml_read_section_descriptors(Fd, &Trail);

    /* Initialize the abstract machine */
    caml_init_gc (minor_heap_init, heap_size_init, heap_chunk_init,
                percent_free_init, max_percent_free_init);
    caml_init_stack (max_stack_init);
    init_atoms();

    /* Load the code */
    caml_code_size = caml_seek_section(Fd, &Trail, (char*)"CODE");
    caml_load_code(Fd, caml_code_size);

    /* Build the table of primitives */
    shared_lib_path = read_section(Fd, &Trail, (char*)"DLPT");
    shared_libs = read_section(Fd, &Trail, (char*)"DLLS");
    req_prims = read_section(Fd, &Trail, (char*)"PRIM");
    if (req_prims == NULL) caml_fatal_error((char*)"Fatal error: no PRIM section\n");
    caml_build_primitive_table(shared_lib_path, shared_libs, req_prims);
    caml_stat_free(shared_lib_path);
    caml_stat_free(shared_libs);
    caml_stat_free(req_prims);

    /* Load the globals */
    caml_seek_section(Fd, &Trail, (char*)"DATA");
    chan = caml_open_descriptor_in(Fd);
    caml_global_data = caml_input_val(chan);
    caml_close_channel(chan); /* this also closes Fd */
    caml_stat_free(Trail.section);

    std::vector<ZInstruction> Instructions;
    readInstructions(Instructions, caml_start_code, caml_code_size);
    convertCodeOffsets(Instructions);
    annotateNodes(Instructions);

    GenModuleCreator GMC(&Instructions);
    Instructions.erase(Instructions.begin() + 955, Instructions.begin() + 965);
    printInstructions(Instructions, true, 941); // 941 is just the number of the instruction at which the program normally begins when compiled with ocamlc

    GenModule* Mod = GMC.generate(941);
    Mod->Print();

    cout << "AFTER MOD PRINT\n";

    auto MainFunc = Mod->MainFunction;
    MainFunc->CodeGen();
    for (auto FuncP : Mod->Functions)
        FuncP.second->LlvmFunc->dump();
    MainFunc->LlvmFunc->dump();

    Mod->FPM->run(*MainFunc->LlvmFunc);
    MainFunc->LlvmFunc->dump();

    void *FPtr = Mod->ExecEngine->getPointerToFunction(MainFunc->LlvmFunc);

    char* (*FP)() = (char* (*)())(intptr_t)FPtr;
    int a = (int) FP();
    cout << "A = " << a << endl;

    cout << "WEREATHEND" << endl;

    

}

#endif
