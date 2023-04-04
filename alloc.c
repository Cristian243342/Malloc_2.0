#include <stdio.h>
#include <stdlib.h>

#include "struct.h"
#include "utils.h"

extern arena_t *arena;

void alloc_arena(char *cmd) {
  if (arena) {
    puts("Arena already allocated.");
    return;
  }
  arena = malloc(sizeof(arena_t));
  DIE(!arena, "Malloc failed\n");
  if (sscanf(cmd, "%*s%lu", &arena->size) != 1) {
    puts("Invalid command. Please try again.");
    free(arena);
    arena = NULL;
    return;
  }
  arena->list = NULL;
  arena->blocks = 0;
  arena->mblocks = 0;
  arena->used_mem = 0;
}

void add_last(uint64_t address, size_t mblock_size, dll_block_t *dll_block,
              char ver) {
  block_t *block = (block_t *)dll_block->data;
  mblock_t *new_mblock = malloc(sizeof(mblock_t));
  DIE(!new_mblock, "Malloc failed\n");
  dll_block_t *new_dll_block = malloc(sizeof(dll_block_t));
  DIE(!new_dll_block, "Malloc failed\n");

  new_mblock->start_address = address;
  new_mblock->size = mblock_size;
  new_mblock->perm = 6;
  new_mblock->rw_buffer = malloc(mblock_size);
  DIE(!new_mblock->rw_buffer, "Malloc failed\n");

  dll_block_t *curr = block->miniblock_list;
  for (; curr->next; curr = curr->next)
    ;

  new_dll_block->data = (void *)new_mblock;
  new_dll_block->prev = curr;
  if (ver == 0)
    new_dll_block->next = NULL;
  else {
    new_dll_block->next = ((block_t *)dll_block->next->data)->miniblock_list;
    new_dll_block->next->prev = new_dll_block;
  }
  curr->next = new_dll_block;

  block->size += mblock_size;
}

void add_first(uint64_t address, size_t mblock_size, block_t *block) {
  mblock_t *new_mblock = malloc(sizeof(mblock_t));
  DIE(!new_mblock, "Malloc failed\n");
  dll_block_t *new_dll_block = malloc(sizeof(dll_block_t));
  DIE(!new_dll_block, "Malloc failed\n");

  new_mblock->start_address = address;
  new_mblock->size = mblock_size;
  new_mblock->perm = 6;
  new_mblock->rw_buffer = malloc(mblock_size);
  DIE(!new_mblock->rw_buffer, "Malloc failed\n");

  new_dll_block->data = (void *)new_mblock;
  new_dll_block->prev = NULL;
  new_dll_block->next = block->miniblock_list;
  new_dll_block->next->prev = new_dll_block;

  block->miniblock_list = new_dll_block;
  block->start_address = address;
  block->size += mblock_size;
}

void add_middle(uint64_t address, size_t mblock_size, dll_block_t *dll_block) {
  add_last(address, mblock_size, dll_block, 1);
  ((block_t *)dll_block->data)->size +=
      ((block_t *)dll_block->next->data)->size;
  dll_block = dll_block->next;
  dll_block->prev->next = dll_block->next;
  if (dll_block->next)
    dll_block->next->prev = dll_block->prev;
  free(dll_block->data);
  free(dll_block);
}

void alloc_block(char *cmd) {
  if (!arena) {
    puts("Arena not allocated");
    return;
  }

  uint64_t address;
  size_t mblock_size;
  if (sscanf(cmd, "%*s%lu%lu", &address, &mblock_size) != 2) {
    puts("Invalid command. Please try again.");
    return;
  }

  if (address >= arena->size) {
    puts("The allocated address is outside the size of arena");
    return;
  }

  if (address > arena->size - mblock_size) {
    puts("The end address is past the size of the arena");
    return;
  }

  dll_block_t *curr = arena->list;
  if (curr) {
    for (; curr->next; curr = curr->next) {
      if (address < ((block_t *)curr->data)->start_address) break;
    }
    block_t *upper = curr->data;
    dll_block_t *lower;
    if (!curr->next && address > ((block_t *)curr->data)->start_address) {
      lower = curr;
      upper = NULL;
    } else if (!curr->prev)
      lower = NULL;
    else
      lower = curr->prev;

    if (lower && upper)
      if ((int64_t)address ==
              (int64_t)(((block_t *)lower->data)->start_address +
                        ((block_t *)lower->data)->size) &&
          (int64_t)address == (int64_t)(upper->start_address - mblock_size)) {
        arena->blocks--;
        arena->used_mem += mblock_size;
        arena->mblocks++;
        add_middle(address, mblock_size, curr->prev);
        return;
      }
    if (lower) {
      if ((int64_t)address <
          (int64_t)(((block_t *)lower->data)->start_address +
                    ((block_t *)lower->data)->size)) {
        puts("This zone was already allocated.");
        return;
      }
      if ((int64_t)address ==
          (int64_t)(((block_t *)lower->data)->start_address +
                    ((block_t *)lower->data)->size)) {
        arena->used_mem += mblock_size;
        arena->mblocks++;
        add_last(address, mblock_size, lower, 0);
        return;
      }
    }

    if (upper) {
      if ((int64_t)address > (int64_t)(upper->start_address - mblock_size)) {
        puts("This zone was already allocated.");
        return;
      }
      if ((int64_t)address == (int64_t)(upper->start_address - mblock_size)) {
        add_first(address, mblock_size, upper);
        arena->used_mem += mblock_size;
        arena->mblocks++;
        return;
      }
    }
  }

  arena->used_mem += mblock_size;
  arena->mblocks++;
  arena->blocks++;
  block_t *new_block = malloc(sizeof(block_t));
  DIE(!new_block, "Malloc failed\n");
  mblock_t *new_mblock = malloc(sizeof(mblock_t));
  DIE(!new_mblock, "Malloc failed\n");
  dll_block_t *new_dll_block_prim = malloc(sizeof(dll_block_t));
  DIE(!new_dll_block_prim, "Malloc failed\n");
  dll_block_t *new_dll_block_sec = malloc(sizeof(dll_block_t));
  DIE(!new_dll_block_sec, "Malloc failed\n");

  new_mblock->start_address = address;
  new_mblock->size = mblock_size;
  new_mblock->perm = 6;
  new_mblock->rw_buffer = malloc(mblock_size);
  DIE(!new_mblock->rw_buffer, "Malloc failed\n");

  new_dll_block_sec->data = (void *)new_mblock;
  new_dll_block_sec->next = NULL;
  new_dll_block_sec->prev = NULL;

  new_block->miniblock_list = new_dll_block_sec;
  new_block->size = mblock_size;
  new_block->start_address = address;

  new_dll_block_prim->data = (void *)new_block;
  if (!curr) {
    new_dll_block_prim->prev = NULL;
    new_dll_block_prim->next = NULL;
  } else if (address < ((block_t *)curr->data)->start_address) {
    new_dll_block_prim->prev = curr->prev;
    new_dll_block_prim->next = curr;
    if (curr->prev) curr->prev->next = new_dll_block_prim;
    new_dll_block_prim->next->prev = new_dll_block_prim;
  } else {
    new_dll_block_prim->prev = curr;
    new_dll_block_prim->next = NULL;
    curr->next = new_dll_block_prim;
  }

  if (!new_dll_block_prim->prev) arena->list = new_dll_block_prim;
}
