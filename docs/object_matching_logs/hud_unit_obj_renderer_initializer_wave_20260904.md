# `hud_unit.obj` renderer/initializer reconstruction — 2026-09-04

## Scope and disposition

This packet reconstructs the last two previously unwritten owners in
`source/interface/hud_unit.c`:

- private `initialize_hud_state`, January `_code_000c65c0`; and
- public `hud_render_unit_interface`.

The PID-isolated focused gate advances the unit from **20 exact / zero
residual / two unwritten** functions to **21 exact / one residual / zero
unwritten**. All 20 inherited exact functions remain strict exact. The private
initializer is newly strict exact. The 3,504-byte renderer is a complete,
natural, evidence-backed **fuzzy reconstruction**, not an exact function and
not a whole-object `Matching` claim.

January Xbox COFF is the sole byte, relocation, and ABI authority. The later
HCEA source and PDB are used only to recover semantic names, types, layouts,
and behavior. The orchestrator owns target metadata, the full Ninja rebuild,
stable regression comparison, park validation, and publication.

## Source and type authority

The primary behavioral corroboration was:

`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research_cache\halocea\src\blam\interface\hud_render_unit_interface.c`

The closer PC disassembly corroboration was the `FUN_000d7d40` body in:

`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research-stian-halo\src\halo\interface\hud_messaging.c`

The type authority was:

`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb`

queried with the workspace `research/tools/DIA2Dump/x64/Release/DIA2Dump.exe`.
The preceding owner/API packet is documented in
`docs/object_matching_logs/hud_renderer_owner_api_preparation_20260904.md`;
its full header blast retained all 6,183 strict owners and all 251 existing
parks before this body was inserted. The review proposal is
`scratch/hud-renderer-owner-proposal-20260904.md`.

The adopted owners provide the complete authentic auxiliary overlay and meter
layouts, named HUD flags, typed draw calls, the real motion-sensor call, and
the one-argument Game Engine motion-sensor policy call. No caller-local public
prototype, guessed raw offset, or opaque byte prefix appears in the retained
source. Unit/object and tag accesses use the existing typed subsystem macros.

DIA independently authenticates these renderer locals:

- `long unit_indices[18]` and `long unit_hud_indices[18]`;
- `real auxilary_values[1]`;
- `pixel32 color[5]`;
- `unsigned long auxilary_flags` and
  `aux_activated_when_disabled_flags`; and
- static local `long overcharge_count`.

The private helper name is a semantic reconstruction from its only caller and
complete body: it initializes every HUD-state sentinel needed before the
renderer consumes that state. It is deliberately not retained as an
address-derived `code_...` identifier. Root owns the corresponding target
symbol mapping.

## Recovered behavior

The renderer now expresses the complete January control flow:

- validate that the supplied player's local-player index is the active render
  index, then use typed player, unit, unit-definition and HUD-definition
  accessors;
- initialize the two 18-entry layer arrays, add a valid parent driver HUD and
  compatible child-seat HUD layers, and render the layers in reverse order;
- derive the integrated-light auxiliary state from the named unit and control
  flags;
- draw the background, shield/overcharge meter, health meter, motion-sensor
  panel, auxiliary overlays and auxiliary meters through their owning APIs;
- preserve scripted HUD suppression/blink flags, split-screen flags, dead and
  disabled states, flash timers, shield fade history, and team-color overlay
  behavior; and
- retain the January return-address/stack-buffer checks and explicit terminal
  `return;`.

Later-build additions without January calls or relocations were excluded,
including HCEA team-icon/change-color rendering and PC-only HUD controls.
The otherwise unusual discarded `game_engine_running()` result is present in
January at renderer `+0x3E0` and independently survives both later
disassemblies; it was not invented to shape code generation.

The motion-sensor placement intentionally initializes only its genuine
`corner` member. January performs that same single store. No fake zero-fill of
the remaining definition was added.

## Bounded semantic corrections

Only target-authenticated source corrections followed the natural first shot:

1. The two layer arrays use declaration initializers with their genuine first
   values. January stores element zero and emits `rep stosd` for exactly the
   remaining 17 dwords in each array. This natural form removes two wrapper
   calls; it is initialization semantics, not a declaration-order experiment.
2. Shield and auxiliary byte conversion uses the project's authentic
   `fast_ftol` helper. January emits inline `fld`/`fistp` sequences in those
   paths. Health conversion deliberately remains `fast_ftol_C`: the two
   source expressions pass through the existing `PIN` macro and produce the
   exact six ordered January `_fast_ftol_C` relocations.
3. `MAX(last_shield_vitality, shield_vitality)` preserves January's selected
   operand for unordered values and signed zero.
4. The health max-cutoff arm is
   `!(body_vitality >= max_cutoff)`, preserving January's less-or-unordered
   behavior rather than a superficially similar `<` test.
5. Auxiliary flashing uses ordered `value <= min_cutoff`; equality flashes,
   while unordered values do not.
6. The assertion expression is spelled exactly as January,
   `player->local_player_index==render.local_player_index`. This restores the
   target's existing 54-byte decorated literal identity instead of creating a
   distinct 56-byte spaced spelling.

No register/volatile/noinline/force-inline steering, optimizer pragma,
barrier, dummy branch, self-assignment, fake dependency, invented lifetime,
representation pun, undefined behavior, or manual assembly expansion was
used. `tools/fake_match_scan.py source/interface/hud_unit.c` reports zero
review leads, and the scoped `git diff --check` is clean.

## Focused COFF evidence

| Owner | Measure | January target | Frozen candidate |
| --- | --- | ---: | ---: |
| `initialize_hud_state` / `_code_000c65c0` | meaningful / padded | 51 / 64 | 51 / 64 |
| | relocations | 1 | 1 |
| | normalized SHA-256 | `117b8363f3f64f6c4d6d19fd8264e389aa8479f00f1a7f588213d802eb430bca` | `117b8363f3f64f6c4d6d19fd8264e389aa8479f00f1a7f588213d802eb430bca` |
| `hud_render_unit_interface` | non-padding / padded | 3,493 / 3,504 | 3,460 / 3,472 |
| | relocations | 102 | 102 |
| | normalized SHA-256 | `b82d575e1154cad1dd27cee874e9af5cc4b86dc302ea41685b56181ba32d93bc` | `4071a4f0c7b1f808423bd5673e822e7f3c6a036e9a3a2db0fe7bd14b413b1817` |

The non-padding measure mechanically retains the small post-return SEH thunk
and strips only terminal `NOP`/`INT3` padding. The primary return is at target
`+0xDA0` and candidate `+0xD7F`. Equal relocation count does not imply an
exact match; instruction selection and x87 scheduling still differ, so this
renderer is parked rather than tuned.

Baseline artifact:

`scratch/hud-unit-renderer-baseline-20260904.obj`

Raw SHA-256:
`5dd945542fb8204395f3c1775653e89b39d21803f586ffb15bfb66ba0f8a8013`

Frozen artifact:

`scratch/hud-unit-renderer-frozen-final-20260904.obj`

Raw SHA-256:
`35c94a866e13c7154f30cf23cf8701355fff7b6a6343f10f98bc54f9e570bfb2`

Frozen `source/interface/hud_unit.c` SHA-256:

`cc98970ee2318245d01706a1fd6941b4207aab62a3ea1e2293879a8738d260eb`

The final focused command was:

```text
python tools/campaign/gate.py source/interface/hud_unit --alias initialize_hud_state=code_000c65c0 --all --forbid-emitted-symbol _point_from_line3d --out scratch/hud-unit-renderer-frozen-final-20260904.obj
```

It reports 21 exact, one residual, zero unwritten, and the exact emitted-symbol
guard passes.

## Complete new-owner census

Compared with the pre-renderer baseline, the candidate adds the two intended
functions and one compiler-emitted code COMDAT:

- `_fast_ftol`, 32 bytes, zero relocations, normalized SHA-256
  `8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248`.

This is not an invented helper and receives no separate progress credit. Its
body is byte-identical to January's selected
`actor_combat.obj::_fast_ftol`, and the current compiler emits the same body
from ordinary helper use in the base `player_effects.obj`,
`periodic_functions.obj`, and `cinematics.obj`. That proves source provenance,
but **not** compatible linked ownership. The HUD candidate section has flags
`0x60501020` and COMDAT selection 2 (`SELECT_ANY`); January's sole selected
`actor_combat.obj` owner has the same body and flags but selection 1
(`SELECT_NODUPLICATES`).

This boundary is known to be incompatible under the ordinary XDK linker, not
merely untested. `player_rumble_obj_jonas_symbol_backed_fast_ftol_closeout_20260831.md`
records XDK 3911 `Link.Exe /dll /noentry /nodefaultlib /force:unresolved` in
both input orders: a rebuilt select-any `_fast_ftol` plus January's
no-duplicates owner fails with `LNK2005` and then `LNK1169`.
`bitmap_utilities_obj_jonas_vector_map_quartet_20260831.md` independently
records the same multi-owner failure. The unchanged body is therefore useful
provenance only; it does not authorize whole-object admission or an assumption
that the linker will discard this extra owner.

New non-code material relative to baseline is:

- static-local `overcharge_count`, `.data` size four, bytes `04 00 00 00`,
  with two renderer relocations. January owns the identical four bytes as
  `_data_002e4c68`; root maps the semantic local-static identity;
- the ordinary `0.2f` constant COMDAT, four bytes, whose decorated symbol is
  referenced by January; and
- the exact 54-byte assertion literal whose decorated symbol is referenced by
  January and selected elsewhere by csplit.

There is no new COMMON, `.bss`, point/line helper, or other candidate-only
code/data owner. The existing four-byte `unit_hud_globals` BSS owner is
unchanged.

## Honest park and integration boundary

The proposed park measurement for `hud_render_unit_interface` is target
3,504 bytes / 102 relocations / hash `b82d575e...` versus candidate 3,472 bytes
/ 102 relocations / hash `4071a4f0...`. The source is complete and coherent;
the remaining 32 padded bytes are ordinary compiler instruction/x87 schedule
differences. No declaration-order, spelling, register, or assembly lottery is
warranted under the campaign rules. This worker did not edit
`config/parked.json`.

Strict progress credit is only the initializer's 51 meaningful bytes. The
renderer supplies 3,460 meaningful bytes of source reconstruction but zero
strict exact-byte credit. Canonical admission remains conditional on root's
fresh target regeneration, full build, stable all-owner sweep, park check,
policy scans, tests, and resolution of the currently incompatible `_fast_ftol`
owner boundary. The natural fuzzy renderer and exact initializer may be
preserved, but this object must not be marked whole-object `Matching` while
that extra owner remains.
