# Roadmap

For integrating [ARMmbed uVisor](https://github.com/ARMmbed/uvisor/)^[uVisor is avalable at https://github.com/ARMmbed/uvisor/ under Apache 2.0 license] with mbed RTOS we propose the following roadmap as part of the Morpheus project:

## First 15 days
- Starts at March 21st, terminates at April 1th
- Deliver API definitions and documentation for:
    * Interprocess/Interthread Communication
    * Coarse Block Memory Allocator
    * Allocator API for retargeting malloc for the active thread
- uVisor-HelloWorld ported to mbed 2.0 NXP Freedom MK64 target
    * reproduce existing uVisor-helloworld-demo from mbedOS 3.0 in mbed Classic
    * no RTOS integration - just plain mbedOS
    * run blinky in mbed OS
- prototype uVisor integration in mbed Classic build process
- prototype security-aware interprocess communication using CMSIS

## Next 15 days
- Starts at April 4th, terminates at April 15th
- uVisor RTOS integration prototype
    * extends CMSIS-RTOS APIs
    * two threads, running each in isolated security domain
    * both thread ping-pong data around in 500ms interval
    * both thread individualy blink LED to indicate receiving and forwarding of data  
- create streamlined binary library distribution for mbed 2.0
    * include unsupported target for mbed 2.0 for fallback to disable uVisor
- debug-LED integration
 
## Final 60 days
- Starts at April 18th
- Virtualize disable_irq per uVisor context
- Implement Interprocess/Interthread Communication
- uVisor-aware Malloc-integration (Coarse per-Process Blocks & per-Thread blocks)
- Thread-Local-Storage (TLS) integration
- Implement Register-Level-Access Gateway
- Integrate Key-Value store in mbed 2.0 to enable key provisoning services
- Dogfood uVisor on simple MK64F connected ethernet application (ping network test)
