CC=gcc
CFLAGS=-Wall -Wextra -std=c99
DEPS=

OBJ += main.o vma.o alloc.o free.o mprotect.o

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