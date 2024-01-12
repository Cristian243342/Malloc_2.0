***Copyright** Lazar Cristian-Stefan 314CA 2022-2023*

# Malloc 2.0 Electric Boogaloo

Malloc 2.0 is a virtual memory allocator, which utilizes a virtual arena of addresses, of a specified size, to map blocks of allocated memory.

---

## Usage

Once the program is run, the user can execute different commands, listed below:

* ALLOC_ARENA \<$size$>
  Allocates a virtual memory arena of the specified size. This is the first command that must be executed after starting the program.

* DEALLOC_ARENA
  Unmaps all allocated blocks and destroys the virtual arena. Then, it closes the program.

* ALLOC_BLOCK \<$start\_address$> \<$size$>
  Maps a continuous block of $size$ bytes at $start\_address$ in the arena. If the block is contiguous with other blocks in the arena, then the respective blocks will be merged into one, continuous block.

* FREE_BLOCK \<$start\_address$>
  Deallocates the memory block with the given $start\_address$.

* WRITE \<$start\_address$> \<$size$>
  This command will read $size$ characters from the command line and store them at the $start\_address$ as a continuous block of data. The $start\_address$ must have been previously allocated.

  If $size$ is greater than the size of the memory block allocated at $start\_address$, then the program will write as many bytes as there is space for in the block.

* READ \<$start\_address$> \<$size$>
  Reads $size$ bytes from $start\_address$ and prints them to the terminal. $start\_address$ must have been previously allocated.

  If $size$ is greater than the size of the memory block allocated at $start\_address$, then the program will read as many bytes as there are in the block.

* PMAP
  Prints a map of the currently allocated blocks of memory, as well as the metadata of the arena.

* MPROTECT \<$start\_address$> \<$prot_type$>
  Changes the permissions of the block with the given start address. The block must have been previously allocated.

  The new permissions are given as a string of instructions, concatenated with the '|' character. The possible instructions are:
  * PROT_NONE : Removes all permissions from the block.
  * PROT_READ : Gives the block the read permission.
  * PROT_WRITE : Gives the block the write permission.
  * PROT_EXEC : Gives the block the execution permission.

  Example:

  ```text
  MPROTECT 120 PROT_NONE | PROT_EXEC | PROT_WRITE
  ```

  In this example, the block with the start_address 120 will first have all its permissions removed and then will be given the execution and the write permissions.

---

## Implementation

* ### The memory arena

  The arena stores addresses as numbers, between 0 (inclusive) and the size of the arena (exclusive), and at each address **one** byte of data is stored.

  The arena stores the allocated [blocks](#the-block-structure "The block structure") of memory in a [doubly linked list](#the-doubly-linked-list-node-structure "The doubly linked list node structure"), sorted form the block with the lowest start address to the block with the highest start address, to facilitate the search for a specific address in the arena.

  Each block of memory represents a list of [miniblock](#the-miniblock-structure "The miniblock structure"). Miniblocks are the building blocks of the arena, the effective memory space being stored in the miniblock. Multiple contiguous miniblock form a block, which may consist of one or more miniblocks. This hierarchical structure of memory is done because data can be read and written implicitly on the span of multiple miniblocks because the memory is contiguous. However, you cannot write or read data on the span of multiple blocks, unless you split your data and explicitly say in which block the fragments of data will be stored or from which blocks the data will be read.

  #### The arena structure

  ```C
  typedef struct {
  uint64_t arena_size, used_mem, blocks, mblocks;
  list_t *alloc_list;
  } arena_t;
  ```

  * $arena\_size$ stores the total size in bytes of the arena.
  * $used\_mem$ stores the amount of memory allocated, which updates dynamically with each block allocated or removed.
  * $blocks$ stores the number of blocks currently allocated in the arena.
  * $mblocks$ store the number of miniblocks currently allocated in the arena.
  * $alloc\_list$ stores the address of the head of the list of blocks.

  <br>

  #### The block structure

  ```C
  typedef struct {
   uint64_t start_address;
   size_t size;
   void *miniblock_list;
  } block_t;
  ```

  * $start\_address$ stores the start address of the block.
  * $size$ stores the size in bytes of the block.
  * $miniblock\_list$ stores the address of the head of the list of miniblocks.

  <br>

  #### The miniblock structure

  ```C
  typedef struct {
  uint64_t start_address;
  size_t size;
  uint8_t perm;
  void *rw_buffer;
  } miniblock_t;
  ```

  * $start\_address$ stores the start address of the miniblock.
  * $size$ stores the size in bytes of the miniblock.
  * $perm$ stores a number in octal, representing the [rwx permissions](#miniblock-permissions "Permissions table") of the miniblock.
  * $rw\_buffer$ stores the start address of the effective memory allocated by [malloc](https://en.cppreference.com/w/c/memory/malloc "Documentation for the malloc function in C").

  <br>

  #### The doubly linked list node structure

  ```C
  typedef struct list_t {
  void *data;
  struct list_t *prev;
  struct list_t *next;
  } list_t;
  ```

  * $data$ stores the address of the data stored in the node (in this program, the node can only store the address of a block or miniblock).
  * $prev$ stores the address of the previous node in the list (or the NULL address if the node is the first one in the list).
  * $next$ stores the address of the next node in the list (or the NULL address if the node is the last one in the list).

<br>

* ### Miniblock permissions

  |Number in octal|Number in binary|Permissions|Description|
  |:-:|:-:|:-:|:-:|
  |0|000|---|No permissions|
  |1|001|--x|Execute only|
  |2|010|-w-|Write only|
  |3|011|-wx|Write and execute|
  |4|100|r--|Read only|
  |5|101|r-x|Read and execute|
  |6|110|rw-|Read and write|
  |7|111|rwx|All permissions|

  The program interrogates the permissions of a miniblock using bitwise operations.
  * If the last bit is set, then the miniblock has the execute permission.
  * If the second to last bit is set, then the miniblock has the write permission.
  * If the third to last bit is set, then the miniblock has the read permission.
