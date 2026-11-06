/*
None for a guest compiled to Mach-O (arm64_32): its constructors are its
converted __mod_init_func sections (.guest_init), and the only .init_array
would be that of the 64-bit runtime library it links (compiler-rt's AArch64
builtins), 8-byte entries, which port/android/guest/guest.ld discards.
*/
