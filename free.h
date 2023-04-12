// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#pragma once

#include "struct.h"

// Frees a node containing a miniblock.
void free_mblock(list_t *block);
// Frees a node containing a block.
void free_block_dealloc(list_t *block);
// Splits a block in two after freeing a miniblock in it.
void split_block(arena_t *arena, list_t *curr_prim, list_t *curr_sec);
