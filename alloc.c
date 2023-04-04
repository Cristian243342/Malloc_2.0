#include <stdio.h>
#include <stdlib.h>

#include "struct.h"
#include "utils.h"


void add_last(uint64_t address, size_t mblock_size, list_t *dll_block,
              char ver) {
  block_t *block = (block_t *)dll_block->data;
  miniblock_t *new_mblock = malloc(sizeof(miniblock_t));
  DIE(!new_mblock, "Malloc failed\n");
  list_t *new_dll_block = malloc(sizeof(list_t));
  DIE(!new_dll_block, "Malloc failed\n");

  new_mblock->start_address = address;
  new_mblock->size = mblock_size;
  new_mblock->perm = 6;
  new_mblock->rw_buffer = malloc(mblock_size);
  DIE(!new_mblock->rw_buffer, "Malloc failed\n");

  list_t *curr = block->miniblock_list;
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
  miniblock_t *new_mblock = malloc(sizeof(miniblock_t));
  DIE(!new_mblock, "Malloc failed\n");
  list_t *new_dll_block = malloc(sizeof(list_t));
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

void add_middle(uint64_t address, size_t mblock_size, list_t *dll_block) {
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
