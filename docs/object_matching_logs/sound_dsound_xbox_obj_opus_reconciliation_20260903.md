# sound_dsound_xbox.obj Opus reconciliation (2026-09-03)

## Scope and provenance

This packet was reconciled in an isolated worktree from canonical
4b84e8fdc3947a8fbd4b7b1b125882a498cda16e. The donor was the committed
tip eeedd72e5ba0fc1761519f68cad241fe606320e7 plus the uncommitted
source/sound/sound_dsound_xbox.c in the read-only
opus/small-families-30k-20260902 worktree. A detached clean donor proved
that the committed source supplied 21 of 39 exact functions; the dirty
continuation supplied additional coherent function bodies.

The donor was not copied blindly. Its typed reconstruction was checked
against the January object, the campaign doctrine, the Claude
sound-DirectSound notes, and the independent Stian reference. Raw-address
lifts were used only as semantic corroboration and were not imported.

## Semantic reconciliation

- All address-shaped private names were replaced with descriptive names and
  correct static linkage. The stdcall callback is represented by its compiler
  spelling _channel_packet_callback@12 in the target inventory.
- The target platform table proves the frame roles: the 16-byte no-op is
  dsound_channel_update, the 64-byte DirectSound service/error drain is
  dsound_begin_scene, and the 784-byte deferred-settings/debug routine is
  dsound_end_scene. The exact 60-byte table now expresses those roles.
- Public declarations live in sound_dsound.h. The TU consumes the
  physical-memory and sound-cache declarations from their owner headers.
- The runtime sound pointer is the real struct sound_permutation from
  sound_definitions.h; its semantic cache-base alias and samples.size replace
  the donor's opaque offset-only shadow structure.
- sound_preferences.h now names its two arrays actual_channel_counts and
  virtual_channel_counts; the sound manager uses the latter for logical
  channels.
- The owned 257-byte error accumulator is named dsound_error_string. The
  60-byte platform_sound_dsound initializer and the error accumulator are
  strict data matches (317 bytes total).

The historical dsound_error call has one fewer vararg than its format appears
to request. That oddity was retained only because the January instructions
match exactly and both independent reverse-engineering references reproduce
the same call shape; it is treated as a genuine original defect, not
code-generation steering.

## Focused verdict

The pre-run gate was **3 exact / 36 unwritten** (144 padded bytes). The
reconciled gate is **35 exact / 4 residual / 0 unwritten** (6,976 padded exact
bytes), for a strict gain of **32 functions and 6,832 padded bytes**.

The four honest residuals remain readable source and are not admitted as
parks:

| Function | Target | Candidate | Reason not parked |
| --- | ---: | ---: | --- |
| channel_queue_packet | 640 | 640 | instruction and relocation offsets diverge |
| channel_set_location | 752 | 720 | padded size differs |
| dsound_initialize | 720 | 720 | control-flow/relocation order differs |
| dsound_set_listener_properties | 816 | 784 | padded size differs |

The 14,940-byte effects image is correctly named dsound_effects_image in the
target inventory but remains an external generated resource in source; no
synthetic byte array was fabricated to claim it.

## Verification

- Focused gate: 35 exact / 4 residual / 0 unwritten.
- Full Ninja source, progress, and semantic-progress build: pass. Repository
  code advances from 821,220 to 827,778 matched bytes (+6,558 exact code
  bytes) and data advances from 1,982,346 to 1,982,663 (+317 exact data
  bytes).
- Rename-stable whole-tree sweep: 5,816 to 5,848 exact functions; exactly 32
  gains and 6,832 padded bytes, with zero regressions.
- Strict data comparison: platform_sound_dsound is 60/60 bytes with all 13
  relocations exact; dsound_error_string is 257/257 bytes exact.
- Parked audit: 187 active / 0 stale / 0 invalid. No new park was added.
- Admission audit: zero unreviewed candidates and zero contradictions.
- Config inventory: 23,171 unique offsets before and after; zero missing or
  added offsets.
- Focused fake-match scan: zero review leads. The whole-source scan reports
  only the six pre-existing reviewed leads outside this packet.
- No inline assembly, volatile steering, optimization pragma, naked function,
  raw-byte emission, or point_from_line3d symbol is present in the rebuilt
  sound object.
- Tooling tests: 261 passed.
- git diff --check: pass.
