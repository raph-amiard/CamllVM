OBJ=obj
SRC=src
Z3INCLUDE=include
LIBPATH=ocaml-3.12.1/byterun
LIBNAME=camlrund
BIN=bin

CCFLAGS= -g -I${Z3INCLUDE} -std=c++0x
CCFINALFLAGS = -L${LIBPATH} -l${LIBNAME}
CC=clang++ ${CCFLAGS} `llvm-config --cppflags` 

all: main

ocaml_runtime:
	cd ocaml-3.12.1/byterun && make && make libcamlrund.a && rm main.d.o;

zamreader: marshal
	${CC} -c -o ${OBJ}/ZamReader.o ${SRC}/ZamReader.cpp 

marshal:
	${CC} -c -o ${OBJ}/Marshal.o ${SRC}/Marshal.cpp

zamreadertest: zamreader ocaml_runtime
	${CC} ${CCFINALFLAGS} -o ${BIN}/ZamReaderTest ${SRC}/Test/ZamReaderTest.cpp ${OBJ}/ZamReader.o ${OBJ}/Marshal.o ${LIBPATH}/prims.o

context:
	${CC} -c -o ${OBJ}/Context.o ${SRC}/Context.cpp 

instructions:
	${CC} -c -o ${OBJ}/Instructions.o ${SRC}/Instructions.cpp 

codegen:
	${CC} -c -o ${OBJ}/CodeGen.o ${SRC}/CodeGen.cpp 

main: instructions context codegen ocaml_runtime 
	${CC} -rdynamic -L${LIBPATH} -o ${BIN}/Z3 ${SRC}/main.cpp ${OBJ}/CodeGen.o ${OBJ}/Instructions.o ${OBJ}/Context.o ${LIBPATH}/*.d.o ${LIBPATH}/prims.o -lcurses `llvm-config --ldflags --libs core jit native`

clean:
	rm ${OBJ}/* -rf;

cleanall: clean
	cd ocaml-3.12.1 && make clean;
