# Current residual census and transferable research priorities

Baseline: `20d0563d508f7836670db62514ffc3d1f1e2217e`, rebuilt with the current default `ninja` target on 2026-09-25. `ninja build` in the repository README is obsolete for this graph. This packet changes no production source, config, park, comparator, ownership, or credit. It is a research map, not permission to reopen any held function.

## Reproduce and inspect

```powershell
ninja
python research/astra_breakthroughs_20260925/census/census.py
python -m pytest -q research/astra_breakthroughs_20260925/census/test_census.py
```

- `remaining_functions.csv`: all 122 report residuals, ranked by meaningful target bytes, with current section sizes, frame deltas, parks and evidence paths.
- `remaining_functions.json`: the same ranking, full current park prose, prologues, unchanged classifier output, and a broad index of matching evidence documents. A document mention is not an endorsement; read the independent review before relying on a proposal.
- `object_closures.json`: all 80 non-Matching Halo objects, ranked by remaining function count, then data-gap presence and code bytes. Function closure does not imply object admission.
- `population_bridge.json`: exact identities explaining the report/strict population difference.
- `stable_current.json`: independently regenerated, rename-stable 8,252-row strict snapshot.
- `summary.json`, `admission_current.json`, `park_validation.json`: production accounting transforms and admission diagnostics, not a new accounting definition.
- `input_hashes.json`: hashes of reports/config, inspected target/base objects, and residual-owner source. Compiler outputs can be timestamp-sensitive; function hashes and relocation evidence are separately recorded.

The script uses `coff_compare.section_infos_equal`, the production accepted ledger, and the existing semantic-progress transforms unchanged. Allocation measurement handles both `sub esp,N` and relocation-verified `mov eax,N; call __chkstk`. It does not interpret a frame delta as proof of an admissible source change. Missing base symbols are retained as explicit inspection errors, not silently dropped.

The 13 focused regression tests cover short/long fixed allocations, `__chkstk`, unrelated calls, absent/wrong relocation address/type/addend, intervening EAX writes, nonadjacent/non-EAX immediates, and call/return/jump/trap boundaries.

## The populations really are different

| Population | Total | Exact/accepted | Residual | Missing same-name owner |
|---|---:|---:|---:|---:|
| Halo production report | 7,574 | 7,452 | 122 report rows | Included in 122 |
| Halo strict function-start owners | 7,561 | 7,440 | 119 | 2 |
| Whole-tree stable owners, including libraries | 8,252 | 7,624 | 124 | 504 |

The bridge is complete, with no accepted-Halo owner failing the strict comparator and no strict-exact Halo owner missing accepted credit:

- The report has 12 accepted `$L` continuations absent from the strict function-start population.
- The report also has `_ai_debug_render_actor_jmptable`, a nonaccepted 388-byte interior table label absent from that population.
- Thus the 122 report residuals are **119 strict residual owners + 2 missing same-name owners + 1 interior table label**. The two missing owners are `source/interface/hud_draw::_fast_ftol_C` (56 meaningful bytes) and `source/main/main::_main_crash` (25).
- The owner/table boundary matters: `_ai_debug_render_actor` is 24,576 meaningful report bytes, while its strict section is 24,976 padded bytes and already contains the table. Do not add the 388-byte table to the 24,976-byte section. The report's disjoint meaningful rows remain the production authority. See `docs/object_matching_logs/ai_debug_jmptable_owner_20260923.md`.

The fresh build reproduces **1,583,050 / 1,770,166 meaningful bytes**, **7,452 / 7,574 functions**, and **388 / 468 objects**. The remaining report ledger is **187,116 meaningful bytes**. The top 10 rows account for 72,415 bytes (38.7%); the top 20 for 105,388 (56.3%). These are prioritization weights, not predicted gains.

## Largest current owners

| Function | Meaningful B | Target/base padded B | Target/base allocation B | Current research boundary |
|---|---:|---:|---:|---|
| `_ai_debug_render_actor` | 24,576 | 24,976 / 25,008 | 2,064 / 2,064 | Old +12 frame defect is already fixed; x87 staging/tail merge remain. |
| `_ai_communication_event` | 8,064 | 8,064 / 8,128 | 4,716 / 4,716 | Diagnostic/source graph plus held original-read behavior; not a new frame lead. |
| `_decal_new_from_collision` | 6,162 | 6,176 / 6,176 | 29,448 / 29,444 | Parameter-slot reuse / frame-entry sort; current `__chkstk` delta is -4. |
| `_biped_update_physics` | 5,366 | 5,376 / 5,376 | 44,972 / 44,980 | +8 `__chkstk` allocation; typed locals/helper provenance remain relevant. |
| `__rasterizer_model_draw` | 5,165 | 5,168 / 5,168 | 164 / 164 | Missing out-of-line helper boundaries; hypothetical folded condition is not source proof. |
| `_physics_update_old` | 5,163 | 5,168 / 5,456 | 364 / 412 | Known /Od reconstruction narrows shape; five M8 leaf sites / slot rotation remain. |
| `_collision_move_point` | 4,744 | 4,752 / 4,752 | 220 / 220 | Proven visible-body escape mechanism; exact patch rejected on owner header/scope policy. |
| `_actor_look_update` | 4,714 | 4,720 / 4,704 | 88 / 88 | Long-lived pointer reload webs / zero-register coloring; 9 evidence-based shapes exhausted. |
| `_build_structure_lens_flares` | 4,331 | 4,336 / 4,336 | 268 / 268 | Known scheduling/local lifetime packet; no new source witness. |
| `_actor_move_vector_avoidance` | 4,130 | 4,144 / 4,128 | 24,800 / 24,804 | +4 `__chkstk` allocation plus known helper boundary. |

Every row's current hashes and evidence index are in the JSON. In particular, do not substitute older donor measurements for these canonical values.

## Object-closure queue is a separate axis

Of 80 open objects, 13 have zero remaining report functions, 39 have one, 12 have two, 12 have three, and four have 4/5/6/8 respectively. Of the one-function objects, **36 have zero reported data gap**. Useful high-byte single-function targets are collisions (4,744), actor_looking (4,714), structure_lens_flares (4,331), environment_fog (3,901), render_cameras (3,370), action_charge (3,246), lightning (2,571), vehicles (2,456), and xbox_lights (2,338). Existing source/provider reviews still govern each.

Nine zero-function-gap objects also have zero reported data gap: path_obstacle_avoidance, path_structure_bsp, game_engine, object_lights, breakable_surfaces, rasterizer_xbox, rasterizer_xbox_dynavobgeom, rasterizer_xbox_hardware_geometry, render_debug. This is **not nine admissions**: explicit rejections, private-state authenticity, extra owners, and link evidence remain independent. For example, dynavobgeom has an explicit rejection; profile has zero function gap but a 132-byte report data gap and a source-layout rejection. `source/linker_common` has no functions and a 1,272,664-byte data gap. No function-matching campaign can close that object by improving a C function.

Current park validation is 79 active / 0 stale / 0 invalid. Only 72 active parks overlap these 122 Halo report residuals; **absence of a park is not absence of prior research or permission to replay experiments**.

## Five transferable research priorities

These rank research mechanisms, not ready-to-land patches or authority to change held source.

1. **Recover genuine inline wrappers, not merely their arithmetic expansion.** The reviewed `_actor_look_idle_find_prop` precedent proves that `cosine()` versus direct `(real)cos()` changes the compiler's x87 join representation and global-load folding, even when both fully inline. The accepted sibling gain is already in this baseline; it is not new credit. Transfer only to a residual with a matching x87-join/fold signature and per-site /Od wrapper evidence. Start with a source/callee-map audit, not a blanket trig rewrite. `_actor_look_update` itself was already tested and did not respond. Evidence: `research/fifty_objects_20260925/results/wave1/fn_actor_looking_ai_actor_looking_actor_look_idle_find_prop.md`, its `REVIEW__source_ai_actor_looking_actor_look_idle_find_prop_FUNCTIONS_EXACT_OBJECT_BLOCKED.md`, and `fn_actor_looking_ai_actor_looking_actor_look_update.md`.

2. **Model address escape using the visible callee body, even at out-of-line call sites.** The collisions investigation isolates a real VC7 mechanism: passing `&clip_plane` to an opaque external prototype prevents a later pointer load crossing a store; a visible noncapturing inline definition removes that barrier even when that call is not inlined. This can explain tiny struct-copy reorder residuals without inventing casts or volatile accesses. The independent reviewer explicitly affirms the mechanism while rejecting the focused-header/scope-steering landing. The genuine `real_math.h` route needs independently evidenced repair of affected consumers, notably `_rasterizer_frame_statistics_draw`, plus a complete stable/ownership sweep. Adjacent research on `_rasterizer_model_draw_environment_shader` and `set_real_vector4d` suggests another testbed, not a proven admission. Evidence: `results/wave1/fn_collisions_source_physics_collisions.md`, `results/wave1/REVIEW__source_physics_collisions.md`, `results/wave4/xbox_models_source_rasterizer_xbox_rasterizer_xbox_models__rasterizer_model_draw_environment_shader.md` under `research/fifty_objects_20260925/`.

3. **Replace local-count probing with a predictive frame-entry/parameter-slot model.** The refreshed scanner finds **28 allocation mismatches / 59,512 meaningful bytes**; 3 of those / 15,658 bytes use `__chkstk` and were invisible to sub-esp-only scanning. The most concrete unresolved case is `_decal_new_from_collision`: January reuses dead `editor_geometry` parameter slot `[ebp+0x20]` for `deviant_surface_count` (19 references), while the candidate chooses `definition`. A model that predicts the compiler's reference-count/size sort and reuse eligibility would transfer to large physics/biped functions. It must predict a held-out known exact case before motivating any source change. The frames here identify differing allocation, not why it differs and not admissibility. Evidence: `docs/object_matching_logs/claude_decals_physics_20260924.md`, especially its explicit frame-allocator reopening condition, and this packet's prologues.

4. **Trace the first backend decision with a predicted register/slot-web outcome.** Equal section sizes, frames and relocation counts are not proof of a generic tie, but they narrow where to instrument. `_actor_look_update` has an unusually concrete target prediction: primary_priority in EDI, zero pin in EBX, long-lived facing/aiming CSE pointer webs, and secondary_aim_valid spilled to `[ebp-7]`. The current allocator does the opposite coloring and keeps that flag in BL. `_ai_debug_render_actor` already has the corrected frame and slots, so it belongs in x87/tail-merge tracing, not another +12 local hunt. Use existing Lane A allocator replay / C2 diagnostics to connect a genuine source/type/helper difference to those exact decisions; stop if a probe only changes name counts. Evidence: `fn_actor_looking_ai_actor_looking_actor_look_update.md`, `docs/object_matching_logs/claude_structural_HANDOFF_20260923.md`, `tools/c2dbg32/README.md`.

5. **Make negative evidence reusable and distinguish laboratory mechanisms from source recovery.** The scout review is the strongest warning: a homed factor `fst [ebp-N]` was claimed to prove `point_from_line2d`, but the reviewer reproduced the same bytes with ordinary component statements. The proposed helper merely added one IL temporary that fixed a distant M8 swap; it was rejected. Likewise, models' `if(TRUE)`/already-nonnull condition demonstrates pre-fold inline-budget behavior, but cannot recover January's missing condition. These are useful compiler experiments only. A reusable probe receipt should state the unique prediction, equivalent counterexamples, current input hashes, independent review, and the exact new evidence required to reopen. Evidence: `results/wave1/REVIEW__source_units_vehicles.md`, `results/wave4/xbox_models_source_rasterizer_xbox_rasterizer_xbox_models___rasterizer_model_draw.md`. This avoids spending another wave on causally ambiguous exactness.

## Explicit stale/exhausted hypotheses

- **Old board counts:** September 20's 300 residuals / 333,973 padded bytes and 56 sub-esp mismatches are not this baseline or population. Do not quote them as current.
- **AI debug +12:** fixed by authentic scoping/type work on September 23; current allocations are 0x810/0x810. Its jump table is an interior dependency, not a separately missing function.
- **Sun-glow +12:** the park's narrative still mentions 0x98 vs 0xA4, but current target/base both allocate 0x98. The park measurement hashes validate; the old narrative is not a current prologue fact.
- **Scout helper-factor signature:** independently falsified as unique evidence. The candidate's additional aggregate view copies are also load-bearing, not all byte-inert. The worker's exact candidate is not an approved source packet.
- **Collision focused header/block scope:** independently rejected. Reopening requires genuine-owner consumer evidence, not relocating the helper to avoid canaries or reverting an attested function-scope local.
- **Decals clip:** 24 loop-tail orders, 7 declaration orders, 0..45 file names, counter/flag/scope spellings and related variants are recorded as inert; the remaining adjacent load swap needs new evidence. Its current automatic classifier label is not permission to rerun those sweeps.
- **Actor look update:** wrapper substitutions, listed U1/U2/U4/U5/Vc source shapes and name-count sweeps are exhausted. The wrapper success in find_prop does not transfer automatically.
- **All44 old call-sequence census:** `docs/object_matching_logs/astra_90pct_wave13_call_sequence_census_20260920.md` explicitly found no new admissible mechanism. Several functions in it have since closed. Its call-count deltas are neither current inventory nor new hypotheses.
- **Generic classifier recipes:** current output is 114 unclassified, 3 control-flow-return-shape, 2 alias-global-ownership, 3 unavailable comparisons. Even a tool label `PROVEN` establishes its measured signature, not that a new source edit is authentic, untried, safe or sufficient. Jump-table bytes also make linear instruction/return interpretation hazardous.

No new exact-byte or object credit is claimed by this packet. No hold is extended to another function merely because it shares a subsystem or helper.
