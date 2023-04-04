#pragma once
#include <inttypes.h>
#include <stddef.h>

/* TODO : add your implementation for doubly-linked list */
typedef struct list_t {
  void *data;
  struct list_t *prev;
  struct list_t *next;
} list_t;

typedef struct {
  uint64_t start_address;
  size_t size;
  void *miniblock_list;
} block_t;

typedef struct {
  uint64_t start_address;
  size_t size;
  uint8_t perm;
  void *rw_buffer;
} miniblock_t;

typedef struct {
  uint64_t arena_size, used_mem, blocks, mblocks;
  list_t *alloc_list;
} arena_t;