# `wcsrchr.obj` exact-match ledger

## Plain-English behavior

This CRT routine returns a pointer to the last occurrence of a requested
UTF-16 character. It searches through the string terminator, so searching for
zero returns the terminator; an absent nonzero character returns null.

## Scope and provenance

- Source translation unit: `libs/libcmt/wcsrchr.c`
- January target object: `libs/libcmt/wcsrchr.obj`
- Function: `_wcsrchr`
- Compiler: XDK 3911 CL 13.00.9254.1
- Object flags: `/O1 /Gy /I"xbox/include"`
- Target inventory: one 48-byte COMDAT function, zero relocations, no owned
  data sections

The prototype follows the XDK CRT declaration: a cdecl function taking a
UTF-16 string and character and returning a non-const pointer.

## Accepted source shape

The first natural CRT reconstruction was exact:

1. Save the start pointer.
2. Walk forward through the zero terminator with `while (*string++);`.
3. Walk backward until reaching the start or finding the requested character.
4. Return the current pointer when it matches, otherwise null.

The single conditional return is intentional and follows the preferred
single-return house rule. Under VC7 it produces January's branchless
`subw/negw/sbb/not/and` pointer mask. The cast only removes the input
qualification required by the standard CRT API; it is not a code-generation
hack.

## Strict verification

| Function | Size T/B | Relocs T/B | Normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `_wcsrchr` | `48/48` | `0/0` | `5b09059c9761d9c8082daa1b91da9b7e9338aba8bf03752cc6422b9ef1e43d64` | exact |

The hardened comparator reports `section_infos_equal = true`. There are no
owned `.rdata`, `.data`, or `.bss` sections to reconcile. Compiler-generated
debug and directive sections are not target-owned CRT data.

Final gates:

- consolidated `ninja halobetacache_build`: exit 0 after rebuilding all 467
  configured Halo translation units;
- `ninja progress`: exit 0, semantic audit accepted `wcsrchr.obj` and reported
  281/833 objects overall, 270/468 Halo objects, and 11/212 libcmt objects;
- `config/config.json`: valid JSON;
- `git diff --check`: clean.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Decision |
| --- | --- | ---: | ---: | --- | --- | --- |
| E01 | Forward scan through terminator, backward short-circuit scan, single conditional pointer/null return | `48/48` | `0/0` | `5b09059c9761d9c8082daa1b91da9b7e9338aba8bf03752cc6422b9ef1e43d64` | none | retained; strict exact |

No rejected production experiments were needed.

## Safety and disposition

The reconstruction uses readable C only. It contains no inline assembly,
`volatile`, forced inlining, undefined behavior, byte patching, altered
compiler flags, or comparator exception. The object is fully strict exact and
is safe to mark `Matching`.

## Current-campaign revalidation

The earlier result was treated as untrusted and transplanted onto current
campaign tip `5ed68e5c`. A fresh XDK 3911 build under the canonical libcmt
`/O1 /Gy /I"xbox/include"` configuration reproduced `_wcsrchr` at 48/48 bytes,
0/0 relocations, and normalized SHA-256
`5b09059c9761d9c8082daa1b91da9b7e9338aba8bf03752cc6422b9ef1e43d64`.
The target owns exactly that external function COMDAT and no runtime data.

The complete Halo and libcmt builds, semantic/progress reports, per-object
regression snapshot/check, and full tool suite were rerun at this campaign
state. House/Berth review also confirmed the one final return and vertically
formatted parameters; the source adds no alignment directive, raw offset,
semantic exception, or code-generation-only cast beyond the API-required
removal of input qualification already explained above. No original target
bug was identified.
