# `libcmt/new.obj` exact-match ledger

## Target inventory

- January object: `libs/libcmt/new.obj`
- Function: `??2@YAPAXI@Z` (`void *__cdecl operator new(unsigned int)`)
- Padded code size: `0xE` (14 bytes)
- Relocations: one `IMAGE_REL_I386_REL32` at `+0x7`, targeting `__nh_malloc`
- Owned data: none

The target pushes the allocation mode `1`, forwards the size argument to
`_nh_malloc`, discards the two stack arguments, and returns its result.

## Source and build shape under test

The implementation is the ordinary readable C++ allocation wrapper.  The
repository path remains `libs/libcmt/new.c`; object-specific `/TP` compiles it
as C++, while the libcmt project retains its original `/O1 /Gy` flags.
`_nh_malloc` has explicit C linkage so its COFF destination remains the CRT
symbol `__nh_malloc` rather than a C++-mangled name.

No assembly, volatile forcing, undefined behavior, byte patching, compiler
substitution, or comparator exception is used.

## Verification

The first source shape passed the hardened comparator without adjustment:

- size: `14/14`
- relocations: `1/1`
- normalized SHA-256, both sides:
  `f936ff6b84ea25652eff3ee78740def510e3767ede1a6483f6e65df32b6412e1`
- relocation identity: `+0x7`, `IMAGE_REL_I386_REL32`, `__nh_malloc + 0`
- owned data in the target: none
- `section_infos_equal`: `true`

The candidate's `.drectve` and debug sections are ordinary compiler-generated
metadata and do not represent target-owned CRT data.  The sole target COMDAT
is strict exact, so the object is safe to mark `Matching`.

## Current-campaign independent revalidation

The older completed result was treated only as a source lead. It was
transplanted onto campaign tip `95908c0f`, configured from scratch, and rebuilt
with the local XDK 3911 compiler before any admission decision. The fresh
object reproduced every measurement above. A whole-object inventory also
proved that the only meaningful target section is the 14-byte `.text` COMDAT;
both objects expose `??2@YAPAXI@Z` at offset zero with function type `0x20` and
external storage class `2`. Neither target nor candidate owns `.rdata`,
`.data`, or `.bss`.

The full Halo and libcmt builds passed, semantic progress reported zero unit
errors, the clean per-unit regression snapshot/check retained the function as
`still_exact`, and all 179 project tool tests passed.

## House/Berth audit

- The body is readable C++ and expresses the real allocator contract directly.
- `/TP` is not a codegen trick: it is required to compile this `.c`-named file
  as C++ and produce the January operator-new symbol. The optimization flags
  remain libcmt's unchanged `/O1 /Gy`.
- The `_nh_malloc` declaration uses explicit C linkage because that is the
  target's proven external ABI.
- The direct return is the natural tail-wrapper shape measured in January; an
  artificial result local would obscure the source and change codegen.
- There is no assembly, `volatile`, undefined behavior, forced inlining,
  byte patching, magic layout, alignment directive, flag substitution, or
  comparator exception.
