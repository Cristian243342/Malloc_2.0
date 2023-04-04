#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "alloc.h"
#include "free.h"
#include "mprotect.h"
#include "pmap.h"
#include "rw.h"
#include "struct.h"
//#include "vma.h"

arena_t *arena;

#define INTERPRET(cmd, string, c)           \
  do {                                      \
    if (strcmp(cmd, string) == 0) return c; \
  } while (0)

char interpret_cmd(char cmd[15]) {
  INTERPRET(cmd, "ALLOC_ARENA", 'a');
  INTERPRET(cmd, "DEALLOC_ARENA", 'b');
  INTERPRET(cmd, "ALLOC_BLOCK", 'c');
  INTERPRET(cmd, "FREE_BLOCK", 'd');
  INTERPRET(cmd, "READ", 'e');
  INTERPRET(cmd, "WRITE", 'f');
  INTERPRET(cmd, "PMAP", 'g');
  INTERPRET(cmd, "MPROTECT", 'h');
  return '0';
}

int main(void) {
  char *cmd, cpy[15];
  while (1) {
    scanf("\n%m[^\n]", &cmd);
    sscanf(cmd, "%s", cpy);
    switch (interpret_cmd(cpy)) {
      case 'a':
        alloc_arena(cmd);
        break;
      case 'b':
        dealloc_arena();
        free(cmd);
        return 0;
      case 'c':
        alloc_block(cmd);
        break;
      case 'd':
        free_block(cmd);
        break;
      case 'e':
        read(cmd);
        break;
      case 'f':
        write(cmd);
        break;
      case 'g':
        pmap();
        break;
      case 'h':
        mprotect(cmd);
        break;
      case '0':
        puts("Invalid command. Please try again.");
        break;
    }
    free(cmd);
  }
}
