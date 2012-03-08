VMKIT_INSTALL_ROOT_DIR ?= ${HOME}/extra/vmkit/vmkit-0.30.opt.install
VMKIT_INCLUDE_DIR = "$(VMKIT_INSTALL_ROOT_DIR)/include"
VMKIT_LIB_DIR = "$(VMKIT_INSTALL_ROOT_DIR)/lib"
VMKIT_OBJ_ROOT_DIR ?= ${HOME}/extra/vmkit/vmkit-0.30.opt.obj
VMKIT_A_DIR = "$(VMKIT_OBJ_ROOT_DIR)/Release/lib"

USEDLIBS = Classpath.a J3.a J3Compiler.a Mvm.a MvmCompiler.a CommonThread.a FinalMMTk.a InlineMMTk.a Precompiled.a BootstrapClasses.a

all: a.out

#$(VMKIT_LIB_DIR)/MMTKRuntime.so
#$(VMKIT_LIB_DIR)/MMTKMagic.so
a.out: Main.o
	clang++ -o a.out Main.o  `llvm-config --ldflags --libs all`

b.out: Main.o
	clang++ -o a.out Main.o -L$(VMKIT_A_DIR) -lClasspath -lJ3 -lJ3Compiler -lMvm -lMvmCompiler -lCommonThread -lFinalMMTk -lInlineMMTk -lPrecompiled -lBootstrapClasses `llvm-config --ldflags --libs all`


Main.o: Main.cpp
	clang++ -c -I$(VMKIT_INCLUDE_DIR) Main.cpp `llvm-config --cxxflags`

clean:
	rm -f a.out Main.o