## Secure Coding Style in uVisor

This document collects uVisor-sepcific coding guidelines for security-aware programming. A good introduction into safe and secure programming is the [MISRA-C standard](http://www.misra.org.uk/?TabId=57#label-c3) and the [The Art of Software Security Assessment: Identifying and Preventing Software Vulnerabilities](http://www.amazon.com/dp/B004XVIWU2).

Here's a list of highlights and uVisor-specific rules:
- security critical checks are done with HALT_ERROR to ensure that uVisor stops on critical errors
- understand that "asserts" are ignored in release builds
  - use asserts only for checking architecture axioms and not dynamic values
  - use HALT_ERROR for everything else: debug texts are removed for release builds, but code still stops and presents meaningful blink error
- avoid using pointer dereferencing to access unprivileged memories like stacks
  - easy to create code that reads data from the user without full sanity checks: hard sto spot
  - by using [unprivileged reads](https://github.com/ARMmbed/uvisor/blob/master/core/system/inc/mpu/vmpu_unpriv_access.h) instead, the CPU pretends access to be unprivileged - even from pribileged code
  - later versions of uVisor can hook into these function to instrument and log unprivilegd access for detecting secuity flaws during fuzzing attacks
- use more brackets or prepare to be [doomed to fail](http://www.dwheeler.com/essays/apple-goto-fail.html)
