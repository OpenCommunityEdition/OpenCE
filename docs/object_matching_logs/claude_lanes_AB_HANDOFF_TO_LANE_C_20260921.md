# Handoff to Lane C - lessons and tools from Lanes A and B and the data lane (2026-09-21)

Lane C = `docs/campaign_prompts/CLAUDE_REMAINING_LANE_C_GRAPHICS_UI_20260920.md`
(30 units: bitmaps, interface, rasterizer, rasterizer/xbox, render; 83,898
meaningful residual bytes by the brief's count).

This document merges what three finished lanes learned, and adds a census of
Lane C's own units measured today at the frozen base. **Everything marked
"measured" was run on 2026-09-21** against a clean build of `6130661c` (the data
lane's baseline shadow, `C:\halo-worktrees\claude-data-section-credit-shadow`).
Nothing here is pushed. Your worktree is cut from the frozen base, so **you start
with none of the edits described below.**

| lane | worktree | branch | result |
|---|---|---|---|
| A (AI core) | `C:\halo-worktrees\claude-lane-a-ai-core-20260920` | `claude/lane-a-ai-core-20260920` | +19,901 meaningful code, 528 exact / 19 residual, 4 objects function-complete; lane data 98.95% |
| B (simulation/physics) | `C:\halo-worktrees\claude-lane-b-ai-physics-units-20260920` | `claude/lane-b-ai-physics-units-20260920` | +15,572 meaningful / +15,744 padded, 24 functions, 0 regressions |
| data (board-wide) | `C:\halo-worktrees\claude-data-section-credit-20260920` | `claude/data-section-credit-20260920` | +119,772 data bytes, 0 worse rows |

The frozen base lives in the BARE repo
`C:\Users\isabe\Documents\Codex\2026-09-20\files-pasted-by-the-user-halo\work\halo-campaign.git`,
not in `work\halo-exact`. Create your worktree from there.

**Your tool kit is ready**: copy
`C:\halo-worktrees\claude-lane-a-ai-core-20260920\scratch\lane_c_kit\` to your
worktree's `scratch\orch\`. It holds 60 scripts from both lanes, four new ones
written for your unit list, and the base census files quoted below
(`census_6130661c\`). Its `README.md` says what each script does.

---

## 1. Start here - the first hour

1. Bootstrap the worktree (section 12 lists the traps), full `ninja`, then take
   the stable snapshot and `semantic_report.json` your brief requires.
2. Copy the kit. Run `lane_inventory.py`, then `lane_triage.py`, and diff the
   result against `census_6130661c\inventory_base.txt`. They should agree
   exactly; if they do not, your build differs from the base and you must find
   out why before anything else.
3. Put the owner questions in section 2 to the owner **in one batch, on day
   one**. Two of them gate your first commits.
4. Decide the data-lane prerequisite (section 3) before touching any file it
   edits.
5. Land the verified closure in 4.1 as your first code commit.
6. Then work the census in section 4 in the order given, not the brief's
   historical lead list: several of those leads are parked, frame-structural,
   or blocked by the relocation-alias class.

---

## 2. Owner questions to ask on day one

Each was open when Lanes A and B finished. Put them as one batch.

1. **Does Lane B's `_point_from_line3d` ruling bind Lane C?** Lane B's owner
   ruling: a TU may emit the COMDAT *iff January's same object references it out
   of line*. Three of those 17 objects are yours: `rasterizer_lights`,
   `rasterizer_xbox_environment`, `rasterizer_xbox_models`. Your brief's house
   rule 6 predates the ruling. The +400 closure in 4.1 depends on the answer.
2. **Does Lane B's all-inlined COMDAT admission bind Lane C?** Admitted with five
   conditions: a genuine shared-header `__inline`, a byte-identical COMDAT, a
   strict-exact caller, a full ownership and build sweep, and zero regressions. No
   hand expansions. It governs, for example, `_magnitude3d` in
   `first_person_weapons` (4.2).
3. **Does ruling 1's logic extend beyond `_point_from_line3d`?** January's objects
   reference other real_math helpers out of line where ours inline them
   (`_normalize3d` in five of your objects, `_cross_product3d` in two; see 4.5).
   Lane B's ruling named one symbol, and your brief forbids extending
   function-specific rulings. Ask before relying on it.
4. **May Lane C take the data lane's edits to its files as a prerequisite commit**
   (as Lane B did with its six)? One of them, the approved `bitmap_group` field
   rename, touches **ten .c files outside your allowlist** (section 3). Your
   brief forbids editing other lanes' .c files, so this needs an explicit yes, or a
   rule that you wait for the data lane to be merged.
5. **Inline asm in `hud_draw`**: `_fast_ftol_C` (unwritten, 56 B) is a
   hand-written fist/fisub/sbb twin in January, and the data lane wrote
   `_real_argb_color_to_pixel32` with two `__asm` blocks (fuzzy, zero credit).
   Rule 10 allows asm only sparingly, in helpers, with direct evidence. Ask
   whether either may land.
6. **January bugs in parked functions**: `_DecodeBlockRGB__single_pixel`'s
   January assert text is `u<=4` / `v<=4`; ours was deliberately "repaired" to
   `< 4`. The standing rule admits a January bug only once the function is
   strict-exact, with a source comment and layout assertion. The data lane
   reverted its respelling for exactly that reason.

State **every newly emitted symbol** in any ruling request. Lane B's owner
reversed an answer once they learned a landing newly emitted `_interpolate_scalar`;
an answer given without the side effect is not a ruling on it.

---

## 3. What is already done in YOUR files - on the data lane's branch, not yours

The data lane edited 15 Lane C files plus `bitmaps/bitmap_group.h`. Measured
effect on Lane C code rows: **only one row moves** -
`_real_argb_color_to_pixel32` goes UNWRITTEN -> residual `[sha]`. Everything else is
data. But three of the edits change relocation IDENTITY that code will need, as in
Lane B: the assert respellings in `rasterizer_lights` and `rasterizer_xbox_shadows`,
and the `bitmap_group` rename in `rasterizer_xbox_environment_fog` (all three
show up in section 4.4's string diff).

| commit | Lane C files | what | data |
|---|---|---|---:|
| `75186cdc` | rasterizer_lights.c, rasterizer_xbox_shadows.c | assert text respelled to January's literal (`animation_color.red >=0.0f && ...`) | part of +2,500 |
| `69faf435` | bitmap_utilities.c, motion_sensor.c, virtual_keyboard.c, render_cameras.c | `= 0` / `= {0}` / `= FALSE` on globals (the `.bss` layout law), render_cameras also reordered to January's order | part of +4,834 |
| `61988b12` | rasterizer_xbox_environment.c | `specular_light_vertex_shader_permutation_index` DEFINED here `= NONE` instead of `extern` | part of +634 |
| `ff162a1b` | ui_widget.c, rasterizer_geometry.c | `SECONDS_PER_MILLISECOND` moved before the memory pool; `rasterizer_vertex_type_sizes[]` unsized with a trailing `0` | part of +71,004 |
| `a37fd956` | ui_widget.c | `real const _one_over_255 = 1.0f / 255.0f;` (owner-approved, January PDB-verified) | part of +9,986 |
| `17c0d630` | hud_draw.c | wrote `_real_argb_color_to_pixel32` (bitmaps_inlines.h copy, **two `__asm` blocks**); 308/308 B, identical relocations, two frame slots swapped; zero code credit | +380 |
| `67208c6f` | bitmap_group.c/.h, hud_draw.c, hud_weapon.c, interface.c, ui_widget.c, rasterizer_xbox_environment_fog.c, render_sprite.c **and out of scope:** bitmap_extract.c, predicted_resources.c, cinematics.c, decals.c, object_lights.c, lightning.c, rasterizer_cinematics.c, rasterizer_xbox.c, rasterizer_xbox_detail_objects.c, rasterizer_xbox_transparent_geometry.c | owner-approved field rename `bitmap_data` -> `bitmaps` (January asserts + HCEX type); 45 consumers swept | +1,356 |

Lane C data went **458,655 -> 489,089** of 526,957 B (+30,434) on that branch.
What still blocks the remaining 37,868 B, per the data lane:

| section | B | blocker |
|---|---:|---|
| progress_bar .data | 18,417 | unwritten 18 KB `compressed_monk_tone` blob |
| progress_bar .bss | 500 | name `wobble_phase` unattested - **owner-held** |
| bitmap_group .rdata / .data | 6,444 / 1,424 | unwritten tag definitions |
| ui_widget .bss | 2,224 | aggregate "storage" struct mirroring a naming gap; needs per-variable reconstruction |
| rasterizer_frame_statistics .bss | 2,144 | same as ui_widget |
| render_particles .data | 1,528 | not diagnosed (99.74%) |
| render_cameras .rdata | 1,216 | `_pi` folding in real_math.h |
| bitmaps .data | 1,116 | not diagnosed |
| render_sprite .rdata / .data | 852 / 24 | not diagnosed |
| rasterizer_xbox_environment_fog .bss | 661 | not diagnosed |
| motion_sensor .rdata | 464 | `_pi` folding; parked function |
| interface .rdata | 328 | not diagnosed |
| rasterizer_xbox_models / environment / shadows .bss | 191 / 181 / 74 | not diagnosed |
| s3tc .rdata | 76 | January's `v<=4` assert (owner question 6) |
| hud .data | 4 | not diagnosed |

The "not diagnosed" rows are the only data work left that is plainly yours. Use
`secdiff.py` (section 7).

---

## 4. Your units at the frozen base - measured census

`census_6130661c\inventory_base.txt`: **692 exact / 77 non-exact / 88,712 padded
bytes** across the 30 units. Of the 77: 42 are parked (47,200 B), 4 unwritten
(2,920 B), 28 carry a size tag (40,544 B), 38 are `[sha]` only (33,600 B), and 5
are `[reloc-identity]` (4,224 B). 18 functions have a differing frame
(`sub esp,N`), worth 28,208 B. No Lane C unit emits `_point_from_line3d` or
`_random_range` at the base.

**Twelve objects are ONE function short**: bitmap_group (1,008),
bitmaps (1,296), first_person_weapons (1,536), hud (1,744), hud_messaging (2,928),
hud_nav_points (1,632), progress_bar (352), rasterizer_xbox_active_camouflage
(2,016), rasterizer_xbox_hardware_bitmaps (400), rasterizer_xbox_hardware_geometry
(352), rasterizer_xbox_lights (2,352), render_particles (1,168). Function-complete
is still not whole-object `Matching`; your brief's admission audit applies.

### 4.1 Verified closure: +400 B by deleting two lines (plus one park entry)

`source/rasterizer/rasterizer_lights.c:83` `#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D`
and `:103` `#undef ...`. Measured with both lines dropped in memory:
`_rasterizer_lens_flares_submit_occlusion_tests` goes `[sha]` -> **EXACT**; the unit
goes 9 -> 10 exact; **no other row in the unit moves**. The object then emits
`_point_from_line3d` (48 B at `.text` offset 0, byte-identical to January's folded
copy per Lane B's `admit_check.py`), so `--forbid-emitted-symbol
_point_from_line3d` now FAILS - which is exactly what owner question 1 must
clear.

**Lane B's note missed one thing: this function is PARKED** (`config/parked.json`
line ~2888, class `unclassified`, objdiff 96.3871). A function that becomes exact
must be unparked in the same commit or ninja's PROGRESS gate fails. Use
`unpark_entry.py` (line surgery, brace-depth aware). Then run a full ninja and a
whole-board row diff, because Lane B measured this TU in isolation.

### 4.2 The inline-suppression switches - `census_6130661c\switch_probe_base.txt`

A `REAL_MATH_EXTERNAL_*` switch forces EVERY site of a header inline out of line.
January's source had no such switches; they were added to satisfy the old
emission guard. Removing one is the authentic shape, but it lands only when a
caller goes strict-exact and the owner rulings cover the emitted COMDAT.

| unit | switch dropped | moved row |
|---|---|---|
| rasterizer_lights | POINT_FROM_LINE3D | `_rasterizer_lens_flares_submit_occlusion_tests` -> **EXACT** |
| rasterizer_xbox_environment | POINT_FROM_LINE3D | `__rasterizer_environment_lightmap_draw` `[size 3984!=4016, relocs 246!=248]` -> `[relocs 244!=248]` (size now equal) |
| rasterizer_xbox_environment | SCALE_VECTOR3D | same function -> `[relocs 242!=248]` (size equal) |
| rasterizer_xbox_environment | both | -> `[size 4048!=4016, relocs 240!=248]` (overshoots) |
| rasterizer_xbox_models | POINT_FROM_LINE3D | no row moves |
| first_person_weapons | MAGNITUDE3D | `_first_person_weapon_update` `[size 1520!=1536, relocs 60!=59]` -> `[relocs 61!=59]` (size equal) |
| first_person_weapons | REAL_LOCAL_RANDOM | same -> `[relocs 61!=59]` (size equal) |
| first_person_weapons | all six | -> `[size 1568!=1536, relocs 60!=59]` |
| first_person_weapons | SIGNED_ANGULAR_DIFFERENCE, SQUARE_ROOT, MAGNITUDE_SQUARED3D, REAL_LOCAL_RANDOM_RANGE | no row moves |

Ownership (Lane B's `inline_ref_census.py`): January's `first_person_weapons`
references `_real_local_random`, `_real_local_random_range` and
`_signed_angular_difference` out of line but **not** `_magnitude3d` (January
inlined every site); January's `rasterizer_xbox_environment` references both
`_point_from_line3d` and `_scale_vector3d` out of line. So in `lightmap_draw`
January kept SOME sites of each helper out of line, and neither all-in nor all-out
reproduces it. It is a per-site inline question - see the inline frequency law in
section 6.

### 4.3 The cheapest-looking bytes are probably a relocation alias - check before spending

Four `ui_widget_game_data_input_functions` residuals have **zero differing code**
(triage: real regions 0) and differ in one relocation's symbol only:
`_player_profile_1wide_list_update` 1,152, `_player_profile_3wide_list_update`
1,232, `_player_profile_update_cache_for_nwide_list` 272 (January spells
`_local_player_index_for_draw_string_and_hack_in_icons`, ours `_cached_player_profile`),
and `_splitscreen_pregame_status_screen_update` 1,216 (January spells a string
literal, ours `_local_player_controller_bitmap_frames`). In `symbols.json`,
`_cached_player_profile` is at 5841376 and the next symbol at 5841532 - 156 B
later, i.e. `cached_player_profile[3]` of 52-byte entries, so January's spelling is
**one past the end**.

That is the known `csplit-relocation-alias` class: the splitter attributes a
one-past-the-end address to the NEXT image symbol, and no C source makes VC7 spell
it that way. A previous lane chased one and produced a false declaration of another
module's type. Resolve both spellings through `symbols.json` image addresses; if
they agree, the disposition is the `csplit-relocation-alias` park class (12 such
parks exist at the base), which **does not earn credit**. Do not open a wave on
these 3,872 B. `_bitmap_sharpen` (352, parked, `-_extract_data
+_bitmap_sharpen_positive_table`) may be the same class; check it the same way.

### 4.4 String-literal fidelity - `census_6130661c\fn_string_diff_base.txt`

Assert and error text fixes the exact source spelling. Seven functions differ:

| function | January text (ours differs by) |
|---|---|
| `_bitmap_group_add_bitmap` | `space_between>=0`, `(byte*)bitmap->base_address>=...` (no spaces; ours spaced) |
| `_DecodeBlockRGB__single_pixel` | `u>=0 && u<=4` (ours `< 4` - owner question 6) |
| `_hud_update_weapon_local_player` | `too many levels in current weapon hud hierarchy` - ours says `HUD` at `hud_weapon.c:782` (line 1067 already has `hud`) |
| `_splitscreen_pregame_status_screen_update` | the alias above, not a text difference |
| `_rasterizer_lens_flares_draw` | five asserts - fixed on the data lane (`75186cdc`) |
| `__rasterizer_environment_fog_screen_begin` | `bitmap_group->bitmaps.count` (rename `67208c6f`); also January-only `alpha>=0.0f && alpha<=1.0f` and `..\bitmaps\bitmaps_inlines.h` - January inlines a bitmaps_inlines.h helper we do not |
| `__rasterizer_environment_shadow_begin` | three asserts - fixed on the data lane (`75186cdc`) |

Measured: the `HUD` -> `hud` fix alone moves no row (the function has a structural
gap, 4.6), so it is a zero-credit fidelity prerequisite, not a closure.

### 4.5 Inline-boundary divergences - `census_6130661c\inline_boundary_base.txt`

External references January's object has and ours does not (and vice versa),
ignoring literals. The systemic ones:

- **`_normalize3d` called out of line by January** in rasterizer_lights,
  rasterizer_xbox_environment, rasterizer_xbox_lights, render_cameras,
  render_sprite; `_cross_product3d` in rasterizer_lights and render_sprite;
  `_plane3d_distance_to_point` in rasterizer_xbox_models and render_cameras;
  `_magnitude_squared3d` in render_sprite. Ours inlines them all. This is the
  known "real_math per-caller inline boundary" family: January calls a header
  inline out of line at SOME call sites. Earlier lanes could not reproduce it by
  .c-level changes; the Lane A inline frequency law (section 6) is the candidate
  mechanism. Owner question 3 applies before any of these emits a COMDAT.
- **`bitmaps_inlines.h` pixel helpers are inverted.** In rasterizer_xbox_environment
  and rasterizer_xbox_environment_fog, January calls `_real_rgb_color_to_pixel32`
  (and in environment `_real_alpha_to_pixel32`) out of line. Ours inlines them, which
  is why ours references `_csprintf`, `_temporary`, and `_valid_real_rgb_color`
  (their assert bodies) and January does not. In hud_draw it is the other way
  round: January references `_valid_real_argb_color`.
- **`_D3DSIMPLERENDERSTATEENCODE`** is referenced by January in six
  rasterizer_xbox objects (active_camouflage, environment, environment_fog, lights,
  models, shadows) and by none of ours - an XDK header inline path we do not take.
  One cause across six objects; find it once.
- `_object_get_bounding_sphere` (January, hud_nav_points and motion_sensor) where
  we call `_object_get_and_verify_type`: a missing real helper call.
- interface: January references ~30 profile/debug symbols - the two UNWRITTEN
  functions `_render_debug_profile` (2,272) and `_render_debug_profile_stall_tick`
  (272).

### 4.6 Structural leads from the triage - `census_6130661c\triage_base.txt`

- **Frame differs (structural, never a tie)** in 18 functions, e.g.
  `_hud_update_weapon_local_player` (January frame 64 B BIGGER, and January has
  no `_csmemset` call where we do - a zero-initialised 64-byte aggregate spelled
  as a memset?), `__rasterizer_environment_fog_screen_begin` (ours 0x44 bigger),
  `_hud_messaging_update` (ours 0x28 bigger), `_render_camera_build_frustum` (ours
  0x18 bigger), `_motion_sensor_update` (ours 0x14 smaller),
  `_rasterizer_environment_specular_spot_light_begin` (ours 0x10 smaller), `_bitmap_2d_get_pixel`, `_build_sprite_rotational`, `_build_sprite`,
  `_EncodeBlockRGBColorKey`, `_rasterizer_sun_glow_draw`, `_bitmap_3d_smooth`,
  `_update_motion_sensor`, `_render_blip`, `_hud_update_weapon`, `_crosshairs_draw`,
  `_RGBToColor`, `_build_sprite_transform_origin_and_direction`.
- **Return count differs** (single-exit law): `_bitmap_2d_get_pixel` (January 6,
  ours 7) and `_eat_my_shorts` (January 2, ours 1). Both parked.
- **hud_weapon's shared signature**: four of its five residuals carry ONE extra
  relocation on our side, and three of those are also 16-32 B short (`_hud_update_weapon` references
  `_hud_update_weapon_local_player` and `_csmemset` where January references
  `_object_get_and_verify_type`). Look for one shared cause before five separate
  waves.
- **Reference-multiset leads** (a constant January uses that we do not pins the
  source expression): `_render_blip` wants `__real@3fe6666666666666` (the DOUBLE
  0.7) where ours emits the float-widened `3fe6666660000000`;
  `_build_sprite_rotational` and `_render_camera_build_frustum` differ in the last
  ulp of a float literal (`3ecf817a` vs `...7b`, `40490e38` vs `...37`) - a
  constant spelled differently (a product or a quotient, as `0.35f*0.35f` was in
  Lane B); `_render_weapon_hud` references `__ftol2` that January does not;
  `_crosshairs_draw` calls `_get_flash_color` that January inlines.
- **Branch-width noise**: on the biggest functions (`__rasterizer_model_draw`
  5,168 B at `+3` code bytes, `_rasterizer_frame_statistics_draw` 4,176 B with
  equal size), padded size partly measures chaotic branch width; judge by
  width-normalised length (`widthcensus.py`).

### 4.7 Header sensitivity - your functions are declaration-count ties on real_math.h

Lane B measured one added prototype in `math/real_math.h` (nothing else). It made
`_bitmap_2d_alpha_bleed` (560, parked) EXACT, but broke nine exact functions
board-wide, including your `_bitmap_2d_uncompress_from_mipmap`. It also moved the
bytes of your parked `_bitmap_group_add_bitmap` and
`__rasterizer_model_transparent_geometry_submit`. Reverted. So
`_bitmap_2d_alpha_bleed` is a declaration-COUNT tie, not a source defect: do not
grind spellings on it. Your brief already bars committing `real_math.h`; keep any
such need as a proposed patch with a full census.

---

## 5. Owner rulings from Lanes A and B - confirm they bind Lane C

| # | ruling | lane |
|---|---|---|
| 1 | `_point_from_line3d` may be emitted iff January's SAME object references it out of line (17 objects; `units` is therefore NOT a violation - **this supersedes Lane A's handoff to Lane B, which said it was**) | B |
| 2 | All-inlined header-inline COMDAT class admitted on five conditions (genuine shared-header `__inline`, byte-identical COMDAT, strict-exact caller, full sweep, zero regressions); no hand expansions | B |
| 3 | Narrow `_random_range` exception (January's copy lives only in action_obey.obj) | A |
| 4 | Manual expansion of a helper whose real call is barred: refused | A, B |
| 5 | A named TU-private accessor macro admitted ONCE, on a specific showing (conventional parenthesisation, idempotent, every undecorated form measured and failing, first-party-attested name). A decoration-only parenthesis with no semantic or naming evidence is steering: refused; the `collisions.c:1572` precedent is itself under review | A then B |
| 6 | Rule 24 (punning): a NEW view cast only with per-site /Od attestation, byte-inert cast, compatible prefix layout, strict exactness and ledger disclosure | B |
| 7 | Prototypes live in the genuine owner header, never a one-consumer header or a consumer-local stand-in | A, B |
| 8 | No `#pragma` steering, ever | A |
| 9 | Semantically inert steering is rejected (a store, flag, reference or dummy local whose only effect is bytes) | A |
| 10 | A January bug (OOB or uninitialised read) is admissible ONLY once the function is strict-exact, with January's instructions quoted, a source comment and a `typedef char name[cond ? 1 : -1];` layout assertion; refused twice while fuzzy | A |
| 11 | Correct source beats a misleading byte score: a proven defect lands even if byte-worse, at zero credit | A |
| 12 | A load-bearing but byte-invisible fact goes in the ledger, not an intrusive source comment | A |
| 13 | A switch-for-if recovery only at the byte-proven site | A |
| 14 | An unreferenced global January defines: admitted only with verified owner, type, value and linkage from January's own `cachebeta.pdb`; unnamed globals HELD | data |
| 15 | Header field renames: approved when backed by January literals plus HCEX type, with a full consumer sweep | data |

---

## 6. Compiler laws (VC7 13.00.9254) - target-proven, merged from both lanes

- **Frame size first.** A differing `sub esp,N` is structural: a local's scope
  or type, or a missing or extra aggregate. It is never a tie. About 15 Lane B
  functions were frame-wrong, and every frame fix was source-reachable.
- **Single-exit law.** January returns once at the bottom far more than we do.
  With ONE `return`, VC7 splits the bottom return per incoming edge before
  building webs; the return-statement COUNT is the lever, not the flag spelling.
  Count January's `ret` epilogues against our return statements first. This
  closed four Lane B functions and Lane A's `_actor_create_for_unit` outright.
- **Return width (16-bit only).** In a `short` function `return <const>` emits
  `mov ax,K`; a constant assigned to a single-exit accumulator emits `mov eax,K`.
  The declared return type flips callers to cleanup-first; a wrong type on an
  already-exact callee blocked two callers (+8,400 B in Lane A). Inert at 8 bits.
- **Reference multiset.** A constant January references and we do not pins the
  source expression: `__real@3f847ae140000000` is `(double)0.01f`, a float literal
  widened, not the double `0.01`; `0.35f*0.35f` is not `0.1225f` (one ulp). A
  missing helper reference means a missing or inlined call.
- **Replace hand expansions with the real header helper.** Three sites in
  Lane B's path.c took the whole unit to 32/32.
- **Inline frequency law.** A header `__inline` stays OUT of line in a `?:` arm,
  in the right operand of `&&`/`||`, or inside an extra nested `if`; calling the
  helper (not its expansion) adds its parameters as IL values and raises block
  pressure. This is the candidate mechanism for January's per-site out-of-line
  calls in 4.2 and 4.5.
- **A switch can be load-bearing for an ALREADY exact function**: in bipeds,
  removing `REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` lost `_biped_update_jumping`.
  Always row-diff the whole unit.
- **Jump threading** carries branch knowledge from `if` compares, not from
  `switch` case edges; a redundant re-test surviving in January is evidence of a
  switch-shaped test.
- **Constant web benefit gate.** A constant keeps a register only if its benefit
  is positive (each materialisation -1, each use +1 only when the co-operand is in
  memory). That decides "January pins -1, we rematerialise".
- **Disjunction arm order**: `if (A || B) {X} else {Y}` lays out the ELSE arm
  first. **Hoisted blocks are source-reachable** (96 functions reproduce them).
- **Inside one function** declaration order, grouping and scope depth are inert;
  the declaration-count effect acts only across a shared header.
- **Genuine bugs surface as "ties"**: `_path_state_estimated_distance` iterated
  the wrong index variable, and its "three-register cycle" was the symptom.
- **Branch width is non-local and chaotic** on big functions: one deleted
  statement flipped 29 branches. It is not a lever.
- **The register allocator is decoded** (Lane A): priority = sum of IL
  references x block weight, ties to the earlier first reference, eax-first
  minimum-cost choice. A replay oracle reproduces 6,638 of 6,646 decisions. **Use
  it to explain a residual, never to rule one out** - it called a function
  unreachable that was then closed.
- Rasterizer-specific, from earlier lanes (read the memories/ledgers before
  touching x87 or SSE): `/QIfist` always emits a 64-bit `fistp qword`, so a
  32-bit inline `fistp` in January is the `fast_ftol` asm helper
  (`cseries.h:309`, already in production); named `__m128` locals force the
  alignment preamble and expression forms cost two `movaps` per row;
  comma-joined store intrinsics drop the first store.

---

## 7. Evidence sources, and what each covers for YOUR units

- **The /Od + /RTC first-party build**
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\symbol-build-h1-tags-20260906\halo_cache_symbols.exe`,
  read (never executed) through `odbuild.py`. It gives statement order, which
  helpers are real calls, every local as a slot, return sites, assert lines, and
  ORIGINAL names of address-taken aggregates via RTC descriptors. Most of Lane B's
  closures started there, several exact on the first compile. **Coverage for
  Lane C** (functions referencing each `__FILE__` string, a lower bound):
  bitmap_utilities 43, ui_widget 36, render_cameras 13, first_person_weapons 10,
  hud_weapon 8, motion_sensor 8, hud_messaging 5, s3tc 1; **render_sprite,
  rasterizer_lights, rasterizer_geometry and all rasterizer_xbox_* : 0** - that
  code is Xbox/D3D-specific and absent from the Win32 build. It is a 2020-era
  build with source differences: names, types, topology only, never byte proof.
  Confirm every fact against January's frame size, reference multiset, ret count
  and store order, and quote both.
- **HCEA compilands**: `DIA2Dump -compiland "..\..\..\build\x360\SHIP\halo\<unit>.obj" HCEX.pdb`
  (and `RELEASE_CACHE` with `HCEX_Release.pdb`) list every function prototype,
  parameter and local with types. DIA2Dump:
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\`;
  PDBs under `research\hcea_jun2011_prototype\payload\Halo CE Anniversary (Jun 24 2011)\`.
  Run from PowerShell with `| Out-File -Encoding utf8`. This is first-party
  evidence for a frame-size mismatch.
- **January `cachebeta.pdb`** publics (linkage) and section contributions (owner,
  extent): grep `] _name(`, never a whitespace-anchored name. It has no Halo
  locals.
- **`config/contribs.json`** is January's section-contribution table.
- For rasterizer/xbox, with no /Od coverage: January's bytes, the XDK headers
  (the `_D3DSIMPLERENDERSTATEENCODE` question in 4.5 lives there), and the HCEA
  compilands.

---

## 8. Method - what actually closed functions, in order of yield

1. **Read January first** - the /Od readout where it exists (7), January's bytes
   everywhere, BEFORE compiling.
2. **Triage table first**: frame, `ret` count, reference multiset, real regions
   (the kit's `lane_triage.py`). Use the gate tag: `[reloc-identity]` means naming
   or alias (4.3); a size tag or a frame difference means structural; `[sha]` at
   equal size means ordering or selection.
3. **A byte ledger that sums exactly** before trying shapes. Lane A's waves that
   required one closed functions; one 144-byte function earlier took 64 shapes
   for nothing. Register-blind diffs only: raw region counts overstate badly (43
   regions became 1 decision).
4. **Data before code grinding.** objdiff credits a data section all-or-nothing;
   one 1-byte `'/'` held back 3,920 B in Lane A. Find sections at 95-99.99% in
   `report.json`, diff with `secdiff.py` (relocation sites separated from
   content), and never census data by symbol.
5. **Re-open "impossible" before grinding.** Lane A's two biggest wins
   (+8,400 and +11,600) re-opened items whose premise had changed. Before writing
   "impossible", name a dimension you did not vary and look for a counterexample
   across the whole board.
6. **Verify every agent claim yourself.** Verifiers and orchestrator gates caught a build-breaking stale
   park, a false "equal size", wrong guard claims, 21 reviewer-approved view casts,
   and an empty `if` arm holding only a comment.

---

## 9. Landing gates and build hazards

- **Land through `land.py`** (kit): git-clean base, unitcheck row diff with ZERO
  exact loss, guard, newly-defined-symbol ownership against January, fake-scan
  DELTA, line endings. Then `castcheck.py` and, for any new COMDAT,
  `admit_check.py`.
- **A function that becomes exact must be unparked in the same commit**, and one
  whose bytes MOVE while parked must be re-measured, or ninja's PROGRESS gate
  fails. It happened twice in Lane A and once in a Lane B intermediate commit.
  `unpark_entry.py` / `refresh_parks.py`, line surgery only, never re-serialise
  `parked.json`.
- **`symbols.json`**: CRLF, line surgery only, never sort (alias precedence at
  shared offsets once flipped 46 functions); hash all 833 `build/split` objects
  before and after a re-split.
- **Row-diff the whole board**, not the tally (`snapshot.py`: all ~11,060
  function and 1,706 section rows). An aggregate count hides swaps.
- **Check every newly DEFINED symbol** against January's split object. Header
  inline COMDATs that production ALREADY emits (`_scale_vector3d` in about 60 objects)
  are systemic, not violations.
- **Asm/vendored parks are never unparked** (`_stristr`, `_matrix4x3_multiply`,
  `__frnd` at the base; `tools/parked_functions.py` inverts the check for them).
- The stable-verdict snapshot lists EVERY function with status E or R; "gained"
  means status flipped, not key appeared.

---

## 10. Agent economics

- One worker plus one reviewer per `.c` cost **165K-330K tokens per agent** in
  Lane B. Lane B wave 1 (30 units, 8 concurrent) finished 21 agents on 6.9M tokens
  before the session limit killed the other 20 and blocked the orchestrator.
  Wave 2 (5 concurrent, hard shape caps, word-limited result fields, a shared
  pre-built evidence tool) ran 33 agents on 5.5M tokens without hitting the limit.
- Owner cap: never more than 8 concurrent agents; Lane B's owner later cut it to 3,
  2, then 0. **Give the owner the estimated cost BEFORE a wave.**
- Build the limiter INTO the workflow script: N queue-pulling lanes, worker THEN
  reviewer inside a lane so reviewers count against N, queue ordered
  highest-yield first.
- Workers must be resume-aware ("if candidate.c exists, do not overwrite it") and
  append to REPORT.md after EVERY attempt. A stopped workflow caches nothing; Lane
  B recovered a finished 3-function candidate from disk only because it was
  written there.
- "No more agents" means no NEW agents: do not stop running ones.
- Landing gates, admission checks, /Od attestation and header sweeps were all
  cheap orchestrator work without agents.

---

## 11. Measured dead - do not spend time here

Loop spelling in general (7,709 of 7,729 functions already agree); declaration
order, grouping or scope depth inside a function; `&&` versus nested `if`; operand
order of `*`; case order and braces in a compare-chain switch; the arm order of
`if (A || B) ... else`; the branch-width "fixpoint"; the return-width law at 8
bits; the COMDAT selection flag (csplit writes no-duplicates for all 8,223
function COMDATs); history mining (Lane B: `branch_sweep.py` over 31 units found
no historical blob better than the base; the board-wide history pass returned only
policy-rejected bodies); a TU-local stand-in prototype or one-consumer header for
a real_math prototype (refused); choosing a header position because it spares the
victims (coincidence-hunting).

---

## 12. Environment traps that cost real time

- `git worktree add` from the bare repo. The `xbox` junction:
  `(Get-Item xbox).Target` is an ARRAY - cast it before `New-Item -ItemType
  Junction`. After EVERY `configure.py`, patch `build.ninja`'s `rule cl` to an
  absolute CL path (`patch_ninja_cl.py`); a relative CL path breaks through the
  junction.
- `gate.py` takes the unit WITH the `source/` prefix
  (`gate.py source/rasterizer/rasterizer_lights --all`), and `--edits` matches
  text-mode source, so use `\n`, never `\r\n`.
- Bash heredocs eat one backslash level (`\b`, `\x36`, `'\\'` were silently
  corrupted four times): write scripts with a file tool or build backslashes with
  `chr(92)`. PowerShell eats `$L...` inside `python -c "..."`. Git Bash mangles
  `--cflag /X`: run flag probes from PowerShell as `--cflag=/Ob0`.
- Source files and `symbols.json` are CRLF; `cat -A` does not show it (count
  `b'\r\n'` in Python). Edit by byte-level replace. PowerShell redirection
  writes UTF-16 with a BOM. Copied logs need trailing-whitespace cleanup or
  `git diff --check` fails.
- A Workflow `scriptPath` must be a path the tool itself issued; an apostrophe in
  the single-quoted `meta.description` is a parse error.
- `| Select-Object -First N` on a Python pipe yields a harmless non-zero exit.

---

## 13. Where the records are

- Lane A: `C:\halo-worktrees\claude-lane-a-ai-core-20260920\docs\object_matching_logs\claude_lane_a_*`,
  especially `..._ai_core_rejected_hypotheses_20260920.md` (R1-R27),
  `..._data_section_credit_20260920.md` and `claude_lane_a_HANDOFF_TO_LANE_B_20260921.md`.
- Lane B: `C:\halo-worktrees\claude-lane-b-ai-physics-units-20260920\docs\object_matching_logs\`
  `claude_lane_b_HANDOFF_20260920.md` (its section 7 was written for you),
  `..._rejected_hypotheses_20260920.md` (RB1-RB8 + a per-function table),
  `..._admission_manifest_20260920.json`, and `..._attempt_logs_20260920\` (every
  shape with its gate line; search it for a function name before spending a shape).
- Data lane: `C:\halo-worktrees\claude-data-section-credit-20260920\docs\object_matching_logs\claude_data_section_credit_20260920.md`
  and `.json` (every touched file, by lane).
- Earlier Lane C-family lanes to search by function name: `opus-30k-rasterizer-widgets-20260905`,
  `opus5-*`, `fable-exact-finish-20260829`, `fable-lens-flares-20260828`.
