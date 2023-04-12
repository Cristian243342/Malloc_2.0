// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#pragma once
#include <inttypes.h>
#include <stddef.h>

// An implementation for nodes of a doubly linked list.
typedef struct list_t {
	// Stores a pointer to the data stored in the node.
	void *data;
	// Stores a pointer to the previous node.
	struct list_t *prev;
	// Stores a pointer to the next node.
	struct list_t *next;
} list_t;

// An implementation for memory blocks.
typedef struct {
	// Stores the start address of the block.
	uint64_t start_address;
	// Stores the size in bytes of the block.
	size_t size;
	// Stores a pointer to the first node in a doubly linked list of miniblocks.
	void *miniblock_list;
} block_t;

// An implementation for memory miniblocks.
typedef struct {
	// Stores the start address of the miniblock.
	uint64_t start_address;
	// Stores the size on bytes of the miniblock.
	size_t size;
	// Stores the permissions of the miniblock.
	uint8_t perm;
	// Stores a pointer to the effective memory allocated with malloc.
	void *rw_buffer;
} miniblock_t;

// An implementation for the memory arena.
typedef struct {
	// Stores the size in bytes of the arena, the amount of memory allocated at
	// any given point, the number of block, and the number of miniblocks
	// respectively.
	uint64_t arena_size, used_mem, blocks, mblocks;
	// Stores a pointer to the first node in a doubly linked list of blocks.
	list_t *alloc_list;
} arena_t;
