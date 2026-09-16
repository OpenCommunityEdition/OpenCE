# `saved_game_files.obj` — opus5 next-150K lane, wave n1 (2026-09-15)

## Baseline and final

Guarded gate at HEAD and after the wave: **45 exact / 1 residual / 0 unwritten**,
`_point_from_line3d` guard passes, park drift 0. The real file
`source/saved games/saved_game_files.c` is **unchanged**. Nothing landed.

`branch_sweep`: no historical blob exceeds 4 exact / 41 unwritten. No donor.

## `_saved_game_files_enumerate_available_to_local_player_index` — SKIPPED-EXHAUSTED (owner-blocked)

Target 368 padded / 359 meaningful / 23 relocations / sha `fe3dc1b00a07ea03`.
Ours 368 / 23 / sha `dd7b18146c75001c`. Parked `unclassified`; the recorded criterion is
"reopen only with authoritative source or evidence for a defined original failure result".

### Census

* **Frame:** identical — both `sub esp,0x20c`, `struct enumerated_saved_game_file file`
  at `[ebp-0x20c]`, the same parameter homes `[ebp+8 … +0x18]`. No slot differs.
* **Relocations:** 23/23, identical targets and order.
* **Instructions:** target 131, ours 134 (three trailing `nop`).

Only two hunks differ, and they are the same fact twice:

* ours emits `push edi; xor edi,edi` at 0x0d, in the prologue;
* January emits `push edi` at 0x7d and `xor edi,edi` at 0x94 — **inside** the
  `take_mutex(general_mutex)` success block — and pops edi only in the success epilogue
  at 0x144.

### New byte-level evidence (strengthens, not lifts, the park)

The mutex-failure epilogue at 0x14a-0x166 is a tail-duplicated copy of the single source
statement `*number_of_profiles = number_of_available_profiles;`, and because the
accumulator has no register on that path it is read from its stack home:

```text
14a  push "failed to take saved game files mutex"   151 call _error
156  mov dx, word ptr [ebp+8]        ; the accumulator's home
15a  mov eax, dword ptr [ebp+0x10]   ; number_of_profiles
160  mov word ptr [eax], dx
```

`[ebp+8]` is `player_index`'s incoming parameter home, which January reuses at 0x91
(`mov dword ptr [ebp+8], eax`) for the block-local `number_of_entries` — laws_w2 A24
parameter-home offer. So on the mutex-failure path January exports the low word of a slot
that holds either `player_index` or a stale entry count. The accumulator is therefore
provably **declared without an initializer** and read indeterminate, exactly as the park
states.

### Alternatives enumerated and refuted by the bytes (no shape spent)

| candidate spelling | refuted by |
| --- | --- |
| `long number_of_available_profiles = 0;` (current) | emits `xor edi,edi` in the prologue |
| zero assigned in both mutex arms | the else arm would emit `xor`/`mov 0`, not a load of `[ebp+8]` |
| the final store moved inside the success arm | there would be no store at all on the failure path, contradicting 0x160 |
| `*number_of_profiles = *number_of_profiles;` on failure | reads `[eax]`, not `[ebp+8]` |

### Reopen criterion (refined)

The park's premise is confirmed and sharpened: the failure-path value is the low word of
the reused `player_index` / `number_of_entries` home. The only byte-exact form needs that
uninitialised read, which brief section 7 prohibits and which the
`saved_game_family_reconciliation_20260909.md` audit already rejected; under laws_w3 R13
that rejection binds this wave, so **no reopen is proposed**. Reopen only on an owner
ruling under the original-bug policy, or on authoritative January source.
