# glow `_get_particle_world_position`: owner ruling requested

## The question

> **Do you admit `glow.obj` as a complete object when `get_particle_world_position` reproduces January's two uninitialised
> stack-array reads, each disclosed with the policy-form BUG comment below?**
>
> 1. The 2- and 3-marker paths never build `traversal_left_vectors`, but the third spline reads all four.
> 2. 3-marker interval 1 writes its midpoint knot into `traversal_time_indices[2]`, so `traversal_time_indices[1]` is read unset.
>
> **And since these are reads of uninitialised automatic arrays, not out-of-bounds struct accesses, do you waive the
> compile-time layout assertion from the 2026-09-20 ruling?**

**YES:**
- Apply 01, 02 and 03 together (see `patches/APPLY_ORDER.txt`).
- Result: +1 strict function (`_get_particle_world_position`, 1,744 B padded / 1,729 objdiff); Halo objects 379 -> 380 (381 with king); complete_code +6,987; complete_data +392; one park retires.

**NO:**
- Apply nothing. The current defined-C park (2,528/55, 49.07%) stays.
- There is no admissible defined alternative. The wave-2 `cand_fixed.c` fallback was rejected by review2, and n3a v3 is still +592 B.

## What January does (target proof, re-read in this wave from build/split)

- The frame is `sub esp,0xc4`. `traversal_left_vectors` occupies `[ebp-0xc4, ebp-0x94)`: four 12-byte vectors at -0xc4, -0xb8, -0xac and -0xa0.
  - Its only writes are the default-loop indexed stores at +0x21b, +0x23e and +0x259 (the `cross_product3d` results).
  - Its only reads are the four `lea`s at +0x637..+0x64c that feed the third, unconditional `nonuniform_cubic_spline_vector3d`.
  - The 2-marker and 3-marker paths never touch it.
- 3-marker interval 1 (+0x2ea..+0x3a3):
  - It loads `marker_time_index[1]` and keeps it on x87.
  - It computes the midpoint, then jumps to +0x5c3, where `fadd [ebp-0x10]; fstp [ebp-8]` stores the midpoint to `traversal_time_indices[2]`.
  - At +0x5d2, `mov ecx,[ebp-0xc]` reads `traversal_time_indices[1]`, which this path never wrote.

The later first-party /Od build (function 0x798de0, `scratch/w/glow/od_gpwp.txt`) has the same two defects:

- RTC descriptor 0x799c44 names `traversal_points` (48 B), `traversal_up_vectors` (48 B), `traversal_left_vectors` (48 B) and `traversal_time_indices` (16 B).
- `[ebp-0xa8]` (`traversal_left_vectors`) is written only through the default loop's `cross_product3d` (0x799887) and is read by the spline-3 `lea`s.
- 3-marker case 1 stores `marker_time_index[1]` to `traversal_time_indices[2]` (0x799533), then stores the midpoint to `traversal_time_indices[2]` again (0x799639).

HCEA HCEX.pdb gives the same local names (`scratch/w/glow/hcex_gpwp.txt`). Earlier waves record that HCEA 2011 has the same shape (n3a).

## Evidence FOR admitting

1. Each defect is target-proven by January's control flow, and double-attested by a later first-party build whose source text still contains it. This is the playbook's original-uninitialised-read gate (docs/exact_match_acceleration_playbook.md, "Original bugs"):
   - January proves the read;
   - an independent build corroborates it;
   - a bounded defined search fails: n3a v3 is +592 B, P8 has 71 REAL regions, and this wave's sA and sB are each 24/1.
2. Each defect is independently load-bearing, measured at cdc8ebd3. Repairing only the knot gives 24/1 `[sha]`. Defining only the left vectors gives 24/1 `[size 1776!=1744, sha]`.
3. The function reaches strict exact, and the whole object passes the section-4 audit:
   - 36/36 symbols;
   - the surplus is identical to production and to January's selected copies;
   - provider link PASS in both orders;
   - PDB storage 0;
   - data 100%;
   - the emulated chain shows +1 object, 0 revocations and parks 85/0/0.
4. Owner precedents from 2026-09-20 admitted target-proven original defects once strict exact, with a policy BUG comment and independent review:
   - **ai_script** (Matching; `source/ai/ai_script.c:2765`; docs/object_matching_logs/claude_lane_a_waveA1_ai_script_20260920.md). A missing vsprintf argument reads an unset stack slot (the saved-EDI home).
   - **`_actor_emotion_update`** (commit 8e4309dd; `source/ai/actor_perception.c:3501`, layout assertions at `actor_perception.c:856-867`). An out-of-bounds read.
5. **The reads have no structure-layout dependency.** Both are reads of the function's own automatic arrays, in bounds, of never-written elements. The 2026-09-20 `_actor_emotion_update` layout assertion guarded a read past a struct member into its neighbour, and nothing here is analogous. That is why the question asks for an explicit waiver instead of inventing an assertion.

## Evidence AGAINST / risks the owner should weigh

1. Brief sections 2 and 7 make preserved uninitialised reads an owner-gated class. dead_camera is held on the same class.
2. On 2026-09-20 the owner refused archived uninitialised-read donors: `_dead_camera_update` and `_compare_profile_sections` (astra_90pct_rejected_hypotheses_20260920.md, item 8). Here the reads are /Od-attested source text, not a donor, but the class is the same.
3. **Behaviour:** on the 2- and 3-marker paths, the particle's left/right offset (`v * cosine`) uses stack garbage. On 3-marker interval 1, the spline's second knot is garbage. So particle positions are visually wrong or unstable for glows with 2-3 markers. This is January's (and the later build's) actual behaviour, but it is real, visible misbehaviour, not a benign quirk.
4. **The packet adds new point/vector representation view casts**, e.g. `(real_point3d *)&traversal_up_vectors[i]` and `(real_vector3d *)&traversal_points[i]`. The brief's view-cast rule applies, and each requirement is met:
   - per-site /Od attestation: callers of 0x79ce50 `point_from_parametric_line` pass `&traversal_up_vectors[]`, and spline 1 writes `particle+0x2c` with `&traversal_points[]` as vector arguments;
   - compatible layout: three `real`s each;
   - byte-inert strip: s1_nocasts is 25/25, but the plain spelling raises C4133;
   - strictly exact caller: yes with this packet;
   - disclosure: here.
5. **Nits (byte-inert; left as packed, owner may request):**
   - `index < NUMBEROF(traversal_time_indices)`: /Od shows a signed `cmp 4; jge`, and the base spelled `index < 4`. Both are 25/25 (sC).
   - The locals `sine`/`cosine` shadow real_math.h's `sine()`/`cosine()` inlines. This is legal and raises no warning. Spelling through the helpers adds `_sine`/`_cosine` surplus COMDATs (review2 p5).
   - The range `[ebp-0xc4..-0x94]` in the first BUG comment is end-exclusive: 48 bytes, last element at -0xa0, and -0x94 is `u`. Correct as written, but could read `[ebp-0xc4,-0x94)`.

## The comments exactly as they would land (in `get_particle_world_position`)

Before the marker-count switch:

```c
	/* BUG (preserved for exact matching): the two- and three-marker paths never build
	 * traversal_left_vectors, yet the left-vector spline below reads all four of them.
	 * January 0x001231f0: only the default (>3-marker) loop writes them ([ebp-0xc4..-0x94]);
	 * the unconditional third nonuniform_cubic_spline_vector3d call pushes them. The later
	 * first-party debug build (0x798de0) has the same omission. A corrected build should
	 * build traversal_left_vectors in these paths too.
	 */
```

In 3-marker `case 1:`, directly above the midpoint store:

```c
					/* BUG (preserved for exact matching): the midpoint knot is written over
					 * traversal_time_indices[2] instead of traversal_time_indices[1], so
					 * traversal_time_indices[1] is never set on this path (also in the later
					 * first-party debug build, 0x798de0).
					 * January 0x001231f0+0x2ea loads marker_time_index[1] first and stores the
					 * midpoint at +0x5c6 ([ebp-0x8]); [ebp-0xc] is read at +0x5d2 unwritten.
					 * A corrected build should assign the midpoint to traversal_time_indices[1].
					 */
					traversal_time_indices[2] = (traversal_time_indices[2] - traversal_time_indices[0]) * 0.5f + traversal_time_indices[0];
```

(0x001231f0 is the function's January file offset, from symbols.json line 5061.)

## House-rule classes touched

- **Authentic January bug (owner-gated, brief sections 2 and 7):** this ruling, for two reads.
- **2026-09-20 layout-assertion condition:** a waiver is requested; no layout dependency exists.
- **New representation view casts (point <-> vector):** /Od-attested per site, byte-inert, required for type correctness, caller strictly exact, disclosed.
- **Named locals from first-party sources:** HCEX.pdb / /Od RTC (`traversal_*`, `forward`, `u`, `v`). `sine`/`cosine` are homed trig results attested by /Od (two homed results, sin first). They are load-bearing: s4_notrig is 24/1, so they are attested locals, not fillers.
- **/Od statement form** `marker_index = PIN(...); particle->parent_marker_index = marker_index;`: byte-inert (s2_pin1 25/25) and attested at /Od 0x798f04.
- **Park retirement:** `config/parked.json`.
- **Not touched:** storage (already static), data, headers, NODUP providers, consumer-local prototypes, `float`.
