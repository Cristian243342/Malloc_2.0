// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#pragma once
#include <inttypes.h>

#include "struct.h"

// Checks if all the miniblock from which the data will be read have the
// read permission.
int8_t check_perm_read(uint64_t mut_size, list_t *curr, uint64_t offset_sec);
// Reads the data from the block.
void read_from_block(uint64_t size, list_t *curr_sec, uint64_t offset);
// Checks if all the miniblock where the data will be writen have the write
// permission.
int8_t check_perm_write(uint64_t size, list_t *curr, uint64_t offset);
// Writes the data in the block.
void write_in_block(uint64_t size, list_t *curr, uint64_t offset, void *data);
