# `particle_systems.obj` Opus5 150K house-clean lane, wave w1 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/effects/particle_systems.c`; target `build/split/source/effects/particle_systems.obj`.
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/150k-house-clean-20260914`,
  base `12f7375d4`. Worker `w1:particle_systems`.
- Compiler: VC7 13.00.9254 with the unchanged repository flags. No source, header, config or tool file was
  modified. All experiments are scratch candidates (`gate.py --source`), micro-TU labs compiled with the
  unit's flags, and one scratch csplit run into `scratch/workers/particle_systems/csplit/`.
- Evidence: January COFF bytes/relocations/.rdata (authority); the prepared Ghidra decompilation and atlas
  for this object (atlas: C++-mangled prototypes for the two state helpers and `new_particle_default`, no
  data names); HCEA `particle_system_update.c` / `particle_system_new_particles.c` /
  `particle_system_render.c` (topology only); the HCEX PDB data names recorded by the 100K ledger.
- Prior ledgers read: `opus5_100k_20260914`, `opus5_house_clean_20260913`, `render_owner_safe_fuzzy_20260912`,
  `hcea_january_semantic_packet_20260908` (in full); the remaining particle ledgers were grepped. The 100K
  lane worker log and its scratch candidates were also read.

## 2. Validated baseline

Real-file gate (identical at finish): `== exact 20  residual 5  unwritten 0`. The point guard passed. The
fake scan found 0 leads. No parks exist for this unit, and none were inherited from the 100K lane.
`branch_sweep` found no blob better than the current file.

| residual | target padded/meaningful/relocs/sha16 | real-file candidate |
|---|---|---|
| `_particle_system_new_particle_jet` | 368/365/11/`322ad2bf7cdffa79` | 368/11 [sha] |
| `_particle_system_new_particles` | 848/843/35/`42b053f34b78452c` | 848/35 [sha] |
| `_particle_system_render` | 1360/1346/33/`c4d841f9803c7f3e` | 1344/35 [size, relocs, sha] |
| `_particle_system_update` | 1664/1651/58/`0a74337d69d2dfdf` | 1616/58 [size, sha] |
| `_particle_system_update_particle_default` | 400/392/12/`f8da0780e495f488` | 400/12 [sha] |

## 3. Outcome

- **Real file unchanged** (`git diff --stat` empty). There is no new strict-exact row in the real-file gate.
- **`_particle_system_new_particles` is EXACT-PENDING-RENAME.** It is strict EXACT against a scratch
  csplit made with the proposed `symbols.json` names. The candidate is
  `scratch/workers/particle_systems/cand_full2.c`
  (sha256 `84f9356bfd6e5653cfe19b176ee87b65de945455b6f3038decf33c184545d502`).

## 4. New measured VC7 law: lazy call-result store

A real call result assigned directly to a structure field is stored immediately:
`call; fstp [mem]; add esp`.

Assigned to a named local that is then stored into the field, the store is deferred to the end of the block:
`call; add esp; <integer instructions>; fstp [mem]; jcc`.

This was measured three ways:

1. **Clean lab** (`scratch/workers/particle_systems/lab7.c`). Only the local form reproduces January's
   `add esp,0xc; test di,di; fstp [esi+0x40]; jl`.
2. **Whole-image scan** (`scan_defer.py`). January has 7 deferred call-result stores in Halo code. The base
   build reproduces 4 of them, all in strict-exact functions:
   - `dead_camera_new` ×2: `distance`, `yaw` locals;
   - `bored_camera_update`: `field_of_view` local;
   - `glow_normal_particle_new`: the switch-case x87 PHI law.
3. **Negative control.** No direct `field = call()` site defers anywhere in the base build.

## 5. `_particle_system_new_particles` (pending rename)

### Body

The body is the 100K lane's `np12` body plus the rotation local:

- `state_definition = initializing ? NULL : TAG_BLOCK_GET_ELEMENT(...)` at its declaration;
- a `marker_index` local before the dispatch;
- `type->particle_count++` before the link stores;
- `target = count + whole` before the fractional update.

The rotation statement becomes:

```c
real rotation;
...
rotation = real_seed_random_range(get_global_local_random_seed_address(), 0.0f, _pi*2.0f);
particle->rotation = rotation;
```

### Measurements

| gate | result |
|---|---|
| vs current `build/split` | 20 exact (every baseline row kept); `new_particles` [reloc-identity]; guard pass |
| vs scratch renamed split | **21 exact**; the other 4 residuals unchanged |

- **Data aggregate:** the candidate's 32-byte `.rdata` matches the renamed split exactly: bytes
  `...cdcc4c3d 8988083d`, six function-pointer relocations, and symbol offsets and storage classes.
- **Split scope:** the scratch csplit output differs from `build/split` only in `particle_systems.obj`
  (whole-tree file compare).
- **Config alone is safe:** against the renamed split, the unchanged real file is still 20 exact.

### Checks

- no parks;
- owner census of candidate `.rdata` equals the renamed January split;
- fake scan 0 leads.

### Authenticity

Each new particle's initial spin is drawn once and stored. The named local is the only VC7 spelling measured
to give January's deferred store (lab-proven). It is the same form as the exact camera reconstructions. The
verifier should rule on admissibility.

### Behaviour note

- The candidate's jet change, `velocity*seconds_per_tick`, is value-identical: the file-static constant
  folds from `1.0f/TICKS_PER_SECOND`.
- `seconds_per_tick` follows the `projectiles.c` precedent: `static real const seconds_per_tick = 1.0f / TICKS_PER_SECOND;`.

## 6. Other functions

### `update` (not landed)

**Lab finding** (`lab2`–`lab6`, including a 32-combination grid). VC7 strength reduction picks as IV base
the pointer defined last before the loop:
- When `sv` is hoisted and `tv` defined after it, the IV is `tv`. This is our build.
- When `tv` is defined first and `sv` after it, the IV is `sv`. That gives January's IV and its
  `fld st(0)` / `fld st(2)` order, but VC7 then loses the destructive `add ebx,0x34` hoist.

No spelling gave both.

Real-TU shapes tried:

| shape | result |
|---|---|
| u1: `sv` declared in the arm after `tv` | 1680 |
| u2: `sv` assigned after `variables =` | 1664 [sha] |
| u3: `sv` assigned before `variables =` | 1664 [sha], IV correct, hoist lost |
| u4: HCEA declaration order, index loop | 1680 |
| u5: HCEA declaration order, pointer loop | 1680 |

All were reverted. The 100K `update_v7b` body remains the best candidate.

### `render` (not landed)

- **Residual:** a label swap between two callee-saved register webs, plus dependent temporaries. No new
  lever was found, so no shapes were tried.
- **Behaviour defect in the real file:** it resets the weights on shader **incompatibility**, which is the
  inverse of January.
- **Candidate:** `scratch/workers/particle_systems/cand_render.c` (real file plus the r13 body) keeps 20
  exact, measures 1360/33 [sha], and passes the guard.
- **Proposed as a zero-credit fix.** A worker may not land it (brief §6).

### `update_particle_default` (skipped, exhausted)

A whole-image scan (`scan_lerp.py`) shows that January's pre-pushed `(1-t)` lerp schedule occurs in exactly
this one function. A lab of eight natural spellings (`lab8.c`) all canonicalise to our schedule.

### `new_particle_jet` (skipped, exhausted)

The remaining difference is the operand roles inside the shared `real_math.h` `cross_product3d` inline.
Data relocations become equal with the proposed names.

## 7. ORCHESTRATOR-PROPOSAL (`config/symbols.json`)

Edit lines 14527–14528 in place; do not re-serialise.

```
replace: { "file_offset": 2467512, "flags": 0, "name": "_rdata_0025a6b8" },
with:    { "file_offset": 2467512, "flags": 0, "name": "_system_update_functions", "static": true },
         { "file_offset": 2467520, "flags": 0, "name": "_particle_creation_functions", "static": true },
         { "file_offset": 2467532, "flags": 0, "name": "_particle_update_functions", "static": true },
keep:    { "file_offset": 2467536, "flags": 0, "name": "_ground_error" },
add:     { "file_offset": 2467540, "flags": 0, "name": "_seconds_per_tick", "static": true },
```

Apply it together with the source packet `cand_full2.c`:
- PDB table names;
- `real const ground_error = 0.05f;` and `static real const seconds_per_tick = 1.0f/TICKS_PER_SECOND;`;
- the new_particles body;
- jet `velocity*seconds_per_tick`.

**Evidence**
- HCEX PDB: names, File Static / Global linkage, element counts, identical 32-byte layout.
- January relocations:
  - `new_particles+0x2c7` → +8;
  - `update+0xff` → +0;
  - `update+0x617` → +0x14;
  - jet → `_ground_error+4`.
- No other January or base object references these symbols.

**Consumers:** `particle_systems.obj` only.

**Expected credit:** `_particle_system_new_particles`, 848 padded / 843 meaningful.

## 8. Reopen criteria

- **update:** a form that defines the state pointer after the transition pointer yet keeps it destructive in
  both arms. Then land `update_v7b` with the names.
- **render:** a colouring-priority lever for the two callee-saved webs. Land the r13 polarity fix with it, or
  by owner ruling.
- **update_particle_default:** a second January exemplar of the pre-pushed lerp.
- **jet:** a measured tree-wide lever for the `cross_product3d` operand-role choice.

## 9. Disposition

- 20 / 25 strict exact, unchanged.
- One function (`new_particles`) is exact pending the data rename.
- Worker log: `scratch/workers/particle_systems.md`.
