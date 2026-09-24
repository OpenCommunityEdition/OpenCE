# source/rasterizer/rasterizer_lights :: _rasterizer_lens_flares_draw (2240, parked)

## verdict
FUZZY_IMPROVED

## newly_exact_functions
null

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\fin2.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\e3.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\P2_draw_on_P1.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\P2_draw_standalone.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\P2_parked_json_rebaseline.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\config\\parked.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_lights\\od_draw.txt"
]

## production_changes
Part of production.patch (on top of P1): P2_draw_on_P1.patch. P2_draw_standalone.patch is the draw change alone and applies to the current tree. P2_parked_json_rebaseline.patch re-baselines the park: base normalized_sha256 39103012d0bd42ef0f16cc285c5617331a95ce432f2d7bae17376d3781d79972, objdiff_percent 99.38626, plus appended evidence.
Changes:
(a) The hand-expanded reflection point (`real offset` plus three component statements, a rule-6 violation) is replaced by the /Od-attested genuine `point_from_line3d(&position, &mirror, reflection->offset, &point);`. It is inlined, so relocations stay at 115/115 and no new COMDAT appears.
(b) `real light_brightness = 1.0f;` together with `light_brightness = definition->far_fade_distance > 0.0f ? PIN(...) : 1.0f;`.
(c) /Od-attested named locals: `brightness_upper_bound`, `radius_upper_bound`, `boolean set_texture_result` (stores the rasterizer_widget_set_texture result before the unchanged `break` test), and a separate `short sun_lens_flare_index` counter for the ray_of_buddha loop.

## evidence
- /Od 0x82fa10 (the later first-party build), by address:
  - 0x82fb92: the light variable [ebp-0x6c] is set to 1.0f at block entry.
  - 0x82fcad-0x82fd8b: the assignment goes through a ?: temporary.
  - 0x8308c5: point_from_line3d(&corona_position, &corona_axis, reflection->offset, &point) is a real call.
  - Named slots [ebp-0xc0]/[ebp-0xc4] (brightness lower/upper), [ebp-0xfc]/[ebp-0x100] (radius lower/upper), [ebp-0x105] (set_texture result byte), [ebp-0x140] (second loop counter).
- RTC names match the 9 HCEX aggregates.
- Measurements:
  - The initializer closes the +0x202 site: January loads occlusion_fraction first; the initializer ages light_brightness, in line with A35.
  - The draw function goes from 6 to 5 REAL regions, hash 25834daf -> 39103012.
  - objdiff one-unit: 99.38321 -> 99.38626.
  - Unit rows are unchanged (10 exact).
  - The surplus section set and hashes are identical to production.
  - fake_match_scan: 0 leads; /W3 warnings unchanged.
  - The genuine call without the four locals is 19 REAL. Any 4 of the /Od-attested locals {T,B,R,S,C} reach 39103012; the choice is disclosed in LEDGER k3.
  - The if/else and ?: forms are byte-identical; ?: is taken as /Od-faithful.
- Negative results this wave (the pre-existing site-3 interleave was never moved by any variant):
  - Dead locals at function top, loop body, window block and visible block, D up to 48: inert, or plateaus, never January.
  - Unused aggregates: inert.
  - TU names (extern K=0..40 before draw; prototypes K=1..33 at file top): inert.
  - Visible-block declaration order in /Od order: inert.
  - Scope moves of camera_offset/direction: inert.
  - dot_product3d argument swap (A44): inert.
  - Operand order `occlusion_fraction*light_brightness`: inert.

## blockers
Two residual sites remain:
(1) +0x31c: the i-term of the inlined dot_product3d(&direction, &camera_offset). January loads direction.i first; we load camera_offset.i first.
(2) +0x738..+0x74e: set_texture argument setup (`xor edx,edx; mov dx,[edi+4]`, `push edx`) is interleaved one x87 slot earlier than in January.
No attested lever moved either site. The four /Od locals serve only to keep the fuzzy state and are disclosed; a reviewer may prefer the minimal genuine-call form, which scores lower (19 REAL).

## reopen_criteria
Reopen with either:
- a first-party fact that makes `direction` younger than `camera_offset` at the light_to_camera dot (a later definition or escape of direction, or an early definition of camera_offset); or
- a decoded C2 scheduling key for the integer/x87 interleave at the set_texture argument setup.
Do not replay dead-local, TU-name, declaration-order, scope or argument-order sweeps.

## task notes
No function became strict-exact without an owner ruling, and the object stays blocked by the draw function, submit_for_cluster, and the owner-gated reset.

**Ready to land** (zero credit, park re-baseline): C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_lights\production.patch. It passes git apply --check and touches three files:
- source/rasterizer/rasterizer_lights.c: the P1 storage/layout packet plus the P2 draw changes.
- config/symbols.json: 6 in-place line edits; the full edited copy is scratch/w/rasterizer_lights/config/symbols.json.
- config/parked.json: the draw park re-baseline.

Integrator checklist:
- Run the csplit regen after applying: only rasterizer_lights.obj changes, verified in the slug with 833 objects.
- No header is touched and only this TU changes, so no consumer sweep is needed beyond the normal full ninja.
- Verified at the regen split: symbols match 41/41, PDB storage has 0 disagreements, and the unit keeps 10 exact rows.
- Every surplus COMDAT is identical to production, so the provider-link status does not change.

**Owner-gated:** scratch/w/rasterizer_lights/owner_gated_reset.patch makes reset exact (48). It needs an owner ruling on the preserved overrun; the new evidence is that P1 makes the overrun's neighbour identical to January's.

Laws re-confirmed this wave:
- A35 leaf aging is driven by the first definition: an /Od-attested declaration initializer flipped an x87 frame-scalar operand order.
- A44: dot_product3d argument order is canonicalised and inert.
- The VC7 .bss law: uninitialised statics come first, and the HCEX names force `= {0}` on results.

One process fact for later workers: Windows file names are case-insensitive, so lab files S1..S5 overwrote s1..s3. The ledger records this, and the affected labs were redone as kc_*.

A one-unit objdiff 3.3.1 report reproduces build/report.json exactly and was used for all fuzzy numbers (slug od_proj/).
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_lights\LEDGER.md
