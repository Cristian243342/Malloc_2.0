// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "vma.h"
#include "rw.h"
#include "alloc.h"
#include "free.h"
#include "mprotect.h"
#include "struct.h"
#include "utils.h"

// Initializes an arena with a given size.
arena_t *alloc_arena(const uint64_t size)
{
	arena_t *arena = malloc(sizeof(arena_t));
	DIE(!arena, "Malloc failed\n");
	arena->arena_size = size;
	arena->alloc_list = NULL;
	arena->blocks = 0;
	arena->mblocks = 0;
	arena->used_mem = 0;
	return arena;
}

// Frees all memory allocated in the arena, as well as the arena itself.
void dealloc_arena(arena_t *arena)
{
	list_t *curr = arena->alloc_list;
	if (curr) {
		while (curr->next) {
			curr = curr->next;
			free_block_dealloc(curr->prev);
		}
		free_block_dealloc(curr);
	}
	free(arena);
}

// Allocates a new block of the given size (in bytes).
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
	// Checks if the arena is initialized.
	if (!arena) {
		puts("Arena not allocated");
		return;
	}

	// Check if the address is outside the arena bounds.
	if (address >= arena->arena_size) {
		puts("The allocated address is outside the size of arena");
		return;
	}

	// Checks if the end address is past the size of the arena.
	if (address > arena->arena_size - size) {
		puts("The end address is past the size of the arena");
		return;
	}

	list_t *curr = arena->alloc_list;
	if (curr) {
		// The block list is sequenced through, untill the upper bound to the
		// address is found.
		for (; curr->next; curr = curr->next)
			if (address < ((block_t *)curr->data)->start_address)
				break;

		if (check_aliniation(arena, address, size, curr))
			return;
	}

	alloc_new_block(arena, address, size, curr);
}

// Frees the block found at the given address.
void free_block(arena_t *arena, const uint64_t address)
{
	// Checks if the arena is initialized.
	if (!arena) {
		puts("Arena not allocated.");
		return;
	}
	// Checks if no block were allocated.
	if (!arena->alloc_list) {
		puts("Invalid address for free.");
		return;
	}

	list_t *curr_prim = arena->alloc_list;
	// Searches the block list for the block that contains the address.
	for (; curr_prim->next; curr_prim = curr_prim->next)
		if (address < ((block_t *)curr_prim->data)->start_address +
			((block_t *)curr_prim->data)->size &&
			address >= ((block_t *)curr_prim->data)->start_address)
			break;

	// Checks if the address is allocated (if a block containing it was found).
	if (!curr_prim) {
		puts("Invalid address for free.");
		return;
	}

	list_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;

	// Searches the miniblock list for the miniblock with the given start
	// address.
	for (; curr_sec; curr_sec = curr_sec->next)
		if (((miniblock_t *)curr_sec->data)->start_address == address)
			break;

	// Check if the given address is the start address of a miniblock.
	if (!curr_sec) {
		puts("Invalid address for free.");
		return;
	}

	arena->mblocks--;
	arena->used_mem -= ((miniblock_t *)curr_sec->data)->size;

	// Checks if the miniblock is at the head of the list.
	if (!curr_sec->prev) {
		// Checks if the miniblock is the only one in the list.
		if (!curr_sec->next) {
			arena->blocks--;
			free_mblock(curr_sec);
			if (curr_prim->prev)
				curr_prim->prev->next = curr_prim->next;
			else
				arena->alloc_list = curr_prim->next;
			if (curr_prim->next)
				curr_prim->next->prev = curr_prim->prev;
			free(curr_prim->data);
			free(curr_prim);
			return;
		}
		((block_t *)curr_prim->data)->size -=
				((miniblock_t *)curr_sec->data)->size;
		((block_t *)curr_prim->data)->miniblock_list = curr_sec->next;
		((block_t *)curr_prim->data)->start_address =
				((miniblock_t *)curr_sec->next->data)->start_address;
		curr_sec->next->prev = NULL;
		free_mblock(curr_sec);
		return;
	}

	// Checks if the miniblock is at the tail of the list.
	if (!curr_sec->next) {
		((block_t *)curr_prim->data)->size -=
			((miniblock_t *)curr_sec->data)->size;
		curr_sec->prev->next = NULL;
		free_mblock(curr_sec);
		return;
	}

	split_block(arena, curr_prim, curr_sec);
}

// Reads size bytes from the given address.
void read(arena_t *arena, uint64_t address, uint64_t size)
{
	// Checks if the arena is initialized.
	if (!arena) {
		puts("Arena not allocated.");
		return;
	}

	// Checks if no block were allocated.
	if (!arena->alloc_list) {
		puts("Invalid address for read.");
		return;
	}

	list_t *curr_prim = arena->alloc_list;

	// Searches the block list for the block that contains the address.
	for (; curr_prim; curr_prim = curr_prim->next)
		if (address < ((block_t *)curr_prim->data)->start_address +
			((block_t *)curr_prim->data)->size &&
			address >= ((block_t *)curr_prim->data)->start_address)
			break;

	// Checks if the address is allocated (if a block containing it was found).
	if (!curr_prim) {
		puts("Invalid address for read.");
		return;
	}

	list_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;

	// Searches the miniblock list for the miniblock containing the address.
	for (; curr_sec; curr_sec = curr_sec->next)
		if (address < ((miniblock_t *)curr_sec->data)->start_address +
			((miniblock_t *)curr_sec->data)->size)
			break;

	miniblock_t *block = ((miniblock_t *)curr_sec->data);

	// Calculates the offset from the start address of the block and the given
	// address.
	uint64_t offset_prim = address -
		((block_t *)curr_prim->data)->start_address;

	// Calculates the offset from the start address of the miniblock and the
	// given address.
	uint64_t offset_sec = address - block->start_address;

	// Copies the value of size in a mutable variable.
	uint64_t mut_size = size;

	// Checks if the given size is greater than the size of the block from the
	// given address.
	if (((block_t *)curr_prim->data)->size - offset_prim < mut_size) {
		mut_size = ((block_t *)curr_prim->data)->size - offset_prim;
		printf("Warning: size was bigger than the block size. Reading %lu "
			   "characters.\n", mut_size);
	}

	if (!check_perm_read(mut_size, curr_sec, offset_sec))
		return;

	read_from_block(mut_size, curr_sec, offset_sec);
}

// Writes size bytes at the given address.
void write(arena_t *arena, const uint64_t address, const uint64_t size,
		   int8_t *data)
{
	// Check if the arena is initialized.
	if (!arena) {
		puts("Arena not allocated.");
		return;
	}

	// Checks if no block were allocated.
	if (!arena->alloc_list) {
		free(data);
		puts("Invalid address for write.");
		return;
	}

	list_t *curr_prim = arena->alloc_list;

	// Searches the block list for the block containing the given address.
	for (; curr_prim; curr_prim = curr_prim->next)
		if (address < ((block_t *)curr_prim->data)->start_address +
			((block_t *)curr_prim->data)->size &&
			address >= ((block_t *)curr_prim->data)->start_address)
			break;

	// Checks if the address is allocated (If there is a block containing it).
	if (!curr_prim) {
		free(data);
		puts("Invalid address for write.");
		return;
	}

	// Searches the miniblock list for the miniblock containing the given
	// address.
	list_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
	for (; curr_sec; curr_sec = curr_sec->next)
		if (address < ((miniblock_t *)curr_sec->data)->start_address +
			((miniblock_t *)curr_sec->data)->size)
			break;

	block_t *block = ((block_t *)curr_prim->data);
	miniblock_t *mblock = ((miniblock_t *)curr_sec->data);

	// Calculates the offset from the start address of the block and the given
	// address.
	uint64_t offset_prim = address - block->start_address;

	// Calculates the offset from the start address of the miniblock and the
	// given address.
	uint64_t offset_sec = address - mblock->start_address;
	void *tmp = data;

	// Copies the value of size to a mutable varible.
	uint64_t mut_size = size;

	// Checks if the given size is greater than the size of the block from the
	// given address.
	if (block->size - offset_prim < mut_size) {
		mut_size = block->size - offset_prim;
		printf("Warning: size was bigger than the block size. Writing %lu "
			   "characters.\n", mut_size);
	}

	if (!check_perm_write(mut_size, curr_sec, offset_sec)) {
		free(data);
		return;
	}

	write_in_block(mut_size, curr_sec, offset_sec, tmp);
	free(data);
}

// Prints a map of the arena.
void pmap(const arena_t *arena)
{
	// Checks if the arena is allocated.
	if (!arena) {
		puts("Arena not allocated.");
		return;
	}
	printf("Total memory: 0x%lX bytes\nFree memory: 0x%lX bytes\nNumber of "
		   "allocated "
		   "blocks: %lu\nNumber of allocated miniblocks: %lu\n",
		   arena->arena_size, arena->arena_size - arena->used_mem,
		   arena->blocks, arena->mblocks);
	if (arena->alloc_list)
		putchar('\n');

	list_t *curr_prim = arena->alloc_list, *curr_sec;
	size_t i, j;
	for (i = 1; curr_prim; curr_prim = curr_prim->next, i++) {
		printf("Block %lu begin\n", i);
		printf("Zone: 0x%lX - 0x%lX\n",
			   ((block_t *)curr_prim->data)->start_address,
			   ((block_t *)curr_prim->data)->start_address +
			   ((block_t *)curr_prim->data)->size);
		curr_sec = ((block_t *)curr_prim->data)->miniblock_list;

		for (j = 1; curr_sec; curr_sec = curr_sec->next, j++) {
			printf("Miniblock %lu:\t\t", j);
			printf("0x%lX\t\t-\t\t0x%lX\t\t| ",
				   ((miniblock_t *)curr_sec->data)->start_address,
				   ((miniblock_t *)curr_sec->data)->start_address +
				   ((miniblock_t *)curr_sec->data)->size);

			if (((miniblock_t *)curr_sec->data)->perm & (1 << 2))
				putchar('R');
			else
				putchar('-');
			if (((miniblock_t *)curr_sec->data)->perm & (1 << 1))
				putchar('W');
			else
				putchar('-');
			if (((miniblock_t *)curr_sec->data)->perm & 1)
				putchar('X');
			else
				putchar('-');
			putchar('\n');
		}
		printf("Block %lu end\n", i);
		if (curr_prim->next)
			putchar('\n');
	}
}

// Changes the permissions of a block.
void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
	// Checks if the arena is initialized
	if (!arena) {
		puts("Arena not allocated.");
		return;
	}

	// Checks if the arena is empty
	if (!arena->alloc_list) {
		puts("No blocks allocated.");
		return;
	}

	list_t *curr_prim = arena->alloc_list;

	// Searches the block list for the block containing the address.
	for (; curr_prim->next; curr_prim = curr_prim->next)
		if (address >= ((block_t *)curr_prim->next->data)->start_address &&
			address < ((block_t *)curr_prim->next->data)->start_address +
			((block_t *)curr_prim->next->data)->size)
			break;

	// Checks if such a block exists.
		if (!curr_prim) {
			free(permission);
			puts("Invalid address for mprotect.");
			return;
	}

	// Searches the miniblock list for the miniblock with the given start
	// address.
	list_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
	for (; curr_sec; curr_sec = curr_sec->next)
		if (((miniblock_t *)curr_sec->data)->start_address == address)
			break;

	// Checks if such a miniblock exists.
	if (!curr_sec) {
		free(permission);
		puts("Invalid address for mprotect.");
		return;
	}

	// This vector stores all permission changes, in the order they were given.
	char *prot = malloc(((strlen((char *)permission) / 9) + 1) * sizeof(char)),
			 *cpy_str;
	DIE(!prot, "Malloc failed.\n");
	size_t dim = 0;

	// The string of commands is analysed, instruction by instruction,
	// interpreted, and the respective number for each instruction is stored in
	// the prot vector.
	cpy_str = strtok((char *)permission, " |");
	for (; cpy_str; cpy_str = strtok(NULL, " |"), dim++) {
		prot[dim] = interpret_prot_str(cpy_str);

		// Checks if an invalid command was inputed.
		if (prot[dim] == -1) {
			free(permission);
			free(prot);
			puts("Invalid command. Please try again.");
			return;
		}
	}
	free(permission);

	miniblock_t *mblock = (miniblock_t *)curr_sec->data;
	size_t i = 0;
	mblock->perm = 0;

	// Applies all changes to the miniblock permissions, as they were given.
	for (; i < dim; i++) {
		if (prot[i] == 0) {
			mblock->perm = 0;
			continue;
		}
		mblock->perm += prot[i] * (1 - ((mblock->perm >> (prot[i] / 2)) & 1));
	}
	free(prot);
}
