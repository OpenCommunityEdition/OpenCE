/*
The constructors of a guest that clang compiles to ELF itself (x32), in
their priority order, after .guest_init (port/android/guest/guest.ld): ELF32
objects, whose .init_array entries are 4-byte pointers.
*/
KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) .init_array))
