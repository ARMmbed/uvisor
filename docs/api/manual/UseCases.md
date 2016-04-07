# High Level Design Overview

## Memory Allocation System

Although many processes or secure domains might be fine with just using statically allocated memories, there's a need for reliable, safe and secure memory allocation. This need for dynamic memory allocation is difficult to satisfy on Cortex-M microcontrollers due to the lack of a Memory Management Unit (MMU).

To avoid memory fragmentation uVisor uses a nested memory allocator.

### Top Level Memory allocation

On the top level three methods exist for allocating memories for a process or thread:

- One static memory region per security context as implemented by uVisor today. The allocation happens during link time and can be influenced by compile time box configuration options. This region contains heap, stack and the thread-local storage.
- After subtracting the per-process memories and the global stack, the remaining memory is split into a coarse set of equally sized large memory pages. For an instance it might make sense to split a 64kb large SRAM block into 8kb pool memory chunks.

The recommended operation for a process is to keep static memory consumption as low as possible. For occasional device operations with large dynamic memory consumption, the corresponding process temporarily allocates one or more pages of from the memory pool.

### Tier-1 Page Allocator

The tier-1 page allocator hands out pages and secures access to them. It is part of the uVisor core functionality, and therefore is only accessible via a secure gateway.

On boot, it initializes the heap memory into correctly aligned and equally-sized memory pages.
The page size is known at compile time, however, the number of pages is known only to the allocator and only at runtime, due to alignment requirements.

#### Requesting Pages

```C
int uvisor_page_malloc(uvisor_page_table_t *const table);
```

A process can request pages by passing a page table to the allocator containing the number of required pages, the required page size as well as the page table.
Note that the tier-1 allocator does not allocate any memory for the page table, but works on the memory provided.
If the tier-2 allocator request 5 pages, it needs to make sure the page table is large enough to hold 5 page origins!
This allows the use of statically as well as dynamically allocated page tables.

A page table looks like this:
```C
typedef struct {
    size_t page_size;       //< the page size in bytes
    size_t page_count;      //< the number of pages in the page table
    void* page_origins[1];  //< table of pointer to the origins of a page
} uvisor_page_table_t;
```

If enough free pages are available, the allocator will mark them as in use by this process, add them to the process's ACLs, zero them, and write each page origin into the page table.

Note that the returned pages are absolutely not guaranteed to be allocated consecutively.
It is the responsibility of the tier-2 allocator to make sure that memory requests that exceed the requested page size are blocked.

However, it is possible to request pages sizes that are multiples of the physical page sizes.
So, if the physical page size is 8kB, but the tier-2 allocator needs to have at least 12kB of continuous memory, it may request one page with a size of 16kB, instead of two pages with a size of 8kB.
The tier-1 allocator will then try to find two free pages next to each other and return them as one big page.

#### Freeing pages

```C
int uvisor_page_free(const uvisor_page_table_t *const table);
```
A process can free pages by passing a page table to the allocator.
The allocator first checks the validity of the page table, to make sure the pages are owned by the calling process, their origins pointer make sense, etc.
Only then will the allocator return the pages to the pool.

Hint: Use the page table that was returned on allocation. :-)


### Tier-2 Memory Allocator

The tier-2 memory allocator provides a common interface to manage memory backed by tier-1 allocated pages or statically allocated memory. The tier-2 allocator is part of the uVisor library and calls to it do not require a secure gateway, since it can only operate on memory owned by the process anyway.

All memory management data is contained within the memory pool and its overhead depends on the management algorithm.
An allocator handle is simply an opaque pointer.
```C
typedef void* uvisor_allocator_t;
```

#### Initializing Static Memory

The process heap is allocated statically, therefore the tier-2 allocator is constrained to this pool:
```C
uvisor_allocator_t uvisor_allocator_create_with_pool(
    void* mem,      ///< origin of pool
    size_t bytes);  ///< size of pool in bytes
```
Note that the process heap is initialized by uVisor-lib when setting up the environment for each process!

#### Initializing Page-Backed Memory

```C
/* Places both the stack and heap in the same page(s) */
uvisor_allocator_t uvisor_allocator_create_with_pages(
    size_t heap_size,       ///< total heap size, can be fragmented
    size_t max_heap_alloc); ///< maximum continuous heap allocation
```

The tier-2 allocator computes the required page size and page count from the `heap` size, taking account the requirement that the `max_heap_alloc` needs to be placed in one continuous memory section.

For example: With a heap size of 12kB (6kB continuous) and a physical page size of 8kB, two non-consecutive pages would satisfy this requirement.
However, if instead of 6kB we would need 9kB of continuous heap allocation, we would require one page of 16kB, ie. two consecutive 8kB.

Note that no guarantee can be made that the maximum continuous heap allocation will be available to the caller.
This depends on the fragmentation properties of the tier-2 memory management algorithm.
This computation only makes it physically possible to perform such a continuous allocation!

Note that the memory for the page table is dynamically allocated inside the process heap!

#### Allocator Lifetime

An allocator for static memory cannot be destroyed during program execution.
Attempting to do so will result in an error.

An allocator for page-backed memory is bound to a process thread. It will be destroyed and its backing pages released only when the thread is destroyed as well.
Other threads within the same process are not allowed to allocate inside the page-backed memory of another thread.
However, they may of course write and read into the page-backed memory of another thread, but that thread must first allocate memory for it and pass the pointer to the other thread.

This restriction ensures that when a thread finishes execution, it is safe to return all its pages to the memory pool, without the risk of deleting an allocation from another thread.
It also removes the need to keep track which thread allocated in what page.

```C
int uvisor_allocator_destroy(uvisor_allocator_t allocator);
```

#### Memory Management

Three functions are provided to manage memory in a process:
```C
void* uvisor_malloc(uvisor_allocator_t allocator, size_t size);
void* uvisor_realloc(uvisor_allocator_t allocator, void *ptr, size_t size);
void uvisor_free(uvisor_allocator_t allocator, void *ptr);
```

These functions simply multiplex the `malloc`, `realloc` and `free` to chosen allocator.
This automatically takes into account non-consecutive page tables.

The tier-2 allocator uses the CMSIS-RTOS `rt_memory` allocator as a backend to provide thread-safe access to memory pools.
An alternative backend is the Two-Level Segregated Fit (TLSF) algorithm, a real-time O(1) algorithm, which is  not thread-safe, however.
`dlmalloc` was also briefly considered as a backend, however, it does not deal with multiple memory pools, and adding that functionality is difficult.

### Allocator Management

The current allocator is swapped out by the scheduler to provide the canonical memory management functions `malloc`, `realloc` and `free` to processes and threads.
The escalation scheme used for this is "Page-backed Thread Heap" -> "Static Process Heap" -> "Static Legacy Process Heap":

1. In a thread with its own page-backed heap, allocations will only be services from its own heap, not the process heap. If it runs out of memory, no fallback is provided.
2. In a thread without its own page-backed heap, allocations will be serviced from the statically allocated process heap. If it runs out of memory, no fallback is provided.
3. In a process with statically allocated heap, allocations will be serviced from this heap. If it runs out of memory, no fallback is provided.
4. In a process without statically allocated heap, allocations will be serviced from the statically allocated insecure process heap.

A thread may force an allocation on the process heap using the `ps_malloc`, `ps_realloc` and `ps_free` functions.
This enables a worker thread with page-backed heap to store for example its final computation result on the process heap, and notify its completion, and then stop execution without having to wait for another thread to copy this result out of its heap.

#### Per-Process Memory Allocator

The Process memory-allocator by default uses the the heap as optionally reserved per process through link time. In a uVisor-less environment the whole program runs in one process.

When starting a seldom-running, but high-memory-impact thread, the developer has the choice to tie the thread to a thread-specific heap. In a uVisor-less environment that would look like this:

```C
uvisor_stack_t thread_stack = process_create_stack(3kB);
uvisor_allocator_t thread_heap = uvisor_allocator_create_with_pages(
    12kB,           /* total heap size */
    6kB);           /* max continuous heap allocation */
/* alternative: allocate stack inside thread_heap */
thread_stack = process_create_stack_with_allocator(thread_heap);
if(thread_stack && thread_heap)
{
    handle = create_thread(
        NORMAL_PRIORITY,
        &thread_stack,
        &thread_heap);
}

/* thread is using no dynamic memory, or allocates on the process heap */
uvisor_stack_t thread_stack = process_create_stack(3kB);
if (thread_stack)
{
    handle = create_thread(
        NORMAL_PRIORITY,
        &thread_stack,
        NULL);
}
```

Note: `process_create_stack(size_t stack_size)` allocates memory depending on the processes preference.
It may allocate on the static process heap, or may request an external page and allocate stacks for multiple threads in this one page. The thread stack may also be allocated inside the page-backed memory.

All memories allocated outside of the thread will be either allocated on the static heap of the process. In case a thread does set the heap pointer to NULL or the heap size to zero, memory allocations will be forwarded to the processes memory.

Once the dynamic operation terminates, the threads are terminated and the corresponding memory blocks are freed. In case allocations happened outside of the process, these will be still around on the process heap or other thread-specific heaps.

As a result memory fragmentation can effectively avoided - independent of uVisor usage.

### Memory Allocation Notes
- ACL's might be used to restrict the maximum amount of memory blocks for each security context.
- Processes might allocate multiple blocks, the allocated blocks would not be continuous - but can be covered by the tread-specific allocator as long as the allocated blocks are smaller than the block size.
- If a complex function requires multiple threads, these can be allocated into the same memory block if needed.
- The Thread-Local Storage is deducted from the box specific heap upon thread creation.


## Unified Process & Thread Communication

To ensure simplicity and portability of applications, uVisor provides a common API for communicating between threads and secure domains. The API provides simple means to send information packets across threads and processes.

Inter-process Communication Endpoints have three levels of visibility:

- **Anonymous Queues** have only validity within that specific process. The corresponding handles are unknown outside of the process context.
- **Local Queues** can be written-to across process boundaries on the same device.
- **Named Queues** combined with the process name-space identifier these queues can be used to address a specific process remotely

Each recipient of a message can determine the origin of that message (sender). uVisor ensures the integrity of the identity information. Each received message has caller-related metadata stored along with it.

### Send Local Messages

The design goal for the inter-process-communication (IPC) is not having to know whether a communication target runs within the same security domain or the security domain of someone else on the same device:

```C
int ipc_send(const IPC_handle* handle, const void* msg, size_t len);
```
The `ipc_send` call copies the provide `msg` of `len` bytes to the recipients receive queue. After ipc_send's return the caller can assume that the msg buffer is no longer needed. This enables easy assembly of messages on the fly on stack.

uVisor intentionally imposes the restriction not to pass messages by reference, as the layout of the MPU protection is very much runtime-specific and of unpredictable performance due the fragmentation and large count of virtualized MPU regions.

Instead uVisor provides means to pass messages effectively by copying data across security boundaries. Advanced implementation can provide reference counted pools to avoid copying messages for the intra-process communication case.

Each target queue imposes custom restrictions for maximum size or slots in the message queue. The message queue is pool-allocation friendly as long as the receiving process or thread implements a ipc_receive_sg call.

The target queue is only operated through a previously registered callback interface API - uVisor does not need to understand the storage format for the message in SRAM. After receiving data the receiving process can notify the thread responsible for handling the message.

The IPC_handle both encodes the target process security domain and the corresponding thread ID.

### Receiving Local Messages

Each process that is interested in receiving messages, registers at least one message queue. Using process specific APIs external processes can ask for a specific message queue handle (IPC_handle). A process has the option restrict inbound communication to individual processes for safety reasons. An rogue process can't spam the message queue buffers as a result. The queue implementation supports multiple writers and multiple readers.

For each message queue entry uVisor provides meta data like message size, origin of the message in form of the process ID of the sender. As process IDs can change over time, the process ID can be turned into a process identity using the [Box ID API of uVisor](https://github.com/ARMmbed/uvisor-lib/blob/master/DOCUMENTATION.md#box-identity).

### Receiving Remote Messages

Remote messages interface with the rest of the system through a **message dispatcher**. The delivered messages are expected to be end-to-end secured at least with regard to their authorization. The dispatcher has the responsibility to verify the authorization of messages at the entry to the system. Each message has an untrusted string ID corresponding to the box name space.

After successfully verifying the inbound security message, the dispatcher will to remember the verified senders namespace string and annotate the received payload with the ID of the saved namespace string.

Using the namespace ID the receiving process can query the namespace string using the dispatcher API. These IDs are locally unique, so the namespace lookup only has to be done on the first occurrence of a new ID. On subsequent messages only the numeric IDs need to be checked.

The message dispatcher optionally runs in a dedicated secure process protected by uVisor. Using API calls each process can register a queue name string - making that queue globally available:

```bash
https://[2001:470:531a::2]/dispatch/com.arm.keyprov/queue.name/key.name
```
The receiving process will do a two-step verification:

- verify the identity of the dispatcher
- if the identity of the dispatcher is trusted, the receiver can trust the namespace annotation, as the dispatcher is trusted to have performed the verification.

### Message Fragmentation

To enable assembly of messages on the fly or fragmentation, also scatter gather is supported:
```C
int ipc_send_sg(const IPC_handle* handle, IPC_Scatter_Gather* msg, int count);
```
Across a security boundary scatter-gather can seamlessly interface with a non-scatter-gather receiver, as uVisor can copy a fragmented message into a non-fragmented receiver queue. In case the target queue is full the individual message too large, the command returns with an error.
