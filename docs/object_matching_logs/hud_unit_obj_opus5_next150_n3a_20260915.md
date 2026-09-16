# hud_unit.obj — opus5 next-150K lane, wave n3a (structural park pass), 2026-09-15

Unit: `source/interface/hud_unit.c`. Worker notes: `scratch/workers/n3_hud_unit.md`.
Scratch: `scratch/workers/n3_hud_unit/` (`w1.c/.obj` control, `v1/d1/d2/d3/d4` probes, `w1.aln`,
`w1.full.txt`, `score.py`).

## Baseline and final state

| | baseline (HEAD `6ebd05a96`) | final |
| --- | --- | --- |
| gate | `== exact 21 residual 1 unwritten 0` | `== exact 21 residual 1 unwritten 0` |
| `_point_from_line3d` guard | passes | passes |
| parkcheck (`build/base/source/interface/hud_unit.obj`) | `parks 1 drift 0` | `parks 1 drift 0` |
| fake_match_scan | 0 leads | 0 leads |
| real file modified | — | **no** (`git diff --stat` empty) |

Nothing landed. Nothing was proposed for the orchestrator. The single non-exact row
`_hud_render_unit_interface` is PARKED and was investigated in scratch only.

## `_hud_render_unit_interface` — NOT-LANDED, park premise reclassified

Target 3504 padded / 3493 meaningful / 102 relocs / sha16 `b82d575e1154cad1`.
Park base (real file) 3472 / 102, gate tag `[size 3472!=3504, sha]`, struct-diff **127 blocks / 308 insns**.
Frontier body `scratch/workers/hud_unit_messaging/hud_unit_candidate.c` (identical to
`scratch/workers/w3c_hud_unit/w1.c`), re-gated at this HEAD: **3504 / 102, tag `[sha]` only**,
struct-diff **41 blocks / 56 insns**, all 21 sibling rows still EXACT.
**The `[size]` half of the park tag therefore describes the stale park BASE, not the frontier; it is
not evidence of a structural source gap.**

### Census performed (method mandated by the wave brief)

1. **Relocation census by target** (`relocdiff --count-by-target --allow-structural`, w1 vs January):
   102 / 102, **every target row equal** across 38 distinct targets in identical order
   (`_local_player_count` 12, `_hud_draw_static_element` 8, `_fast_ftol_C` 6, `_game_time_get` 5, …).
   The only deltas are relocation ADDRESS shifts after the first code divergence (reloc 29,
   0x30b vs 0x30e) and the known `--source` literal-COMDAT naming artifact.
   ⇒ no missing call, no missing global, no missing/extra re-fetch ⇒ **no missing statement**.
2. **Frame census** (January `sub esp,0x424`, ours `sub esp,0x420`): aggregates deepest first —
   January `stack_buffer(512) unit_indices(72) unit_hud_indices(72) health_meter(104)
   absolute_placement(36) overcharge_meter(104) color(20)`; ours `stack_buffer(512) health_meter(104)
   unit_indices(72) unit_hud_indices(72) absolute_placement(36) overcharge_meter(104) color(20)`.
   Exactly two deltas: `health_meter` ranked 2nd instead of 4th, and January owns **one extra 4-byte
   scalar** (35 vs 34 dwords in −0x8c…−4), which is the entire frame gap.
   That scalar is a compiler staging cell, not a source local: at January `+0x55e fst dword [ebp-0x44]`
   ours emits `+0x573 fst dword [ebp-8]`, reusing the `draw_flags` home that January has spilled
   (`+0x3ac mov [ebp-8],ebx`, `+0x601`, `+0x630`) because it freed EBX for the `color[]` pointer IV.
3. **Local-set and type census — new evidence this wave.** `DIA2Dump.exe -sym hud_render_unit_interface`
   on the HCEX SHIP PDB (brief §10.5) lists `unit_count`, `player_index`, `long unit_indices[0x12]`,
   `long unit_hud_indices[0x12]`, `auxilary_flags`, `aux_activated_when_disabled_flags`,
   `float auxilary_values[0x1]`, static `long overcharge_count`, `overcharge_meter`, `health_meter`
   (both `struct meter_hud_element_definition`), `unsigned long color[0x5]`,
   `struct hud_absolute_placement_definition absolute_placement`, `union point2d corner`.
   **Our reconstruction matches 1:1 in names, counts and types**; no local is missing or invented
   (`stack_buffer`/`return_eip` are January's debug stack-guard pair, absent from the 2011 build).
   `short value_scale` is independently 16-bit signed on both sides (16-bit `test`, `movsx` before
   `fild`) and matches the field type at `unit_hud_interface_definition.h:119`.

Instruction totals are 1062 vs 1062 and every diff block is a register/slot substitution.
⇒ The census **refutes** every source-defect hypothesis the structural pass looks for (missing
statement, extra/missing local, wrong width, wrong control flow) and **confirms** the park's own
recorded verdict: register/x87 colouring plus one frame-rank decision.

### Probes (5, scratch-only, none landed, real file untouched)

| # | shape | measurement |
| --- | --- | --- |
| w1 | control: prior best body re-gated at this HEAD | 3504/102 `[sha]`, 41 blocks / 56 insns, frame 0x420 |
| v1 | `health_meter` declared one scope out (B13, the `health_dont_show` if) | 3520/102 (**+16 B worse**), frame 0x420, rank unchanged |
| d1 | diagnostic rename `health_meter`→`zz_meter` (never a landing; tests whether the rank is name-hash driven the way `.bss` order is) | 3504/102, identical 41/56, rank unchanged ⇒ **rank is NOT name-driven** |
| d2 | `health_meter` declared in B1 (outer if, after `auxilary_values`) | 3504/102; order becomes `ui, uhi, abs, och, health, color` — the index arrays reach January's ranks 2-3 but `health_meter` overshoots to rank 6; 42/58 |
| d3, d4 | `health_meter` declared in B6 (`hud_definition` block) / B5 (`while` body) | identical layout to d2; 41/56 and 42/58 |

**Law candidate (offered, not landed):** VC7's frame ranking of same-alignment aggregates is neither
refs/size (w3c v2), nor lexical depth by one level (v1), nor the local's name (d1); it flips between
"deepest" and "next-to-last" on whether the aggregate is declared inside the innermost if-chain that
uses it (d2/d3/d4), and no setting of that switch reproduces January's intermediate rank.

### Rejected without building (with reason)

- Hand pointer walk / alias over `color[]` to force the IV — R16 and laws_w2 A5 counter-law.
- `volatile`, barriers, operand permutations for x87 order, redundant stores — prohibited (brief §7);
  this residual is precisely the class such devices would fake.
- `if (body_vitality < max_cutoff)` — refuted by the bytes: January `+0x727 test ah,1` tests C0 alone
  (`!(a >= b)`); an ordered `<` compiles to `test ah,5`. Already ruled a semantic regression 20260904.
- `color[]`/`overcharge_meter` declared with initializers at the top of B9 — refuted by the bytes:
  January initialises `color` (`+0x46e`) after `_game_engine_running` (`+0x3e1`) and after the
  `overcharge_meter` copy (`+0x430`), so the nested block is January's own C89-forced shape.
- `health_meter` as a declaration initializer — impossible in C89 here, and January's `rep movsd`
  (`+0x70e`) follows the `value_scale` test (`+0x6eb`), which is our current form.
- A1 static linkage, A23/A24/A28/A40 detectors — do not fire (statics already static and covered by the
  2026-09-15 census; all helpers out-of-line on both sides; the only parameter has no packed temp).

### Reopen criterion (supersedes the w3c wording)

Reopen only when a source-level lever (a) stops VC7 keeping `hud_unit` in EBX across the shield
`draw_flags` chain — reproducing January's `+0x380 mov edx,[ebp-0x24]` rematerialisation, which alone
yields the `color[]` pointer IV, the `draw_flags` spill and the extra `fast_ftol_C` staging cell
(frame 0x424) — AND (b) ranks `health_meter` between `unit_hud_indices` and `absolute_placement`;
refs/size, lexical scope and the local's name are now all measured inert or overshooting. A C2
frame-allocator decode is the natural next evidence source. The park's own clause ("reopen only for new
authentic January source/type/compiler-context evidence, not spelling or allocation searches") keeps
this otherwise closed.

### Orchestrator options (zero credit, not proposed)

The park BASE could be re-measured to `scratch/workers/n3_hud_unit/w1.c` (3504 / 102 — January's exact
size and relocation count, 41 blocks / 56 insns versus the current base's 127 / 308), which would also
retire the misleading `[size 3472!=3504]` gate tag. This was already offered by waves w1 and w3c and not
taken; R11 applies (strictly-closer body that changes size, not tied to a reopen). No header, config or
`symbols.json` change is needed for this unit.
