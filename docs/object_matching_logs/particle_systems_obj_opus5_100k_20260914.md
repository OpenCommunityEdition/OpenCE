# `particle_systems.obj` Opus5 100K consolidated lane (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/effects/particle_systems.c`; target
  `build/split/source/effects/particle_systems.obj`
  (SHA-256 `a3a9effbae9943cf78d3dd16ca61cd6ceb1c6f57ebb9610e7bca6083c361b185`).
- Lane worktree `C:\halo-worktrees\opus5-100k-consolidated-20260914`, branch
  `opus/100k-consolidated-20260914` (worker waveB:particle_systems).
- Compiler: VC7 13.00.9254 with the unchanged repository flags. No source,
  header, config, symbols.json or tool file was modified; all experiments are
  scratch candidates gated with `gate.py --source`.
- Evidence: January COFF bytes/relocations/.rdata (authority); HCEA
  reconstructions `halocea-review/src/blam/effects/particle_system_update.c` and
  `particle_system_new_particles.c` (topology/semantics only); Stian lift
  `_research-stian-halo/src/halo/effects/particle_systems.c` FUN_000a0180;
  `HCEX_Release.pdb` (SHA-256 `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`) via DIA2Dump for private data
  names; January symbol atlas (no data names for this object). The lane's
  prepared Ghidra/atlas extracts do not exist for this object.
- Prior ledgers read: opus5_house_clean_20260913, render_owner_safe_fuzzy_20260912,
  hcea_january_semantic_packet_20260908, jonas_public_update_20260821,
  natural_init_owner_schema_20260906, fable_five_claim_intake_20260906 (full);
  remaining particle/weather/effects ledgers searched for these functions.

## 2. Validated baseline

Real-file gate (unchanged at finish): `== exact 20  residual 5  unwritten 0`,
point guard passed, fake scan 0 leads, no `config/parked.json` entry for the unit.

| residual | target padded/meaningful/relocs/sha16 | baseline candidate |
|---|---|---|
| `_particle_system_new_particle_jet` | 368/365/11/`322ad2bf7cdffa79` | 368/11/`ad590a196990edc3` |
| `_particle_system_update_particle_default` | 400/392/12/`f8da0780e495f488` | 400/12/`e3611bbaac37349f` |
| `_particle_system_new_particles` | 848/843/35/`42b053f34b78452c` | 848/35/`c29055f1631f8ebe` |
| `_particle_system_render` | 1360/1346/33/`c4d841f9803c7f3e` | 1344/35/`eb89022c439f93d2` |
| `_particle_system_update` | 1664/1651/58/`0a74337d69d2dfdf` | 1616/58/`1dd732b69d65b2d6` |

Data: January's 32-byte `.rdata` aggregate `_rdata_0025a6b8` (public
`_ground_error` at +0x18) is not owned by the current candidate (it emits a
24-byte table section and uses the global `__real@3d088889`).

## 3. Accepted controls

None. No function became strict EXACT; the real file is byte-identical to the
lane baseline (`git diff --stat` empty).

## 4. Experiment matrix (scratch only)

Files under `scratch/workers/particle_systems/`.

| function | shape | result | disposition |
|---|---|---|---|
| update | v1: frame-driven rewrite (inline `real_local_random_range` per call site, `state_length` store before `+=`, `< 0.0f` time tests with the variables block as the else-arm + `break`, short particle handle, `particle->state_index = 0` before the first-state lookup, struct assignment for the randomized-variable copy, extra state lookup in the alive transition arm, January assert text and lines 0x2E1/0x3AF, `system_definition` local) | 1680 [sha] | reverted |
| update | v2: + direct `type->transition_state_index` reads, randomize as then-arm | 1680, particle half aligned | reverted |
| update | v3a index loop without pointer locals | 1664 [sha] | reverted |
| update | v4a `state_variables` pointer at else-arm top (hoisted `add ebx,0x34` matched) | 1664 [sha] | reverted |
| update | v5a/v5b/v6a/v6b/v9a/v11/v13a pointer/declaration/index-width probes | inert or worse | reverted |
| update | v7b pointer-increment loop, destination assigned after the clamp | 1664/58 [sha]; only the loop induction-variable base differs (+ table naming) | best, not landed |
| update | v8a/v8b operand order, v10 increment order x3, v12a size_t bound, v12b long index | inert / unrolled | reverted |
| new_particles | np1 store-first initial count | ABI change, worse | reverted |
| new_particles | np4 ternary count | inert | reverted |
| new_particles | np5 `state_definition = initializing ? NULL : element(...)` at declaration (HCEA) | block layout matched | kept in chain |
| new_particles | np7 `marker_index` local before dispatch (HCEA) | dispatch sequence matched | kept in chain |
| new_particles | np8 `type->particle_count++` before the link stores | store order matched | kept in chain |
| new_particles | np12 `target = count + whole` before the fractional update (HCEA) | 848/35 [sha]; only the rotation `fstp` flush point differs (+ table naming) | best, not landed |
| new_particles | np9/np11/np13 inline `real_local_random_range` rotation; np14 sprite store first; diag1/diag2 assert removal (diagnostic) | inert / worse | reverted |
| update_particle_default | upd1 named `inverse_t`; upd2/upd3 operand order | inert | reverted |
| jet | j1 replay of the 0913 data shape with the PDB-authenticated `seconds_per_tick` | only inlined cross_product3d operand-role diffs | not landed |
| render | r10 replay of 0913 v10 | 1360/33 [sha] | reverted |
| render | r11 no `types` local (`&definition->types`) | register swap becomes pure | kept in chain |
| render | r12 ternary sequence index | int-promoted increment | reverted |
| render | r13 if/else sequence index assignment | 1360/33 [sha]; ebx/edi + temp register choices only | best, not landed |
| all | combo1 = update v7b + new_particles np12 + render r13 + jet data shape + PDB table names | 20 exact kept; all five residuals size/reloc equal, [sha] | scratch only |
| all | scratch csplit (`build/tools/csplit.exe -p <scratch config>`) with the proposed symbols.json names, compared against combo1 | relocation identities and the 32-byte data aggregate (bytes and owners) equal; only the three ties remain | measurement only |

## 5. Do-not-repeat

- update loop: operand order, increment order, index width (long unrolls),
  size_t bound, pointer declaration placement (7 probes) - IV base choice did not
  move.
- new_particles rotation store: inline wrapper, store order, assert removal.
- update_particle_default: named `inverse_t`, commutative operand text (plus 0913
  and Fable probes).
- jet: commutative operand spellings in `cross_product3d` (0913 + breakable_surfaces).
- render: `types` local vs inline block, ternary sequence index.

## 6. Residual classification

- `update`: measured - strength-reduced loop IV base (January: current-state
  pointer `ebx`, counter `edx`; ours: transition pointer `ecx`, counter `ebx`);
  ownership - table relocations `_rdata_0025a6b8+0/+0x14`.
- `new_particles`: measured - one deferred x87 store flush; ownership -
  `_rdata_0025a6b8+8`.
- `jet`: measured - operand roles in two inlined `cross_product3d`; ownership -
  `_ground_error+4` resolves once the file-scope constants exist.
- `update_particle_default`: measured - 2-byte lerp x87 schedule.
- `render`: measured - callee-saved register swap (types pointer vs
  type/state definition) and dependent temp choices. Behaviour: the retained
  real-file body resets the transition weights on shader INcompatibility, the
  inverse of January (recorded 0913; corrected in `render_r13.c`).

## 7. ORCHESTRATOR-PROPOSAL (config/symbols.json)

Edit lines in place (do not re-serialise):

```
replace: { "file_offset": 2467512, "flags": 0, "name": "_rdata_0025a6b8" },
with:    { "file_offset": 2467512, "flags": 0, "name": "_system_update_functions", "static": true },
         { "file_offset": 2467520, "flags": 0, "name": "_particle_creation_functions", "static": true },
         { "file_offset": 2467532, "flags": 0, "name": "_particle_update_functions", "static": true },
keep:    { "file_offset": 2467536, "flags": 0, "name": "_ground_error" },
add:     { "file_offset": 2467540, "flags": 0, "name": "_seconds_per_tick", "static": true },
```

Evidence: `HCEX_Release.pdb` DIA2Dump `-ps 0x11D4E0` lists, in the same order and
spacing, `system_update_functions` (File Static, function* const[2]),
`particle_creation_functions` (const[3]), `particle_update_functions` (const[1]),
`ground_error` (Global const float) and `seconds_per_tick` (File Static const
float); January relocations update+0xff (+0), new_particles+0x2c7 (+8),
update+0x617 (+0x14), jet (`_ground_error+4`). No other January object references
these symbols. Consumers: particle_systems only (the accompanying source change
renames the three C tables and adds the two file-scope constants in
`particle_systems.c`). Measured with a scratch csplit: no relocation or data
owner difference remains for combo1. Adopting the names alone closes no function
(all three consumers still carry a backend tie).

## 8. Reopen criteria

- update: an IV-base lever for VC7 3-pointer strength reduction -> land
  `update_v7b.c` with the names.
- new_particles: a deferred x87 store-flush lever -> land `newp_v12.c` with the names.
- jet: an operand-role lever for inlined `cross_product3d` -> land the
  ground_error/seconds_per_tick shape with the names.
- update_particle_default: a lever that pre-pushes a single-use `(1-t)`.
- render: a register-allocation lever for `&definition->types` (EBX) vs
  `type_definition` (EDI) -> land `render_r13.c` (includes the polarity fix).

## 9. Disposition

Still active, not Matching: 20 / 25 functions strict exact, unchanged. No new
exact credit; no data credit. Final object
`scratch/workers/particle_systems/final.obj`
(SHA-256 `247b2aa5aff38903be4a2d062db01d4de559c18fca7c61d13a35e4ab8bc4c26d`);
source SHA-256 `4ad25437e1c5e9d417c4776db7c9d57e10f28c99fb5f8d0fbb4dad75140a40ea`
(unchanged). Worker log: `scratch/workers/particle_systems.md`.
