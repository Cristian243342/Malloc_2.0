// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#pragma once

#include <stdio.h>

#define DIE(assertion, call_description)						\
	do {														\
		if (assertion) {										\
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);	\
			perror(call_description);							\
			exit(1);											\
		}														\
	} while (0)
