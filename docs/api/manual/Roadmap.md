# Roadmap

For integrating [ARMmbed uVisor](https://github.com/ARMmbed/uvisor/)^[uVisor is avalable at https://github.com/ARMmbed/uvisor/ under Apache 2.0 license] with mbed RTOS we propose the following roadmap as part of the Morpheus project:

## First 15 days
- Starts at March 17st, terminates at March 30th
- Deliver API definitions and documentation for:
    * Interprocess/Interthread Communication
    * Coarse Block Memory Allocator
    * Allocator API for retargeting malloc for the active thread
- uVisor-HelloWorld ported to mbed Morpheus NXP Freedom MK64 target
    * reproduce existing uVisor-helloworld-demo from mbedOS 3.0 in mbed Classic
    * no RTOS integration - just plain mbedOS
    * run blinky in mbed OS
- prototype uVisor integration in mbed Classic build process
- prototype security-aware interprocess communication APIs in CMSIS-RTOS

## Next 15 days
- Starts at March 31th, terminates at April 13th
- uVisor RTOS integration prototype
    * extends CMSIS-RTOS APIs
    * single-threaded/uVisor-protected blinky (one Timer, one GPIO-out)
- create streamlined binary library distribution for mbed Morpheus
    * include unsupported target for mbed Morpheus for fallback to disable uVisor
- debug-LED integration with mbed Classic

## Next 30 days
- Starts at April 14th, terminates at May 5th
- uVisor-aware Malloc-integration (Coarse per-Process Blocks & per-Thread blocks)
- Implement Interprocess/Interthread Communication
- Virtualize disable_irq per uVisor context
- uVisor RTOS integration prototype
    * extends CMSIS-RTOS APIs
    * two threads, running each in isolated security domain
        - ping-pong data around in 500ms interval
        - threads individually blink LED to indicate receiving and forwarding of data
    * Integrate Key-Value store in mbed Morpheus to enable key provisoning services
- Dogfood uVisor on simple MK64F connected ethernet application (ping network test)

## Final 30 days
- Starts at May 6th, terminates at June 7th
- Thread-Local-Storage (TLS) integration
- Implement Register-Level-Access Gateway
- mbedTLS integration
- Integration with Nanostack / mbed Client
- Debug improvements
- Create private release for selected customers
