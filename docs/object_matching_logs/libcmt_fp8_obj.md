# `libs/libcmt/fp8.obj` exact-match ledger

This atomic lane starts from campaign commit `a34f4dde`. At lane start the
object was `MISSING`, absent from `config/parked.json`, absent from every Git
ref and worktree, and explicitly cleared by the other active agents. The
January split target exists at index 758.

## Plain-English behavior

The single routine restores the x87 floating-point unit's ordinary 53-bit
precision mode. It asks `_controlfp` to replace only the precision-control
bits, leaving exception masks, rounding mode, and every unrelated control bit
alone. The function does not return a value and owns no runtime data.

## Provenance and target inventory

The XDK 3911 `libcmt.lib` used for provenance has SHA-256
`60aa4c87c97eeb6662b31d9d37cf3caba1afabaec72c47b090e7a0ce8b632f55`.
Its `obj\i386\fp8.obj` member has SHA-256
`b7edcb09ad051d4b628a4ee9ff82417715a1183b2830c1e7be0f2885d2f2e8e2`,
records source path
`d:\xbox-finalfre\private\sdktools\crt\crtw32\startup\i386\fp8.c`, compiler
identifier `0x001c2426`, C line records for source lines 15 through 17, and
the external C function `_setdefaultprecision`. This is direct provenance for
ordinary CRT C, not a vendor-assembly unit.

The January target and the XDK member's runtime function already pass
`section_infos_equal` against each other:

- padded size `0x12/0x12`;
- one `IMAGE_REL_I386_REL32` relocation at `+0x0b` to `_controlfp`, addend 0;
- normalized SHA-256
  `96e189a2020338ed95a992bce139d8b47a57ad9595a36b473c14a11cad4f7745`;
- external COMDAT owner `_setdefaultprecision`, pick-no-duplicates, one-byte
  alignment;
- no January-owned `.rdata`, `.data`, or `.bss`.

The code bytes push `0x00030000`, push `0x00010000`, call `_controlfp`, pop
the two cdecl arguments, and return. XDK `Float.h` identifies the constants as
`_MCW_PC` (precision-control mask) and `_PC_53` (53-bit precision), proving
the readable call `_controlfp(_PC_53, _MCW_PC)` rather than unexplained magic
numbers.

## Experiment matrix

| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | Direct void wrapper calling `_controlfp(_PC_53, _MCW_PC)` | `0x12/0x12` | `1/1` | `96e189a20203...` | none | `1/1` exact | accepted: strict exact on first build |

No alternative spelling is warranted unless E01 fails: the archive member,
target bytes, target relocation, and XDK constant declarations already agree
on the exact operation. Any failed shape will be reverted and measured here.

## Strict verification

The production build uses XDK 3911 CL `13.00.9254.1` with the configured
libcmt flags `/O1 /Gy /I"xbox/include"`. The first build passes strict whole-
object comparison: `0x12/0x12` bytes, `1/1` relocation at `+0x0b` to
`_controlfp` with addend zero, matching COMDAT ownership/selection/alignment,
and normalized SHA-256
`96e189a2020338ed95a992bce139d8b47a57ad9595a36b473c14a11cad4f7745`.
There are no aliases and no target-owned runtime data sections. Candidate
`.drectve`, `.debug$S`, and `.debug$F` are ordinary compiler metadata rather
than runtime ownership. A clean forced rebuild produces the same strict hash,
size, relocation, and ownership result.

Both `halobetacache_build` and `libcmt_build` pass. `progress` reports 308/833
objects overall, 267/468 Halo objects, and 41/212 libcmt objects; the current
tree has 35/476 exact libcmt functions and 2,252/55,015 exact libcmt code
bytes. Against the lane baseline this object contributes one function and 18
bytes; concurrent completed-object admissions account for the other movement
visible in the aggregate report. The semantic audit scans 412 units, accepts
3,388 exact functions, and reports zero unit errors. All 179 project tooling
tests pass. The committed per-unit regression snapshot/check passes with
`_setdefaultprecision` still exact and no failures or warnings.

## House/Berth and bug audit

The routine is direct readable C; its no-argument `void` is on its own line,
and the void routine ends with an explicit `return;`. Both arguments are
named, typed, and vertically formatted. It contains no assembly, `volatile`,
forced inlining, undefined behavior, byte patch, opaque cast, raw offset,
alignment override, flag change, or comparator exception. The precision reset
is intended CRT behavior, not an original bug, so no bug/fix annotation is
needed.
