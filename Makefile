CC=gcc
SOURCE=p2.c
EXECUTABLE=p2
CFLAGS=-g -std=c11 -Wall -Wextra -pedantic -pthread

all: clean
	${CC} ${CFLAGS} ${SOURCE} -o ${EXECUTABLE}

run: all
	./${EXECUTABLE} ${args}

clean:
	rm -rf ${EXECUTABLE}