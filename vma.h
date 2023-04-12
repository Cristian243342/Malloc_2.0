// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#pragma once
#include <inttypes.h>

#include "struct.h"

// Initializes an arena with a given size.
arena_t *alloc_arena(const uint64_t size);
// Frees all memory allocated in the arena, as well as the arena itself.
void dealloc_arena(arena_t *arena);

// Allocates a new block of the given size (in bytes).
void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size);
// Frees the block found at the given address.
void free_block(arena_t *arena, const uint64_t address);

// Reads size bytes from the given address.
void read(arena_t *arena, uint64_t address, uint64_t size);
// Writes size bytes at the given address.
void write(arena_t *arena, const uint64_t address,  const uint64_t size,
		   int8_t *data);

// Prints a map of the arena.
void pmap(const arena_t *arena);
// Changes the permissions of a block.
void mprotect(arena_t *arena, uint64_t address, int8_t *permission);
