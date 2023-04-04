CC=gcc
CFLAGS=-Wall -Wextra -std=c99
DEPS=

OBJ += alloc.o main.o free.o mprotect.o rw.o pmap.o

%.o: %.c
	$(CC) -g -c -o $@ $< $(CFLAGS) -lm

build: $(OBJ) $(DEPS)
	$(CC) -g -o vma $^ $(CFLAGS) -lm

vma: build

.PHONY: clean

run_vma:
	./vma

clean:
	rm -f *.o vma