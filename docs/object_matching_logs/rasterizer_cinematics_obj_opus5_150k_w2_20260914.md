# `rasterizer_cinematics.obj` — Opus5 150K wave w2 (2026-09-14)

## Baseline

`python -B tools/campaign/gate.py source/rasterizer/rasterizer_cinematics --all --forbid-emitted-symbol _point_from_line3d`
on the unmodified real file: **12 exact / 4 residual / 0 unwritten**, guard passed. All four residuals are parked.

| Function | Target size/relocs/sha16 | Real-file candidate |
| --- | --- | --- |
| `_rasterizer_screen_effects_initialize_for_new_map` | 48 / 3 / e53fee00ab5bd5eb | 48 / 3, `add esp` before all four stores |
| `_rasterizer_screen_effect_set_convolution` | 112 / 4 / 7beef58f5328b776 | 96 / 4, `fst` instead of `fld st(0)`/`fstp` |
| `_rasterizer_screen_effect_set_filter` | 112 / 4 / de4709a02eba43f1 | 112 / 4, store interleave |
| `_rasterizer_screen_effect_set_video` | 288 / 20 / eb1b757f0f5e5dbf | 288 / 20, two `add esp` placements |

## Outcome

- The real file is unchanged. All four bodies are parked.
- **Park-reopen proposed for all four.**
- Candidate: `scratch/workers/rasterizer_xbox_rasterizer_xbox_hardware_geometry_etc/cin_v2.c`
  (sha256 `e862d17fa64285b93fa2e43e4a5596541290624b421fca971890a85763b92cfb`).
  - It is the current real file plus two kinds of change:
    - `static` on `cinematic_screen_effect_globals`;
    - the four parked bodies rewritten to access the global directly.
  - Whole-TU gate: **16 / 0 / 0**, and the guard passed.
  - Every baseline-exact row stays exact.
  - `scratch/parkcheck.py` reports that all four parks move to the exact target hashes.
  - Owner census against base: no candidate-only or base-only owner.
  - `tools/fake_match_scan.py`: 0 leads, the same as HEAD.

## New evidence: January linkage

1. `_cinematic_screen_effect_globals` (file offset 4612852) has **no public** in the January PDB (`scratch/pdb200-current/pdb_symbols.json`).
   The PDB public list is complete for external Halo symbols; for example `_global_d3d_device` and `_global_rasterizer_data` are both present.
2. Calibration against first-party Bungie `random_math.c`: its `static` globals (`random_math_globals`, `global_random_seed`, `random_seed_lock_count`) have no publics.
3. In the range 4.40M–4.80M there are 60 non-static `symbols.json` data names with no public. 59 of them are referenced only by their own January object (`scratch/xref_census.py`).
4. No January object other than `rasterizer_cinematics.obj` references `_cinematic_screen_effect_globals`.

## Compiler law (VC7 13.00.9254)

An address-not-taken global with internal linkage is treated as un-aliased by indirect stores:
- VC7 reloads it after calls only;
- an extern global is reloaded after every indirect store.

January's bytes show exactly one `mov eax,[bss]` after each call and none after stores. The previous bodies imitated this with a `globals` local and artificial `globals = cinematic_screen_effect_globals;` refresh assignments. Those refreshes are removed.

The same law was measured on `__rasterizer_window_begin` (rasterizer_xbox) and, per donor commit `ff1144479`, in rasterizer_xbox_models.

## Corroboration

- **Deferred caller cleanup.** A corpus scan of accepted-exact January functions (`scratch/workers/rasterizer_xbox_rasterizer_xbox_hardware_geometry_etc/espscan.py`) found January placing `add esp` before the *last* store of a block in four functions:
  - `game_sound_initialize_for_new_map`
  - `objects_initialize_for_new_map`
  - `vehicle_reset`
  - `unit_get_head_position`

  This is the placement January uses in new_map and set_video once the refresh locals are removed.
- **Inline frequency law.** The private helper `rasterizer_screen_effects_time()` is inlined in the plain statement context of set_convolution and set_filter (a direct `_game_time_get` call). `get_cinematic_parameters` keeps out-of-line helper calls inside its `PIN` arms (January relocations at +0x65, +0x89, +0xF2 and +0x116).

## Shapes (scratch only)

| Shape | Result |
| --- | --- |
| nm1/nm3: 4-trip loop over `script_values`, local pointer | 48 sha; unrolled, but IV strength-reduced (`add eax,0x64`) |
| nm2: loop, extern global | not unrolled |
| nm4: direct stores, extern global | reload before every store (proves aliasing) |
| nm5: `static` global only, old bodies | new_map and convolution EXACT; filter and video still sha |
| f1/f2: set_filter with direct global access (inline time expression / helper) | EXACT (f2 retained) |
| v1: set_video with direct global access, bitmap pointer locals removed | EXACT |
| v2: set_convolution with direct global access plus helper | EXACT; 16/16 (candidate) |
| v3 (optional hygiene): also drops the refresh local from exact `rasterizer_screen_effect_start` | 16/16; not part of the reopen |

## Proposals

`config/symbols.json` line 22673, edited in place:
`{ "file_offset": 4612852, "flags": 0, "name": "_cinematic_screen_effect_globals", "static": true },`
The only consumer is `rasterizer_cinematics.obj`. Regenerate csplit, then unpark the four functions.

## Park criterion

Each park asked for a natural same-compiler caller-cleanup or x87-lifetime donor. The premise, a scheduling tie, is refuted: the residuals were the aliasing consequence of wrong (extern) linkage. The correction is PDB-proven and needs no steering.
