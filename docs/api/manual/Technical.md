## Technical Details

### Boot Process

* uVisor initialised right after device start-up.
* Runs in privileged mode.
* Sets up a protected environment using a Memory Protection Unit (the ARM Cortex-M MPU or a vendor-specific alternative). In particular:
    * Its own memories and the security-critical peripherals are protected from the unprivileged code.
    * Unprivileged access to selected hardware peripherals and memories is limited through Access Control Lists (ACLs).
* Allows interaction from the unprivileged code by exposing SVCall-based APIs.
* Forwards and de-privileges interrupts to the unprivileged handler that has been registered for them.
* Prevents registers leakage when switching execution between privileged and unprivileged code and between mutually untrusted unprivileged modules.
* Forces access to some security-critical peripherals (like DMA) through SVCall-based APIs.

