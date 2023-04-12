// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "struct.h"
#include "utils.h"
#include "vma.h"

// Checks if the given command is $string, and makes $car $c if so.
#define INTERPRET(cmd, string, c)			\
	do {									\
		if (strcmp(cmd, string) == 0)		\
			car = c;						\
	} while (0)

// Interprets the first argument of the command.
char interpret_cmd(char cmd[15])
{
	char car = '0';
	INTERPRET(cmd, "ALLOC_ARENA", 'a');
	INTERPRET(cmd, "DEALLOC_ARENA", 'b');
	INTERPRET(cmd, "ALLOC_BLOCK", 'c');
	INTERPRET(cmd, "FREE_BLOCK", 'd');
	INTERPRET(cmd, "READ", 'e');
	INTERPRET(cmd, "WRITE", 'f');
	INTERPRET(cmd, "PMAP", 'g');
	INTERPRET(cmd, "MPROTECT", 'h');
	return car;
}

int main(void)
{
	arena_t *arena = NULL;
	char *cmd;
	int8_t *data;
	uint64_t size, address;

	// The program loops until the "DEALLOC_ARENA" command is given.
	while (1) {
		// Reads the command.
		scanf("%ms", &cmd);
		// Interprets the command.
		switch (interpret_cmd(cmd)) {
		case 'a':
			// Checks of the arena was already allocated.
			if (arena) {
				puts("Arena already allocated.");
				break;
			}
			if (scanf("%lu", &size) != 1) {
				puts("Invalid command. Please try again.");
				break;
			}
			arena = alloc_arena(size);
			break;
		case 'b':
			if (arena)
				dealloc_arena(arena);
			free(cmd);
			return 0;
		case 'c':
			if (scanf("%lu%lu", &address, &size) != 2) {
				puts("Invalid command. Please try again.");
				break;
			}
			alloc_block(arena, address, size);
			break;
		case 'd':
			if (scanf("%lu", &address) != 1) {
				puts("Invalid command. Please try again.");
				break;
			}
			free_block(arena, address);
			break;
		case 'e':
			if (scanf("%lu%lu", &address, &size) != 2) {
				puts("Invalid command. Please try again.");
				break;
			}
			read(arena, address, size);
			break;
		case 'f':
			if (scanf("%lu%lu", &address, &size) != 2) {
				puts("Invalid command. Please try again.");
				break;
			}
			data = malloc(size * sizeof(int8_t));
			DIE(!data, "Malloc failed.\n");
			// This fseek call is used in order skip over the blank space before
			// the string of data starts.
			fseek(stdin, 1, SEEK_CUR);
			fread(data, sizeof(int8_t), size, stdin);
			write(arena, address, size, data);
			break;
		case 'g':
			pmap(arena);
			break;
		case 'h':
			if (scanf("%lu", &address) != 1) {
				puts("Invalid command. Please try again.");
				break;
			}
			scanf("%m[^\n]", &data);
			mprotect(arena, address, data);
			break;
		case '0':
			puts("Invalid command. Please try again.");
			break;
		}
		free(cmd);
	}
}
