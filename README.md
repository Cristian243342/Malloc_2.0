***Copyright** Lazar Cristian-Stefan 314CA 2022-2023*

# Malloc 2.0 Electric Boogaloo

Malloc 2.0 is a virtual memory allocator, which utilizes a virtual arena of addresses, of a specified size, to map blocks of allocated memory.

---

## Who is this program for?

* People who hate dealing with the ~~guaranteed~~ potential errors of working directly with memory.
* People who can't be bothered to learn a programming language in which memory is allocated automatically.

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

---

## The hardest thing <!-- ribbit!-->

By far the hardest part of this homework was understanding it. The ideas presented were a bigger little difficult to understand, which lead to misunderstandings, which, in turn, lead to wastes of time. I think I spent three hours reformating my code because of such a misunderstanding. On the one hand, it's a pretty good learning oportunity, trying to figure what is being asked of me. On the other hand, life at Poli is **BUSY**. I wish I had the time to sit around for three hours and try to understand a task, but I don't, and then I rush into it and stupid mistakes are made. What I will say is, if you wish to challenge us with foreign concepts, try to put more information in the task description. Perhaps some external links to simplified explenations of the concepts, like how a memory allocator works. Even if what this task asked of us isn't nearly as complex as an actual memory allocator, some prior insight might have come in handy in avoiding unnecessary misunderstandings. Plus, having a deeper understanding can make the task seem more purposeful, more exciting and fun, because you do something you understand the meaning of, something you can be proud of, instead of working on something that in your mind is abstract mumbo-jumbo.

All in all, this task would have been fun, if I didn't have, like, 3 other homeworks to worry about **while** trying to understand all of this. I wish I had the time to make stupid mistakes, but I really don't, and I don't think I'm the only one. If I am the only one then, I don't know... Guess I'll throw myself a pity party.
