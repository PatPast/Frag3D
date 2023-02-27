CC=gcc
FLAGS=-Wall -g

BIN_DIR=./bin
DOC_DIR=./doc
LIB_DIR=./lib
SRC_DIR=./src
TEST_DIR=./test

SDL_DIR=${HOME}/SDL2
LIB_DIR=/usr/lib/x86_64-linux-gnu
INC_DIR=/usr/include/

LIBS=-L${LIB_DIR} -ldl -lSDL2 -lGL  -lm 
INCS=-I${INC_DIR} -I./inc

PROG=Frag3D

all: ${BIN_DIR}/${PROG} 

${BIN_DIR}/${PROG}: ${SRC_DIR}/*.c 
	${CC} -o $@ $^ ${LIBS} ${INCS} ${FLAGS}

clean:
	rm -f ${PROG}
	rm -f *.o
