# Opus5 PDB/IDA lane — handoff (2026-09-14)

## Verdict

The lane verifies **+5 strict exact functions / +1,297 meaningful / +1,328 padded
code bytes** against canonical `c3e257e9a`, with **zero regressions**. The
15,000-byte objective was **not met**: the shortfall is 13,703 meaningful bytes.

The new IDA/PDB evidence authenticated names, but it contained no Halo types, and
the five portfolio objects are dominated by ownership blockers rather than
unwritten-but-writable code.

| total | value |
| --- | ---: |
| `verified_new_exact_meaningful` | **1,297** |
| `verified_new_exact_padded` | **1,328** |
| `fuzzy_meaningful_target_bytes_zero_credit` (5 retained bodies) | **11,051** |

The accepted ledger moves from 6,972 / 1,140,161 to **6,977 / 1,141,458**.

The machine-readable record is `opus5_pdb_ida_admission_manifest_20260914.json`,
with per-function section index, hashes, ordered relocations, provenance, ABI
owner, census, guard, review and commit.

## Identity and baseline proofs

- Worktree `C:\halo-worktrees\opus5-pdb-ida-15k-20260914`, branch
  `opus/pdb-ida-15k-20260914`, created from `c3e257e9ab9a5d7e1c9b39862f87ac2abde4359c`
  by hash. The canonical checkout was never modified, and nothing was pushed.
- Baseline reproduced exactly: 6,972 accepted functions / 1,140,161 meaningful
  bytes, 465 parks, and 327 open unparked functions (215 residual + 112 unwritten).
- Frozen stable snapshot `scratch/before.json`: 8,245 / 6,937 exact, SHA-256
  `39557d49a2652d18d1286e119e861983e71a0e7e0e86a80592a3f58bca69e41d`.

## Evidence used, and its limits

- **January PDB** (`cachebeta.pdb` / `cachebeta.exe`, SHA-256 `4cc87b45…`,
  matching the target). `tools/pdb200_extract.py` regenerated
  `scratch/pdb200-current` with 18,674 symbols and 1,836 types. Its symbol `rva`
  equals the `symbols.json` file offset. It authenticates **public** names only:
  every type is XDK/NT/D3D, none of its 170 enums has enumerators, and private
  module streams (e.g. projectiles) are stripped.
- **IDA Free 9.4**, using a private copy of `cachebeta-pal-january.i64` in
  `scratch/ida`. IDA Free rejects `-S` and has no IDAPython or `idat`, so nothing
  could be exported headlessly. Its names and type library derive from the same
  PDB, so the PDB export was used as the machine-readable equivalent. No supplied
  binary was executed and nothing was uploaded.
- **Ghidra 12.1.2** headless decompilation of the five January split objects
  (`scratch/ghidra/out`).
- **January debug map atlas** (exact tier). It supplied the private names
  `projectile_detonate` and `projectile_collision`, plus six actor_combat private
  routines.
- **HCEA, the 2020 H1 Tags build and Marathon** were used only for names, semantics
  and topology.

## Ordered commits (base..tip)

| commit | kind | content | new exact (padded) |
| --- | --- | --- | ---: |
| `2364150f9` | config | projectile private function names (5) + `seconds_per_tick`, `projectile_update_section`, `effect_marker_names` (static) | 0 |
| `3be9c1703` | source | stack_walk_windows: `symbol_sort_proc` exact via the genuine loader; `load_symbol_table` fuzzy | 48 |
| `7b6bbfbb7` | source | projectiles: `set_action`, `effect_new`, `detonate` exact; `update`, `collision`, `collision_test_line` fuzzy | 1,168 |
| `1086dd183` | source | active_camouflage: `real_alpha_to_pixel32` exact (admitted inline emitted by its genuine caller); `draw` fuzzy | 112 |
| `022df460b` | docs | actor_combat owner blockers; interface stall-enum negative result | 0 |
| (this commit) | docs | handoff + manifest | 0 |

**Changed files:**
- `config/symbols.json`
- `source/cseries/stack_walk_windows.c`
- `source/items/projectiles.c`
- `source/rasterizer/xbox/rasterizer_xbox_active_camouflage.c`
- five `*_opus5_pdb_ida_20260914.md` ledgers, this handoff and the manifest

**No header was changed**, and nothing on the no-overlap list was touched.

## Per-target outcomes

1. **interface.obj:** bounded search complete. No stall enum exists in the PDB,
   IDA or Ghidra evidence. Both functions stay blocked with zero credit, and
   `interface.c` is untouched.
2. **projectiles.obj (principal):**
   - 21 → **24 exact**.
   - Private identities were established by atlas records (`detonate`,
     `collision`) and by January caller graph, register ABI, relocations and
     HCEA bodies (`set_action`, `effect_new`, `collision_test_line`).
   - The TU-private runtime representation is preserved, and no public
     `projectile_datum` field or header changed.
   - Fuzzy: `update` and `collision` are allocator/x87 ties at identical size and
     relocations. `collision_test_line` cannot use January's inline
     `point_from_line3d` expansion without emitting the forbidden COMDAT.
3. **rasterizer_xbox_active_camouflage.obj:** 10 → **11 exact**. `draw` is fuzzy
   with three store-order ties at identical size, frame and relocations. No
   `/QIfist`, and no assembly added.
4. **stack_walk_windows.obj:** 8 → **9 exact**.
   - The PDB has no signature; the `int` signature comes from HCEX/HCEA evidence
     in the 2026-09-04 ledger.
   - A `cseries_windows.h` declaration regressed
     `game_engine::_populate_statistic_buffer`, so it was reverted.
   - The wrong TU-local `void` prototype was removed instead: the loader is
     defined before its only same-TU caller and keeps external linkage.
5. **actor_combat.obj:** blocked, no source change.
   - Seven small private owners are diagnostic-exact (928 padded), but they are
     only emitted through `actor_combat_update`.
   - `actor_combat_update` requires `weapon_aim` / `weapon_estimate_time_to_target`
     declarations in `weapons.h` (weapons family reserved) and out-of-line
     `real_math` declarations (`real_math.h` reserved).
   - The proposals, including the six atlas renames, are in the actor_combat ledger.

## Verification (tip before this docs commit)

| gate | result |
| --- | --- |
| configure + `ninja all_source progress semantic_progress` | pass. The first attempt failed on the `cseries_windows.h` trial (1 regression); after the revert it passes. |
| stable diff base → final | 5 gained / 1,328 padded / **0 regressions** |
| parked functions | 465 active / 0 stale / 0 invalid |
| object admission | 0 candidates / 0 contradictions / 7 rejections |
| pytest tools | 1,151 passed / 5 skipped / 26 subtests |
| `git diff --check` | pass |
| fake-match scan | no new findings (stack_walk keeps its two inherited EBP/ESP captures) |
| `_point_from_line3d` guard | pass on all changed units |

## Owner census disclosures

projectiles gains:
- header-inline SELECT_ANY copies. January references `_cross_product3d`,
  `_distance3d`, `_valid_real_point3d` and `_valid_real_vector3d_axes2` as
  undefined; the others are unreferenced;
- `xtl.h` select-any D3D data through `physics/collision_usage.h`;
- one candidate-only double literal used by a fuzzy body.

This is the systemic class already present in canonical base objects. No new
non-inline code, BSS or COMMON owner was added.

## Rejected trials (all reverted or replaced)

| trial | why rejected | status |
| --- | --- | --- |
| `cseries_windows.h` `load_symbol_table` declaration | regressed `_populate_statistic_buffer` | reverted |
| raw `display_assert` / `system_exit` in the new `projectile_collision` default | house rule 23 | replaced by byte-identical `match_vassert(file, 1487, FALSE, NULL)` |
| actor_combat graph | reserved `weapons.h` / `real_math.h` ownership | not landed |
| interface stall enum | no evidence | not landed |

## Owner-actionable unlocks

1. **actor_combat:** when the weapons and real_math owners are free, add the
   `weapon_aim` / `weapon_estimate_time_to_target` prototypes to `weapons.h` (a
   scratch simulation across 19 consumers changed 0 rows), rule on out-of-line
   `real_math` declarations, then apply the six atlas renames. This would unlock
   about 928 padded bytes of diagnostic-exact helpers, plus `update`/`start_burst`
   near-ties.
2. **`_point_from_line3d` policy** for `projectile_collision_test_line` (and weather
   `update_physics` in the other lane).
3. **stack_walk:** a `cseries_windows.h` declaration once its consumers tolerate the
   declaration count.

## Overlap statement

- The parallel lane `opus/30k-fresh-graphs-20260914` (same base; now complete)
  changed `symbols.json` on different lines. It did not touch projectiles,
  actor_combat, active_camouflage, stack_walk_windows or interface.
- This lane's `symbols.json` renames were applied only after that lane finished.
- Both lanes' `symbols.json` edits are in-place line edits on disjoint entries and
  should merge textually.
- Before integration, re-check canonical for new commits touching this lane's
  three source files and `symbols.json`.

Scratch evidence stays untracked under this worktree's `scratch/`:
- PDB export, IDA copy and attempt log, Ghidra exports, atlas dumps;
- worker reports, verifier probes, snapshots and logs.
