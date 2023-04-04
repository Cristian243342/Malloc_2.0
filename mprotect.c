#include <string.h>

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
