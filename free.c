#include <stdio.h>
#include <stdlib.h>

#include "struct.h"
#include "utils.h"

void free_mblock(list_t *block) {
  free(((miniblock_t *)block->data)->rw_buffer);
  free(block->data);
  free(block);
}

void free_block_dealloc(list_t *block) {
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
