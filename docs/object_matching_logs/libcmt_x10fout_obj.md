# `libs/libcmt/x10fout.obj` authentic-source reconstruction

## Decision

Retain the source-authentic `strcpy` implementation as a credible fuzzy
reconstruction and keep the object `NonMatching`. Do not admit the previously
reported 631-byte exact candidate: it replaced the four original `strcpy`
operations with invented six- and seven-byte aggregate types solely to change
VC7 instruction selection.

This follows the campaign rule that authentic, meaningful source takes
precedence over a coincidental exact result. The function is complete and
readable; only its compiler intrinsic/library-copy lowering remains unresolved.

## First-party provenance

- XDK 3911 CodeView identifies the original source as
  `d:\xbox-finalfre\private\sdktools\crt\fpw32\conv\x10fout.c`.
- The matching Microsoft CRT source is public in
  `xerohour/xbox_leak_may_2020` commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`, blob
  `7e9106227dc9543ac5bca9a216e0fdf1b0b486cc`.
- That source explicitly defines `STRCPY` as `strcpy` and uses it for the
  signaling-NaN, indefinite, infinity, and quiet-NaN strings. It contains no
  fixed-size aggregate-copy types.
- The reconstructed `cv.h` owns the shared floating-conversion types, macros,
  and helper prototypes, matching the original source organization instead of
  placing external function declarations in the use-site `.c` file.

The conversion behavior is otherwise complete: it handles zero and special
values, estimates and applies the decimal exponent, extracts decimal digits,
rounds, and writes the CRT `FOS` output state.

## Rejected exact candidate

The earlier `x10fout-final-20260813` lane observed:

- direct first-party `strcpy`: 638 bytes and 14 relocations;
- custom fixed-size aggregate copies: 631 bytes and 12 relocations, strict
  exact.

The latter is now rejected because direct first-party source proves the custom
types and assignments were not present. They were a code-generation carrier,
not recovered program structure. Changing the object to `/Oi`, adding a
pragma, forcing inline expansion, or retaining the aggregate substitute would
likewise violate the current compiler-policy or anti-fake rules.

## House-rule audit

The retained implementation keeps the authentic CRT names and integer types,
uses one parameter per line, ends every value path with an explicit return,
and places shared prototypes in `libs/libcmt/cv.h`. It uses no assembly,
volatile/register forcing, optimizer pragma, forced inline/noinline, raw-byte
emission, undefined behavior, fake dependency, or nonsensical control flow.

## Reopen criterion

Reopen exact matching only with authenticated evidence for the January CRT
intrinsic configuration or an original header/compiler option that naturally
lowers the four `strcpy` calls to the target's fixed copy instructions. Until
then, keep the authentic source and park the compiler-lowering residual.
