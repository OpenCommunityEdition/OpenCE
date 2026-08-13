# `libs/libcmt/strtok.obj` exact-match ledger

## Target inventory

- Runtime section: one 165-byte `/Gy` `.text` COMDAT
- Function: `_strtok`
- Runtime relocations: one
  - `+0x0d`: `IMAGE_REL_I386_REL32` to `__getptd`
- Owned data: none

## Provenance

The January target is byte-identical to XDK 3911's shipped `libcmt.lib`
member `obj\i386\strtok.obj`: size, normalized bytes, relocation identity,
and object ownership all agree. Its COFF debug records identify the historical
source as
`d:\xbox-finalfre\private\sdktools\crt\crtw32\string\strtok.c` and the
compiler identifier as VC7 build 9254. The shipped member timestamp is
2001-08-11 18:19:37. Its debug records also corroborate the source parameter
names `string` and `control`, local names `map` and `ptd`, and `_token` at
thread-data offset `+0x18`. The target/member defines only `_strtok`; it has no
OLDNAMES alias or second runtime body.

## Recovered source and data layout

The readable implementation follows the historical Microsoft CRT source:

1. Obtain the calling thread's CRT data through `_getptd()`.
2. Clear a 32-byte stack bitmap, then set one bit for every delimiter byte,
   including the terminating null byte.
3. Start at the supplied string or the saved per-thread token pointer at
   `thread_data + 0x18`.
4. Skip leading delimiters, remember the token start, scan to the next
   delimiter, replace it with null, and save the continuation pointer.
5. Return the token start, or null when the start and continuation pointers
   are equal.

The local `thread_data` declaration is the shared CRT prefix already proven by
the exact `wcstok.obj`; its narrow token pointer is at `+0x18` and its wide
token pointer at `+0x1c`. The function has one explicit final return and uses
ordinary source types and casts for signedness/pointer correctness.

No assembly, volatile forcing, `__forceinline`, undefined behavior, byte
patching, compiler-flag change, or comparator exception is used.

## Strict verification

- size: `165/165`
- relocations: `1/1`
- normalized SHA-256, both sides:
  `6fe1548171f75d4b34a9281b1f9798c4b8acf2e452511e6ad43a63345d6a45ed`
- relocation address/type/destination/addend: identical
- `section_infos_equal = true`
- candidate runtime ownership: one 165-byte `.text` COMDAT
- owned data: none on either side

## Experiment record

| ID | Source shape | Result |
|---|---|---|
| E01 | Historical Microsoft CRT bitmap implementation with `_getptd()->token` state | Strict exact on first build |

No alternative codegen shapes were necessary. The object is safe to mark
`Matching`.

## Current-campaign independent revalidation

The completed branch was treated only as a source lead. Its three relevant
files were transplanted onto campaign tip `81d24350`, the object was
configured anew, and XDK 3911 rebuilt it before admission. The current result
reproduces all 165 bytes, the single `__getptd + 0` relocation at `+0x0D`, and
the hash above. Both target and candidate own one external `_strtok` function
COMDAT at offset zero with COFF type `0x20` and storage class `2`; neither owns
runtime `.rdata`, `.data`, or `.bss`.

## House/Berth and bug audit

- The per-thread prefix is typed and limited to fields proven by this exact
  object and the independently exact `wcstok.obj`; no opaque magic offset is
  used.
- The 32-byte delimiter bitmap and its bit arithmetic express the historical
  algorithm directly rather than forcing instructions.
- Parameter and local names are corroborated by the XDK object's CodeView
  records, not guessed from behavior alone.
- The function has one explicit final return and uses ordinary multiline
  signatures and readable control flow.
- No assembly, `volatile`, undefined behavior, forced inlining, byte forcing,
  alignment directive, compiler flag change, or comparator exception exists.
- No original shipped bug was identified in this routine; therefore no bug
  workaround or corrective code is hidden in the implementation.

Final admission additionally requires successful full Halo/libcmt builds,
zero semantic unit errors, a clean forced-rebuild regression check, and all
179 project tool tests.
