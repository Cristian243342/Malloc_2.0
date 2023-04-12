// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#include <string.h>

// Interprets the type of permission.
#define INTERPRET(cmd, string, c)				\
	do {										\
		if (strcmp(cmd, string) == 0)			\
			car = c;							\
	} while (0)

// Interprets a permission string and returns it's corresponding number.
char interpret_prot_str(char prot_str[15])
{
	char car = -1;
	INTERPRET(prot_str, "PROT_NONE", 0);
	INTERPRET(prot_str, "PROT_READ", 4);
	INTERPRET(prot_str, "PROT_WRITE", 2);
	INTERPRET(prot_str, "PROT_EXEC", 1);
	return car;
}
