# uVisor API documentation

Here you can find detailed documentation for:

1. [Configuration macros](#configuration-macros), to configure a secure box and protect data and peripherals.
1. [Box Identity](#box-identity), to retrieve a box-specific ID or the namespace of the current or calling box.
1. [Low level APIs](#low-level-apis), to access uVisor functions that are not available to unprivileged code (interrupts, restricted system registers).
1. [Type definitions](#type-definitions).
1. [Error codes](#error-codes).

## Configuration macros

```C
UVISOR_BOX_CONFIG(box_name
                  const UvBoxAclItem *module_acl_list,
                  uint32_t module_stack_size,
                  [struct __your_context])
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Secure box configuration</td>
  </tr>
  <tr>
    <td>Type</td>
    <td colspan="2">C/C++ pre-processor macro (pseudo-function)</td>
  </tr>
  <tr>
    <td rowspan="4">Parameters</td>
    <td><pre>box_name<code></td>
    <td>Secure box name</td>
  </tr>
  <tr>
    <td><pre>const UvBoxAclItem *module_acl_list<code></td>
    <td>List of ACLs for the module</td>
  </tr>
  <tr>
    <td><pre>uint32_t module_stack_size<code></td>
    <td>Required stack size for the secure box</td>
  </tr>
  <tr>
    <td><pre>struct __your_context<code></td>
    <td>[optional] Type definition of the struct hosting the box context data</td>
  </tr>
</table>

Example:
```C
#include "uvisor-lib/uvisor-lib.h"

/* Required stack size */
#define BOX_STACK_SIZE 0x100

/* Define the box context. */
typedef struct {
    uint8_t secret[SECRET_SIZE];
    bool initialized;
    State_t current_state
} BoxContext;

/* Create the ACL list for the module. */
static const UvBoxAclItem g_box_acl[] = {
    {PORTB,  sizeof(*PORTB),  UVISOR_TACLDEF_PERIPH},
    {RTC,    sizeof(*RTC),    UVISOR_TACLDEF_PERIPH},
    {LPTMR0, sizeof(*LPTMR0), UVISOR_TACLDEF_PERIPH},
};

/* Configure the secure box compartment. */
UVISOR_BOX_NAMESPACE("com.example.my-box-name");
UVISOR_BOX_CONFIG(my_box_name, g_box_acl, BOX_STACK_SIZE, BoxContext);
```

---

```C
UVISOR_SET_MODE(uvisor_mode);
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">[temporary] Set mode for the uVisor</td>
  </tr>
  <tr>
    <td>Type</td>
    <td colspan="2">C/C++ pre-processor macro (object declaration)</td>
  </tr>
  <tr>
    <td rowspan="3">Parameters</td>
    <td rowspan="3"><pre>uvisor_mode<code></td>
    <td><pre>UVISOR_DISABLED<code> = disabled [default]</td>
  </tr>
    <tr>
    <td><pre>UVISOR_PERMISSIVE<code> = permissive [currently n.a.]</td>
  </tr>
  <tr>
    <td><pre>UVISOR_ENABLED<code> = enabled</td>
  </tr>
</table>

Example:
```C
#include "uvisor-lib/uvisor-lib.h"

/* Set the uVisor mode. */
UVISOR_SET_MODE(UVISOR_ENABLED);
```

**Note:**

1. This macro is only needed temporarily (uVisor disabled by default) and will be removed in the future.

2. This macro must be used only once in the top level yotta executable.

---

```C
UVISOR_SET_MODE_ACL(uvisor_mode, const UvBoxAcl *main_box_acl_list);
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">[temporary] Set mode for the uVisor and provide background
                    ACLs for the main box
    </td>
  </tr>
  <tr>
    <td>Type</td>
    <td colspan="2">C/C++ pre-processor macro (object declaration)</td>
  </tr>
  <tr>
    <td rowspan="4">Parameters</td>
    <td rowspan="3"><pre>uvisor_mode<code></td>
    <td><pre>UVISOR_DISABLED<code> = disabled [default]</td>
  </tr>
    <tr>
    <td><pre>UVISOR_PERMISSIVE<code> = permissive [currently n.a.]</td>
  </tr>
  <tr>
    <td><pre>UVISOR_ENABLED<code> = enabled</td>
  <tr>
    <td><pre>const UvBoxAclItem *main_box_acl_list<code></td>
    <td>List of ACLs for the main box (background ACLs)</td>
  </tr>
</table>

Example:
```C
#include "uvisor-lib/uvisor-lib.h"

/* Create background ACLs for the main box. */
static const UvBoxAclItem g_background_acl[] = {
    {UART0,       sizeof(*UART0), UVISOR_TACL_PERIPHERAL},
    {UART1,       sizeof(*UART1), UVISOR_TACL_PERIPHERAL},
    {PIT,         sizeof(*PIT),   UVISOR_TACL_PERIPHERAL},
};

/* Set the uVisor mode. */
UVISOR_SET_MODE_ACL(UVISOR_ENABLED, g_background_acl);
```

**Note:**

1. This macro is only needed temporarily (uVisor disabled by default) and will be removed in the future.

2. This macro must be used only once in the top level yotta executable.

---

```C
UVISOR_BOX_NAMESPACE(static const char const namespace[])
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2"><p>Specify the namespace for a box.</p>

      <p>The namespace of the box must be a null-terminated string no longer than <code>MAX_BOX_NAMESPACE_LENGTH</code> (including the terminating null). The namespace must also be stored in public flash. uVisor will verify that the namespace is null-terminated and stored in public flash at boot-time, and will halt if the namespace fails this verification.</p>

      <p>For now, use a reverse domain name for the box namespace. If you don't have a reverse domain name, use a GUID string identifier. We currently don't verify that the namespace is globally unique, but we will perform this validation in the future.</p>

      <p>Use of this configuration macro before <code>UVISOR_BOX_CONFIG</code> is required. If you do not wish to give your box a namespace, specify <code>NULL</code> as the namespace to create an anonymous box.</p>
  </tr>
  <tr>
    <td>Type</td>
    <td colspan="2">C/C++ pre-processor macro (pseudo-function)</td>
  </tr>
  <tr>
    <td rowspan="1">Parameters</td>
    <td><code>namespace</code></td>
    <td>The namespace of the box</td>
  </tr>
</table>

Example:
```C
#include "uvisor-lib/uvisor-lib.h"

/* Configure the secure box. */
UVISOR_BOX_NAMESPACE("com.example.my-box-name");
UVISOR_BOX_CONFIG(my_box_name, UVISOR_BOX_STACK_SIZE);
```

## Box Identity

A box identity identifies a security domain uniquely and globally.

The box identity API can be used to determine the source box of an inbound secure gateway call. This can be useful for implementing complex authorization logic between mutually distrustful security domains.

uVisor provides the ability to retrieve the box ID of the current box (`uvisor_box_id_self`), or of the box that most recently called the current box through a secure gateway (`uvisor_box_id_caller`).

The box ID number is not constant and can change between reboots. But, the box ID number can be used as a token to retrieve a constant string identifier, known as the box namespace.

A box namespace is a static, box-specific string, that can help identify which box has which ID at run-time. In the future, the box namespace will be guaranteed to be globally unique.

A full example using this API is available at [example-uvisor-box-id](https://github.com/ARMmbed/example-uvisor-box-id).

```C
int uvisor_box_id_self(void)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Get the ID of the current box</td>
  </tr>
  <tr>
    <td>Return value</td>
    <td colspan="2">The ID of the current box</td>
  </tr>
</table>

---

```C
int uvisor_box_id_caller(void)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Get the ID of the box that is calling the current box through the most recent secure gateway</td>
  </tr>
  <tr>
    <td>Return value</td>
    <td colspan="2">The ID of the caller box, or -1 if there is no secure gateway calling box</td>
  </tr>
</table>

---

```C
int uvisor_box_namespace(int box_id, char *box_namespace, size_t length)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Copy the namespace of the specified box to the provided buffer.</td>
  </tr>
  <tr>
    <td>Return value</td>
    <td colspan="2">Return how many bytes were copied into <code>box_namespace</code>. Return <code>UVISOR_ERROR_INVALID_BOX_ID</code> if the provided box ID is invalid. Return <code>UVISOR_ERROR_BUFFER_TOO_SMALL</code> if the provided <code>box_namespace</code> is too small to hold <code>MAX_BOX_NAMESPACE_LENGTH</code> bytes. Return <code>UVISOR_ERROR_BOX_NAMESPACE_ANONYMOUS</code> if the box is anonymous.</td>
  </tr>
  <tr>
    <td rowspan="3">Parameters</td>
    <td><code>int box_id</code></td>
    <td>The ID of the box you want to retrieve the namespace of</td>
  </tr>
  <tr>
    <td><code>char *box_namespace</code></td>
    <td>The buffer where the box namespace will be copied to</td>
  </tr>
  <tr>
    <td><code>size_t length</code></td>
    <td>The length in bytes of the provided <code>box_namespace</code> buffer</td>
  </tr>
</table>

## Low-level APIs

Currently the following low level operations are permitted:

1. Interrupt management.

### Interrupt management

```C
void vIRQ_SetVector(uint32_t irqn, uint32_t vector)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Register an ISR to the currently active box</td>
  <tr>
    <td rowspan="2">Parameters</td>
    <td><pre>uint32_t irqn<code></td>
    <td>IRQn</td>
  </tr>
  <tr>
    <td><pre>uint32_t vector<code></td>
    <td>Interrupt handler; if 0 the IRQn slot is de-registered for the current
        box</td>
  </tr>
</table>

---

```C
uint32_t vIRQ_GetVector(uint32_t irqn)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Get the ISR registered for IRQn</td>
  </tr>
  <tr>
    <td>Return value</td>
    <td colspan="2">The ISR registered for IRQn, if present; 0 otherwise</td>
  </tr>
  <tr>
    <td rowspan="1">Parameters</td>
    <td><pre>uint32_t irqn<code></td>
    <td>IRQn</td>
  </tr>
</table>

---

```C
void vIRQ_EnableIRQ(uint32_t irqn)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Enable IRQn for the currently active box</td>
  </tr>
  <tr>
    <td rowspan="1">Parameters</td>
    <td><pre>uint32_t irqn<code></td>
    <td>IRQn</td>
  </tr>
</table>

---

```C
void vIRQ_DisableIRQ(uint32_t irqn)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Disable IRQn for the currently active box</td>
  </tr>
  <tr>
    <td rowspan="1">Parameters</td>
    <td><pre>uint32_t irqn<code></td>
    <td>IRQn</td>
  </tr>
</table>

---

```C
void vIRQ_ClearPendingIRQ(uint32_t irqn)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Clear pending status of IRQn</td>
  </tr>
  <tr>
    <td rowspan="2">Parameters</td>
    <td><pre>uint32_t irqn<code></td>
    <td>IRQn</td>
  </tr>
</table>

---

```C
void vIRQ_SetPendingIRQ(uint32_t irqn)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Set pending status of IRQn</td>
  </tr>
  <tr>
    <td rowspan="2">Parameters</td>
    <td><pre>uint32_t irqn<code></td>
    <td>IRQn</td>
  </tr>
</table>

---

```C
uint32_t vIRQ_GetPendingIRQ(uint32_t irqn)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Get pending status of IRQn</td>
  </tr>
  <tr>
    <td rowspan="1">Parameters</td>
    <td><pre>uint32_t irqn<code></td>
    <td>IRQn</td>
  </tr>
</table>

---

```C
void vIRQ_SetPriority(uint32_t irqn, uint32_t priority)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Set priority level of IRQn</td>
  </tr>
  <tr>
    <td rowspan="2">Parameters</td>
    <td><pre>uint32_t irqn<code></td>
    <td>IRQn</td>
  </tr>
  <tr>
    <td><pre>uint32_t priority<code></td>
    <td>Priority level (minimum: 1)</td>
  </tr>
</table>

---

```C
uint32_t vIRQ_GetPriority(uint32_t irqn)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Get priority level of IRQn</td>
  </tr>
  <tr>
    <td>Return value</td>
    <td colspan="2">The priority level of IRQn, if available; 0 otherwise</td>
  </tr>
  <tr>
    <td rowspan="1">Parameters</td>
    <td><pre>uint32_t irqn<code></td>
    <td>IRQn</td>
  </tr>
</table>

---

```C
int vIRQ_GetLevel(void)
```

<table>
  <tr>
    <td>Description</td>
    <td colspan="2">Get level of currently active IRQn, if any</td>
  </tr>
  <tr>
    <td>Return value</td>
    <td colspan="2">The priority level of the currently active IRQn, if any; -1 otherwise</td>
  </tr>
</table>

## Type definitions

```C
typedef uint32_t UvisroBoxAcl;    /* Permssion mask */
```

---

```C
typedef struct
{
    const volatile void* start;   /* Start address of the protected area */
    uint32_t length;              /* Size of the protected area */
    UvisorBoxAcl acl;             /* Permission mask for the protected area */
} UvisorBoxAclItem;
```
## Error codes

| Error reason          | Error code |
|-----------------------|------------|
| `PERMISSION_DENIED`   | 1          |
| `SANITY_CHECK_FAILED` | 2          |
| `NOT_IMPLEMENTED`     | 3          |
| `NOT_ALLOWED`         | 4          |
| `FAULT_MEMMANAGE`     | 5          |
| `FAULT_BUS`           | 6          |
| `FAULT_USAGE`         | 7          |
| `FAULT_HARD`          | 8          |
| `FAULT_DEBUG`         | 9          |
