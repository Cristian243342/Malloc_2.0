#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "struct.h"

extern arena_t *arena;

#define INTERPRET(cmd, string, c)           \
  do {                                      \
    if (strcmp(cmd, string) == 0) return c; \
  } while (0)

char interpret_prot_str(char prot_str[15]) {
  INTERPRET(prot_str, "PROT_NONE", 0);
  INTERPRET(prot_str, "PROT_READ", 4);
  INTERPRET(prot_str, "PROT_WRITE", 2);
  INTERPRET(prot_str, "PROT_EXEC", 1);
  return -1;
}

void mprotect(char *cmd) {
  if (!arena) {
    puts("Arena not allocated.");
    return;
  }
  if (!arena->list) {
    puts("No blocks allocated.");
    return;
  }

  char *prot_str, *cpy_str;
  uint64_t address;
  if (sscanf(cmd, "%*s%lu%m[^\n]", &address, &prot_str) != 2) {
    puts("Invalid command. Please try again.");
    return;
  }

  dll_block_t *curr_prim = arena->list;
  for (; curr_prim->next; curr_prim = curr_prim->next)
    if (address < ((block_t *)curr_prim->next->data)->start_address) break;

  dll_block_t *curr_sec = ((block_t *)curr_prim->data)->miniblock_list;
  for (; curr_sec; curr_sec = curr_sec->next)
    if (((mblock_t *)curr_sec->data)->start_address == address) break;

  if (!curr_sec) {
    free(prot_str);
    puts("Invalid address for mprotect.");
    return;
  }

  char *prot = malloc(((strlen(prot_str) / 9) + 1) * sizeof(char));
  size_t dim = 0;
  cpy_str = strtok(prot_str, " |");
  for (; cpy_str; cpy_str = strtok(NULL, " |"), dim++) {
    prot[dim] = interpret_prot_str(cpy_str);
    if (prot[dim] == -1) {
      free(prot_str);
      free(prot);
      puts("Invalid command. Please try again.");
      return;
    }
  }
  free(prot_str);

  mblock_t *mblock = (mblock_t *)curr_sec->data;
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