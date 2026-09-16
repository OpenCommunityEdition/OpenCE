# `motion_sensor.obj` — opus5 next-150K lane, wave n3a (2026-09-15), STRUCTURAL PARK PASS

Unit `source/interface/motion_sensor`. Worker scope: `source/interface/motion_sensor.c` only.
All five non-exact rows are PARKED, so every measurement here is a scratch candidate or a shadow-tree probe.
**The real file is unchanged** (`git diff --stat -- source/interface/motion_sensor.c` is empty).
Final real-file gate: `== exact 14  residual 5  unwritten 0` (identical to baseline); the `_point_from_line3d`
guard passes; `scratch/parkcheck.py` reports `parks 5 drift 0`; `tools/fake_match_scan.py` reports 0 leads.

Worker notes and every probe: `scratch/workers/n3_motion_sensor.md`, `scratch/workers/n3_motion_sensor/`.

## Baseline

| Function | January padded / relocs | Parked body | Gate tag |
| --- | ---: | ---: | --- |
| `_blip_begin` | 112 / 10 | 112 / 10 | sha |
| `_motion_sensor_update` | 1168 / 43 | 1136 / 44 | size, relocs, sha |
| `_render_blip` | 400 / 18 | 384 / 17 | size, relocs, sha |
| `_render_motion_sensor` | 768 / 40 | 768 / 40 | sha |
| `_update_motion_sensor` | 864 / 39 | 816 / 37 | size, relocs, sha |

Duplicate prevention: the three prior ledgers (`..._opus5_250k_w3c_20260915.md` — wave w3c, the same day —
`..._opus5_150k_w1_20260914.md`, `..._jonas_census_tick_one_shot_20260829.md`),
`lane_reconciliation_batch5_fuzzy_ui_world_20260909.md`, laws_w2 and laws_w3 in full,
`git log --all -- source/interface/motion_sensor.c` (last body change `800179a20`, byte-equal to the current file)
and `branch_sweep` (11 blobs; the current file is the best at `exact 14`).

## 1. Structural census (run fresh, before any shape)

### Frame census

| Function | January | parked body | best scratch body |
| --- | --- | --- | --- |
| `_update_motion_sensor` | `sub esp,0x2c0` | `sub esp,0x2c4` (+4, one extra cell) | `w3c/u7.c` = `0x2c0`, slot set identical |
| `_motion_sensor_update` | `sub esp,0x284` | `sub esp,0x270` (-20, different short-array topology) | `w1/v13.c` = `0x284`, slot set identical |
| `_render_blip` | `sub esp,0xc` | `sub esp,0xc` | identical; the gap is not the frame |

### Relocation census by target

`_motion_sensor_update` — January owns `_blip_type_get` ×1, `_object_get_and_verify_type` ×1, `_tag_get` ×1 and
`_object_try_and_get_and_verify_type` ×2 where we call `_motion_sensor_blip_set_type_and_size` ×1 and
`_object_try_and_get_and_verify_type` ×1; we additionally own two surplus `_local_player_get_player_index`, one
`_datum_get` and one `_player_data`. **Both builds emit two `_game_engine_running` calls**, so the recorded park
criterion ("January emits the otherwise-discarded `game_engine_running` call") is refuted: the residual is the
inlined blip type/size helper plus a duplicated local-player fetch.

`_render_blip` — January owns `__real@3f800000` ×2, `__real@3fe6666666666666` (double 0.7) and `__real@3dd6774d`;
we own one `__real@3f800000`, the widened `__real@3fe6666660000000` and `__real@3dd67750`.

`_update_motion_sensor` — January owns two `__real@00000000` loads we lack; everything else is address shift only.

### Owner census (candidates vs `build/base`, checked against `build/split`)

`w3c/bb1.c` clean. `w3c/rb6.c` swaps two literals for January's own. **`w3c/u7.c` and `w1/v13.c` each add
`_distance_squared3d` and `_vector_from_points3d`** — `real_math.h:1177` defines `distance_squared3d` as
`magnitude_squared3d(vector_from_points3d(a, b, &v))`, so the pair is inseparable from any call. The pending
laws_w3 R14 `_vector_from_points3d` ruling therefore gates **2,032 padded bytes in this unit**, not the 1,168 the
w3c ledger recorded.

### Candidate re-verification at this HEAD

`w3c/bb1.c` → `exact 15 residual 4` (`_blip_begin` strict EXACT). `w1/v13.c` → `exact 16 residual 3`
(`_blip_begin` + `_motion_sensor_update` strict EXACT). `w3c/u7.c` → `_update_motion_sensor` **864 / 39, [sha] only**.
`w3c/rb6.c` → `_render_blip` **384 / 18** against the real headers.

## 2. Outcomes

### `_motion_sensor_update` (1168 / 43) — OWNER-BLOCKED, 0 shapes spent

The structural gap is solved (`w1/v13.c` is strict EXACT and reproduces January's frame and relocation set exactly).
Both recorded blockers are policy, so no laws_w2/laws_w3 law applies and spending shapes a standing policy forbids
is itself a defect (R13):

1. January leaves `camera_positions[i]` uninitialised for a local player whose unit is NONE, and the object loop
   reads it. Verified independently this wave that the array is not covered by any existing clear: January's single
   `csmemset` is `csmemset(&sentinel, 0x62, 0x200)` at `[ebp-0x284]`, the stack-corruption sentinel; the 0x84 bytes
   of real locals above it are never written at entry.
2. The owner census gains `_distance_squared3d` + `_vector_from_points3d` (R14).

### `_update_motion_sensor` (864 / 39) — NOT LANDED; structural gap closed, residual is a reassociation tie

`w3c/u7.c` reaches **864 / 39 with 285 vs 285 instructions**. `alndiff` leaves exactly one hunk, 8 bytes at
`0x16b..0x178` in loop 1: January associates the squared length as `(j*j + i*i) + k*k`, ours as `(i*i + k*k) + j*j`
(laws_w2 A22 / laws_w3 A35(2); January = the S4 ranking, ours = S5). The leaf bytes at `0x14b..0x16a` are identical
and loop 2 already matches, so the leaves and their ages are equal — the summation lives inside
`magnitude_squared3d` (real_math.h:1123), which a worker may not edit.

**The recorded park criterion is refuted.** January's two `__real@00000000` relocations are not "unresolved x87
scheduling": they are the `k*k` term of `distance_squared3d(&reference_position, &object_position)` where
`reference_position.z = object_position.z`. Copy forwarding folds `z-z` to the zero literal while the multiply that
was already built survives — a source shape, reproduced exactly by u7.

No shapes spent this wave: the difference is pure x87 term order (the orchestrator's instruction is not to grind
such ties), w1 and w3c together spent 15 shapes on it with a clear tie verdict, and an exact body would still be an
R17 conditional reopen behind the R14 ruling.

### `_render_blip` (400 / 18) — NOT LANDED; E7 is now measured NOT to be count-neutral

`w3c/rb6.c` is 384 / 18 against the real headers and **400 / 18** under a shadow `real_math.h` carrying the laws_w3
**E7** `rotate_vector2d` body (re-measured this wave with `sgate2.py`). The remaining 16 bytes are entirely the
helper's body: January emits the j row into a temporary and copies it back with an integer move AFTER the i store,
which the repo body (`result->i` first, then a second read of `v->i` — the in-place aliasing bug) cannot produce.

**New measured negative.** Six shadow-header probes (`scratch/workers/n3_motion_sensor/sh{A,B,D,E,G,H}`):

| shadow `real_math.h` | `bitmap_drawing` | `game_engine_race` |
| --- | --- | --- |
| A — two temps, j first (E7 text) | `_bitmap_copy` LOST (2784/69 both sides, sha only) | — |
| B — one temp (`real j`), i stored inline | `_bitmap_copy` LOST | `_race_engine_player_update` LOST (160/9), `_race_touch_flag` LOST (336/28 vs 30) |
| G — one temp caching the input (`real i = v->i`) | `_bitmap_copy` LOST | — |
| H — two temps, i first | `_bitmap_copy` LOST | — |
| D — ORIGINAL body + 1 extra prototype | `7 of 7 EXACT` | — |
| E — ORIGINAL body + 2 extra prototypes | `7 of 7 EXACT` | — |

Neither `bitmap_drawing.c` nor `game_engine_race.c` contains a single textual reference to `rotate_vector2d`, yet
every correct body flips them while pure declaration additions do not. This **refutes the declaration-count
attribution in laws_w3 A41 for these victims and confirms the body-IL attribution**, and it settles E7's open
question: **no count-neutral compensation exists**, so a duplicate-prototype cleanup cannot rescue the packet.
E7 is +400 (this row) against -3,280 unless the three victim rows are re-closed in their own units under the header.

### `_blip_begin` (112 / 10) — PARK REOPEN PROPOSED (re-verified; duplicates the w3c proposal)

`scratch/workers/w3c_motion_sensor/bb1.c` = the current real file plus the January four-slot ABI and a disclosure
comment. Whole-TU gate at this HEAD `== exact 15 residual 4`; no baseline-EXACT row lost; guard passes; owner census
empty in both directions; fake-match scan 0 leads.

January evidence re-derived this wave: the body asserts `reference` from **ESI** (`test esi,esi; jne` with the
`"reference"` literal), reads `in_multiplayer` at `[ebp+0xc]` and `local_player_index` at `[ebp+0x10]`, and never
touches `[ebp+8]`; the only caller does `lea esi,[ebp-0x10]; push esi; push eax; push 0; call _blip_begin;
add esp,0xc`, so the literal zero is argument two with `reference` enregistered.

Independent name search (new this wave): HCEA `halocea-review/src/blam/rasterizer/dx9/blip_begin.c` is the later
build's **three**-parameter `void blip_begin(const real_point2d *center, uint8_t in_multiplayer,
int16_t local_player_index)` and its "4th num_blips param" line is an explicit decompiler DEVIATION note, not a
name; a whole-tree grep of `_research-stian-halo`, `stian-current`, `reference/halocea` and
`public-donor-census-20260829` finds no `blip_begin` at all. `long 0` and `real 0.0f` emit identical bytes.
`long unused` plus the disclosure comment remains the only honest spelling (precedents
`actor_perception_find_sense_position`, `profile_rasterizer_stalls`).

**Coupled park re-baseline required in the same commit:** `_render_motion_sensor` 768 / 40 → **784 / 40**,
sha `de9701add36aae94` (laws_w2 A8 callee-ABI cascade).

### `_render_motion_sensor` (768 / 40) — SKIPPED-EXHAUSTED

Not a target of this wave. w1 spent 15 shapes, w3c ran A5/A12/A23/A24/A25 as detectors and none fires; the residual
is the C2 scheduler interleaving the sensor-index integer statement with the x87 weight chain. Listed only because
the `_blip_begin` reopen re-baselines it.

## Proposals

1. **Park reopen — `_blip_begin` (112 B).** `scratch/workers/w3c_motion_sensor/bb1.c`, coupled with the
   `_render_motion_sensor` re-baseline to 784 / 40 sha `de9701add36aae94`. Needs the owner ruling on the never-read
   second parameter's name and type. (Same candidate the w3c ledger proposed; re-verified unchanged at this HEAD,
   with an independent, now-exhaustive donor-tree name search.)
2. **Owner ruling — `_vector_from_points3d` surplus COMDAT (R14), scope corrected.** It gates `_motion_sensor_update`
   (1,168) *and* `_update_motion_sensor` (864) in this unit — 2,032 B — on top of sun_glow's 2,352.
3. **Owner ruling — `_motion_sensor_update` uninitialised `camera_positions` read.** Exact body `w1/v13.c`.
4. **laws_w3 E7 re-evaluation.** E7 cannot be made count-neutral (measured above). If the orchestrator still wants
   the 400 B, E7 must ride with a worker owning `source/bitmaps/bitmap_drawing.c` and `source/game/game_engine_race.c`
   who re-closes `_bitmap_copy`, `_race_engine_player_update` and `_race_touch_flag` under the E7 header.
5. **parked.json evidence text.** The recorded criteria for `_motion_sensor_update` (the "otherwise-discarded
   `game_engine_running` call") and `_update_motion_sensor` ("zero-real relocations reflect unresolved x87
   scheduling") are both refuted by this census and should be replaced by the causes recorded here.

## Reopen criteria

- `_blip_begin`: owner ruling on the unused parameter's name and type.
- `_motion_sensor_update`: owner rulings on the uninitialised read AND the `_vector_from_points3d` census.
- `_update_motion_sensor`: the `_vector_from_points3d` ruling PLUS a spelling in which the folded-zero `k` term is
  not reassociated into the first partial sum (start from `w3c/u7.c`).
- `_render_blip`: E7 landed together with re-closures of its three measured victims, plus a lever that moves
  `fmul dword [esi+0x2d0]` three integer instructions later inside its basic block.
- `_render_motion_sensor`: a spelling that emits `sub eax, dword ptr [ebp-0x18]` as a memory operand and finishes
  the integer sensor-index statement before the x87 weight chain.
