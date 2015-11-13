CFLAGS=-O2 -Wall
CC=gcc
LIB=git2
LIB_PATH=/usr/local/lib
OBJ=log.o common.o
PROG=log

all: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJ) -L$(LIB_PATH) -l$(LIB)

common.o: common.h

.PHONY: clean
clean:
	rm -f $(OBJ) $(PROG)
