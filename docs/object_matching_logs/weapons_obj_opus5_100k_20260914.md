# weapons.obj — opus5 100K consolidated lane (2026-09-14)

## Disposition

- Landed in `source/items/weapons.c`: **`_weapon_aim` strict EXACT** (272 padded / 257 meaningful, 14 relocations,
  normalized SHA-256 prefix `ee62f54c1d38995c`). Real-file gate `== exact 50 residual 3 unwritten 26` (baseline
  `== exact 49 residual 3 unwritten 27`), `_point_from_line3d` guard passed, no baseline-exact loss, park unchanged.
- Everything else is **not landed**: the whole trigger graph under `weapon_update` needs four owner-header declarations
  plus three runtime flag names that workers may not add (ORCHESTRATOR-PROPOSAL A), and `trigger_create_projectiles`
  additionally needs the genuine inline `point_from_line3d` that the lane guard forbids in this object
  (ORCHESTRATOR-PROPOSAL B). A complete measured candidate is prepared for the orchestrator:
  scratch-built weapons.obj gates **72 exact / 5 residual / 2 unwritten** against the proposed headers.
- Object remains `NonMatching`.

## 1. Scope and provenance

- Translation unit `source/items/weapons.c`; target `build/split/source/items/weapons.obj`.
- Compiler: XDK VC7 CL 13.00.9254, unit flags `/O2 /Oy- /DDEBUG /Dxbox` with the repository include paths.
- Names and topology: January map atlas `scratch/atlas/weapons.obj.txt` (all trigger-graph statics exact-tier,
  including the mangled `?projectile_distribute@@YAXPATreal_vector3d@@0FMFF@Z`), January PDB public names,
  January relocation call graph, Ghidra decompilation of the split object, HCEA per-function reconstructions
  (`halocea-review/src/blam/items/*.c`, semantics and DB enum names only).

## 2. Validated baseline (lane HEAD b8cd50113)

`== exact 49 residual 3 unwritten 27 (of 79)`. Residuals: `_weapon_can_be_fired` (parked, sha),
`_weapon_magazine_finish_reload` (224/6, sha), `_weapon_update` (64/2 stub vs 2800/97).
Nineteen of the unwritten rows already had bodies in the file (`weapon_trigger_idle`, `weapon_trigger_change_state`,
`weapon_magazine_start_chamber`, ...) but were culled because no emitted caller references them.

## 3. Accepted controls

- `weapon_aim`: first-natural Fable body (history blob 45d79151); prior hold was only the missing `weapons.h`
  declaration, satisfied by header commit 12e295220. New selection-2 COMDATs `_valid_real_normal3d`,
  `_valid_realcmp`, `_magnitude_squared3d` and two assert strings are the systemic header-inline COMDAT property
  (January references them undefined; present in 31/32/89 base objects).

## 4. Experiment matrix (candidate only; scratch/workers/weapons/)

| Function | Target | Shapes (result) | Best |
| --- | --- | --- | --- |
| projectile_distribute | 160/3 | HCEA ternary 144; in-place short step 160 sha; + one-case `switch (distribution_function)` | EXACT |
| weapon_trigger_overcharged | 128/9 | first natural | EXACT |
| weapon_trigger_release_charge | 256/14 | first natural | EXACT |
| weapon_trigger_overload | 208/12 | short trigger_index (sha); `long trigger_index` (HCEA DB prototype, eax ABI) | EXACT |
| weapon_trigger_begin_firing | 624/29 | force-first 512/22; `!force &&` 496/21; sequential `if (underwater) can_fire= FALSE; if (can_fire)` | EXACT |
| weapon_trigger_fire | 1984/62 | 2000 first; age `!(flag && age>=1)`; single message call; decl order; `epicenter= center; origin= epicenter` | 1984/62 sha, sim 0.964 |
| trigger_create_projectiles | 1824/41 | explicit component math 1792; offset locals 1808 (0.780); + store order L1 1808 (0.802); point_from_line3d form 1824/41 (0.9947, guard-forbidden) | 1808 landable form |
| weapon_update | 2800/97 | v5 2848 (0.640); fractional-first + `state_timer-1<=0` + January case order 2832 (0.661); single idiv via `short rounds_recharged` 2816 (0.672) | 2816/97 fuzzy |
| weapon_magazine_finish_reload | 224/6 | short rounds_to_load; `-=` form; MIN() — all sha | unchanged |

Candidate-exact once callers exist (bodies unchanged): weapon_detonate, weapon_effect_looping_new,
weapon_get_projectile_owner_object_index, weapon_magazine_finish_chamber, weapon_magazine_idle,
weapon_magazine_start_chamber, weapon_magazine_state_interruptable, weapon_state_key_frame,
weapon_trigger_can_fire_again, weapon_trigger_change_state, weapon_trigger_finish_tracking,
weapon_trigger_fully_charged, weapon_trigger_idle, weapon_trigger_locked, weapon_trigger_recover,
weapon_trigger_start_ejection_port, `_random` (header-inline COMDAT from `random()`).
Total candidate gain beyond weapon_aim: 22 functions / 3,280 padded bytes.

## 5. Do-not-repeat list

- weapon_trigger_fire effect-selection block: nested fired-first, ternary heat error, reordered branch stores
  (T3–T7) do not stop VC7 moving the fired/misfired blocks and tail-merging `effect_error= 0.0f`.
- begin_firing: the `!underwater && can_fire` condition blocks VC7 inlining `weapon_trigger_change_state`
  (diagnostic: removing only the underwater call restores inlining; removing the magazine block does not).
- trigger_create_projectiles: moving `use_aiming_vector= TRUE` after `adjust_origin` reshuffles frame slots (0.88).
- weapon_magazine_finish_reload: short/MIN/compound-subtract spellings exhausted.
- Header blast: declaration position inside units.h is irrelevant (begin/end of the UNITS.C section identical).
- Never land weapon_place's long-return form; never emit `_point_from_line3d` here without policy approval.

## 6. Residual classification

- weapon_trigger_fire: block placement / tail-merge scheduling (size and relocations exact).
- trigger_create_projectiles: source/topology — January used inline `point_from_line3d` (frame proof); remaining
  two items in that form are a store-position/slot tie and else-block placement.
- weapon_update: register/slot homes of the loop counters and switch-join placement (size 2816 vs 2800).
- weapon_magazine_finish_reload: 16-bit add scheduling tie.
- weapon_can_be_fired: parked, untouched.

## 7. Blast radius of proposal A (measured, not applied)

All 558 buildable units compiled against control and proposal header mirrors: no exact function changes status
anywhere. Normalized changes: `ai_communication:_ai_communication_update_speech_timers` (parked; actors.h or
units.h), `hud_weapon:_render_weapon_hud` (aim_assist.h or units.h), `physics:_physics_update_old` (units.h; gone with
unnamed parameters). `particle_systems:_particle_system_update` differs even with no header edit (probe artifact).
projectiles.h and weapon_datum_flags.h edits are clean. The perturbation is insensitive to position and absent for a
struct forward declaration — consistent with the declaration-count oracle.

## 8. Reopen criteria

- Proposal A accepted (with the ai_communication park re-baselined or an owner arrangement that avoids it): land
  `scratch/workers/weapons/candidate_full_proposalA.c` body (+22 exact, fuzzy weapon_update/trigger_fire/
  trigger_create_projectiles).
- Proposal B accepted: use the point_from_line3d form of trigger_create_projectiles
  (`scratch/workers/weapons/candidate_full_proposalB_scratchprotos.c`).
- weapon_place: a genuine scenario weapon record owner header.
- animation_update: an authentic caller or return-type evidence (unchanged).

## Header packet (orchestrator, checkpoint 3)

Proposal A landed as a separate header commit:
- `actor_firing_blindly` in actors.h
- `player_aim_projectile` in aim_assist.h
- `projectile_kill_tracer` in projectiles.h
- `unit_adjust_projectile_ray` in units.h
- the remaining weapon datum flag bits in weapon_datum_flags.h

The first full build regressed `units::_unit_preprocess_node_orientations` (declaration-count oracle). Bisection showed that either the units.h or the aim_assist.h declaration alone is harmless, but both together are not. The same commit removes the verbatim duplicate `collision_surface_polygon` prototype at the end of `collision_bsp.h`, a genuine cleanup since the owner declaration stays in the COLLISION_BSP.C prototype block. Full build: +22 exact weapons functions (3,280 padded), 0 regressions, parks 465/0/0. Proposal B (`_point_from_line3d` emission) is declined under the lane rule, so `trigger_create_projectiles`, `weapon_trigger_fire` and `weapon_update` stay fuzzy at zero credit.
