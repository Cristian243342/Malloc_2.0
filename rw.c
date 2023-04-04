#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "struct.h"
#include "utils.h"

extern arena_t *arena;

void write(char *cmd) {
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }

  uint64_t address;
  size_t dim;
  // TO DO........................................................
  char *data_prim;
  if (sscanf(cmd, "%*s%lu%lu %m[^\n]", &address, &dim, &data_prim) < 2) {
    puts("Invalid command. Please try again.");
    return;
  }

  char *data = malloc(dim * sizeof(char));
  if (!data_prim) {
    fread(data, sizeof(char), dim, stdin);
  } else {
    fread(data + strlen(data_prim), sizeof(char), dim - strlen(data_prim),
          stdin);
    memcpy(data, data_prim, strlen(data_prim));
    free(data_prim);
  }

  if (!arena->list) {
    free(data);
    puts("Invalid address for write.");
    return;
  }

  dll_block_t *curr_prim = arena->list;
  for (; curr_prim; curr_prim = curr_prim->next)
    if (address < ((block_t *)curr_prim->data)->start_address +
                      ((block_t *)curr_prim->data)->size &&
        address >= ((block_t *)curr_prim->data)->start_address)
      break;

  if (!curr_prim) {
    free(data);
    puts("Invalid address for write.");
    return;
  }

  dll_block_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
  for (; curr_sec; curr_sec = curr_sec->next)
    if (address < ((mblock_t *)curr_sec->data)->start_address +
                      ((mblock_t *)curr_sec->data)->size)
      break;

  block_t *block = ((block_t *)curr_prim->data);
  mblock_t *mblock = ((mblock_t *)curr_sec->data);
  uint64_t offset_prim = address - block->start_address;
  uint64_t offset_sec = address - mblock->start_address;
  void *tmp = data;

  if (block->size - offset_prim < dim) {
    dim = block->size - offset_prim;
    printf(
        "Warning: size was bigger than the block size. Writing %lu "
        "characters.\n",
        dim);
  }

  while (dim) {
    if (!(mblock->perm & (1 << 1))) {
      free(data);
      puts("Invalid permissions for write.");
      return;
    }

    if (mblock->size - offset_sec >= dim) {
      memcpy(mblock->rw_buffer + offset_sec, tmp, dim);
      break;
    }
    memcpy(mblock->rw_buffer + offset_sec, tmp, mblock->size - offset_sec);
    tmp += mblock->size - offset_sec;
    dim -= mblock->size - offset_sec;
    curr_sec = curr_sec->next;
    mblock = ((mblock_t *)curr_sec->data);
    offset_sec = 0;
  }
  free(data);
}

void read(char *cmd) {
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }
  if (!arena->list) {
    puts("Invalid address for read.");
    return;
  }

  uint64_t address;
  size_t dim;
  if (sscanf(cmd, "%*s%lu%lu", &address, &dim) != 2) {
    puts("Invalid command. Please try again.");
    return;
  }

  dll_block_t *curr_prim = arena->list;
  for (; curr_prim; curr_prim = curr_prim->next)
    if (address < ((block_t *)curr_prim->data)->start_address +
                      ((block_t *)curr_prim->data)->size)
      break;

  if (!curr_prim) {
    puts("Invalid address for read.");
    return;
  }

  dll_block_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
  for (; curr_sec; curr_sec = curr_sec->next)
    if (address < ((mblock_t *)curr_sec->data)->start_address +
                      ((mblock_t *)curr_sec->data)->size)
      break;
  
  if (!curr_sec) {
    puts("Invalid address for read.");
    return;
  }

  mblock_t *block = ((mblock_t *)curr_sec->data);
  uint64_t offset_prim = address - ((block_t *)curr_prim->data)->start_address;
  uint64_t offset_sec = address - block->start_address;

  if (((block_t *)curr_prim->data)->size - offset_prim < dim) {
    dim = ((block_t *)curr_prim->data)->size - offset_prim;
    printf(
        "Warning: size was bigger than the block size. Reading %lu "
        "characters.\n",
        dim);
  }

  size_t dim_tmp = dim, offset_sec_tmp = offset_sec;
  dll_block_t *curr_tmp = curr_sec;

  while (dim_tmp) {
    block = ((mblock_t *)curr_tmp->data);
    if (!(block->perm & (1 << 2))) {
      puts("Invalid permissions for read.");
      return;
    }
    if (block->size - offset_sec >= dim) break;
    dim_tmp -= block->size - offset_sec_tmp;
    curr_tmp = curr_tmp->next;
    offset_sec_tmp = 0;
  }

  block = ((mblock_t *)curr_sec->data);
  while (dim) {
    block = ((mblock_t *)curr_sec->data);
    if (block->size - offset_sec >= dim) {
      printf("%.*s\n", (int)dim, (char *)block->rw_buffer + offset_sec);
      break;
    }
    printf("%.*s", (int)(block->size - offset_sec),
           (char *)block->rw_buffer + offset_sec);
    dim -= block->size - offset_sec;
    curr_sec = curr_sec->next;
    offset_sec = 0;
  }
}
