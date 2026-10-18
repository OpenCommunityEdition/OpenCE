# The macOS host

The game runs on Android as ILP32 AArch64 code (`arm64_32`) in a 64-bit
process: 64-bit instructions with 32-bit pointers (`port/android/README.md`,
"ILP32 code"). The guest image, `halo_guest.elf`, does not depend on
Android, so a macOS host can load the same image. It needs three things from
the system:

1. The guest's fixed addresses: the Xbox memory at `0x80000000` to
   `0x88000000` and the image above it, to `0x8c000000`. The cache files
   hold pointers to these addresses, so they cannot move.
2. An executable mapping of the guest image.
3. A thread pointer for each guest thread. On Android the guest does not
   keep it in `TPIDR_EL0` (the host's bionic uses the register): the guest
   asks the host for it (`host_get_tp`, `pthread_arch.h`). The spike checks
   whether macOS would let a guest use the register, because asking the
   host costs an exit in a VM.

The programs in `port/macos/spikes` check each one on arm64 macOS, and
`tools/test_macos_spikes.py` builds and runs them.

## Results

Checked on macOS 26.6.2 (SDK 26.5), Apple Silicon, Apple clang 21.0.0.

### Memory below 4 GB: refused

`low_memory.c` tries to map `0x80000000` to `0x8c000000`.

Linked with a small page-zero (`-Wl,-pagezero_size,0x4000`), the program
is killed as it starts (status 137, SIGKILL), and nothing is logged.
`otool -l` shows the `__PAGEZERO` segment as asked (`vmsize 0x4000`), so
the kernel refuses the binary itself. An arm64 executable must have a 4 GB
page-zero.

Linked as usual, the program starts, but nothing below 4 GB can be mapped:

```
region at 0x80000000: free; next region 0x100958000-0x10095c000
mmap with a hint: placed at 0x105624000 instead
mach_vm_allocate fixed: (os/kern) invalid address
mach_vm_deallocate of the range: (os/kern) successful
mach_vm_allocate fixed: (os/kern) invalid address
FAIL: no way to map 0x80000000-0x8c000000
```

The lowest address the process may use is 4 GB. Freeing the range changes
nothing, because it is outside the process's address space altogether. Thus
the guest cannot live in the host's own address space, as it does on
Android.

### Thread pointer: overwritten

`thread_pointer.c` sets `TPIDR_EL0` on two threads and checks it after each
yield or sleep:

```
TPIDR_EL0 at start: 0x2006
FAIL: thread 0 lost its TPIDR_EL0 at round 1
```

macOS uses `TPIDR_EL0` itself (it holds a value before the program sets
it), and the kernel writes it again at each context switch. A guest in the
host's process keeps asking the host for its thread pointer, as on Android.
Inside a VM the register is the guest's own, so there the guest can read it
directly and save an exit on each access to thread-local data.

### Executable image

`hv_vm_map` maps the guest's memory executable inside a VM (below), so no
separate check of executable mappings in the host process was made. That
check matters only to the in-process approach (option B).

### Hypervisor.framework: works

`hypervisor.c` creates a VM, maps a page of host memory at guest physical
address `0x80000000`, and runs two instructions there at EL1 with the MMU
off: an add, and an `hvc` that exits to the host. It is signed ad hoc with
the `com.apple.security.hypervisor` entitlement
(`hypervisor.entitlements`):

```
hv_vm_create: success
hv_vm_map at 0x80000000: success
guest code at 0x80000000 ran 100000 times; x0 = 100000
round trip guest -> host -> guest: 1004 ns
PASS
```

A VM has an address space and registers of its own, so the fixed addresses
and `TPIDR_EL0` are the guest's. Each exit to the host costs about 1 µs.

## The ways forward

### A. The guest in a VM (Hypervisor.framework)

The guest image is unchanged. The host:

- creates a VM, maps the guest's memory at its fixed addresses, and one
  vCPU for each guest thread (each on a host thread, as Hypervisor.framework
  requires);
- gives the guest a small EL1 runtime: exception vectors, and page tables
  that map the guest's range as normal, cacheable memory (with the MMU off,
  all memory is device memory: slow, and unaligned accesses fault);
- turns the guest's host imports into exits. Every call the guest makes
  to the host, system calls included (`__guest_syscall` calls
  `host_syscall`), goes through one generated stub
  (`tools/android_imports.py`) that branches through
  `__host_import_table`. In the VM build the stub is `mov w16, #index;
  svc #0; ret`, and the EL1 vector turns the `svc` into an `hvc`. The host
  reads the arguments from the vCPU's registers, turns guest addresses into
  host addresses, and carries out the call as the Android host does
  (`host_syscall.c`, `host_sdl.c`). The game's sources do not change;
- keeps the thread pointer in `TPIDR_EL0`, which is the guest's own in a
  VM, instead of the `host_get_tp` import, which would be an exit on each
  access to thread-local data;
- sends rendering in batches: a command buffer in shared memory, handed to
  the host once per buffer or per frame, not one exit per call. At about
  1 µs per exit, the thousands of draw calls of a frame cannot each be an
  exit at 120 fps (8.3 ms per frame).

The app needs the `com.apple.security.hypervisor` entitlement. Ad hoc
signing is enough for development. A distributed app is signed with it.

### B. The guest in the host's process, with rewritten code

The guest's code would be changed so that it does not need low addresses.
Every memory access would add a base register (`[x28, w1, uxtw]`). The place for
this is the step that already converts the guest's assembly
(`tools/android_asm_convert.py`). There would be no VM exits, but every
load, store, atomic and address computation in the game must be found and
rewritten, and one that is missed corrupts memory. The image would also need
an executable mapping in the host process (`MAP_JIT` or the hardened
runtime's exceptions).

### Recommendation

Option A: the guest's code stays as it is, the host carries over most of
the Android host, and the cost (an exit per host call) is limited by
batching the renderer, which the Metal renderer needs in any case.
