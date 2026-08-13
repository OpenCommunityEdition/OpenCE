# `libcmt/wcsnicmp.obj` exact-match ledger

## Target inventory

- January object: `libs/libcmt/wcsnicmp.obj`
- Function: `__wcsnicmp` (with a csplit alias at `_wcsnicmp`)
- Padded code size: `0x9E` (158 bytes)
- Relocations: three
  - `IMAGE_REL_I386_DIR32` to `___lc_handle + 8`
  - two `IMAGE_REL_I386_REL32` calls to `_towlower`
- Owned data: none

The target returns zero for a zero count.  Otherwise it selects a C-locale
ASCII fold or a locale-aware `towlower` path, then stops when the count is
exhausted, the folded left character is NUL, or the folded characters differ.
The result is the unsigned 16-bit left-minus-right difference.

## Source shape under test

The first candidate follows the canonical CRT structure: one result local,
two parallel `do/while` loops, named locale constants, and one final return.
It produced 156 bytes against 158 with all three relocation destinations
correct.  Post-increment operands shortened the pointer live ranges, while
`if`-based ASCII folding emitted an in-place add rather than January's
conditional-expression selection.

The second candidate uses canonical ternary ASCII folds and separate pointer
increments after both character assignments.  This made the ASCII path
byte-identical and reached the exact 158-byte size.  In the locale path,
however, January advances the left pointer immediately after the first
`towlower` call and interleaves the right increment with argument cleanup;
the separate-increment form delayed both until after the second call.

The third candidate keeps the exact ternary/separate-increment ASCII path but
uses post-increment operands for the two locale-aware calls.  It retains
libcmt's unchanged `/O1 /Gy` flags and passes exactly.

No assembly, volatile forcing, undefined behavior, byte patching, compiler
substitution, or comparator exception is used.

## Verification

The third canonical source shape passes the hardened comparator:

- size: `158/158`
- relocations: `3/3`
- normalized SHA-256, both sides:
  `a2ea67ff7a8deb8c084750e33779054e2c1600f32aa9aee0aee0e8914e36c926`
- relocation identities:
  - `+0x10`, `IMAGE_REL_I386_DIR32`, `___lc_handle + 8`
  - `+0x6B`, `IMAGE_REL_I386_REL32`, `_towlower + 0`
  - `+0x7A`, `IMAGE_REL_I386_REL32`, `_towlower + 0`
- owned data in the target: none
- `section_infos_equal`: `true`

The candidate's directive and debug sections are ordinary compiler-generated
metadata.  The sole target COMDAT is strict exact, so the object is safe to
mark `Matching`.
