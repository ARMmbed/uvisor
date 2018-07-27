# uVisor Onboarding Guide

Welcome to the uVisor team, where the fun never ends! You've discovered our
wonderful onboarding guide, designed to get you on board and making meaningful
contributions as quickly as is reasonable.

The uVisor Onboarding Guide is broken into multiple segments, of which there is
only one segment currently.

 - [uVisor Design Rules](#uvisor-design-rules)

## uVisor Design Rules

These rules were learned the hard way: through detailed technical oversight
working on uVisor. Often, a design would arise from any of our team members
that'd solve a problem in a nice way, except that it'd break some unwritten
rule the rest of the team kept stored in their heads. The design would be
rejected, and the team member dejected. Sometimes, the team member would have
to do as many as five design iterations until design approval. If only the team
member had known the unwritten rules when they set out on their initial design!
After all these design iterations, the resulting design truly was better, but
much time could have been saved had the design rules been written down.

The written uVisor Design Rules are designed to help you, the potential
contributor, come up with workable designs quicker, by writing down those
previously unwritten design rules. This should also help save the rest of the
team's time as well, as less "architecture review" technical oversight meetings
should be required. This also makes design work possible from contributors
outside the core uVisor team employed by ARM.

Any new feature, design, or redesign will have to follow these rules. The
business priorities of the day will also impose additional constraints on
designs, but we make no attempt to document those volatile constraints here.
The rules we focus on here will remain relatively static over the course of
uVisor development.

These are clumped together in rough categories, in no particular order.

### Portability

Keep uVisor independent from other software components as much as possible.
This maximizes uVisor's resiliency and usefulness. During the transition
between mbed OS 3 and mbed OS 5, uVisor core didn't have to change much at all;
most changes came from integrating uVisor with an RTOS on ARMv7-M.

 - uVisor core should not contain OS-specific code.
 - uVisor APIs should not match one-to-one with an OS API
   - It will end up being a lot of work to port the uVisor API to a different
     OS where the API doesn't match as well. We don't want to write code to
     abstract differences between operating systems.
 - uVisor core should only contain CPU-core- or MPU-specific code.
 - uVisor lib can contain OS-specific code.
   - Two different operating systems may use two different uVisor lib binaries.


### Usability

Make uVisor APIs easy to use.

 - Where possible, perform operations on behalf of the user that the user might
   forget to do. Perform higher-level operations all together as a whole, even
   if that makes the API less orthogonal. For instance, don't make a user obtain
   a handle in one function call and then open a handle in another function
   call, obtain and open the handle for the user all in one go.
 - Make APIs explicit and flat. Don't require the user to have knowledge of
   layers of APIs, or require them to do things in certain orders. For example,
   make the user provide a list of functions as RPC targets when they want to
   wait for incoming RPC, as opposed to making the user provide the list up
   front in exchange for a cookie they use later on when waiting. This makes
   the information relevant to the API call available as close to the point of
   use as possible.


### Security

 - Don't trust user provided values. Check and sanitize everything.
 - Use the uVisor threat model to decide what inputs you can trust. For
   example, values provided from flash can be trusted (perhaps until we have
   modular firmware update). Values provided from box-private SRAM can be
   trusted as having come from the box that owns that SRAM, but still must be
   sanitized and validated should those values be used by another box or uVisor
   itself (even indirectly).


### Performance

 - Minimize the number of transitions between user mode and uVisor. On ARMv7-M,
   we use SVCs to transition to uVisor. SVCs come with a relatively high cost.
   On ARMv8-M, we use SG to transition to uVisor (or uVisor Secure Device
   Services [SDS]). SG also has to stack a lot of state.


### Updating the Design Rules

If you find your solutions getting rejected due to some "unwritten rule" you
wished you would have known about before designing and implementing your
solution, please document that rule here. Be sure the rational of the new rule
is well understood and communicated by your addition.

Also, these rules aren't set in stone; these aren't permanent, never to be
changed rules. There has been an attempt to clearly document the rationale
behind each rule so that we have the institutional memory sufficient to change
the rules where and when necessary.
