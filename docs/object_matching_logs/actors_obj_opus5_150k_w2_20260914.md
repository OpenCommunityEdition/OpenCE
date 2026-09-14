# `actors.obj` Opus5 150K lane, wave w2: park re-investigation and residuals (2026-09-14)

## Scope and provenance

- Translation unit `source/ai/actors.c`, target `build/split/source/ai/actors.obj`.
- Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, HEAD `ea9175577`. The worker run was cut off once; the
  resume kept only verified notes. The real file was never modified.
- Compiler: XDK VC7 13.00.9254 `/O2 /Oy- /DDEBUG /Dxbox` (build.ninja, unchanged).
- Evidence:
  - January split COFF;
  - Ghidra `scratch/ghidra/out/actors.obj.decomp.c` (`_actor_place` at :3347), and the atlas
    `scratch/atlas/actors.obj.txt` (names only);
  - HCEA `blam/ai/actors_spawn_from_unit.c`, `actor_place.c` and `actor_create_for_unit.c` (behaviour and topology
    only);
  - the 100K wave-D scratch bodies (`vsfu_c.c`, `viu3.c`, `vpl2.c`), re-measured on the current tree.
- Worker notes: `scratch/workers/actors.md`. Scratch candidates: `scratch/workers/actors/`.

## Baseline and final (real file)

`gate.py source/ai/actors --all --forbid-emitted-symbol _point_from_line3d`:

- **Baseline:** 70 exact, 6 residual, 0 unwritten. The point guard passes.
- **Final:** identical row for row (`scratch/workers/actors/final_gate.txt`). The point guard passes.
- **Parks:** `scratch/parkcheck.py` reports 5 parks and 0 drift.
- **Owner census:** no candidate-only and no base-only owner.
- **Fake scan:** `tools/fake_match_scan.py`, 0 leads.
- **Diff:** `git diff --stat -- source/ai/actors.c` is empty.

| Function | Target size/relocs | Real size | Park | Outcome |
| --- | ---: | ---: | --- | --- |
| `_actor_create_for_unit` | 624 / 29 | 624 | register-allocation | not landed (5 shapes) |
| `_actor_customize_unit` | 640 / 26 | 672 | unclassified | not landed (1 shape) |
| `_actors_spawn_from_unit` | 672 / 29 | 672 | instruction-scheduling | not landed (2 shapes) |
| `_actor_place` | 800 / 41 | 816 | unclassified | not landed (2 shapes); type-fact proposal |
| `_actor_input_update` | 2384 / 89 | 2400 | unclassified | not landed (2 shapes) |
| `_actor_unit_control` | 784 / 48 | 784 | — | owner-blocked (units.c protected) |

## Per-function findings

### `_actor_create_for_unit`

**January facts from the bytes:**
- `or esi,-1` is used for the two entry compares and for two separate NONE exits: `0x266`, which does not pop ebx, and
  `0x9f`, which does.
- ebx is pushed only after the entry tests.
- After that, esi becomes `disallow_actor_index` inside the swarm loop.
- The `actor_index == NONE` test is jump-threaded away on the not-found and biped-OK paths. Only the found path tests
  it (`0xa8`).
- An `actor_new` failure jumps straight to the bottom `mov eax,edi` epilogue.

So January's NONE pin is a separate constant web.

**Shapes:**
- Every outer-if or single-exit spelling coalesces the pin with `actor_index` in edi, pushes ebx at entry and merges
  the exits: cfuA (flag), cfuC (outer if).
- Mixed `return NONE` and `return actor_index` spellings fold to base: cfuB, cfuD, cfuE.

**Residual:** the placement of the constant-register pin, which then drives the ebx shrink-wrap and the exit split.

### `_actor_customize_unit`

- The frame is identical (`sub esp,0x98`).
- The +32 bytes are 3 unit reloads into a register at joins, plus far jumps to an equipment-error block that is laid
  out after the epilogue.
- This is the callee-saved permutation variant/unit_index/unit.
- Passing the grenade count directly as the call argument (cuA) is hash-identical to base.

### `_actors_spawn_from_unit`

The waveD body `vsfu_c` was re-verified on the current tree as sfu1: 688/29, 211 against 207 instructions. The
remaining differences are:

1. **The −1 compare pin.**
   - January compares the swarm and actor indices with an immediate −1.
   - It keeps encounter/squad in cx/dx through the join, then emits `or eax,-1; cmp cx,ax` and spills after the join.
   - Ours pins −1 in ecx before the swarm test, stores encounter inside the actor arm, and duplicates the fake-arm tail.
2. **The launch operand order.**
   - January emits `fld forward.i; fmul speed`. Ours emits the reverse.
   - The x87 operand-order law predicts ours for a named call-result local. No January evidence shows a later-created
     forward copy.

The HCEA fake-first `== &&` polarity (sfuA) is code-identical to sfu1.

### `_actor_place`

**One allocation decision explains every hunk:**
- January gives edi to `actor_definition` (1 use).
- It keeps `unit_index` memory-only in the `initial_variant` home `[ebp+0x1c]`.
- Ours gives edi to `unit_index`, plus a new home `[ebp-8]`. That is the +4 frame.
- Ours spills `actor_definition` to `[ebp+0x18]` and shrink-wraps ebx.

**New measured law** (lab `scratch/workers/actors/lab/t11.c`, VC7 13.00.9254 `/O2`):

| Source field | Parameter | Emitted load |
| --- | --- | --- |
| 16-bit field | `short` | `xor r,r; mov r16,[base+d]` |
| 16-bit field, destination register is the base register | `short` | `movsx r32, word ptr [r+d]` |
| `signed char` | `short` | `movsx r16, byte ptr` |
| `signed char` | `char` | `xor r,r; mov r8` |

- January's argument 11 (`xor ecx,ecx; mov cx,[eax+0x1a]`) against ours (`movsx eax,word ptr [eax+0x1a]`) is
  therefore an allocation consequence.
- January's argument 12 (`movsx ax, byte ptr [eax+0x12]`) **is** a type fact: the last parameter of
  `actor_create_for_unit` is `short`.

**Shapes:**
- plA: single exit with `actor_index = NONE` assigned in the unit-failure arm. The alndiff is identical to base.
- ov/ncs: overlay `actors.h` plus the definition changed to `short noncombat_sequence_id`.
  - January's push form is reproduced.
  - Size stays 816 with the new sha 4d7d2f90 (park drift).
  - All 70 exact rows and the other 4 parks are unchanged.

### `_actor_input_update`

The waveD body `viu3` was re-verified as iu1: 2384/89 (equal size), 690 against 691 instructions, about 20 real
hunks. The remaining differences are:

1. **Store scheduling:** the third swarm_center store comes after the loop-guard compare.
2. **Unit register lifetime at the aiming join.**
   - In the aiming else arm, January copies through `lea edx,[ebx+0x1ec]`, so unit stays in ebx for the
     looking-vector copy.
   - Ours folds `add ebx,0x1ec` and reloads unit from the frame.
   - That reload gives unit 5 frame references instead of 4. By the frame reference-density law, unit moves from
     `[ebp-8]` to `[ebp-4]`.
   - The same change reorders 5 x87 operands in the two inlined `cross_product3d` calls.
3. **The unit cache register:** eax in January against ebx in ours before the threat loop, plus its knock-on argument
   loads.

A single-exit if/else spelling (iu2) is identical to iu1. The accessor-binding law does not apply: this is a plain
struct copy with no accessor to bind.

### `_actor_unit_control`

- Re-verified: the only code hunk is the animation-impulse argument.
- January emits `xor eax,eax; mov ax,...`, a short parameter (lab t1 form). Ours emits `movsx edx,ax` from
  `long animation_impulse` in `units.h:638-644`.
- The fix is the wave-D units.h/units.c proposal. `units.c` is protected in this lane, so this function is
  owner-blocked.

## Orchestrator proposals

1. **Type fact, zero credit, optional.**
   - **Change:** in `source/ai/actors.h:1038`, change `char noncombat_sequence_id);` to
     `short noncombat_sequence_id);`, and make the same change on the definition line of `actor_create_for_unit` in
     `source/ai/actors.c`.
   - **Evidence:**
     - January `_actor_place` +0x252 emits `movsx ax, byte ptr [eax+0x12]` (lab t1/t2).
     - The callee reads `mov al,[ebp+0x34]`.
     - The other January callers push the constant 0: `ai.c` `ai_disconnect_from_structure_bsp`, and `ai_script.c`
       `ai_scripting_attach_unit` / `ai_scripting_attach_free`. All three are EXACT and unaffected.
   - **Consequence:** the `_actor_place` park base must be re-measured (816/41, sha 4d7d2f90). The edit is in place,
     so the declaration count does not change.
   - **Consumers of `actors.h`:** 46 files (`grep -rl 'actors.h"' source`), comprising 43 files under
     `source/ai/` (every `action_*.c`, `actor*.c`, `ai*.c`, `encounters.c`, `props.c`, and `ai_debug.h`) and
     `items/weapons.c`, `units/bipeds.c`, `units/units.c`, `units/unit_dialogue.c`.
2. **units.h/units.c short `animation_impulse`.** This is unchanged from the wave-D ledger, proposal 2. It closes
   `_actor_unit_control` (784) and is owner-blocked on units.c.

## Do-not-repeat (additions)

| Function | Spellings that are inert or rejected |
| --- | --- |
| create_for_unit | outer valid-input if; mixed `return NONE` / `return actor_index` exits |
| spawn_from_unit | HCEA `== &&` fake-first polarity |
| place | single exit with a failure-arm `actor_index = NONE` |
| input_update | single-exit if/else for the swarm arm |

## Reopen criteria

- **Parks:** unchanged. A future wave should start from:
  - `scratch/workers/actors/sfu1.c` (spawn);
  - `iu1.c` (input_update);
  - `ov/ncs.c` (place, with the type fact).
- **`_actor_place`:** re-measure the park if proposal 1 lands.
- **`_actor_unit_control`:** closes when units.c is unblocked.

## Disposition

The object is still active at 70/76 strict. Nothing was landed and no fuzzy credit was taken.
