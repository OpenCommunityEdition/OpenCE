# `hud_sounds.obj` source-authenticity review, 2026-09-04

## Verdict

The published `hud_play_sound` body is byte-exact at 352 padded bytes, but its
source is **not admissible under the current house rules**. Its final parameter
is declared `word *volatile played_flags`. That qualifier makes the function's
local pointer object volatile; it does not make the pointed-to shared flag word
volatile. The local pointer's address never escapes, and no hardware, thread,
callback, SDK, or ownership evidence gives those local reloads runtime meaning.
The qualifier is therefore compiler-schedule steering, a construct explicitly
barred by the campaign's matching methodology.

The void function also lacks the required explicit terminal `return;`.

The correct disposition is to preserve the historical exact source in Git
history, publish the natural unqualified API as `word *played_flags`, add the
explicit return, and accept an honestly measured fuzzy park. This is a
policy-driven revocation of one prior exact owner, not an accidental regression
and not lost reconstruction knowledge.

## Source-shape finding

The impulse arm currently performs a second
`sound_indices[absolute_sound_index] != NONE` check inside a branch where the
same fact has already been established. The target does contain the repeated
comparison, so the machine behavior is real, but the retained nested source
form is needlessly contradictory with source-recovery goals. A natural and
behaviorally equivalent original-source candidate is the usual
`handle == NONE || !played` guard followed by `if (handle != NONE) stop` and
then start. That topology explains why an older frontend can retain the second
comparison without embedding a logically redundant nested test in source.

This finding authorizes one bounded semantic cleanup; it does not authorize
declaration-order, register, volatile, pragma, assembly, or control-flow
lotteries to recover the old 352-byte object.

## Evidence boundary

- Current target and legacy candidate are strict exact at 352 padded bytes.
- The minimal diagnostic that only removes the unsupported volatile qualifier
  and adds the required return emits 384 padded bytes and is nonexact:
  `scratch/hud-sounds-natural-pointer-20260904.obj`.
- The diagnostic edit manifest is
  `scratch/hud-sounds-natural-pointer-20260904.json`.
- The exact source entered history at commit
  `32580a870cbbd165e2038bc931e24cb8e6093711`; that commit provides no
  provenance or concurrency justification for the qualifier.
- Later HCEA source independently corroborates the sound-start/stop semantics
  but uses an ordinary pointer and clean rising-edge control flow.

`hud_play_sound` keeps the same cdecl ABI when the top-level local pointer
qualifier is removed. Its proper declaration belongs in the closest existing
HUD owner header, `interface/hud.h`, not as a caller-local prototype. New HUD
callers may depend on the cleaned fuzzy helper without claiming that
`hud_sounds.obj` remains `Matching`.
