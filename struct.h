#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct dll_block_t {
  void *data;
  struct dll_block_t *prev;
  struct dll_block_t *next;
} dll_block_t;

typedef struct block_t {
  uint64_t start_address;  // adresa de început a zonei, un indice din arenă
  size_t size;             // dimensiunea totală a zonei, suma size-urilor
                           // miniblock-urilor
  dll_block_t* miniblock_list;    // lista de miniblock-uri adiacente
} block_t;

typedef struct mblock_t {
  uint64_t start_address;  // adresa de început a zonei, un indice din arenă
  size_t size;             // size-ul miniblock-ului
  uint8_t perm;            // permisiunile asociate zonei, by default RW-
  void* rw_buffer;  // buffer-ul de date, folosit pentru opearțiile de read()
                    // și write()
} mblock_t;

typedef struct arena_t {
  uint64_t size;
  size_t used_mem, blocks, mblocks;
  dll_block_t *list;
} arena_t;
