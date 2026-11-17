/* The Custom Edition tag cache's window, 0x40440000 to 0x41B40000
(port/linux/game/cache_file_formats.h), as part of the armv7 executable.

The program asks for the window at start-up (xbox_memory.c), but by then the
dynamic loader has mapped the shared libraries, and 32-bit ARM Linux places
those from 0x40000000 upwards in the legacy mmap layout (a process with an
unlimited stack) and in qemu-user: on top of the window, so that it cannot be
reserved ("File exists") and Custom Edition maps cannot run. A segment of the
executable is mapped by the kernel before the loader runs, so the libraries
go elsewhere. Its pages are zero and are backed as they are touched.

The marker before it is a section with contents: without it the linker joins
the window to .bss, as one segment of a gigabyte. */
SECTIONS
{
  .ce_tag_cache_marker 0x40430000 : { LONG(0) }
  .ce_tag_cache 0x40440000 (NOLOAD) :
  {
    __ce_tag_cache_start = .;
    . += 0x1700000;
    __ce_tag_cache_end = .;
  }
}
INSERT AFTER .bss;
