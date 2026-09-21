# Claude Lane B handoff - AI actions/path, game, physics/collision, units (2026-09-21)

| | |
|---|---|
| worktree | `C:\halo-worktrees\claude-lane-b-ai-physics-units-20260920` |
| branch | `claude/lane-b-ai-physics-units-20260920` (local only, never pushed, never merged) |
| git database | the BARE repo `C:\Users\isabe\Documents\Codex\2026-09-20\files-pasted-by-the-user-halo\work\halo-campaign.git` (the frozen base does NOT exist in `work\halo-exact`) |
| frozen base | `6130661c9585fdb2785b3ff6bdb2ad0cfef0520c` |
| state | clean tree; canonical checkout, other lanes, vendor code and remotes untouched |

## 1. Result

| measure | frozen base | final | delta |
|---|---:|---:|---:|
| Halo strict meaningful code bytes | 1,443,979 | **1,459,551** | **+15,572** |
| Halo functions | 7,291 | 7,315 | **+24** |
| stable-verdict exact rows (whole tree) | 7,431 | 7,455 | +24 / **0 regressions** |
| newly strict PADDED bytes (stable diff) | | | +15,744 |
| lane board (gate.py) | 850 exact / 61 non-exact / 75,776 B | **874 / 37 / 60,032 B** | |
| parks | 184 / 0 stale / 0 invalid | 168 / 0 / 0 | 16 retired as exact |
| pytest `tools` | 1,152 passed / 5 skipped | see section 6 | |

The per-function ledger in the manifest sums EXACTLY to the build's own Code delta
(15,572), and three independent sources agree on the 24 functions: the gate inventory,
the stable-verdict diff against the frozen baseline, and the build. The brief's target
was +50,000; this lane delivered 31% of that. Section 5 says precisely why the rest is
not closed and what each function needs. No function was declared unmatchable.

Function boards now complete (objects NOT marked `Matching` - the whole-object
code/data/ownership audit was not done, and several have owner-held data names):
`path` 32/32, `action_obey` 27/27, `action_converse` 9/9, `actor_type_flood` 2/2,
`path_smoothing` 6/6, `path_structure_bsp` 6/6, `game_engine_king` 29/29,
`game_engine_race` 36/36, `player_control` 49/49, `stack_memory_pool` 33/33,
`wind` 9/9, `unit_dialogue` 14/14. The admission audit now lists 23 candidates
(19 at base), 0 contradicted.

## 2. Commits, in order (each message carries units, bytes, evidence, reviewer result)

1. `7583df5d` prerequisite - the data lane's six Lane B hunks taken VERBATIM (zero Lane B
   credit; diff against `claude/data-section-credit-20260920` is empty so the branches
   merge cleanly). Three of them change relocation IDENTITY that code needs.
2. `01e16607` wave 1 strict: 6 functions, +2,727.
3. `47934104` wave 1 zero-credit fuzzy (infection, bipeds, physics).
4. `d57ba30c` wave 2 strict: 9 functions, +4,715.
5. `74c9cb2a` wave 2 zero-credit fuzzy (path, collisions, action_charge).
6. `f0b191bc` owner COMDAT admission: 3 functions, +2,545.
7. `3ba2eb91` owner per-site rule-24 admission: 3 functions, +2,802.
8. `ccf3b412` wave 3 strict: path.c complete, 3 functions, +2,783.
9. this documentation commit.

Known blemish: in `d57ba30c` one park-refresh hunk for the path functions rode along
with the unparks, so THAT intermediate commit fails ninja's PROGRESS gate on its own.
Every wave TIP was fully built and swept. Do not bisect across it with the park gate on.

Changed files (all inside the allowlist, plus `config/parked.json` and lane docs):
ai/{action_charge, action_converse, action_obey, action_vehicle, actions, actor_combat,
actor_type_flood, actor_type_infection, path, path_smoothing, path_structure_bsp}.c,
game/{aim_assist, game_engine_king, game_engine_race, player_control}.c,
memory/stack_memory_pool.c, physics/{collision_debug, collisions, physics}.c,
scenario/wind.c, units/{biped_limp_noodle, bipeds, unit_dialogue, vehicles}.c.
**No header, no `symbols.json`, no build flag, no tool, no semantic override changed.**

## 3. Owner rulings made in this lane (2026-09-21) - ask whether they bind the other lanes

1. **`_point_from_line3d` guard relaxed narrowly**: a TU may emit it iff January's SAME
   object REFERENCES it out of line. There are 17 such objects (list in section 7).
   `units` (189/189) emitting it is therefore NOT a violation - Lane A's handoff said it
   was, and its only "fix" cost seven exact functions.
2. **All-inlined header-inline COMDAT class ADMITTED**, on proof: "Require a genuine
   shared-header __inline, byte-identical COMDAT, strict-exact caller, full
   ownership/build sweep, and zero regressions. No hand expansions." Proof accepted:
   (i) VC7 13.00.9254 with the real flags emits the out-of-line COMDAT for a non-static
   `__inline` whose EVERY site inlined, and nothing when unused; (ii) in January's own
   image 65 of 104 header-inline COMDATs are owned by an object holding ZERO references
   to them, the owners clustering at the earliest link-order TUs.
3. **Rule 24 (no representation punning) - per-site admission**: "Admit only with
   per-site /Od attestation, byte-inert cast, compatible prefix layout, strict
   exactness, and ledger disclosure."
4. **Prototypes live only in the genuine owner header** ("Use real_math.h ... Do not
   invent a one-consumer header"), with a full sweep and zero regressions.
5. **A decoration-only parenthesis is codegen steering** - not admissible; the
   `collisions.c:1572` precedent is to be revisited separately.
6. An owner answer given without a side effect spelled out is not a ruling on it: the
   owner first said "land the human-plane", then - told it newly emits
   `_interpolate_scalar` - reversed to "hold", and later admitted the class on proof.
   State EVERY newly emitted symbol in any ruling request.

## 4. Ready-but-held candidates (exact in scratch, zero credit)

| function | bytes | file | status |
|---|---:|---|---|
| `_update_alien_fighter_physics_new` | 1,088 | `scratch/lane_b/orch_land/vehicles_fighter.c` | passes EVERY condition of rulings 1, 2 and 3 (COMDATs byte-identical; /Od fn 0x8f6a20 pushes `&vehicle->object.translational_velocity` and `&desired_rotation` straight into cross_product2d at 0x8f6cbf-0x8f6ccd; casts stripped -> still exact; 0 regressions). Held ONLY because the owner said twice "keep the fighter held" before its site was attested. One "yes" lands it. |
| `_player_aim_projectile` | 832 | `scratch/lane_b/w/aim_assist/candidate.c` (or `review/rv_P3_od_declorder.c`) | exact, /Od-attested statement for statement; needs the `pin_normal_to_cone3d` prototype, and that real_math.h edit regresses NINE exact functions (rejected-hypotheses RB1). |
| `_player_examine_nearby_device` | 176 | `scratch/lane_b/w/players/candidate.c` | exact only with a decoration-only parenthesis - refused (RB2). |
| `_action_vehicle_find_destination` | 1,232 | `scratch/lane_b/w/action_vehicle/review/landing.c` | NOT exact (`[sha]`, equal size); 16 new view casts in a fuzzy body - not admissible. |

For owner awareness (LANDED, first-party attested, revert on request): two locals that
look like steering but are named/attested by the /Od build - `short jet_marker_count = 0`
in `_create_ghost_effect` (01e16607) and `struct path_node previous_node_values`, written
once and never read, in `_path_state_traverse` (ccf3b412; the /Od RTC descriptor NAMES it).

## 5. What is left, and why

37 functions / 60,032 padded bytes. Every one had at least one fresh evidence pass and
has a SPECIFIC measured blocker plus reopen evidence in the rejected-hypotheses ledger's
generated table (and in the manifest's `remaining` array). By size:
`_biped_update_physics` 5,376, `_physics_update_old` 5,168, `_collision_move_point`
4,752 (SIX bytes = one transposition of two independent instructions from January),
`_collision_debug_render` 4,192, `_biped_update_moving` 4,080,
`_breakable_surface_effect` 4,032, `_infection_swarm_control` 3,616 (now `[sha]` only,
equal size and relocations), `_action_charge_perform` 3,248, `_physics_compute_new`
2,944, `_update_alien_scout_physics` 2,464.

Wave 3 was cut short by the token budget: `path` finished (3/3 exact); `physics` reached
its final gate with no exact closure; **`biped_limp_noodle`, `actor_type_infection` and
`bipeds` never started their second pass.** Their refreshed packets
(`scratch/lane_b/w/<unit>/PACKET.md`) hold the leads: limp noodle's frame is 24 bytes
short of January's and the /Od RTC descriptor will name the missing aggregates; bipeds'
first pass ran WITHOUT the shared /Od tool.

Highest expected yield next: (a) the fighter (one word from the owner); (b) limp noodle
and bipeds with `odbuild.py` (frame-structural, so source-reachable); (c) physics with
the REAL `point_from_line3d` now that emission is admitted - it lands the moment one
caller goes exact.

## 6. Verification at the final tip

- `ninja -j4 all_source progress semantic_progress`: pass. Code 1,459,551 / 1,770,166,
  7,315 / 7,574.
- `stable_verdicts` frozen baseline -> tip: gained 24 / 15,744 bytes, **regressions 0**.
- Row diff of all 11,060 function and 1,706 section report rows: 0 lost 100%, 0 missing.
- Parks 168 active / 0 stale / 0 invalid. Admission audit 23 / 0 contradicted / 8
  rejected / 0 revoked.
- Every landing went through `scratch/orch/land.py`: unitcheck row diff (zero exact
  loss), `_point_from_line3d` guard, newly-DEFINED-symbol ownership vs January,
  fake_match_scan delta, line endings; plus `castcheck.py` and `admit_check.py`.
- Final pytest / fake-scan / allowlist results are appended at the end of this file.

## 7. FOR LANE C (graphics / UI: bitmaps, interface, rasterizer, rasterizer/xbox, render)

Everything here was measured in this lane on 2026-09-21. Lane C's worktree is cut from
the same frozen base, so it starts WITHOUT Lane B's source edits; none of them is in a
Lane C file. The scripts named below live in Lane B's UNCOMMITTED scratch
(`C:\halo-worktrees\claude-lane-b-ai-physics-units-20260920\scratch\orch\`) - copy them.

### 7.1 A verified, ready closure: +400 B for deleting two lines

`source/rasterizer/rasterizer_lights.c` carries
`#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` / `#undef ...` (lines ~83 and ~103).
Deleting that pair makes **`_rasterizer_lens_flares_submit_occlusion_tests` (400 B)
strict EXACT, 0 regressions**, and `admit_check.py` prints PASS (the object then emits
`_point_from_line3d`, 48 B, byte-identical to January's folded copy).
rasterizer_lights is one of the 17 objects owner ruling 1 covers. Ask the owner to
confirm the ruling binds Lane C, then land it as your first commit.

### 7.2 The 17 January objects that reference `_point_from_line3d` out of line

ai_debug, real_math, collision_features, units, action_vehicle, actor_combat,
actor_moving, actor_perception, ai, items, projectiles, collisions,
**rasterizer_lights**, **rasterizer_xbox_environment**, **rasterizer_xbox_models**,
bipeds, vehicles (plus action_charge, the credited owner). The three in bold are yours.
Measured with `define_probe.py`: rasterizer_lights -> the closure above;
rasterizer_xbox_environment -> `__rasterizer_environment_lightmap_draw` moves
`[size 3984!=4016, relocs 246!=248, sha]` -> `[relocs 244!=248, sha]` (size now EQUAL;
it also suppresses `REAL_MATH_EXTERNAL_SCALE_VECTOR3D`, not yet probed);
rasterizer_xbox_models -> no row moves.

### 7.3 Inline-suppression macros in YOUR files - each is now a lever

real_math.h has 14 `REAL_MATH_EXTERNAL_*` switches that turn a header `__inline` into an
extern prototype for one TU, forcing EVERY site out of line. They were added to satisfy
the old emission guard. Lane C files using them:
`interface/first_person_weapons.c` (SIGNED_ANGULAR_DIFFERENCE, SQUARE_ROOT,
MAGNITUDE_SQUARED3D, MAGNITUDE3D, REAL_LOCAL_RANDOM, REAL_LOCAL_RANDOM_RANGE),
`rasterizer_lights.c`, `rasterizer_xbox_environment.c` (+SCALE_VECTOR3D),
`rasterizer_xbox_models.c`; also `render/render_debug.c` (not in your allowlist).
With the COMDAT class admitted, removing a switch is January's real source shape - but
it lands ONLY if a caller goes strict exact. Generalise `define_probe.py` (one constant)
and probe each switch separately; then `inline_ref_census.py <symbol>` tells you whether
January's same object defines / references / fully inlines it. WARNING: a switch can be
load-bearing for an ALREADY exact function (in bipeds, removing one LOSES
`_biped_update_jumping`), so always row-diff the whole unit.

### 7.4 Your functions are declaration-count ties on real_math.h - hands off shared headers

Adding ONE prototype to `math/real_math.h` (nothing else) moved these Lane C rows:
`_bitmap_2d_alpha_bleed` (bitmap_utilities, 560 B) became EXACT;
`_bitmap_2d_uncompress_from_mipmap` (bitmap_utilities) and
`_rasterizer_dynamic_geometry_initialize` (rasterizer_xbox_draw_primitives) LOST
exactness; and the bytes of parked `_bitmap_group_add_bitmap` and
`__rasterizer_model_transparent_geometry_submit` moved. Net board-wide: 9 regressions
for 1 gain - rejected and reverted. Two lessons: (1) `_bitmap_2d_alpha_bleed`'s residual
is a declaration-COUNT tie, not a source-shape defect - do not grind spellings on it;
it closes if a GENUINE missing declaration lands upstream of it, and re-parks otherwise;
(2) any shared-header edit needs a full-tree build and a row diff of all ~8,245
functions, and the owner's bar is ZERO regressions.

### 7.5 The /Od first-party build: strong for UI/bitmaps/cameras, ABSENT for the Xbox rasterizer

`research\symbol-build-h1-tags-20260906\halo_cache_symbols.exe` is an unoptimised
/Od + /RTC Win32 build. It gives statement order, which helpers are REAL calls, every
local as its own slot (pointer locals, single-exit result accumulators, second counters,
widths), return sites, assert line numbers, and - via RTC descriptors - the ORIGINAL
NAMES and sizes of address-taken aggregate locals. Most of Lane B's 24 closures started
there, several exact on the FIRST compile. Tool: `scratch/orch/odbuild.py`
(`str`, `file`, `callers`, `fn --out`, `rtc`). Functions that reference each
`__FILE__` string (a lower bound - only asserting functions are found this way):
bitmap_utilities.c 43, ui_widget.c 36, render_cameras.c 13, first_person_weapons.c 10,
hud_weapon.c 8, motion_sensor.c 8, hud_messaging.c 5, s3tc 1; **render_sprite.c,
rasterizer_lights.c, rasterizer_geometry.c and rasterizer_xbox_* : 0** - that code is
Xbox/D3D8-specific and not in the Win32 build. Plan accordingly: lead with interface/,
bitmaps/ and render_cameras; for rasterizer/xbox lean on January's bytes, the XDK
headers and the HCEA compilands instead.
RULE: it is a 2020-era build with known source differences - names, types, topology
and intent only, NEVER byte proof. Confirm every fact against January's own bytes
(frame size, reference multiset, ret count, store order) before relying on it, and
quote both. That double attestation is also what makes an otherwise suspicious shape
(a zero-initialised second counter, a never-read snapshot local, a view cast)
admissible to the owner.

### 7.6 HCEA compilands give per-function parameter and local NAMES and TYPES

    DIA2Dump -compiland "..\..\..\build\x360\SHIP\halo\<unit>.obj"          HCEX.pdb
    DIA2Dump -compiland "..\..\..\build\x360\RELEASE_CACHE\halo\<unit>.obj" HCEX_Release.pdb

Both verified working (run from PowerShell, `| Out-File -Encoding utf8`, then grep).
Every function with its full prototype incl. `static`, each parameter and each local
with its type. First-party evidence for a FRAME-size mismatch. 2011 build: names, types
and topology only. File-scope debug statics are usually compiled out of both.

### 7.7 What closed functions here, in order of yield

1. /Od source-shape readout BEFORE any compile (7.5).
2. Single-exit law: count `ret` epilogues in January against `return` statements; one
   result accumulator returned once closed 4 functions. `ret` and `sub esp,N` counts are
   in `lane_triage.py`'s table - build that table for your units first.
3. Frame size first: a differing `sub esp,N` is structural (a local's scope/type, a
   missing or surplus aggregate), never a tie. ~15 Lane B functions were frame-wrong.
4. Reference MULTISET diff: a constant January references and we do not pins the source
   expression - `__real@3f847ae140000000` is `(double)0.01f` (a FLOAT literal widened),
   not the double `0.01`; a missing helper reference means a missing call or an inlined
   one.
5. Replace hand expansions with the real header helper (path.c: three sites -> the
   whole unit went 32/32).
6. Genuine reconstruction bugs surface as "ties": `_path_state_estimated_distance`
   iterated the wrong index variable; its "three-register cycle" was the symptom.

### 7.8 Orchestrator gates that caught what reviewers PASSED - copy them

- `land.py <unit> <candidate.c> [--apply] [--admit-inline-comdat]`: git-clean base,
  unitcheck row diff (zero exact loss), guard, ownership, fake-scan DELTA, line endings.
  Its fake-scan delta caught an empty `if` arm holding only a `//` comment.
- `castcheck.py`: counts NEW `(real_vector2d const *)&v3`-style view casts. It caught
  21 across three reviewer-approved files. Rule 24 is strict unless ruling 3's five
  conditions are met per site.
- `admit_check.py`: ruling 2's conditions per newly emitted symbol.
- `newsyms.py`: newly DEFINED symbols vs January's object. Header-inline COMDATs the
  PRODUCTION object already emits are systemic, not violations.
- `refresh_parks.py` / Lane A's `unpark_entry.py`: park maintenance by line surgery.
  A function that BECOMES exact must be unparked or ninja's PROGRESS gate fails; one
  whose bytes MOVE while parked must be re-measured. Never re-serialise parked.json.
- `lane_inventory.py`, `inventory_diff.py`, `lane_triage.py`, `make_deliverables.py`:
  board, row diff, frame/ret/refs/real-regions triage, manifest from the journals.
  The unit list is a constant at the top of `lane_inventory.py` - replace it.
- `revert_fn.py`: build a partial landing by putting one function's PRODUCTION text
  back into a candidate (or the reverse). `split_park_patch.py`: split parked.json by
  hunk so strict and fuzzy commits each build.

### 7.9 Agent economics - read before launching anything

One worker + one reviewer per `.c` costs roughly **165K-330K tokens PER AGENT**.
Wave 1 (30 units, 8 concurrent): 21 agents finished, 6.9M tokens, then the session limit
killed the other 20 and blocked the orchestrator until reset. Wave 2 (18 units, 5
concurrent, hard shape caps, word-limited result fields, a shared pre-built evidence
tool): 33 agents, 5.5M tokens, no limit hit. The owner then cut the cap to 3, to 2, and
finally to zero. So: tell the owner the estimated cost BEFORE a wave; build the limiter
INTO the script (N queue-pulling lanes, worker THEN reviewer inside a lane so reviewers
count against N); order the queue highest-yield first; make workers resume-aware
("if candidate.c exists, do not overwrite it") and have them append to REPORT.md after
every attempt - when a wave was stopped mid-run here, the finished `path` candidate
(3 functions exact) was recovered from disk and reviewed solo at no agent cost.
"No more agents" means no NEW agents: do not stop the ones already running.
Much verification is cheap solo work: landing gates, admission checks, /Od attestation
and header sweeps were all done by the orchestrator without agents.

### 7.10 Environment traps that cost time here

- The frozen base is in the BARE repo `halo-campaign.git`; `git worktree add` from
  there. `(Get-Item xbox).Target` is an ARRAY - cast it before `New-Item -ItemType
  Junction`. After EVERY `configure.py` patch `build.ninja`'s `rule cl` to an absolute
  CL path (`scratch/orch/patch_ninja_cl.py`).
- PowerShell eats `$L...` inside `python -c "..."`; write scripts to files.
  `| Select-Object -First N` on a Python pipe yields a harmless non-zero exit code.
- Source files are CRLF; edit by byte-level replace, never re-serialise. Copied logs
  need trailing-whitespace and space-before-tab cleanup or `git diff --check` fails.
- A Workflow `scriptPath` must be a path the tool itself issued; an apostrophe inside
  the single-quoted `meta.description` is a parse error.
- The stable-verdict snapshot lists EVERY function with `status` 'E' or 'R' (its sha
  is the TARGET's) - "gained" means status flipped, not key appeared.

## 8. Tables (generated from the manifest)

### 8.1 Newly strict-exact functions (24)

| unit | function | target section | meaningful | padded | relocs | wave | reviewer |
|---|---|---|---:|---:|---:|---|---|
| ai/action_converse | `_action_converse_perform` | section:3 | 248 | 256 | 8 | wave2 | ACCEPT_EXACT |
| ai/action_obey | `_action_obey_control` | section:27 | 984 | 992 | 16 | wave2 | HOLD_FOR_OWNER |
| ai/action_vehicle | `_action_vehicle_setup_impromptu` | section:17 | 454 | 464 | 18 | wave1 | ACCEPT_EXACT |
| ai/actions | `_actor_action_try_to_dive` | section:56 | 720 | 720 | 29 | wave2 | ACCEPT_EXACT |
| ai/actions | `_actors_searching_same_position` | section:57 | 307 | 320 | 14 | wave2 | ACCEPT_EXACT |
| ai/actor_combat | `_actor_aim_projectile` | section:30 | 713 | 720 | 27 | wave2 | ACCEPT_EXACT |
| ai/actor_type_flood | `_actor_type_flood_desire_shamble` | section:2 | 1295 | 1296 | 44 | wave2 | HOLD_FOR_OWNER |
| ai/path | `_path_state_traverse` | section:31 | 2093 | 2096 | 65 | wave3 | HOLD_FOR_OWNER |
| ai/path | `_path_state_estimated_distance` | section:30 | 495 | 496 | 13 | wave3 | ACCEPT_EXACT |
| ai/path | `_closest_point_to_attractor` | section:28 | 195 | 208 | 2 | wave3 | ACCEPT_EXACT |
| ai/path_smoothing | `_find_tangent_point` | section:2 | 326 | 336 | 5 | wave2 | HOLD_FOR_OWNER |
| ai/path_structure_bsp | `_structure_test_line2d` | section:4 | 1098 | 1104 | 29 | wave2 | ACCEPT_EXACT |
| game/aim_assist | `_local_player_aim_assist` | section:16 | 353 | 368 | 12 | wave1 | ACCEPT_EXACT |
| game/game_engine_king | `_king_calculate_hill_state` | section:14 | 444 | 448 | 32 | wave1 | ACCEPT_EXACT |
| game/game_engine_race | `_race_engine_get_score` | section:25 | 215 | 224 | 4 | wave2 | ACCEPT_EXACT |
| game/player_control | `_player_control_modify_desired_angles` | section:45 | 1577 | 1584 | 83 | wave2 | ACCEPT_EXACT |
| memory/stack_memory_pool | `_stack_memory_pool_allocate` | section:20 | 637 | 640 | 36 | wave2 | ACCEPT_EXACT |
| memory/stack_memory_pool | `_stack_memory_pool_resize_block` | section:27 | 184 | 192 | 13 | wave2 | ACCEPT_EXACT |
| scenario/wind | `_wind_variance_initialize` | section:5 | 380 | 384 | 9 | wave2 | ACCEPT_EXACT |
| units/biped_limp_noodle | `_biped_limp_noodle_relax_nodes_onto_environment` | section:6 | 244 | 256 | 9 | wave1 | ACCEPT_EXACT |
| units/unit_dialogue | `_unit_make_damage_sound` | section:10 | 776 | 784 | 18 | wave1 | ACCEPT_EXACT |
| units/unit_dialogue | `_unit_describe_speech` | section:6 | 265 | 272 | 15 | wave1 | ACCEPT_EXACT |
| units/vehicles | `_update_human_plane_physics` | section:36 | 924 | 928 | 34 | wave1 | NO_CHANGE |
| units/vehicles | `_create_ghost_effect` | section:29 | 645 | 656 | 22 | wave1 | ACCEPT_EXACT |

Totals: 24 functions, 15572 meaningful, 15744 padded; stable-verdict regressions: none.

### 8.2 Remaining functions

See `claude_lane_b_rejected_hypotheses_20260920.md` (last section) for the measured blocker and
reopen evidence of each, and the manifest `remaining` array for the machine-readable form.

- `_biped_update_physics` (units/bipeds) 5376 B [sha]
- `_physics_update_old` (physics/physics) 5168 B [size 5456!=5168, relocs 118!=115, sha]
- `_collision_move_point` (physics/collisions) 4752 B [sha]
- `_collision_debug_render` (physics/collision_debug) 4192 B [size 4112!=4192, relocs 275!=278, sha]
- `_biped_update_moving` (units/bipeds) 4080 B [size 4000!=4080, sha]
- `_breakable_surface_effect` (physics/breakable_surfaces) 4032 B [sha]  PARKED
- `_infection_swarm_control` (ai/actor_type_infection) 3616 B [sha]
- `_action_charge_perform` (ai/action_charge) 3248 B [size 3216!=3248, sha]
- `_physics_compute_new` (physics/physics) 2944 B [size 3120!=2944, relocs 51!=49, sha]  PARKED
- `_update_alien_scout_physics` (units/vehicles) 2464 B [sha]
- `_biped_limp_noodle_move_relax_and_constrain_positions` (units/biped_limp_noodle) 1968 B [size 1888!=1968, sha]  PARKED
- `_biped_update_turning` (units/bipeds) 1712 B [size 1760!=1712, sha]  PARKED
- `_bsp3d_test_pill_recursive` (physics/collision_bsp) 1504 B [sha]  PARKED
- `_path_avoid_obstacles` (ai/path_obstacle_avoidance) 1360 B [size 1312!=1360, sha]  PARKED
- `_player_teleport_internal` (game/players) 1296 B [size 1312!=1296, relocs 62!=61, sha]
- `_biped_limp_noodle_valid_joint_rotation` (units/biped_limp_noodle) 1248 B [size 1216!=1248, sha]  PARKED
- `_action_vehicle_find_destination` (ai/action_vehicle) 1232 B [size 1248!=1232, sha]
- `_physics_compute_vehicle_collision` (physics/physics) 1168 B [size 1152!=1168, sha]
- `_update_alien_fighter_physics_new` (units/vehicles) 1088 B [relocs 33!=32, sha]
- `_actor_action_handle_vehicle_entry` (ai/actions) 960 B [sha]  PARKED
- `_collision_surface_test_sphere` (physics/collision_bsp) 880 B [size 848!=880, sha]  PARKED
- `_player_aim_projectile` (game/aim_assist) 832 B
- `_bsp3d_test_sphere_recursive` (physics/collision_bsp) 800 B [sha]  PARKED
- `_actor_aim_grenade` (ai/actor_combat) 544 B [size 576!=544, sha]
- `_biped_find_nearby_support_surface` (units/bipeds) 544 B [sha]  PARKED
- `_biped_accelerate` (units/bipeds) 480 B [sha]  PARKED
- `_biped_get_sight_position` (units/bipeds) 416 B [sha]  PARKED
- `_compute_ground_plane` (physics/physics) 336 B [sha]  PARKED
- `_biped_get_autoaim_pill` (units/bipeds) 336 B [sha]  PARKED
- `_actor_combat_plan_grenade_trajectory` (ai/actor_combat) 288 B [sha]  PARKED
- `_player_set_action_result` (game/players) 240 B [sha]
- `_render_debug_obstacle_path` (ai/path_obstacle_avoidance) 224 B [reloc-identity]
- `_player_examine_nearby_device` (game/players) 176 B [size 160!=176, sha]
- `_biped_check_discard` (units/bipeds) 176 B [sha]  PARKED
- `_obstacles_test_circle` (ai/path_obstacles) 160 B [sha]  PARKED
- `_game_set_game_variant_from_name` (game/game) 96 B [size 48!=96, relocs 2!=4, sha]  PARKED
- `_biped_adjust_placement` (units/bipeds) 96 B [sha]  PARKED

## 9. Final checks (run at the documentation commit)

- Allowlist audit of `git diff --name-only 6130661c..HEAD`: 25 files changed, **0 outside the allowlist**.
- `tools/fake_match_scan.py` over the 24 changed C files: Scanned 24 file(s); 0 review lead(s); skipped 0 vendored path(s).
- `python -m pytest -q tools`: **1152 passed, 5 skipped, 26 subtests passed** (identical to the frozen-base run).
- `git diff --check`: clean. No push, no merge into `jonas/exact-pilots`, canonical checkout untouched.
