// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "struct.h"
#include "utils.h"

// Frees a node containing a miniblock.
void free_mblock(list_t *block)
{
	free(((miniblock_t *)block->data)->rw_buffer);
	free(block->data);
	free(block);
}

// Frees a node containing a block.
void free_block_dealloc(list_t *block)
{
	list_t *curr = ((block_t *)block->data)->miniblock_list;
	if (curr) {
		while (curr->next) {
			curr = curr->next;
			free_mblock(curr->prev);
		}
		free_mblock(curr);
	}
	free(block->data);
	free(block);
}

// Splits a block in two after freeing a miniblock in it.
void split_block(arena_t *arena, list_t *curr_prim, list_t *curr_sec)
{
	arena->blocks++;
	list_t *new_dll_block = malloc(sizeof(list_t));
	DIE(!new_dll_block, "Malloc failed.\n");
	block_t *new_block = malloc(sizeof(block_t));
	DIE(!new_block, "Malloc failed.\n");

	// Calculates the new size of the first block (lower block).
	uint64_t size_b1 = ((miniblock_t *)curr_sec->data)->start_address -
						((block_t *)curr_prim->data)->start_address;

	// Moves the higher address then curr_sec (the miniblock to be freed)
	// miniblocks to the new block (higher block).
	new_block->miniblock_list = curr_sec->next;
	new_block->start_address =
		((miniblock_t *)curr_sec->next->data)->start_address;
	new_block->size = ((block_t *)curr_prim->data)->size -
		((miniblock_t *)curr_sec->data)->size - size_b1;

	((block_t *)curr_prim->data)->size = size_b1;

	curr_sec->next->prev = NULL;
	curr_sec->prev->next = NULL;
	free_mblock(curr_sec);

	// Creates a new node to store the newly created block.
	new_dll_block->data = (void *)new_block;
	new_dll_block->prev = curr_prim;
	new_dll_block->next = curr_prim->next;
	curr_prim->next = new_dll_block;
	if (new_dll_block->next)
		new_dll_block->next->prev = new_dll_block;
}
