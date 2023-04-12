// Copyright Lazar Cristian-Stefan 314CA 2022-2023
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include "struct.h"

// Checks if all the miniblock from which the data will be read have the
// read permission.
int8_t check_perm_read(uint64_t size, list_t *curr, uint64_t offset)
{
	miniblock_t *block;
	while (size) {
		block = ((miniblock_t *)curr->data);
		if (!(block->perm & (1 << 2))) {
			puts("Invalid permissions for read.");
			return 0;
		}
		if (block->size - offset >= size)
			break;
		size -= block->size - offset;
		curr = curr->next;
		offset = 0;
	}

	return 1;
}

// Reads the data from the block.
void read_from_block(uint64_t size, list_t *curr, uint64_t offset)
{
	uint64_t i;

	// Reads the requested data.
	while (1) {
		miniblock_t *block = ((miniblock_t *)curr->data);

		// Checks if this is the last miniblock from which the data is read.
		if (block->size - offset >= size) {
			for (i = 0; i < size; i++)
				if (((char *)block->rw_buffer + offset)[i] != '\000')
					printf("%c", ((char *)block->rw_buffer + offset)[i]);
			printf("\n");
			break;
		}
		for (i = 0; i < block->size - offset; i++)
			if (((char *)block->rw_buffer + offset)[i] != '\000')
				printf("%c", ((char *)block->rw_buffer + offset)[i]);

		// We substract from the mutable size the number of bytes read.
		size -= block->size - offset;

		// We move to the next miniblock from which data will be read.
		curr = curr->next;

		// An offset can only exist between the start address of the miniblock
		// containing the given address. For all other miniblock from which data
		// is read, the data is read from the start address, so the offset is
		// made 0.
		offset = 0;
	}
}

// Checks if all the miniblock where the data will be writen have the write
// permission.
int8_t check_perm_write(uint64_t size, list_t *curr, uint64_t offset)
{
	miniblock_t *block;
	while (size) {
		block = ((miniblock_t *)curr->data);
		if (!(block->perm & (1 << 1))) {
			puts("Invalid permissions for write.");
			return 0;
		}
		if (block->size - offset >= size)
			break;
		size -= block->size - offset;
		curr = curr->next;
		offset = 0;
	}

	return 1;
}

// Writes the data in the block.
void write_in_block(uint64_t size, list_t *curr, uint64_t offset, void *data)
{
	miniblock_t *mblock;
	while (size) {
		mblock = ((miniblock_t *)curr->data);

		// Checks if this is the last miniblock where the data is writen.
		if (mblock->size - offset >= size) {
			memcpy(mblock->rw_buffer + offset, data, size);
			break;
		}
		memcpy(mblock->rw_buffer + offset, data, mblock->size - offset);
		data += mblock->size - offset;

		// We substract from the mutable size the number of bytes writen.
		size -= mblock->size - offset;

		// We move to the next miniblock where data will be written.
		curr = curr->next;

		// An offset can only exist between the start address of the miniblock
		// containing the given address. For all other miniblock whare data is
		// written, the data is written from the start address, so the offset is
		// made 0.
		offset = 0;
	}
}
