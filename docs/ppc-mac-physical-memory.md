# Native Xbox physical-memory provider

`port/mac/engine_physical_memory.c` implements the clean XDK declarations
for `XPhysicalAlloc`, `XPhysicalFree`, `XPhysicalProtect`, and
`XQueryMemoryProtect`. Allocations use Tiger Mach virtual mappings in the
game's `0x80000000..0x88000000` CPU address window. Fixed requests require an
available exact address; any-address requests scan aligned candidates and
use non-clobbering `vm_allocate`, so existing model/resource mappings are
left intact. Each allocation and page permission is tracked under a mutex;
only owned ranges can be protected or freed. The provider refuses startup on
hosts whose Mach page size is not 4096 bytes.

Fixed offsets are required by `source/cache/physical_memory_map.c`: the game
state is requested at `HALO_PORT_GAME_STATE_BASE_ADDRESS` (`0x81a00000`) and
the tag cache at `0x803a6000`. Their call sites pass each address masked with
`0x7fffffff`; the provider translates that physical-window offset back to the
corresponding CPU virtual address. The probe checks both exact placements.

`PAGE_NOCACHE` and `PAGE_WRITECOMBINE` are accepted as allocation/protection
hints but have no Mach equivalent here. `XQueryMemoryProtect` reports the
effective CPU permission without those hints. This is CPU virtual backing,
not a claim that Tiger provides Xbox GPU-visible physical memory or cache
behavior.

Run `tools/mac_physical_memory_checks.sh` after the common native semantics
header has been generated at `build/mac-audit/mac_engine_semantics.h`. The
checker builds PPC and i386 executables with Apple GCC 4.0 and the Tiger SDK,
then runs i386 locally. It covers exact and any-address allocation, alignment,
zero-filled pages, effective protection changes (including partial ranges),
unowned/invalid operations, fixed-address overlap refusal, and teardown.
Evidence from the current build is in `build/mac-audit/physical-memory-build-intel.txt`;
PPC executable MD5 is `3ae531a4040f9881aaa9398ef806691e` and i386 is
`2186a71159d8d58653ba59d8f9578449`. The PPC binary passed 38 checks on both
the PowerBook G4 and iBook G4. Logs: `build/mac-audit/physical-memory-{powerbook,ibook}.txt`.
