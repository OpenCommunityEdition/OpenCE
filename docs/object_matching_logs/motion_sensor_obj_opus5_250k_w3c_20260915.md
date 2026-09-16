# `motion_sensor.obj` — opus5 250K house-clean lane, wave w3c (2026-09-15)

Unit `source/interface/motion_sensor`. Worker scope: `source/interface/motion_sensor.c` only.
All five non-exact rows are PARKED, so every measurement in this ledger is a scratch candidate.
**The real file is unchanged** (`git diff --stat -- source/interface/motion_sensor.c` is empty).
Final real-file gate: `== exact 14  residual 5  unwritten 0`; the `_point_from_line3d` guard passes;
`scratch/parkcheck.py` reports `parks 5 drift 0`.

Worker notes and every candidate: `scratch/workers/w3c_motion_sensor.md`,
`scratch/workers/w3c_motion_sensor/`.

## Baseline

| Function | January padded / relocs | Real-file parked body | Gate tag |
| --- | ---: | ---: | --- |
| `_blip_begin` | 112 / 10 | 112 / 10 | sha |
| `_motion_sensor_update` | 1168 / 43 | 1136 / 44 | size, relocs, sha |
| `_render_blip` | 400 / 18 | 384 / 17 | size, relocs, sha |
| `_render_motion_sensor` | 768 / 40 | 768 / 40 | sha |
| `_update_motion_sensor` | 864 / 39 | 816 / 37 | size, relocs, sha |

Duplicate prevention: the two prior ledgers (`..._opus5_150k_w1_20260914.md`,
`..._jonas_census_tick_one_shot_20260829.md`), `lane_reconciliation_batch5_fuzzy_ui_world_20260909.md`,
the 100K handoff, the w1 worker notes and their do-not-repeat lists, `git log --all` (last body change
`800179a20`, the w1 candidate-A landing, which is byte-equal to the current real file), and a grep of
`scratch/w2/w2-results.json` / `scratch/w3/w3-results.json` (this unit appears only in header-consumer
lists; no w2 or w3 group worked it).

Tooling added: `scratch/workers/w3c_motion_sensor/sgate2.py` — a shadow-tree gate that places the shadow
include directory FIRST. The existing `sgate.py` relies on the probe living in the shadow directory, which
does not work when the header is reached as `#include "real_math.h"` through `/I source/math`; it silently
compiled against the real header.

## Outcomes

### 1. `_blip_begin` (112 / 10) — PARK REOPEN PROPOSED, strict EXACT

Candidate: `scratch/workers/w3c_motion_sensor/bb1.c` = the current real file plus one hunk (the January
four-slot ABI). Whole-TU gate `== exact 15 residual 4`; no baseline-EXACT row is lost; the guard passes;
`tools/fake_match_scan.py` reports 0 leads; the owner census against `build/base` adds nothing
(candidate minus base is empty, and candidate minus target is exactly the surplus set the base already has).

The body was recovered in w1 as candidate B and reported with an owner-ruling caveat; commit `800179a20`
landed only candidate A, and no verifier rejection of this body exists anywhere in the lane's results,
ledgers or handoff. It is re-verified unchanged at this HEAD.

Evidence: January's callee reads `reference` in ESI, `in_multiplayer` at `[ebp+0xc]` and
`local_player_index` at `[ebp+0x10]`, and never reads `[ebp+8]`; the only caller `_render_motion_sensor`
pushes `esi, eax, 0` and pops `add esp,0xc`. A fourth, never-read, zero-valued 4-byte stack parameter is
therefore declared before `in_multiplayer`.

New for w3c: a direct search for an authentic NAME. HCEA
`src/blam/rasterizer/dx9/blip_begin.c` is a **three**-parameter function in the later PPC build (the
parameter was removed after January), and the Stian PC tree contains no `blip_begin` at all. The name and
type are genuinely unrecoverable (a `long 0` and a `real 0.0f` emit identical bytes — measured in w1), so
`long unused` with a disclosure comment remains the only honest spelling. Repo precedent for `long unused`:
`actor_perception_find_sense_position`, `profile_rasterizer_stalls`.

**Coupled park re-baseline (required in the same commit).** With the four-argument call site,
`_render_motion_sensor` moves 768 / 40 -> **784 / 40**, sha `de9701add36aae94`. That is the expected
laws_w2 A8 callee-ABI cascade: the call site now pushes three arguments exactly like January and the outer
history loop head lands on January's offset; the extra 16 bytes are loop-alignment padding.

### 2. `_render_blip` (400 / 18) — NOT LANDED; blocked only by a real_math.h packet

Best candidate `scratch/workers/w3c_motion_sensor/rb6.c`. Against the **real** headers it is 384 / 18;
under a shadow `real_math.h` carrying the laws_w3 **E7** `rotate_vector2d` body it is **400 / 18**, with
141 vs 141 instructions and every relocation target equal to January's.

Three defects in the parked body were identified from January's own bytes:

1. **Exponent literal.** January loads the *double* `__real@3fe6666666666666` (0.7). The parked body calls
   `power(x, 0.7f)`; `power(real, real)` (real_math.h:818) can only widen a float and gives
   `__real@3fe6666660000000`. January called `pow(x, 0.7)` directly.
2. **Pulse constant.** January holds `__real@3dd6774d` (0.10471973568). The parked body's `0.10471976f`
   was imported from an HCEA comment describing a LATER build and gives `__real@3dd67750`. `_pi/30` with
   this repo's `_pi = (real)M_PI` also gives `3dd67750`, so January's constant is not pi/30 at project
   precision; `float(3.141592f/30.0f)` is exactly `0x3dd6774d`. `_pi` itself is unchanged in January — the
   same object's `_update_motion_sensor` loads `__real@3fc90fdb` = `float(M_PI)/2`. House precedent for a
   hand-rounded pi decimal: progress_bar.c:883 `3.14159`, periodic_functions.c:389 `3.1415927f`,
   collision_debug.c:274 `0.104719758f`. This is a cross-build transfer (brief section 7) and should be
   fixed whatever happens to E7.
3. **Range test polarity.** `fcompp; test ah,5; jp <epilogue>` is `!(distance_squared < range*range)`:
   January wrapped the remainder of the function in `if (distance_squared < range*range) { ... }` and falls
   through to a single `return;` (SINGLE-EXIT LAW). The parked `if (>=) return;` gives `test ah,1; je`.

Further January facts that the candidate now reproduces: the frame (`sub esp,0xc`) with `distance` at
`[ebp-4]`, the rotation temporary in the **parameter home** `[ebp+8]` (A24) and the blip-size local reusing
`[ebp-4]`; `pulse_scale = 1.0f` as a pure x87 phi (`fld __real@3f800000` — the missing 18th relocation)
rather than an entry store, which requires the declaration to sit in a block at its use (A38 / A10); two
separate `scale_vector2d` steps (mapped distance, then relative scale) instead of one combined factor; a
`blip_size` local initialised before the custom test with the draw size spelled `pulse_scale * radius +
blip_size`; and `sine(-sensor->yaw)` / `cosine(-sensor->yaw)` passed straight into `rotate_vector2d`
instead of through named locals (named locals age the scalars and flip the j-sum operand order — laws_w2
A22 / laws_w3 A35).

**New evidence for E7.** January's inline copy at `0x4d..0x6e` is
`fld v.j; fmul cos; fld v.i; fmul sin; faddp; fstp [ebp+8]` followed by
`fld v.i; mov ecx,[ebp+8]; fmul cos; fld v.j; mov [ebp-8],ecx; fmul sin; fsubp; fstp [ebp-0xc]`: the j
component goes to a TEMPORARY and is copied back with an integer move AFTER the i store. The current header
body (`result->i = cosine*v->i - sine*v->j; result->j = sine*v->i + cosine*v->j;`, real_math.h:876-884)
cannot produce that, because with `result == v` it reads `v->i` after overwriting `result->i` — the
in-place aliasing bug. Until w3c, E7 rested only on bipeds' surplus-COMDAT sha; this is direct January byte
evidence from a second unit. All four body spellings (two-temporary and one-temporary, both sum orders)
compile byte-identically here (A44 canonicalisation), so E7's recorded text is sufficient.

Residual under E7: a single scheduling window after `call __CIpow` — January issues
`cmp bl,5; movsx edx,[ebp+0xc]; movsx eax,bl` before `fmul [esi+0x2d0]`, ours issues the `fmul` first and
spreads the three integer instructions through the x87 chain. Same instruction multiset, same registers,
byte-identical from `0x10d` to the end. Classification: C2 list-scheduler tie.

### 3. `_update_motion_sensor` (864 / 39) — NOT LANDED; one 8-byte hunk left

Best candidate `scratch/workers/w3c_motion_sensor/u7.c`: **864 / 39, 285 vs 285 instructions**, against the
real headers, up from the parked 816 / 37.

The w1 blocker was "every legal spelling of a literal-zero third component is folded by VC7". w3c found
the actual mechanism, and it is not a literal:

- A corpus scan with an x87 stack simulator (`zscan2.py`) over every January object that references
  `__real@00000000` shows that `_update_motion_sensor` is the ONLY function in the image that multiplies
  the zero-literal value by itself. (The one other hit, real_math `_point_to_line_distance_squared3d`, is a
  false positive of the linear scan: its zero is a `PIN` arm.)
- A micro-TU lab (17 spellings in `lab/lab.c`, `lab2.c`, `lab3.c`) confirms that every SOURCE literal zero
  is folded away entirely — `offset.k = 0.0f`, `= {0.0f,0.0f,0.0f}`, `set_real_vector3d(..., 0.0f)`,
  `dot_product3d`, `k = z - z` on globals, and a k assigned before a loop or in an enclosing block all
  produce a two-term sum.
- The ONLY spelling that reproduces January's `fld __real@00000000` followed by a self-multiply is a
  `real_point3d` local whose z is copied from the OTHER argument's call-filled LOCAL. VC7 forwards that
  copy, value numbering folds `z - z` to the constant, and the multiply that was already built survives.

Applied to the real function, that is `real_point3d reference_position;` with x and y from
`sensor->reference_point` and `reference_position.z = object_position.z;`, then
`distance_squared3d(&reference_position, &object_position)` — "measure the horizontal distance by putting
the reference at the object's height". January's otherwise-unexplained `mov eax,[ebx+0x74];
mov [ebp-0x3c],eax` integer copy is that local's y component.

Four further January facts closed the remaining structure:

* the comparison is the positive form `if (draw && distance <= range*range) {set} else {none}`
  (`test ah,1; jne`), not `if (!draw || distance > ...)` (`test ah,5; jnp`);
* the custom-blip loop counter is a `long` (`cmp ecx,eax` with no movsx against the byte-derived count, and
  a strength-reduced `add esi,8`);
* `player_index` is NOT reassigned before the custom-positions call — that call is written inline as the
  first argument, so January pushes the other four arguments first;
* `camera_position` is block-scoped at each of its two uses, so January shares ONE 12-byte cell
  `[ebp-0x18]` between the first camera fetch, the loop's `object_position` and the second camera fetch
  (laws_w2 A6); a function-scope declaration costs 16 frame bytes. `out_of_range_count` is declared with
  the second camera block, and the array element is updated component-wise
  (`position->x = position->x - camera_position.x;`), not by struct assignment.

Sole remaining difference, 8 bytes at `0x16b..0x178` in loop 1 only: January associates the squared length
as `(j*j + i*i) + k*k` and ours as `(i*i + k*k) + j*j`. The 0x14b..0x16a leaf bytes are identical, so the
leaves and their ages are identical; this is the laws_w2 A22 / laws_w3 A35(2) sum-association tie (January =
the S4 `j,i,k` ranking, ours = the S5 `i,k,j` ranking). Loop 2's sum already matches exactly (`k,j,i` in
both). The likely mechanism is that our k term is a known constant at reassociation time, so the
reassociator pairs it with the first term, while January's is still a subtraction at that point.

### 4. `_motion_sensor_update` (1168 / 43) — OWNER-BLOCKED, unchanged

w1 recovered a strict-EXACT body (`scratch/workers/motion_sensor/v13.c`, shape mu10). Both recorded
blockers are policy rather than codegen, so no laws_w2/laws_w3 law applies and no shapes were spent
(spending arrangements a standing policy forbids is itself a defect — R13):

1. January leaves `camera_positions[i]` uninitialised for a local player whose unit is NONE and the object
   loop then reads it. WORKER_BRIEF section 5 forbids preserving uninitialised-read UB.
2. The candidate census gains `_distance_squared3d` and `_vector_from_points3d` inline COMDATs; the
   `_vector_from_points3d` surplus is exactly the R14 class the sun_glow rejection left to an owner ruling.

### 5. `_render_motion_sensor` (768 / 40) — SKIPPED-EXHAUSTED; re-baseline required

w1 spent 15 shapes with a recorded do-not-repeat list. Each of A23, A24, A25, A5 and A12 was run as a
detector over the byte diff in w3c and none fires. With the four-argument `blip_begin` the body is 239 vs
235 instructions and the residual is entirely the C2 scheduler interleaving the five integer instructions
of the sensor-index statement (`sub eax,[ebp-0x18]; add eax,0xa; cdq; mov ecx,0xa; idiv ecx`) with the x87
weight chain (`fmul __real@3dcccccd; fst; fmul; fstp`), plus the `[ebp-8]`/`[ebp-0x20]` slot swap and two
consequent alignment pads. January completes the integer statement first.

If `_blip_begin` is reopened, this park must be re-baselined to **784 / 40**, sha `de9701add36aae94`.

## Rejected / do-not-repeat (new in w3c)

`_render_blip`: cosine computed before sine (emits `fcos` before `fsin`); `blip_color` declared beside
`distance` (hoists the colour lea above `__CIpow`); the mapped-distance product inlined into the
`scale_vector2d` argument (adds an `fxch`); a `boolean custom` local (materialises the byte and grows the
frame to 0x10); all four E7 body spellings are byte-identical here.

`_update_motion_sensor`: `set_real_point3d` for the reference (880, folds the zero away); swapping the
`distance_squared3d` arguments; every assignment-order permutation of the three reference components; a
named `offset` vector with `vector_from_points3d` plus `magnitude_squared3d` or `dot_product3d` (880);
swapping the comparison operands; declaration order; a named `range` local (880); a whole-aggregate
`reference_position = object_position;` copy (896); computing the displacement through
`reference_position` (diverges earlier); a shadow `real_math.h` with `distance_squared3d` split into two
statements (inert).

## Proposals

1. **Park reopen — `_blip_begin` (112 B).** Candidate `scratch/workers/w3c_motion_sensor/bb1.c`, coupled
   with a `_render_motion_sensor` park re-baseline to 784 / 40 sha `de9701add36aae94` in the same commit.
   Needs the owner ruling on the unused parameter's name and type (`long unused` plus a disclosure comment).
2. **Header packet — laws_w3 E7 (`real_math.h` `rotate_vector2d`).** Unlocks `_render_blip` 400 B, with
   this unit supplying the first direct January byte proof of the two-value body. E7's measured blast
   (268 TUs) loses `_bitmap_copy` 2,784, `_race_engine_player_update` 160 and `_race_touch_flag` 336 by
   declaration count, so the packet still needs a count-neutral compensation; it is not landable on these
   400 bytes alone.
3. **Owner ruling — `_motion_sensor_update` (1168 B).** Unchanged from w1: the uninitialised
   `camera_positions` read plus the `_vector_from_points3d` census.

## Reopen criteria

- `_render_blip`: E7 landed with a count-neutral compensation, plus a lever that moves
  `fmul dword [esi+0x2d0]` three integer instructions later inside its basic block.
- `_update_motion_sensor`: a spelling in which the folded-zero k term is not reassociated into the first
  partial sum — i.e. still a subtraction at reassociation time but a constant at code generation.
  Start from `u7.c`.
- `_render_motion_sensor`: a spelling that emits `sub eax, dword ptr [ebp-0x18]` as a memory operand and
  finishes the integer sensor-index statement before the x87 weight chain.
- `_blip_begin`: if the ruling is declined, an authentic name or type for the never-read fourth parameter.
- `_motion_sensor_update`: an owner ruling on both blockers, or a body that avoids them.
