# Copyright Lazar Cristian-Stefan 314CA 2022-2023
CC=gcc
CFLAGS=-Wall -Wextra -std=c99
DEPS= vma.h utils.h struct.h rw.h mprotect.h free.h alloc.h

OBJ= main.o vma.o alloc.o free.o mprotect.o rw.o

build: $(DEPS) vma

vma: $(OBJ)
	$(CC) -g -o vma $^ $(CFLAGS) -lm

%.o: %.c
	$(CC) -g -c -o $@ $< $(CFLAGS) -lm

.PHONY: clean

run_vma:
	./vma

clean:
	rm -f *.o vma