#pragma once

void add_last(uint64_t address, size_t mblock_size, list_t *dll_block,
              char ver);
void add_first(uint64_t address, size_t mblock_size, block_t *block);
void add_middle(uint64_t address, size_t mblock_size, list_t *dll_block);

