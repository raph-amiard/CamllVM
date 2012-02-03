OBJ=obj
SRC=src
INCLUDE=include
BIN=bin

CCFLAGS= -I${INCLUDE} -std=c++0x
CC=clang++ ${CCFLAGS}

zamreader:
	${CC} -c -o ${OBJ}/ZamReader.o ${SRC}/ZamReader.cpp 

zamreadertest: zamreader
	${CC} -o ${BIN}/ZamReaderTest ${SRC}/Test/ZamReaderTest.cpp ${OBJ}/ZamReader.o
