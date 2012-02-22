OBJ=obj
SRC=src
Z3INCLUDE=include
OCAMLPATH=ocaml-3.12.1
LIBPATH=${OCAMLPATH}/byterun
LIBNAME=camlrund
BIN=bin

CCFLAGS= -g -Wall -Wextra -Wno-unused-parameter -I${Z3INCLUDE} -std=c++0x
CCFINALFLAGS = -L${LIBPATH} -l${LIBNAME}
CC=clang++ ${CCFLAGS} `llvm-config --cppflags` 
STDLIBCC=clang++ -O3 -Wall -Wextra -Wno-unused-parameter -I${Z3INCLUDE} -std=c++0x

all: main

ocaml_runtime: ${OCAMLPATH}/config/Makefile
	cd ${LIBPATH} && make && make libcamlrund.a && rm main.d.o;

${OCAMLPATH}/config/Makefile:
	cd ${OCAMLPATH} && ./configure

${OBJ}/%.o: ${SRC}/%.cpp
	${CC} -c -o $@ $<

${BIN}/StdLib.ll:
	${STDLIBCC} -S -emit-llvm -o ${BIN}/StdLib.ll ${SRC}/StdLib.cpp

main: ${OBJ}/Instructions.o ${OBJ}/Context.o ${OBJ}/CodeGen.o ocaml_runtime ${BIN}/StdLib.ll
	${CC} -rdynamic -L${LIBPATH} -o ${BIN}/Z3 ${SRC}/main.cpp ${OBJ}/CodeGen.o ${OBJ}/Instructions.o ${OBJ}/Context.o ${LIBPATH}/*.d.o ${LIBPATH}/prims.o -lcurses `llvm-config --ldflags --libs bitreader asmparser core jit native ipo`


clean:
	rm ${OBJ}/* -rf;

cleanall: clean
	cd ${OCAMLPATH} && make clean;

