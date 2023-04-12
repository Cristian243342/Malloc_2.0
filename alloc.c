// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "struct.h"
#include "utils.h"

// Adds a miniblock to the end of the miniblock list of the l_block. If ver is
// 0, then the new miniblock is in the terminal node, who's next pointer will
// point to NULL. If ver is 1, then the function was called in order to
// concatenate two blocks, so, instead of NULL, the otherwise terminal node's
// next pointer is made to point to the head of the miniblock list of
// l_block->next.
void add_last(uint64_t address, size_t mblock_size, list_t *l_block, char ver)
{
	block_t *block = (block_t *)l_block->data;
	miniblock_t *new_mblock = malloc(sizeof(miniblock_t));
	DIE(!new_mblock, "Malloc failed.\n");
	list_t *new_l_block = malloc(sizeof(list_t));
	DIE(!new_l_block, "Malloc failed.\n");

	// The new miniblock is initialized.
	new_mblock->start_address = address;
	new_mblock->size = mblock_size;
	new_mblock->perm = 6;
	new_mblock->rw_buffer = calloc(1, mblock_size);
	DIE(!new_mblock->rw_buffer, "Calloc failed.\n");

	// The miniblock list of the l_block is sequenced through until the last
	// node is reached.
	list_t *curr = block->miniblock_list;
	for (; curr->next; curr = curr->next)
		;

	// A new list node, containing the new miniblock, is initialized.
	new_l_block->data = (void *)new_mblock;
	new_l_block->prev = curr;

	// Checks the context in which the function was called.
	if (ver == 0) {
		new_l_block->next = NULL;
	} else {
		new_l_block->next =
			((block_t *)l_block->next->data)->miniblock_list;
		new_l_block->next->prev = new_l_block;
	}
	curr->next = new_l_block;

	// The block metadata is updated.
	block->size += mblock_size;
}

// Adds a miniblock to the start of the miniblock list of the h_block.
void add_first(uint64_t address, size_t mblock_size, block_t *h_block)
{
	miniblock_t *new_mblock = malloc(sizeof(miniblock_t));
	DIE(!new_mblock, "Malloc failed.\n");
	list_t *new_dll_block = malloc(sizeof(list_t));
	DIE(!new_dll_block, "Malloc failed.\n");

	// The new miniblock is initialized.
	new_mblock->start_address = address;
	new_mblock->size = mblock_size;
	new_mblock->perm = 6;
	new_mblock->rw_buffer = calloc(1, mblock_size);
	DIE(!new_mblock->rw_buffer, "Calloc failed.\n");

	// A list node containing the new miniblock is initialized.
	new_dll_block->data = (void *)new_mblock;
	new_dll_block->prev = NULL;
	new_dll_block->next = h_block->miniblock_list;
	new_dll_block->next->prev = new_dll_block;

	// The block metadata is updated accordingly.
	h_block->miniblock_list = new_dll_block;
	h_block->start_address = address;
	h_block->size += mblock_size;
}

// Creates a new miniblock and adds it to the end of the miniblock list of the
// l_block, and then concatenates the l_block and l_block->next.
void add_middle(uint64_t address, size_t mblock_size, list_t *l_block)
{
	// Creates and adds the new miniblock to the end of the miniblock list of
	// the l_block, and concatenates the two blocks.
	add_last(address, mblock_size, l_block, 1);

	// Increases the size of the l_block by the size of the h_block.
	((block_t *)l_block->data)->size +=
			((block_t *)l_block->next->data)->size;

	// l_block is made to point to the l_block->next, which is then removed.
	l_block = l_block->next;
	l_block->prev->next = l_block->next;
	if (l_block->next)
		l_block->next->prev = l_block->prev;
	free(l_block->data);
	free(l_block);
}

// Creates a new block entry in the block list.
void alloc_new_block(arena_t *arena, uint64_t address, uint64_t size,
					 list_t *curr)
{
	// The metadata of the arena are moddified.
	arena->used_mem += size;
	arena->mblocks++;
	arena->blocks++;

	block_t *new_block = malloc(sizeof(block_t));
	DIE(!new_block, "Malloc failed.\n");
	miniblock_t *new_mblock = malloc(sizeof(miniblock_t));
	DIE(!new_mblock, "Malloc failed.\n");
	list_t *new_dll_block_prim = malloc(sizeof(list_t));
	DIE(!new_dll_block_prim, "Malloc failed.\n");
	list_t *new_dll_block_sec = malloc(sizeof(list_t));
	DIE(!new_dll_block_sec, "Malloc failed.\n");

	// The miniblock is initialized.
	new_mblock->start_address = address;
	new_mblock->size = size;
	new_mblock->perm = 6;
	new_mblock->rw_buffer = calloc(1, size);
	DIE(!new_mblock->rw_buffer, "Calloc failed.\n");

	// A new list node is created, containing the new miniblock.
	new_dll_block_sec->data = (void *)new_mblock;
	new_dll_block_sec->next = NULL;
	new_dll_block_sec->prev = NULL;

	// This new node is added as the head of the miniblock list of the new
	// block, and the new block is initialized.
	new_block->miniblock_list = new_dll_block_sec;
	new_block->size = size;
	new_block->start_address = address;

	// A new list node is created, storing the new block.
	new_dll_block_prim->data = (void *)new_block;
	// Checks if the block list is empty.
	if (!curr) {
		new_dll_block_prim->prev = NULL;
		new_dll_block_prim->next = NULL;
	// Checks if curr is the upper bound of the address.
	} else if (address < ((block_t *)curr->data)->start_address) {
		new_dll_block_prim->prev = curr->prev;
		new_dll_block_prim->next = curr;
		if (curr->prev)
			curr->prev->next = new_dll_block_prim;
		new_dll_block_prim->next->prev = new_dll_block_prim;
	// Otherwise, curr must be the last block in the block list.
	} else {
		new_dll_block_prim->prev = curr;
		new_dll_block_prim->next = NULL;
		curr->next = new_dll_block_prim;
	}

	// Checks if the block was placed at the start of the block list.
	if (!new_dll_block_prim->prev)
		arena->alloc_list = new_dll_block_prim;
}

// Checks if the new miniblock is in a valid position, and then if it borders
// a block below it or above it.
int8_t check_aliniation(arena_t *arena, uint64_t address, uint64_t size,
						list_t *curr)
{
	block_t *upper = curr->data;
	list_t *lower;

	// If the address is between the start address of the last block and the
	// end of the arena, then the block returned is instead the lower bound.
	// A check is done in order to check this.
	if (address > ((block_t *)curr->data)->start_address && !curr->next) {
		lower = curr;
		upper = NULL;
	} else {
		lower = curr->prev;
	}

	// Checks if the new block is adjasent to both the lower and upper
	// blocks.
	if (lower && upper)
		if ((int64_t)address ==
			(int64_t)(((block_t *)lower->data)->start_address +
			((block_t *)lower->data)->size) &&
			(int64_t)address == (int64_t)(upper->start_address - size)) {
			arena->blocks--;
			arena->used_mem += size;
			arena->mblocks++;
			add_middle(address, size, curr->prev);
			return 1;
		}

	// Checks if the new block is adjasent to the lower block.
	if (lower) {
		if ((int64_t)address < (int64_t)(((block_t *)lower->data)
			->start_address + ((block_t *)lower->data)->size)) {
			puts("This zone was already allocated.");
			return 1;
		}
		if (upper)
			if ((int64_t)address > (int64_t)(upper->start_address - size)) {
				puts("This zone was already allocated.");
				return 1;
			}
		if ((int64_t)address ==
				(int64_t)(((block_t *)lower->data)->start_address +
									((block_t *)lower->data)->size)) {
			arena->used_mem += size;
			arena->mblocks++;
			add_last(address, size, lower, 0);
			return 1;
		}
	}

	// Checks if the block is adjasent to the upper block.
	if (upper) {
		if ((int64_t)address > (int64_t)(upper->start_address - size)) {
			puts("This zone was already allocated.");
			return 1;
		}
		if (lower)
			if ((int64_t)address < (int64_t)(((block_t *)lower->data)
				->start_address + ((block_t *)lower->data)->size)) {
				puts("This zone was already allocated.");
				return 1;
			}
		if ((int64_t)address == (int64_t)(upper->start_address - size)) {
			add_first(address, size, upper);
			arena->used_mem += size;
			arena->mblocks++;
			return 1;
		}
	}

	return 0;
}

