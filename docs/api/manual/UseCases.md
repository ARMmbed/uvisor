# High Level Design Overview

## Memory Allocation System

Although many processes or secure domains might be fine with just using statically allocated memories, there's a need for reliable, safe and secure memory allocation. This need for dynamic memory allocation is difficult to satisfy on Cortex-M microcontrollers due to the lack of a Memory Management Unit (MMU). 

To avoid memory fragmentation uVisor uses a nested memory allocator.

### Top Level Memory allocation

On the top level three methods exist for allocating memories for a process or thread:

- One static memory region per security context as implemented by uVisor today. The allocation happens during link time and can be influenced by compile time box configuration options. This region contains heap, stack and the thread-local storage.
- After subtracting the per-process memories and the global stack, the remaining memory is split into a coarse set of equally sized large memory pages. For an instance it might make sense to split a 64kb large SRAM block into 8kb pool memory chunks.
- Traditional `sbrk()`-compatible interface:
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
```https://github.com/ARMmbed/example-uvisor-box-id
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
