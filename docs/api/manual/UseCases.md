# High Level Design Overview

## Memory Allocation System

Although many processes or secure domains might be fine with just using statically allocated memories, there's a need for reliable, safe and secure memory allocation. This need for dynamic memory allocation is difficult to satisfy on Cortex-M microcontrollers due to the lack of a Memory Management Unit (MMU). 

To avoid memory fragmentation uVisor uses a nested memory allocator.

### Top Level Memory allocation

On the top level three methods exist for allocating memories for a process or thread:

- One static memory region per security context as implemented by uVisor today. The allocation happens during link time and can be influenced by compile time box configuration options. This region contains heap, stack and the thread-local storage.
- After subtracting the per-process memories and the global stack, the remaining memory is split into a coarse set of equally sized large memory pages. For an instance it might make sense to split a 64kb large SRAM block into 8kb pool memory chunks.
- Traditional `sbrk()` compatible interface:
    * sbrk() grows upwards block by block
    * stack grows downwards block by block - never frees a block even if the stack shrinks. 
    * the pool-allocator sits in-between and expands in both directions 

The recommended operation for a process is to keep static memory consumption as low as possible. For occasional device operations with large dynamic memory consumption, the corresponding process temporarily allocates one or more blocks of from the memory pool.

### Per-Process Memory Allocator

The Process memory-allocator by default uses the the heap as optionally reserved per process through link time. In a uVisor-less environment the whole program runs in one process.

When starting a seldom-running, but high-memory-impact thread, the developer has the choice to tie the thread to a thread-specific heap. In a uVisor-less environment that would look like this:

```C
/* rounded to block size, */
uint8_t* thread_mem = malloc_block(MAX_UPDATE_HEAP_SIZE+MAX_UPDATE_STACK_SIZE);
if(thread_heap)
{
    /* create thread with default heap size and stack size, heap pointer
       and heap size are optional and can be set to zero - allocations
       within the thread will happen in the process heap. 
     */  
    handle = create_thread(
        NORMAL_PRIORITY,
        thread_mem+MAX_UPDATE_STACK_SIZE, /* heap above stack */
        MAX_UPDATE_HEAP_SIZE,
        thread_mem, /* stack below heap */
        MAX_UPDATE_STACK_SIZE)

    /* do error handling here */
}
```

All memories allocated outside of the thread will be either allocated on the static heap of the process. In case a thread does set the heap pointer to NULL or the heap size to zero, memory allocations will be forwarded to the processes memory.

Once the dynamic operation terminates, the threads are terminated and the corresponding memory blocks are freed. In case allocations happened outside of the process, these will be still around on the process heap or other thread-specific heaps.

As a result memory fragmentation can effectively avoided - independent of uVisor usage.

### Memory Allocation Notes
- ACL's might be used to restrict the maximum amount of memory blocks for each security context.
- Processes might allocate multiple blocks, the allocated blocks would not be continuous - but can be covered by the tread-specific allocator as long as the allocated blocks are smaller than the block size.
- If a complex function requires multiple threads, these can be allocated into the same memory block if needed.
- The Thread-Local Storage is deducted from the box specific heap upon thread creation.

## Inter-Process Communication


