# `sound_manager.obj`: opus5 250K house-clean lane, wave w3c (2026-09-15)

## 1. Scope and provenance

- Translation unit `source/sound/sound_manager.c`; target `build/split/source/sound/sound_manager.obj`.
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/250k-house-clean-20260915`,
  HEAD `0483bde89`. Compiler XDK VC7 13.00.9254 with the repository flags; no flag was changed or passed.
- Nothing was written to `sound_manager.c`, to any header, or to `config/*.json`. No ninja, configure, pytest or
  git write command was run. `git diff --stat -- source/sound/sound_manager.c` is empty.
- Worker notes `scratch/workers/w3c_sound_manager.md`; candidates and dumps in
  `scratch/workers/w3c_sound_manager/` (`base.obj`, `d1.c/d2.c`, `rl1.c`..`rl3.c` with objects,
  `rl_target.txt`, `rl_ours.txt`).
- Evidence: January split-object bytes, relocations and section order (authoritative); HCEA
  `cache/sound_dispose_from_old_map.c`, `cache/sound_refresh_looping.c`,
  `game/update_channel_for_impulse_sound.c` (later build; semantics and corroboration only);
  `config/parked.json` park entries; all ten earlier `sound_manager_obj_*` ledgers and
  `scratch/workers/sound_manager.md`; `scratch/w2/laws_w2.md` and `scratch/w3/laws_w3.md` applied as detectors.

## 2. Baseline (unchanged at the end of the wave)

`python -B tools/campaign/gate.py source/sound/sound_manager --all --forbid-emitted-symbol _point_from_line3d`
reports `== exact 58  residual 7  unwritten 0  (of 65 listed)`; the guard passed; `scratch/parkcheck.py`
reports `parks 5 drift 0`; `tools/fake_match_scan.py` reports 0 review leads.

| Function | Target padded / relocs | Base | Tag | Park |
|---|---:|---|---|---|
| `_refresh_listener` | 496 / 25 | 496 / 25 | sha | - |
| `_sound_dispose_from_old_map` | 240 / 21 | 272 / 23 | size, relocs, sha | - |
| `_sound_find_like_channel` | 272 / 16 | 272 / 16 | sha | P |
| `_sound_refresh_looping` | 1136 / 43 | 1088 / 42 | size, relocs, sha | P |
| `_sound_set_definition_end` | 208 / 9 | 208 / 9 | sha | P |
| `_update_channel_for_impulse_sound` | 528 / 29 | 528 / 29 | sha | P |
| `_update_channel_for_looping_sound` | 1360 / 61 | 1376 / 61 | size, sha | P |

## 3. Result

Nothing landed. Two of the seven rows are unparked and both are measured ties or provably spelling-inert; three
parks are ties whose known levers are already measured; two parks are blocked on one owner ruling. The wave's
substantive result is on `_sound_refresh_looping`: a new, byte-proven structural finding that fixes the park's own
recorded blocker (the missing relocation) in scratch and sharpens the residual to a single register-colouring fact.

## 4. `_sound_refresh_looping` - new structural finding (scratch only, park untouched)

### 4.1 The missing relocation is a source fact, not a merge accident

`relocdiff --count-by-target` shows January with **four** `_sound_start_fade` relocations and the parked body with
three. The park entry itself describes this as "a compiler-merged duplicate fade-call tail". The merge is visible in
our object: the alternate-fade call site ends `push ecx; jmp 0x3bf` and lands inside the fade-in site's push
sequence. Its cause is the parked source's `real fade_in_time;` local: an if / else-if / else phi that is homed at
`[ebp-0x18]` (the extra frame cell - frame 0x18 against January's 0x14) and lets both arms share one call.

January's bytes give the authentic shape directly (0x2d2-0x305):

```
2d2  cmp word ptr [ebp+0x14], 0     ; refresh_state != _looping_sound_refresh_start
2d7  jne 0x2e7                      ;   -> inaudible arm
2d9  test byte ptr [edi], 1         ; TEST_FLAG(track->flags, _fade_in_at_start_bit)
2dc  je  0x2fb                      ;   -> no fade, straight to the store
2de  mov ecx, [edi+8]               ; track->fade_in_duration
2e1  push -1 / push esi / push ecx / jmp 0x2f1
2e7  mov edx, [_sound_inaudible_fade_out_time]
2ed  push -1 / push esi / push edx
2f1  push 0 / call _sound_start_fade
2fb  mov [ebx+eax*4+0xd4], esi      ; *playing_sound_index = new_sound_index
```

Each arm builds its own argument triple and the two merge only at `push 0; call`. That is two source-level calls,
no `fade_in_time` local and no `continue`; the store is the common statement after the if/else-if.
HCEA's later build refactored this into one call with a `fade` local, so HCEA is not evidence here - January's
relocation count is.

Rewriting the block that way (`scratch/workers/w3c_sound_manager/rl1.c`) gives **1120 / 43 /
`035c62df5f0f973d`** from 1088 / 42: the relocation identity is now exact (43/43, per-target counts equal) and the
frame is exactly January's 0x14. All 58 exact siblings are unchanged; only this park drifts; fakescan is clean.

### 4.2 Exit decode: the remaining 16 bytes are one colouring fact

January has four epilogues:

| Address | Form | Reached from |
|---|---|---|
| 0x134 | `mov al,1` | the `component_sound_count == 0` datum_delete path |
| 0x448 | `xor al,al` | the `looping_sound_new` failure **and** the fall-through from the bottom of the enabled block |
| 0x451 | `mov al,cl` (`mov cl,1` hoisted at 0x84, before the found test) | the stop + not-found path |
| 0x45a | `mov al,[ebp-1]` | both disabled tests (`initialized`, `enabled`) |

`[ebp-1]` carries `result` at entry (`sete byte [ebp-1]` at 0x10) and is reused for `new_looping_sound`
(`mov byte [ebp-1],0` at 0x80, `,1` at 0xa5), so `result` is dead inside the enabled block - only possible for
locals in different nested blocks (laws_w2 A6 side law). HCEA's tail `return 0;` corroborates that the bottom of
the enabled block is an explicit `return FALSE;`, which is what shares the `xor al,al` epilogue with the
new-failed return; and the register-carried TRUE at 0x451 (rather than a second `mov al,1`) indicates a returned
variable constant-folded on that path.

Our build colours `result` into `BL` instead of spilling it. That single decision produces every remaining
difference: `source` is pushed out of EBX into ESI, ESI is then re-used for `definition_index` so `[ebp+0x10]` has
to be reloaded before `looping_sound_new` (January pushes EBX at 0x93), and every `return` of a register-carried
boolean merges with the disabled-path `mov al,bl`, collapsing January's four epilogues into three (the 16 bytes).

### 4.3 Shapes measured this wave (5, the whole budget)

| id | shape | size / relocs | verdict |
|---|---|---|---|
| rl1 | two separate `sound_start_fade` calls, no `fade_in_time` local, no `continue`, common store after | 1120 / 43 | best; relocs + frame exact |
| rl2 | rl1 + early `return result` + bottom `return FALSE` | 1104 / 43 | epilogues merge further |
| rl2a | rl1 + early `return result` only | 1104 / 43 | merges with the disabled-path exit |
| rl2b | rl1 + bottom `return FALSE` only (HCEA-corroborated) | 1104 / 43 | merges with the early FALSE exit |
| rl3 | rl2 + split declaration/assignment of `result` (A38 stack-home probe) | 1104 / 43 | inert |

### 4.4 Reopen criterion (replaces the w1 criterion)

A source-level fact that gives `result` a frame home shared with `new_looping_sound` at `[ebp-1]` - that is,
`sete byte [ebp-1]` at entry with EBX left to the `source` parameter - applied on top of `rl1.c` together with
rl2's exits (`return result` in the stop + not-found arm, `return FALSE` at the bottom of the enabled block).
Everything else in `rl1.c` is byte-proven. Do not re-spend the five shapes above, the waveC R1-R10 fade/stop-arm
variants, or the w1 S1-S8 return-materialisation variants.

### 4.5 Proposal (orchestrator; zero strict credit)

Re-baseline the `config/parked.json` entry for `_sound_refresh_looping` from `1088 / 42 / 4820b25c...` to
`rl1.c`'s `1120 / 43 / 035c62df...` and land the `rl1.c` body with that edit. It is strictly closer (size +32
toward 1136, relocation identity exact, frame exact), it resolves the mechanism the park entry itself names, and
it deletes an unnecessary local rather than adding anything. This is not the R11 pattern: it is strictly closer
and it is tied to the sharpened reopen criterion above.

## 5. The other six rows

### `_sound_dispose_from_old_map` (active residual, 240 / 21 vs 272 / 23)

Target and candidate are byte-identical through 0xa6, including the entire ten-instruction wait-loop test
(`call _system_milliseconds; test eax,eax; mov [ebp-8],eax; fild; jge; fadd __real@4f800000; fcomp [ebp-4];
fnstsw; test ah,5; jp exit`). January closes the loop with one `jmp 0x82` back to that test; our build duplicates
the whole test into the latch (`jnp 0xa1`), which is the +12 instructions, +32 padded bytes and +2 relocations
(one `_system_milliseconds`, one `__real@4f800000`). The same function shows January rotating the preceding fade
loop into a guarded do-while, so the decision is per loop, not per function. HCEA spells the wait loop exactly as
we do.

New negatives this wave: `for (; cond; ) { sound_idle(); }` (d1) and the A23 for-increment probe
`for (; cond; sound_idle()) { }` (d2) are both byte-identical to the baseline. Together with the four earlier
lanes (while, while + continue, `while (TRUE)` + real local, `for(;;)` + break, inline deadline, HCEA `any_sounds`
flag, captured `paused`, definition position, whole-TU compile-order reorder) this proves the loop **spelling** is
not the lever; the header duplication is decided after the IL. Reopen: an exact January sibling whose
`while (call() < real)` loop keeps a single header copy, with a nameable source cause.

### `_refresh_listener` (active residual, 496 / 25, 178/178 instructions)

One transposition at the loop latch: January `mov edi,[ebp-4]; mov esi,[ebp-8]`, ours the reverse (the two loop
IVs reloaded after `matrix4x3_inverse_transform_vector`). Detector pass over laws_w2/laws_w3: A24 does not fire
(no parameter home is involved, both IVs own cells), A12 is about preheaders, A6 does not fire (frames are equal),
A20 needs stores. Declaration order, in-loop declaration, void single exit, the HCEA inverted arm and a whole-TU
compile-order reorder are already measured inert. No shape spent; criterion unchanged.

### `_update_channel_for_impulse_sound` (park, 528 / 29, 170/170 instructions)

Three ordering deltas remain inside one basic block: (a) January pre-pushes the
`sound_definition_get_minimum_distance` argument (`mov eax,[esi+8]; push eax`) into the gain product; (b)
`fld [esi+0x88]; fmul [edx+0x30]` against our `fld [edx+0x30]; fmul [esi+0x88]` for
`sound->pitch * pitch_range->playback_rate` (A35 leaf age); (c) January groups the three cone loads and wedges the
`FLT_MAX` store between the cone stores (the A4 "all loads then all stores" fingerprint). Both admissible levers
for (b) are already measured - a source operand swap is canonicalised away (waveC I1) and the named pitch local is
worse (waveC I2, 544) - and the only source form that would produce (c) is an aggregate copy or helper that does
not exist in January's tag layout (an invented owner, R14/R15). Exactness is therefore unreachable through (a)/(c)
alone, so no shape was spent.

### `_sound_set_definition_end` (park, 208 / 9) and `_update_channel_for_looping_sound` (park, 1360 vs 1376)

Both carry the same single mechanism, and laws_w3 A34 names it: January loads the `short` field
`sound->pitch_range_index` for a call argument as `xor eax,eax; mov ax,word [esi+0x8e]` (set_definition_end 0x3c;
looping 0x2ea), the fingerprint of a `short` prototype parameter, while `sound_definitions.h` publishes
`long pitch_range_index`, whose C semantics force our genuine conversion `movzx`. For `_sound_set_definition_end`
that load plus the resulting two-store shift is the *only* difference (72 vs 73 instructions);
`_update_channel_for_looping_sound` additionally shows a frame-cell permutation and one extra `xor eax,eax`, and
waveC measured it at exactly 1360/61 under an emulated `short` prototype. The owner ruled that prototype an ABI
lie (the `sound_definitions.c` definition is `long` and casts every use), so both rows stay blocked on that owner
decision; nothing in `sound_manager.c` can close them, and a mask or cast trick would be R7. When the ruling
unblocks the looping row, also fix the behavioural defect the w1 ledger recorded in the parked body (the
next-permutation test is nested, so the permutation advance is skipped).

### `_sound_find_like_channel` (park, 272 / 16, 91/91 instructions)

One callee-saved colouring swap (January `si`/`ebx` where we use `bx`/`esi`, including the `mov ax,si` return
copy). No law fires; declaration order and a named channel pointer are measured inert (waveC F1-F3). No shape
spent.

## 6. Checks

- Final real-file gate identical to baseline row for row: `== exact 58  residual 7  unwritten 0`;
  `_point_from_line3d` guard passed.
- `scratch/parkcheck.py` on the real object: `parks 5 drift 0`.
- `tools/fake_match_scan.py source/sound/sound_manager.c`: 0 review leads (also 0 on `rl1.c`).
- `git diff --stat -- source/sound/sound_manager.c`: empty.

## 7. Disposition

`sound_manager.obj` stays at 58/65 strict exact; this wave claims no bytes. It leaves one actionable zero-credit
proposal (the `_sound_refresh_looping` park re-baseline to `rl1.c`), one owner ruling that would unlock 1,568
padded bytes across `_sound_set_definition_end` and `_update_channel_for_looping_sound`, and four rows classified
as measured ties with their levers exhausted.
