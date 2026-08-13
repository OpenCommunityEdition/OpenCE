# `libs/libcmt/strerror.obj` exact-match ledger

This atomic lane starts from campaign commit `8446572d`. The object was
`MISSING` at config index 764, absent from `config/parked.json`, every Git ref,
worktree, and existing object log. January owns one function and one 40-byte
BSS section.

## Plain-English behavior

`strerror` obtains the current thread's CRT state and lazily allocates a
134-byte message buffer. If allocation fails, it uses a shared 40-byte backup
buffer. It clamps invalid negative or out-of-range error numbers to the CRT's
fallback entry, copies that message into the selected buffer, and returns it.

## Provenance and ownership

The XDK 3911 `libcmt.lib` archive has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\strerror.obj` member has SHA-256
`de82ead29d2e9e81e594492a79f54222ae6212db642447ad5d593d6ac48070d6`,
compiler identifier `0x001c2426`, ordinary C CodeView records, and source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\misc\strerror.c`.
The public historical Microsoft Xbox CRT source at the same path supplies the
original compound allocator condition and `_sys_err_msg` expression topology.
January bytes and strict ownership remain authoritative.

Strict target inventory:

- `_strerror`: 81 bytes, five relocations, normalized SHA-256
  `13ea92a78de571574228df99f0bcbabf5329174c0d63c86d587515698f25f25b`;
- relocations at `+2` to `__getptd`, `+20` to `_malloc`, `+31` to
  `_bss_004e3eb0`, `+48` to `__sys_nerr`, and `+63` to `__sys_errlist`, all
  with exact types and zero addends;
- `_bss_004e3eb0`: external symbol at section offset zero, 40 zero bytes,
  zero relocations, SHA-256
  `2c34ce1df23b838c5abf2a7f6437cca3d3067ed509ff25f11df6b11b582b51eb`;
- no other runtime data or function aliases.

The original compiler called the backup buffer a function-local static.
January's csplit ownership instead exposes the same bytes as external
`_bss_004e3eb0`. The production declaration follows the authoritative split
object, and the hardened comparator verifies that both the function relocation
and the data symbol storage/value agree—this is not an alias waiver.

## Experiment matrix

| ID | One source factor | Size T/B | Relocs T/B | Result | Decision |
|---|---|---:|---:|---|---|
| E01 | Nested allocator tests plus explicit range assignment | `81/79` | `5/5` | correct operations, but cached message pointer and two-byte-short bounds CFG | rejected |
| E02 | Authentic Microsoft compound `field == NULL && (field = malloc(...)) == NULL` | `81/81` | `5/5` | allocator half exact; bounds compare direction still differs | retained as proven control |
| E03 | Authentic `_sys_err_msg` conditional expression instead of mutating `error_number` | `81/81` | `5/5` | all code bytes/hash exact; only local-static name disagrees with csplit owner | retained |
| E04 | Name the proven external 40-byte csplit BSS owner and reference it directly | `81/81` | `5/5` | function, BSS bytes, relocations, linkage, and ownership all strict exact | accepted |

## Strict verification

XDK 3911 CL `13.00.9254.1` compiled with unchanged libcmt
`/O1 /Gy /I"xbox/include"` flags. The hardened comparator reports
`section_infos_equal` for `_strerror`; a separate section comparison reports
the 40-byte BSS equal. Both objects expose `_bss_004e3eb0` as external storage
class 2 at offset zero. No relocation identity, addend, or section ownership is
being masked.

The full `halobetacache_build` and `libcmt_build` pass. Regenerated progress
reports 330/833 objects overall, 267/468 Halo objects, and 63/212 libcmt
objects. Libcmt has 85/476 exact functions, 4,298/55,015 exact code bytes, and
364/8,637 exact data bytes. This object contributes one object, one function,
81 code bytes, and 40 BSS bytes. The semantic audit reports zero unit errors.

The clean-commit fail-closed per-unit snapshot/check was followed by deletion
of the generated object and a forced rebuild. `_strerror` remains
`still_exact`; the gate also freezes the BSS bytes and symbol ownership. There
are no failures, warnings, changed non-exact siblings, newly exact surprises,
or ownership changes. The complete tooling suite passes 179/179 tests.

## House/Berth and original-bug audit

The source uses named types, constants, parameters, a typed thread-data prefix,
and readable control flow recovered from primary provenance. It uses no
assembly, `volatile`, forced inlining, undefined behavior, type-punning, opaque
cast, packing/alignment trick, flag change, byte patch, or comparator waiver.
The compound condition is retained because it is the authentic concise source
and measurably controls the exact store/test schedule.

The original design has an unavoidable thread-safety fallback: if allocation
fails, all threads share the 40-byte backup. That behavior is preserved for
exact matching. A defensive implementation would return a constant immutable
fallback or synchronize access, but would intentionally change ownership and
runtime behavior.
