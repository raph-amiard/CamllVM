OBJ=obj
SRC=src
Z3INCLUDE=include
LIBPATH=ocaml-3.12.1/byterun
LIBNAME=camlrund
BIN=bin

CCFLAGS= -g -Wall -Wextra -Wno-unused-parameter -I${Z3INCLUDE} -std=c++0x
CCFINALFLAGS = -L${LIBPATH} -l${LIBNAME}
CC=clang++ ${CCFLAGS} `llvm-config --cppflags` 
STDLIBCC=clang++ -O3 -Wall -Wextra -Wno-unused-parameter -I${Z3INCLUDE} -std=c++0x

all: main

ocaml_runtime:
	cd ocaml-3.12.1/byterun && make && make libcamlrund.a && rm main.d.o;

context:
	${CC} -c -o ${OBJ}/Context.o ${SRC}/Context.cpp 

instructions:
	${CC} -c -o ${OBJ}/Instructions.o ${SRC}/Instructions.cpp 

codegen:
	${CC} -c -o ${OBJ}/CodeGen.o ${SRC}/CodeGen.cpp 

stdlib:
	${STDLIBCC} -S -emit-llvm -o ${BIN}/StdLib.ll ${SRC}/StdLib.cpp

main: instructions context codegen ocaml_runtime stdlib
	${CC} -rdynamic -L${LIBPATH} -o ${BIN}/Z3 ${SRC}/main.cpp ${OBJ}/CodeGen.o ${OBJ}/Instructions.o ${OBJ}/Context.o ${LIBPATH}/*.d.o ${LIBPATH}/prims.o -lcurses `llvm-config --ldflags --libs bitreader asmparser core jit native ipo`

clean:
	rm ${OBJ}/* -rf;

cleanall: clean
	cd ocaml-3.12.1 && make clean;
