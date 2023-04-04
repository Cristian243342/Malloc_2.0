#include "vma.h"
#include "utils.h"
#include "alloc.h"
#include "free.h"
#include "struct.h"
#include "mprotect.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

arena_t *alloc_arena(const uint64_t size)
{
  arena_t *arena = malloc(sizeof(arena_t));
  DIE(!arena, "Malloc failed\n");
  arena->arena_size = size;
  arena->alloc_list = NULL;
  arena->blocks = 0;
  arena->mblocks = 0;
  arena->used_mem = 0;
  return arena;
}

void dealloc_arena(arena_t *arena)
{
  list_t *curr = arena->alloc_list;
  if (curr) {
    while (curr->next) {
      curr = curr->next;
      free_block_dealloc(curr->prev);
    }
    free_block_dealloc(curr);
  }
  free(arena);
}

void alloc_block(arena_t *arena, const uint64_t address, const uint64_t size)
{
  if (!arena) {
    puts("Arena not allocated");
    return;
  }

  if (address >= arena->arena_size) {
    puts("The allocated address is outside the size of arena");
    return;
  }

  if (address > arena->arena_size - size) {
    puts("The end address is past the size of the arena");
    return;
  }

  list_t *curr = arena->alloc_list;
  if (curr) {
    for (; curr->next; curr = curr->next) {
      if (address < ((block_t *)curr->data)->start_address) break;
    }
    block_t *upper = curr->data;
    list_t *lower;
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
          (int64_t)address == (int64_t)(upper->start_address - size)) {
        arena->blocks--;
        arena->used_mem += size;
        arena->mblocks++;
        add_middle(address, size, curr->prev);
        return;
      }
    if (lower) {
      if ((int64_t)address < (int64_t)(((block_t *)lower->data)->start_address +
                                       ((block_t *)lower->data)->size)) {
        puts("This zone was already allocated.");
        return;
      }
      if ((int64_t)address ==
          (int64_t)(((block_t *)lower->data)->start_address +
                    ((block_t *)lower->data)->size)) {
        arena->used_mem += size;
        arena->mblocks++;
        add_last(address, size, lower, 0);
        return;
      }
    }

    if (upper) {
      if ((int64_t)address > (int64_t)(upper->start_address - size)) {
        puts("This zone was already allocated.");
        return;
      }
      if ((int64_t)address == (int64_t)(upper->start_address - size)) {
        add_first(address, size, upper);
        arena->used_mem += size;
        arena->mblocks++;
        return;
      }
    }
  }

  arena->used_mem += size;
  arena->mblocks++;
  arena->blocks++;
  block_t *new_block = malloc(sizeof(block_t));
  DIE(!new_block, "Malloc failed\n");
  miniblock_t *new_mblock = malloc(sizeof(miniblock_t));
  DIE(!new_mblock, "Malloc failed\n");
  list_t *new_dll_block_prim = malloc(sizeof(list_t));
  DIE(!new_dll_block_prim, "Malloc failed\n");
  list_t *new_dll_block_sec = malloc(sizeof(list_t));
  DIE(!new_dll_block_sec, "Malloc failed\n");

  new_mblock->start_address = address;
  new_mblock->size = size;
  new_mblock->perm = 6;
  new_mblock->rw_buffer = malloc(size);
  DIE(!new_mblock->rw_buffer, "Malloc failed\n");

  new_dll_block_sec->data = (void *)new_mblock;
  new_dll_block_sec->next = NULL;
  new_dll_block_sec->prev = NULL;

  new_block->miniblock_list = new_dll_block_sec;
  new_block->size = size;
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

  if (!new_dll_block_prim->prev) arena->alloc_list = new_dll_block_prim;
}

void free_block(arena_t *arena, const uint64_t address)
{
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }
  if (!arena->alloc_list) {
    puts("Invalid address for free.");
    return;
  }

  list_t *curr_prim = arena->alloc_list;
  for (; curr_prim->next; curr_prim = curr_prim->next)
    if (address < ((block_t *)curr_prim->data)->start_address +
                      ((block_t *)curr_prim->data)->size &&
        address >= ((block_t *)curr_prim->data)->start_address)
      break;

  if (!curr_prim) {
    puts("Invalid address for free.");
    return;
  }

  list_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
  for (; curr_sec; curr_sec = curr_sec->next)
    if (((miniblock_t *)curr_sec->data)->start_address == address) break;

  if (!curr_sec) {
    puts("Invalid address for free.");
    return;
  }

  arena->mblocks--;
  arena->used_mem -= ((miniblock_t *)curr_sec->data)->size;
  if (!curr_sec->prev) {
    if (!curr_sec->next) {
      arena->blocks--;
      free_mblock(curr_sec);
      if (curr_prim->prev)
        curr_prim->prev->next = curr_prim->next;
      else
        arena->alloc_list = curr_prim->next;
      if (curr_prim->next) curr_prim->next->prev = curr_prim->prev;
      free(curr_prim->data);
      free(curr_prim);
      return;
    } else {
      ((block_t *)curr_prim->data)->size -= ((miniblock_t *)curr_sec->data)->size;
      ((block_t *)curr_prim->data)->miniblock_list = curr_sec->next;
      ((block_t *)curr_prim->data)->start_address =
          ((miniblock_t *)curr_sec->next->data)->start_address;
      curr_sec->next->prev = NULL;
      free_mblock(curr_sec);
      return;
    }
  }
  if (!curr_sec->next) {
    ((block_t *)curr_prim->data)->size -= ((miniblock_t *)curr_sec->data)->size;
    curr_sec->prev->next = NULL;
    free_mblock(curr_sec);
    return;
  }

  arena->blocks++;
  list_t *new_dll_block = malloc(sizeof(list_t));
  DIE(!new_dll_block, "Malloc failed\n");
  block_t *new_block = malloc(sizeof(block_t));
  DIE(!new_block, "Malloc failed\n");

  uint64_t size_b1 = ((miniblock_t *)curr_sec->data)->start_address -
                     ((block_t *)curr_prim->data)->start_address;
  new_block->miniblock_list = curr_sec->next;
  new_block->start_address = ((miniblock_t *)curr_sec->next->data)->start_address;
  new_block->size = ((block_t *)curr_prim->data)->size -
                    ((miniblock_t *)curr_sec->data)->size - size_b1;

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

void read(arena_t *arena, uint64_t address, uint64_t size)
{
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }
  if (!arena->alloc_list) {
    puts("Invalid address for read.");
    return;
  }

  list_t *curr_prim = arena->alloc_list;
  for (; curr_prim; curr_prim = curr_prim->next)
    if (address < ((block_t *)curr_prim->data)->start_address +
                      ((block_t *)curr_prim->data)->size)
      break;

  if (!curr_prim) {
    puts("Invalid address for read.");
    return;
  }

  list_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
  for (; curr_sec; curr_sec = curr_sec->next)
    if (address < ((miniblock_t *)curr_sec->data)->start_address +
                      ((miniblock_t *)curr_sec->data)->size)
      break;

  if (!curr_sec) {
    puts("Invalid address for read.");
    return;
  }

  miniblock_t *block = ((miniblock_t *)curr_sec->data);
  uint64_t offset_prim = address - ((block_t *)curr_prim->data)->start_address;
  uint64_t offset_sec = address - block->start_address;
  uint64_t mut_size = size;

  if (((block_t *)curr_prim->data)->size - offset_prim < mut_size) {
    mut_size = ((block_t *)curr_prim->data)->size - offset_prim;
    printf(
        "Warning: size was bigger than the block size. Reading %lu "
        "characters.\n",
        mut_size);
  }

  size_t mut_size_tmp = mut_size, offset_sec_tmp = offset_sec;
  list_t *curr_tmp = curr_sec;

  while (mut_size_tmp) {
    block = ((miniblock_t *)curr_tmp->data);
    if (!(block->perm & (1 << 2))) {
      puts("Invalid permissions for read.");
      return;
    }
    if (block->size - offset_sec >= mut_size) break;
    mut_size_tmp -= block->size - offset_sec_tmp;
    curr_tmp = curr_tmp->next;
    offset_sec_tmp = 0;
  }

  block = ((miniblock_t *)curr_sec->data);
  while (mut_size) {
    block = ((miniblock_t *)curr_sec->data);
    if (block->size - offset_sec >= mut_size) {
      printf("%.*s\n", (int)mut_size, (char *)block->rw_buffer + offset_sec);
      break;
    }
    printf("%.*s", (int)(block->size - offset_sec),
           (char *)block->rw_buffer + offset_sec);
    mut_size -= block->size - offset_sec;
    curr_sec = curr_sec->next;
    offset_sec = 0;
  }
}

void write(arena_t *arena, const uint64_t address, const uint64_t size, int8_t *data)
{
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }

  if (!arena->alloc_list) {
    free(data);
    puts("Invalid address for write.");
    return;
  }

  list_t *curr_prim = arena->alloc_list;
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

  list_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
  for (; curr_sec; curr_sec = curr_sec->next)
    if (address < ((miniblock_t *)curr_sec->data)->start_address +
                      ((miniblock_t *)curr_sec->data)->size)
      break;

  block_t *block = ((block_t *)curr_prim->data);
  miniblock_t *mblock = ((miniblock_t *)curr_sec->data);
  uint64_t offset_prim = address - block->start_address;
  uint64_t offset_sec = address - mblock->start_address;
  void *tmp = data;

  uint64_t mut_size = size;
  if (block->size - offset_prim < mut_size) {
    mut_size = block->size - offset_prim;
    printf(
        "Warning: size was bigger than the block size. Writing %lu "
        "characters.\n",
        mut_size);
  }

  while (mut_size) {
    mblock = ((miniblock_t *)curr_sec->data);
    if (!(mblock->perm & (1 << 1))) {
      free(data);
      puts("Invalid permissions for write.");
      return;
    }

    if (mblock->size - offset_sec >= size) {
      memcpy(mblock->rw_buffer + offset_sec, tmp, mut_size);
      break;
    }
    memcpy(mblock->rw_buffer + offset_sec, tmp, mblock->size - offset_sec);
    tmp += mblock->size - offset_sec;
    mut_size -= mblock->size - offset_sec;
    curr_sec = curr_sec->next;
    offset_sec = 0;
  }
  free(data);
}

void pmap(const arena_t *arena)
{
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }
  printf(
      "Total memory: 0x%lX bytes\nFree memory: 0x%lX bytes\nNumber of "
      "allocated "
      "blocks: %lu\nNumber of allocated miniblocks: %lu\n",
      arena->arena_size, arena->arena_size - arena->used_mem, arena->blocks,
      arena->mblocks);
  if (arena->alloc_list) putchar('\n');

  list_t *curr_prim = arena->alloc_list, *curr_sec;
  size_t i, j;
  for (i = 1; curr_prim; curr_prim = curr_prim->next, i++) {
    printf("Block %lu begin\n", i);
    printf("Zone: 0x%lX - 0x%lX\n", ((block_t *)curr_prim->data)->start_address,
           ((block_t *)curr_prim->data)->start_address +
               ((block_t *)curr_prim->data)->size);
    curr_sec = ((block_t *)curr_prim->data)->miniblock_list;

    for (j = 1; curr_sec; curr_sec = curr_sec->next, j++) {
      printf("Miniblock %lu:\t\t", j);
      printf("0x%lX\t\t-\t\t0x%lX\t\t| ",
             ((miniblock_t *)curr_sec->data)->start_address,
             ((miniblock_t *)curr_sec->data)->start_address +
                 ((miniblock_t *)curr_sec->data)->size);

      if (((miniblock_t *)curr_sec->data)->perm / 4 == 1)
        putchar('R');
      else
        putchar('-');
      if (((miniblock_t *)curr_sec->data)->perm / 2 % 2 == 1)
        putchar('W');
      else
        putchar('-');
      if (((miniblock_t *)curr_sec->data)->perm % 2 == 1)
        putchar('X');
      else
        putchar('-');
      putchar('\n');
    }
    printf("Block %lu end\n", i);
    if (curr_prim->next) putchar('\n');
  }
}

void mprotect(arena_t *arena, uint64_t address, int8_t *permission)
{
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }
  if (!arena->alloc_list) {
    puts("No blocks allocated.");
    return;
  }

  list_t *curr_prim = arena->alloc_list;
  for (; curr_prim->next; curr_prim = curr_prim->next)
    if (address < ((block_t *)curr_prim->next->data)->start_address) break;

  list_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
  for (; curr_sec; curr_sec = curr_sec->next)
    if (((miniblock_t *)curr_sec->data)->start_address == address) break;

  if (!curr_sec) {
    free(permission);
    puts("Invalid address for mprotect.");
    return;
  }

  char *prot = malloc(((strlen((char *)permission) / 9) + 1) * sizeof(char)), *cpy_str;
  size_t dim = 0;
  cpy_str = strtok((char *)permission, " |");
  for (; cpy_str; cpy_str = strtok(NULL, " |"), dim++) {
    prot[dim] = interpret_prot_str(cpy_str);
    if (prot[dim] == -1) {
      free(permission);
      free(prot);
      puts("Invalid command. Please try again.");
      return;
    }
  }
  free(permission);

  miniblock_t *mblock = (miniblock_t *)curr_sec->data;
  size_t i = 0;
  mblock->perm = 0;
  for (; i < dim; i++) {
    if (prot[i] == 0) {
      mblock->perm = 0;
      continue;
    }
    mblock->perm += prot[i] * (1 - ((mblock->perm >> (prot[i] / 2)) & 1));
  }
  free(prot);
}
