// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#pragma once
#include <inttypes.h>

#include "struct.h"

void add_last(uint64_t address, size_t mblock_size, list_t *dll_block,
			  char ver);
void add_first(uint64_t address, size_t mblock_size, block_t *block);
void add_middle(uint64_t address, size_t mblock_size, list_t *dll_block);
void alloc_new_block(arena_t *arena, uint64_t address, uint64_t size,
					 list_t *curr);
int8_t check_aliniation(arena_t *arena, uint64_t address, uint64_t size,
						list_t *curr);
