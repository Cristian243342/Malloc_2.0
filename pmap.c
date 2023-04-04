#include <stdint.h>

#include "struct.h"

extern arena_t *arena;

void pmap(void) {
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }
  printf(
      "Total memory: 0x%lX bytes\nFree memory: 0x%lX bytes\nNumber of "
      "allocated "
      "blocks: %lu\nNumber of allocated miniblocks: %lu\n",
      arena->size, arena->size - arena->used_mem, arena->blocks,
      arena->mblocks);
  if (arena->list)
    putchar('\n');

  dll_block_t *curr_prim = arena->list, *curr_sec;
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
             ((mblock_t *)curr_sec->data)->start_address,
             ((mblock_t *)curr_sec->data)->start_address +
                 ((mblock_t *)curr_sec->data)->size);

      if (((mblock_t *)curr_sec->data)->perm / 4 == 1)
        putchar('R');
      else
        putchar('-');
      if (((mblock_t *)curr_sec->data)->perm / 2 % 2 == 1)
        putchar('W');
      else
        putchar('-');
      if (((mblock_t *)curr_sec->data)->perm % 2 == 1)
        putchar('X');
      else
        putchar('-');
      putchar('\n');
    }
    printf("Block %lu end\n", i);
    if (curr_prim->next)
      putchar('\n');
  }
}