# `actors.obj` Opus5 250K lane, wave w3d: structural pass on the three size-gap parks (2026-09-15)

## Scope and provenance

- Translation unit `source/ai/actors.c`, target `build/split/source/ai/actors.obj`.
- Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/250k-house-clean-20260915`.
  `source/ai/actors.c` is unchanged since `5a84a96dd`; **the real file was not modified in this wave**
  (`git diff --stat -- source/ai/actors.c` is empty).
- Compiler: XDK VC7 13.00.9254 `/O2 /Oy- /DDEBUG /Dxbox` (build.ninja, unchanged).
- Assignment: STRUCTURAL PASS over `_actor_input_update` (2384 vs 2400), `_actor_place` (800 vs 816, w3 C1
  third-pass row with law A24) and `_actor_customize_unit` (640 vs 672). All three are parked, so every
  measurement is scratch-only.
- Evidence used: the January split COFF and its listings; `relocdiff --count-by-target --allow-structural`;
  per-function `[ebp±N]` frame censuses; `scratch/w2/laws_w2.md` and `scratch/w3/laws_w3.md` applied as
  detectors; HCEA `blam/ai/actor_place.c`, `actor_customize_unit.c`, `actor_input_update.c` (behaviour and
  statement order only); the w2/w3 scratch bodies re-gated at HEAD.
- Worker notes: `scratch/workers/w3d_t3_actors.md`. Scratch: `scratch/workers/w3d_t3_actors/`.

## Baseline and final (real file)

`gate.py source/ai/actors --all --forbid-emitted-symbol _point_from_line3d`

- **Baseline:** 70 exact, 6 residual, 0 unwritten; point guard passes
  (`scratch/workers/w3d_t3_actors/baseline_gate.txt`).
- **Final:** byte-identical listing (`final_gate.txt`; `diff` against the baseline is empty). Guard passes.
- **Parks:** `scratch/parkcheck.py` reports 5 parks, 0 drift.
- **Fake scan:** `tools/fake_match_scan.py source/ai/actors.c`, 0 review leads.
- **Owner census:** not applicable to the real file (unchanged); measured for the scratch candidates below.
- **branch_sweep:** 34 unique blobs, current HEAD blob is the best (70 exact).

| Function | Target | Real file | Park class | w3d outcome |
| --- | ---: | ---: | --- | --- |
| `_actor_create_for_unit` | 624 / 29 | 624 | register-allocation | not a target this wave |
| `_actor_customize_unit` | 640 / 26 | 672 | unclassified | **not landed** — full census, 2 shapes, tie |
| `_actors_spawn_from_unit` | 672 / 29 | 672 | instruction-scheduling | not a target this wave |
| `_actor_place` | 800 / 41 | 816 | unclassified | **not landed; STRUCTURAL GAP CLOSED in scratch (800/41)** |
| `_actor_input_update` | 2384 / 89 | 2400 | unclassified | **not landed** — census shows no new structural gap |
| `_actor_unit_control` | 784 / 48 | 784 | — | owner-blocked (units.c protected, E1) |

## 1. `_actor_place` — the structural gap is a STATEMENT ORDER defect (A20)

### Census

- **Relocations:** 41/41, identical target multiset. No missing or extra call.
- **Frame:** January `sub esp,0x190`, ours `sub esp,0x194`. Ours carries **one extra own 4-byte cell**.
  - January: `encounter_name[256]` at `[ebp-0x190]`, `placement_data` (0x88) at `[ebp-0x90]`,
    `swarm` at `[ebp-8]`, `initially_braindead` at `[ebp-4]`; `unit_index` lives in `initial_variant`'s
    parameter home `[ebp+0x1c]` and `initial_state` in `upgrade_major`'s home `[ebp+0x18]`;
    `default_state` and `actor_definition` never leave EDI.
  - Ours: the same aggregates shifted by 4, **plus `unit_index` in its own cell `[ebp-8]`**, `swarm` pushed to
    `[ebp-0xc]`; `actor_definition` spilled to `[ebp+0x18]` at 0x9e and `default_state` homed as well.
  - So ours spills six values into five slots where January spills four into four. The two extra spilled values
    are `actor_definition` and `default_state`, both of which January keeps in EDI, because January takes EBX
    for `starting_location` at entry (`push ebx` precedes the `starting_location` assert) while ours puts
    `starting_location` in EDI and shrink-wraps EBX onto `squad_index` at 0x17b.

### Derived defect and fix

January's independent stores inside `if (encounter_index != NONE)` are, in order, `mov byte [ebp-4],dl`
(`initially_braindead`, 0x224) and then `mov word [ebp+0x18],cx` (`initial_state`, 0x227). By law A20
(independent stores keep source order) the braindead flag is assigned FIRST. HCEA `blam/ai/actor_place.c` has
exactly that order. Our body assigned `initial_state`, `default_state`, then the flag.

**`pl_o1`** — move
`initially_braindead = TEST_FLAG(encounter_definition->flags, _encounter_braindead_bit);`
ahead of the two squad-state assignments, nothing else:

- `_actor_place` **800 / 41**, normalized sha
  `ce8124335417a81a5e1448d24f1fa9176741033aa2c9ffc59b15faa6c8b23c03` (target 800/41 `a4258acc…`).
- frame becomes 0x190 and the whole slot map becomes January's, including `unit_index` in `[ebp+0x1c]`
  and `initial_state` in `[ebp+0x18]`;
- whole-TU gate: 70 exact, 6 residual, guard passes, no baseline-exact row changes;
- owner census vs `build/base/source/ai/actors.obj`: 190/190, no candidate-only and no base-only owner;
- **no header or config change is needed for this.**

### Remaining residual after `pl_o1` (+ the E11 type fact)

With E11 applied in a shadow tree (`pl_n1`, 800/41,
`3a33538607b4cb44c23e441317ca7e4236b613b3e226db7a7f93269a71ee2295`) January's
`movsx <16-bit>, byte ptr [eax+0x12]` argument form is reproduced and the body is byte-identical up to 0x249.
The entire residual is then the emission order of three argument pre-loads for `actor_create_for_unit`:

```
January: mov edx,[ebp+0x18]            (arg 9  initial_state)
         xor ecx,ecx / mov cx,[eax+0x1a] (arg 11 command_list_index)
         movsx ax, byte [eax+0x12]     (arg 12 noncombat_sequence_id)
ours:    movsx dx, byte [eax+0x12]     (arg 12)
         movsx eax, word [eax+0x1a]    (arg 11, dest==base -> the w2 lab-t11 movsx form)
         mov ecx,[ebp+0x18]            (arg 9)
```

January evaluates 9, 11, 12 (left to right), ours 12, 11, 9. Everything after that (ecx/eax/edx rotations in the
two warning blocks and the epilogues) follows from it. 270 vs 269 instructions; sizes and relocations equal.

### Shapes (7)

| shape | change | result |
| --- | --- | --- |
| pl_d1 | `long unit_index;` declared before `placement_data` | 816/41, identical to base |
| pl_d2 | pointer/scalar declarations moved after the aggregates | 816/41, identical to base |
| **pl_o1** | braindead assignment first (A20 + HCEA) | **800/41 `ce812433…` — structural gap closed** |
| pl_o2 | pl_o1 + squad fetch and state assignments in a nested block | 800/41 `9466a0dc` |
| pl_n1 | pl_o1 + E11 `short noncombat_sequence_id` (shadow header) | 800/41 `3a335386` |
| pl_n2 | pl_o2 + E11 | 800/41 `c932b184` |
| pl_n3 | pl_n1 + `initial_state`/`default_state` declaration swap | identical to pl_n1 (inert) |

## 2. `_actor_customize_unit` — the `[size]` tag is a coalescing tie, not a structural gap

### Census

- **Relocations:** 26/26, identical target multiset.
- **Frame:** `sub esp,0x98` in both, four dword slots plus `placement_data` in both, identical parameter homes.
  The only difference is the occupant of `[ebp-0xc]`: January spills the `change_colors` element pointer there,
  ours spills `actor_variant_definition`. A24 does not fire.

### Byte accounting (+21 real bytes)

3 (`actor_variant_definition` spill at 0x25) + 2 (memory-only loop-counter init) + 4 (`jle` rel8->rel32)
− 3 (`change_colors` kept in EBX) + 3 (memory-only loop increment) + 3 + 3 + 3 (three `unit` reloads at joins)
+ 8 (two `je error` rel8->rel32 because the error block is sunk past the first epilogue) − 5 (camo block).

Root cause: callee-saved coalescing. January pairs EDI = `unit` then the loop counter and keeps EBX =
`actor_variant_definition` for the whole function; ours pairs EBX = `unit_index` then `change_colors` and keeps
`unit` re-cached in ESI at three joins, which leaves the loop counter in memory. The error-block sink and all
three jump growths are downstream of the extra join blocks.

### Shapes (2)

- **cu1** — equipment powerup test written positive-first
  (`!= _equipment_powerup_none && != _equipment_powerup_grenade`, placement body as the then-arm, `error()` in
  the else arm), which is the form HCEA `blam/ai/actor_customize_unit.c` keeps. Result 672/26, sha `0a9c42b8`,
  **byte-identical to base**: VC7 canonicalises the polarity and still sinks the error block.
- **cu2** — `short change_color_index;` declared ahead of `struct object_placement_data placement_data;`.
  672/26, byte-identical to base.

Classification: SKIPPED-EXHAUSTED / register-coalescing tie.

## 3. `_actor_input_update` — no structural gap remains beyond the existing scratch body

### Census

- **Relocations:** 89/89, identical target multiset.
- **Frame:** `sub esp,0x24` in both, the same seven slots plus `[ebp+8]`. Only the reference counts differ
  (`[ebp-4]` 4 vs 9, `[ebp-8]` 7 vs 3, `[ebp-0x10]` 2 vs 4): ours makes one extra `unit` reload, which raises
  `unit`'s reference density and swaps the `-4`/`-8` occupants (the w2 finding, re-confirmed).

### The structural gap is already closed by `iu3`

`scratch/workers/w3_actors/iu3.c`, re-gated at this HEAD as `scratch/workers/w3d_t3_actors/iu3.obj`:
**2384 / 89**, sha `f6ff0151006bb86f` — target size and target relocation count, 70 exact rows unchanged,
guard passes. Its residual has one root:

1. `0x6a6` — January `lea edx,[ebx+0x1ec]` keeps EBX = `unit` live into the join, where `add ebx,0x210` is its
   last use; ours does `add ebx,0x1ec` and then reloads `mov eax,[ebp-4]` at the join.
2. That reload produces the `-4`/`-8` slot swap and the register rotations at 0x4d3 / 0x517 / 0x590 / 0x5b0
   (all pure renames — the apparent `game_team_is_enemy` argument difference in the aligned diff is an
   alignment artifact; the two byte streams are structurally identical there).
3. Five of the twelve products in the two inlined `cross_product3d` bodies `fld` the other leaf (A22/A35). The
   helper argument roles were re-verified against HCEA and are correct, as is the `normalize2d` polarity
   (January `test ah,0x41; jne` makes the `> 0` case the fall-through, which is our then-arm) and the
   `burning_to_death` / `melee_attacker_attached` / `delayed_attached_projectile_index` store order.

No new shapes were spent: the census produces no structural defect beyond what `iu3` already fixes, and the w3
do-not-repeat table (iu3, iu4) plus R13 bind the tie budget.

## Orchestrator proposals

### P-A — `_actor_place` park re-baseline (actors.c only, no header edit)

Apply the `pl_o1` statement reorder and re-baseline the `_actor_place` entry in `config/parked.json` from
`816 / 41 / ff120b98736b6eb6…` to `800 / 41 / ce8124335417a81a5e1448d24f1fa9176741033aa2c9ffc59b15faa6c8b23c03`.
Evidence: A20 store order in January (0x224 braindead before 0x227 initial_state) plus HCEA
`blam/ai/actor_place.c`. Effect: frame 0x194 -> 0x190 with January's exact slot map; 70 exact rows unchanged;
owner set identical to `build/base`; guard passes; other four parks unchanged. Strictly closer, zero strict
credit (R11), so it is worth landing as the prerequisite for the reopen criterion below or together with P-B.

### P-B — E11 upgraded: `short noncombat_sequence_id` (header packet)

- Text: `source/ai/actors.h:1038` `\tchar noncombat_sequence_id);` -> `\tshort noncombat_sequence_id);`, and the
  matching definition line in `source/ai/actors.c`. Do **not** change the struct field at `actors.h:457`
  (`char noncombat_sequence_id;`) — January's `movsx <16-bit>, byte ptr [eax+0x12]` proves a signed *byte
  field* read into a *short parameter*.
- Evidence: January `_actor_place` +0x252; the w2 lab t11 table (`signed char` field + `short` param ->
  `movsx r16, byte ptr`; + `char` param -> `xor r,r; mov r8`, which is today's form).
- Consumers: 46 `actors.h` includers. Only three TUs call `actor_create_for_unit`: `actors.c` (2 sites),
  `ai.c` (`ai_disconnect_from_structure_bsp`), `ai_script.c` (`ai_scripting_attach_unit`,
  `ai_scripting_attach_free`); the latter two push the constant 0.
- Measured through a shadow header (pickup proven with an injected `#error`): `source/ai/ai` stays 42/46 and
  `source/ai/ai_script` stays 112/116 with identical residual rows; `actors.obj` stays 70/76 with
  `_actor_place` at 800/41 `3a33538607b4cb44…`. A full-board rebuild is still required.
- Note: VC7 13.00.9254 does not reject a `char` prototype against a `short` definition, so both halves must be
  changed together for source correctness.

### P-C — unchanged

units.h/units.c `short animation_impulse` (E1) remains the only blocker for `_actor_unit_control` 784;
`units.c` is protected in this lane.

## Do-not-repeat (additions)

| Function | Spellings that are inert or rejected |
| --- | --- |
| customize_unit | positive-first equipment powerup test (byte-identical); `change_color_index` declaration moved ahead of `placement_data` (byte-identical) |
| place | `unit_index` / `actor_definition` declaration-position moves (byte-identical); `initial_state`/`default_state` declaration swap on top of pl_o1 (inert); squad fetch in a nested block (pl_o2/pl_n2, 800/41 but a different sha and no closer) |

## Reopen criteria

- `_actor_place`: any source form that makes VC7 evaluate the 9th argument (`initial_state`) before the two
  `starting_location->` field arguments of `actor_create_for_unit`. With pl_o1 + P-B in place that is the only
  remaining difference and the function becomes strict exact (800 B).
- `_actor_customize_unit`: a same-compiler allocation donor (or January local records) explaining why EBX goes
  to the definition pointer and the loop counter is enregistered while `change_colors` spills.
- `_actor_input_update`: unchanged — a lever for the `unit` live-range split at the aiming join; start from
  `scratch/workers/w3d_t3_actors/iu3.c` (2384/89).

## Disposition

The object is still 70/76 strict. Nothing was landed, no fuzzy credit was taken, and the real file is
byte-identical to HEAD.
