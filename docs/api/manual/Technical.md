# Technical Details

The uVisor is initialized right after device reset. For allowing application of System-On-Chip (SoC) specific quirks or clock initialization a the SystemInit hook is available for early hardware clock initialization.
After initializing the *A*ccess *C*ontrol *L*ists (ACL's) for each individual security domain (process) the uVisor sets up a protected environment using a Memory Protection Unit (the ARM Cortex-M MPU or a vendor-specific alternative).

After initializing itself, the uVisor turns the memory protection turned on, drops execution to unprivileged mode and starts the operating system initialization and continues the boot process.

In case of privileged interrupts, uVisor forwards and de-privileges them and passes execution to the unprivileged operating system or applications interrupt handler.
To protect from information leakage between mutually distrustful security domains, uVisor saves and clears all relevant CPU core registers when interrupting one security context by another   registers leakage when switching execution between privileged and unprivileged code and between mutually untrusted unprivileged modules.

Memory access via DMA engines can potentially bypass uVisor security policies - therefore access to security-critical peripherals (like DMA) need to be restricted to permitted memory ranges by SVCall-based APIs.


## Unified Inter-Process & Inter-Thread Communication

To ensure simplicity and portability of applications, uVisor provides a common API for communicating between threads and secure domains. The API provides simple means to send information packets across threads and processes.

Inter-process Communication Endpoints have three levels of visibility:
- **Anonymous Queues** have only validity within that specific process. The corresponding handles are unknown outside of the process context.
- **Local Queues** can be written-to across process boundaries on the same device.
- **Named Queues** combined with the process name-space identifier these queues can be used to address a specific process remotely

Each recipient of a message can determine the origin of that message (sender). uVisor ensures the integrity of the identity information. Each received message has caller-related metadata stored along with it.

## Sending Messages

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

### Receiving Messages

Each process that is interested in receiving messages, registers at least one message queue.

### Message Fragmentation

To enable assembly of messages on the fly or fragmentation, also scatter gather is supported:
```C
int ipc_send_sg(const IPC_handle* handle, IPC_Scatter_Gather* msg, int count);
```
Across a security boundary scatter-gather can seamlessly interface with a non-scatter-gather receiver, as uVisor can copy a fragmented message into a non-fragmented receiver queue. In case the target queue is full the individual message too large, the command returns with an error.

## Memory Layout and Management

Different memory layouts can be used on different platforms, depending on the implemented memory protection scheme and on the MPU architecture. The following figure shows the memory layout of a system where the uVisor shares the SRAM module with the operating system (ARMv7-M MPU).

uVisor provides only methods for allocating and de-allocating large blocks of uniformely sized memory to avoid fragmentation (for example: 4kb chunk size on a 64kb sized SRAM).

Each process gets at compile-time a fixed memory pool assigned which will be used for heap, stack, thread-local storage and messaging.

![](images/memory_layout.png){ width=15cm }

The uVisor secures two main memory blocks, in flash and SRAM respectively. In both cases, it protects its own data and the data of the secure boxes it manages for the unprivileged code.

All the unprivileged code that is not protected in a secure domain is referred to as the *main process*.

The main memory sections protected by uVisor are detailed in the following table:

------------------------------------------------------------------------
**Memory section**          **Description**
--------------------------- --------------------------------------------
uVisor code                 The uVisor code is readable and executable
                            by unprivileged code, so that code sharing
                            is facilitated and privileged-unprivileged
                            transitions are easier.

uVisor data/BSS/stack       The uVisor places all its constants,
                            initialized and uninitialized data and the
                            stack in secured areas of memory, separated
                            from the unprivileged code.

Secure boxes data/BSS/stack Through a configuration process,
                            unprivileged code can set up a secure
                            process for which data and stack can be
                            secured by the uVisor and placed in
                            isolated and protected memory areas.

Vector table                Interrupt vectors are relocated to the SRAM
                            but protected by the uVisor. Access to IRQs
                            is made through specific APIs.
------------------------------------------------------------------------
