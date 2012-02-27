OBJ=obj
dummy := $(shell test -d ${OBJ} || mkdir ${OBJ})
SRC=src
Z3INCLUDE=include
OCAMLPATH=ocaml-3.12.1
LIBPATH=${OCAMLPATH}/byterun
LIBNAME=camlrund
BIN=bin
dummy := $(shell test -d ${BIN} || mkdir ${BIN})

CCFLAGS= -g -Wall -Wextra -Wno-unused-parameter -I${Z3INCLUDE} -std=c++0x -lboost_program_options
CCFINALFLAGS = -L${LIBPATH} -l${LIBNAME}
CC=clang++ ${CCFLAGS} `llvm-config --cppflags` 
CSTDLIBCC=clang -O3 -Wall -Wextra -Wno-unused-parameter -I${Z3INCLUDE}

OBJECTS=$(OBJ)/Context.o $(OBJ)/GenBlock.o $(OBJ)/GenFunction.o $(OBJ)/GenModule.o $(OBJ)/GenModuleCreator.o $(OBJ)/Instructions.o

all: main

ocaml_runtime: ${OCAMLPATH}/config/Makefile ${OCAMLPATH}/config/m.h ${OCAMLPATH}/config/s.h
	cd ${LIBPATH} && make && make libcamlrund.a && rm main.d.o;

${OCAMLPATH}/config/Makefile ${OCAMLPATH}/config/m.h ${OCAMLPATH}/config/s.h:
	cd ${OCAMLPATH} && ./configure

${OBJ}/%.o: ${SRC}/%.cpp ${OCAMLPATH}/config/m.h ${OCAMLPATH}/config/s.h
	${CC} -c -o $@ $<

stdlib:
	${CSTDLIBCC} -S -emit-llvm -o ${BIN}/StdLib.ll ${SRC}/CStdLib.c

main: $(OBJECTS) ocaml_runtime stdlib
	${CC} -rdynamic -L${LIBPATH} -o ${BIN}/Z3 ${SRC}/main.cpp $(OBJECTS) ${LIBPATH}/*.d.o ${LIBPATH}/prims.o -lcurses `llvm-config --ldflags --libs bitreader asmparser core jit native ipo`


clean:
	rm ${OBJ}/* -rf;

cleanall: clean
	cd ${OCAMLPATH} && make clean;

