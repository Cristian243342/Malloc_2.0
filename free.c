#include <stdio.h>
#include <stdlib.h>

#include "struct.h"
#include "utils.h"

extern arena_t *arena;

void free_mblock(dll_block_t *block) {
  free(((mblock_t *)block->data)->rw_buffer);
  free(block->data);
  free(block);
}

void free_block_dealloc(dll_block_t *block) {
  dll_block_t *curr = ((block_t *)block->data)->miniblock_list;
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

void free_block(char *cmd) {
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }
  if (!arena->list) {
    puts("Invalid address for free.");
    return;
  }

  uint64_t address;
  if (sscanf(cmd, "%*s%lu", &address) != 1) {
    puts("Invalid command. Please try again.");
    return;
  }

  arena->mblocks--;
  dll_block_t *curr_prim = arena->list;
  for (; curr_prim->next; curr_prim = curr_prim->next)
    if (address < ((block_t *)curr_prim->next->data)->start_address) break;

  dll_block_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
  for (; curr_sec; curr_sec = curr_sec->next)
    if (((mblock_t *)curr_sec->data)->start_address == address) break;

  if (!curr_sec) {
    puts("Invalid address for free.");
    return;
  }

  arena->used_mem -= ((mblock_t *)curr_sec->data)->size;
  if (!curr_sec->prev) {
    if (!curr_sec->next) {
      arena->blocks--;
      free_mblock(curr_sec);
      if (curr_prim->prev)
        curr_prim->prev->next = curr_prim->next;
      else
        arena->list = curr_prim->next;
      if (curr_prim->next) curr_prim->next->prev = curr_prim->prev;
      free(curr_prim->data);
      free(curr_prim);
      return;
    } else {
      ((block_t *)curr_prim->data)->size -= ((mblock_t *)curr_sec->data)->size;
      ((block_t *)curr_prim->data)->miniblock_list = curr_sec->next;
      ((block_t *)curr_prim->data)->start_address =
          ((mblock_t *)curr_sec->next->data)->start_address;
      curr_sec->next->prev = NULL;
      free_mblock(curr_sec);
      return;
    }
  }
  if (!curr_sec->next) {
    ((block_t *)curr_prim->data)->size -= ((mblock_t *)curr_sec->data)->size;
    curr_sec->prev->next = NULL;
    free_mblock(curr_sec);
    return;
  }

  arena->blocks++;
  dll_block_t *new_dll_block = malloc(sizeof(dll_block_t));
  DIE(!new_dll_block, "Malloc failed\n");
  block_t *new_block = malloc(sizeof(block_t));
  DIE(!new_block, "Malloc failed\n");

  uint64_t size_b1 = ((mblock_t *)curr_sec->data)->start_address -
                     ((block_t *)curr_prim->data)->start_address;
  new_block->miniblock_list = curr_sec->next;
  new_block->start_address = ((mblock_t *)curr_sec->next->data)->start_address;
  new_block->size = ((block_t *)curr_prim->data)->size -
                    ((mblock_t *)curr_sec->data)->size - size_b1;

  ((block_t *)curr_prim->data)->size = size_b1;

  curr_sec->next->prev = NULL;
  curr_sec->prev->next = NULL;
  free_mblock(curr_sec);

  new_dll_block->data = (void *)new_block;
  new_dll_block->prev = curr_prim;
  new_dll_block->next = curr_prim->next;
  curr_prim->next = new_dll_block;
  if (new_dll_block->next) new_dll_block->next->prev = new_dll_block;
}

void dealloc_arena(void) {
  dll_block_t *curr = arena->list;
  if (curr) {
    while (curr->next) {
      curr = curr->next;
      free_block_dealloc(curr->prev);
    }
    free_block_dealloc(curr);
  }
  free(arena);
}