VMKIT_INSTALL_ROOT_DIR ?= ${HOME}/extra/vmkit/vmkit-0.30.opt.install
VMKIT_INCLUDE_DIR = $(VMKIT_INSTALL_ROOT_DIR)/include
VMKIT_LIB_DIR = $(VMKIT_INSTALL_ROOT_DIR)/lib
VMKIT_OBJ_ROOT_DIR ?= ${HOME}/extra/vmkit/vmkit-0.30.opt.obj
VMKIT_A_DIR = $(VMKIT_OBJ_ROOT_DIR)/Release/lib
VMKIT_INSTALL_SRC_DIR ?= ${HOME}/extra/vmkit/vmkit-0.30

USEDLIBS = Classpath.a J3.a J3Compiler.a Mvm.a MvmCompiler.a CommonThread.a FinalMMTk.a InlineMMTk.a Precompiled.a BootstrapClasses.a

all: correct

correct: /tmp/MyMain.o
	clang++ -g `llvm-config --cxxflags` -fno-exceptions -Wno-variadic-macros -fno-omit-frame-pointer -fno-strict-aliasing -Wno-deprecated -ansi -fno-rtti  -O3 -o a.out /tmp/MyMain.o -L$(VMKIT_A_DIR) -lClasspath -lJ3 -lJ3Compiler -lMvm -lMvmCompiler -lCommonThread -lFinalMMTk -lInlineMMTk -lPrecompiled -lBootstrapClasses -lMMTk `llvm-config --libs --ldflags  all` -lz

#$(VMKIT_LIB_DIR)/MMTKRuntime.so
#$(VMKIT_LIB_DIR)/MMTKMagic.so
a.out: Main.o
	clang++ -o a.out Main.o  `llvm-config --ldflags --libs all`

#-DNDEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual
# -fno-exceptions -Wno-variadic-macros -fno-omit-frame-pointer -fno-strict-aliasing -Wno-deprecated -ansi -fno-rtti  -O3
# -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings 
b.out: Main.o
	clang++ -g `llvm-config --cxxflags` -fno-exceptions -Wno-variadic-macros -fno-omit-frame-pointer -fno-strict-aliasing -Wno-deprecated -ansi -fno-rtti  -O3 -o a.out Main.o -L$(VMKIT_A_DIR) -lClasspath -lJ3 -lJ3Compiler -lMvm -lMvmCompiler -lCommonThread -lFinalMMTk -lInlineMMTk -lPrecompiled -lBootstrapClasses -lMMTk `llvm-config --libs --ldflags  all` -lz


#-DNDEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual
#-fno-exceptions -Wno-variadic-macros -fno-omit-frame-pointer -fno-strict-aliasing -Wno-deprecated -ansi -fno-rtti   
#-pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings

Main.o: Main.cpp FrametablesExterns.inc FrametablesSymbols.inc
	clang++ -g -c -I"$(VMKIT_INCLUDE_DIR)" -I"$(VMKIT_INSTALL_SRC_DIR)/lib/Mvm/MMTk" `llvm-config --cxxflags` $< -o $@


FrametablesExterns.inc FrametablesSymbols.inc: $(VMKIT_OBJ_ROOT_DIR)/tools/j3/FrametablesExterns.inc $(VMKIT_OBJ_ROOT_DIR)/tools/j3/FrametablesSymbols.inc
	cp $^ .

clean:
	rm -f a.out Main.o FrametablesExterns.inc FrametablesSymbols.inc

#-I/home/multani/extra/vmkit/vmkit-0.30.opt.obj/tools/jouet -I/home/multani/extra/vmkit/vmkit-0.30/tools/jouet
/tmp/MyMain.o: Main.cpp FrametablesExterns.inc FrametablesSymbols.inc
	rm -f /tmp/MyMain.o /tmp/MyMain.d.tmp /tmp/MyMain.d
	clang++ -I/home/multani/extra/llvm/llvm-3.0.opt.obj/include -I/home/multani/extra/llvm/llvm-3.0.src/include -I/home/multani/extra/vmkit/vmkit-0.30.opt.obj/../vmkit-0.30/include -I/home/multani/extra/vmkit/vmkit-0.30.opt.obj/include   -DNDEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -I/home/multani/extra/vmkit/vmkit-0.30.opt.obj/../vmkit-0.30/lib/Mvm/MMTk -fno-exceptions -Wno-variadic-macros -fno-omit-frame-pointer -fno-strict-aliasing -Wno-deprecated -ansi -fno-rtti    -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings  -c -MMD -MP -MF "/tmp/MyMain.d.tmp" -MT "/tmp/MyMain.o" -MT "/tmp/MyMain.d" Main.cpp -o /tmp/MyMain.o
	mv /tmp/MyMain.d.tmp /tmp/MyMain.d
	cp /tmp/MyMain.o Main.o 


#-I/home/multani/extra/vmkit/vmkit-0.30.opt.obj/tools/jouet -I/home/multani/extra/vmkit/vmkit-0.30/tools/jouet
#-Wl,-R -Wl,'$ORIGIN/../lib' -Wl,-R -Wl,/home/multani/extra/vmkit/vmkit-0.30.opt.obj/Release/bin -Wl,-export-dynamic
#-lJouet
z: /tmp/MyMain.o
	clang++ -I/home/multani/extra/llvm/llvm-3.0.opt.obj/include -I/home/multani/extra/llvm/llvm-3.0.src/include -I/home/multani/extra/vmkit/vmkit-0.30.opt.obj/../vmkit-0.30/include -I/home/multani/extra/vmkit/vmkit-0.30.opt.obj/include -DNDEBUG -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -O3 -fomit-frame-pointer -fno-exceptions -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual -I/home/multani/extra/vmkit/vmkit-0.30.opt.obj/../vmkit-0.30/lib/Mvm/MMTk -fno-exceptions -Wno-variadic-macros -fno-omit-frame-pointer -fno-strict-aliasing -Wno-deprecated -ansi -fno-rtti  -O3  -L/home/multani/extra/vmkit/vmkit-0.30.opt.obj/Release/lib -L/home/multani/extra/llvm/llvm-3.0.opt.obj/Release/lib    -pedantic -Wno-long-long -Wall -W -Wno-unused-parameter -Wwrite-strings   -o d.out /tmp/MyMain.o -lClasspath -lJ3 -lJ3Compiler -lMvm -lMvmCompiler -lCommonThread -lFinalMMTk -lInlineMMTk -lPrecompiled -lBootstrapClasses -lLLVMipo -lLLVMInstrumentation -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Info -lLLVMX86AsmPrinter -lLLVMX86Utils -lLLVMSelectionDAG -lLLVMAsmPrinter -lLLVMMCParser -lLLVMJIT -lLLVMExecutionEngine -lLLVMCodeGen -lLLVMScalarOpts -lLLVMInstCombine -lLLVMTransformUtils -lLLVMipa -lLLVMAnalysis -lLLVMTarget -lLLVMMC -lLLVMCore -lLLVMSupport   -lpthread -ldl -lm  -lz