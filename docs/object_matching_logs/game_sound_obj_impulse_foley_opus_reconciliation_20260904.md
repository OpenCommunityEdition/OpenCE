# `game_sound.obj` impulse/foley Opus reconciliation (2026-09-04)

## Result

This packet continues from the admitted lifecycle core
`cddde9d7273ce19e418ceb226b26fa0de1a7e9c8`. Six previously unwritten owners
are reconstructed in ordinary typed C and are strict exact.

| January owner | Padded bytes | Verdict |
| --- | ---: | --- |
| `_scripted_foley_predict` | 176 | strict exact |
| `_track_object_impulse_sound` | 240 | strict exact |
| `_scripted_sound_new` | 352 | strict exact |
| `_object_impulse_sound_new` | 272 | strict exact |
| `_unattached_impulse_sound_new` | 176 | strict exact |
| `_unspatialized_impulse_sound_new` | 128 | strict exact |

The gain is **six functions, 1,344 padded bytes, and 1,291 meaningful bytes**.
The unit advances from 17 exact / 14 unwritten to **23 exact / 8 unwritten**.
No new park or whole-object completion claim is made.

## Authority and source ownership

January's split COFF is the sole authority for bytes, calling conventions,
relocations, and ownership. The interrupted Opus donor at
`eeedd72e5ba0fc1761519f68cad241fe606320e7` supplies research leads.
Independent HCEA files under `_research_cache/halocea/src/blam/sound` and
`src/headers` corroborate semantic names, layouts, and behavior only.

The complete looping-sound definition, track, and detail layouts reside in
their existing owner, `sound_definitions.h`. Their asserted sizes are 0x54,
0xA0, and 0x68; the track block remains at 0x3C and the runtime scripting index
at 0x1C. The definition-flags enum owns the fake-impulse flag. Tag and tag-block
access use the existing typed subsystem macros.

The semantic private callback `track_object_impulse_sound` updates the source
from the object's BSP location, transformed node point/normal, and velocity.
Its callback has the owner-declared generic `void const *` attachment
parameter, converted to the real typed payload without a function-pointer cast.
The source and attachment structures are ordinary local definitions for the
opaque sound-manager interface.

The scripted constructor stops the previous instance, computes its expiration,
resolves the head marker or default node basis, starts the appropriate impulse,
notifies the object type, and stores the scripting index. The foley predictor
walks looping tracks and requests the first permutation of a sound with one
pitch range. Public declaration names use `definition_index` in
`game_sound.h`.

## Corrected January calling convention

Independent review found that the previously inherited seven-argument
`sound_new_impulse` declaration came from later HCEA behavior. January's
actual `build/split/source/sound/sound_manager.obj` proves six arguments:

- the callee reads parameter slots only through `[ebp+0x1C]`;
- the promotion recursion at offsets 0x379 through 0x38E pushes six arguments;
- that recursive call cleans 0x18 bytes at 0x393;
- all three January game-sound callers likewise supply six arguments.

The unused later `is_local_player` parameter was therefore removed from the
owning `sound_manager.h`, its definition, and the recursive call. Every
rebuilt caller uses that same six-argument interface. No implicit declaration,
caller-local conflicting prototype, ABI shim, or function-pointer cast exists.

Both unattached constructors became exact immediately. The attached constructor
then differed only in three independent initialization stores. Three bounded
ordinary assignment-order probes found the exact order: copy position/forward,
set source mode/gain, assign the attachment node index, and invalidate the
cluster. These are the same necessary stores without a fabricated temporary,
alias, barrier, or behavior change.

The three provisional park entries from the interrupted working draft were
withdrawn before commit. Their seven-argument explanation was incorrect.
January evidence resolves the issue, and no part of this packet is parked.

## Verification

- Full build passes with both changed sound headers and all their consumers.
- Rename-stable sweep against the lifecycle-core snapshot: six gains / 1,344
  padded bytes / zero regressions.
- `game_sound.obj`: 23 exact / 0 residual / 8 unwritten.
- `sound_manager.obj`: 27 exact / 6 residual / 32 unwritten; existing residual
  bodies are not claimed exact.
- Protected `units.obj`: 189 exact / 0 residual / 0 unwritten.
- Isolated campaign: 889,354 / 2,198,102 meaningful code bytes and
  6,096 / 11,060 functions.
- Semantic report: 6,153 semantic exact, 6,169 accepted exact, 169 hidden
  functions / 101,147 hidden bytes, zero unit errors.
- Parks: 234 active / zero stale / zero invalid.
- Admission audit: zero candidates, contradictions, or revocations; the two
  inherited explicit rejections remain.
- Changed-source fake-match scan: zero review leads.
- Neither rebuilt sound object emits a `point_from_line3d` symbol or COMDAT.
- Tool tests passed 261/261 in the isolated lane before the ABI correction;
  canonical admission reruns the current suite after integration.

The code uses project types, typed accessors, semantic names, per-line
parameters, explicit returns, and ordinary direct control flow. No raw offset,
false type, representation pun, assembly, volatile/register forcing, optimizer
barrier, pragma, fake dependency, forced inline/noinline, undefined behavior,
or nonsensical branch is introduced. The January inline schedule is preserved.

## Canonical admission

Integrated as `c673081e2` after the independent source review, including
authenticated structure/member names and source/attachment layout assertions.
The canonical full build and every changed-header consumer pass. The stable
sweep against `cda32ef8b` confirms six gains / 1,344 padded bytes / zero
regressions. Current canonical metrics are 889,424 meaningful matched code
bytes, 6,097 credited functions, 6,154 semantic exact functions, and 6,170
accepted exact functions, with zero semantic errors. Units remains 189/189;
all 235 parks validate; the admission audit has zero contradictions or
revocations; all 272 tool tests pass. Neither sound object emits
`point_from_line3d`, and both protected bitmap file hashes are unchanged.
