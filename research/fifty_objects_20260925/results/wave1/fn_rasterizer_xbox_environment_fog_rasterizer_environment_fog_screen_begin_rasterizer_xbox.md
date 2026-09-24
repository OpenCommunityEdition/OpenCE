# __rasterizer_environment_fog_screen_begin (rasterizer/xbox/rasterizer_xbox_environment_fog)

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_environment_fog\\candidate_production.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_environment_fog\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_environment_fog\\candidate_diag_DE_R3.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\rasterizer_xbox_environment_fog\\LEDGER.md"
]

## production_changes
Apply scratch/w/rasterizer_xbox_environment_fog/production.patch to source/rasterizer/xbox/rasterizer_xbox_environment_fog.c. It was checked with git apply against the current production file, and the result is byte-identical to candidate_production.c (== c8.c, CRLF preserved). No header, config, symbols.json or parked.json change for this function, and there is no park to retire (the function is not parked). The patch carries:
(1) The structural-lane fixes from c0: the shadowed previous_camera_matrix local (it was an uninitialised read), D3DCLEAR_TARGET_A, animation_period predicates, short offset / OR base-z, short width and height, (real)tan, scope moves, vsh_constants__texscale_1, nested chicago verify, and skinning count-first.
(2) New in this lane, each backed by January's bytes:
  (a) near_density==far_density written as if/else instead of an early return (single-exit). VC7 jump-threads it to the same bytes, and it raises the alpha-site inline budget from 14 to 15, which equals the real_alpha_to_pixel32 cost. January's inline expansion of the helper follows with no header change, so ruling A is retired.
  (b) `word flags = global_window_parameters.fog.screen->flags;` for the two local flags.
  (c) `clear_z_buffer = !(A && B) && (A || (B && opaque_model_count > 0) || (water && draw_water))`, written with TEST_FLAG.
  (d) The colour pointer declared in each arm, `real animation_time = animation_times[index];`, and (1.0f - animation_time) written in place.
  (e) texture_transforms typed as real_vector4d[LAYERS][2] and filled by two calls to this TU's own set_real_vector4d.
  (f) The first-time reseed as `real v = real_local_random(); layer_state = &...; layer_state->u = real_local_random(); layer_state->v = v;`.
  (g) The rotation as `window->rotation -= (real)atan2(vector.j, vector.i) * -screen->rotation_multiplier;` (January shows fchs;fsubr). This spelling and `-= -(real)atan2(...)*mult` produce identical bytes; this one was picked. Reviewer: please check the unary-minus placement.
Zero strict credit: the function stays residual. Gate after the patch: exact 26 / residual 1 / unwritten 0, and only the screen_begin row changes.

## evidence
Baseline gate: residual 3904 [size 4000!=3904, relocs 232!=235, sha]. Candidate c8: [size 4032!=3904, relocs 240!=235, sha]. The only relocation-multiset difference from January is +5 _csmemcpy (the float-bit temps). The alpha helper's assert literals, display_assert/system_exit 7/7, 0.0/1.0 literals and the three previous_camera_matrix references now all match. By metric.py (slot-normalised LCS), unmatched January instructions go from 277 on the production base to 105 in c8. c8 adds no new text or COMDAT symbols (41/41 match the production base).
Key measurements:
- Probe harness probe.py: the alpha-site budget is 14 in c0; removing either early return raises it to 15.
- Cost lab costlab.py: real_alpha_to_pixel32 inlines at budget-15 points and not at 14, so its cost is exactly 15.
- In c1 the near==far path bytes, `pop esi;pop ebx;mov [flag],0;mov [model],0;pop edi;...;ret`, equal January 0x97f..0x993.
- The live C2 frame-table instrument (p2/, retargeted from Lane C / the structural lane) shows previous_matrix+projection_scale and the scale*cos temp now join January's entries.
Diagnostic c7_DE (adds rulings D and E plus the set_real_point2d change): [sha] only, size 3904==3904, relocs 235==235, frame 0x184 vs 0x180, 58 unmatched instructions. Every probe is recorded in LEDGER.md sections P0..P22 and FINAL.

## blockers
Strict exactness still needs owner rulings, plus residuals with no source form found yet.
Rulings needed:
- D: read float bits in place (`*(long *)&x`) in the five %f[%x] error dumps. January reads the float memory directly. Precedent: units.c:11494, which the /Od build attests. The /Od fog-screen body is a dx9 stub, so there is no per-site /Od attestation.
- E: the texscale [0][3] term is `dot_product3d((real_vector3d const *)&camera.position, &camera.forward)` in January; lab_dot.c reproduces January's x,z,y evaluation order exactly. This is a new point-to-vector view cast under rule 24. Precedent: decals.c:891, matrix_math.c:902/914, collisions.c:1400.
- R3: layers typed real_point2d with set_real_point2d at the offset reseed sites matches January's lea/int-copy packet. It would newly emit a _set_real_point2d COMDAT from a caller that is not exact, which ruling 5 forbids; provider_link passes.
Residual machine differences in the best diagnostic:
- Frame +4. vector joins the double-temp entry instead of January's phase/point entry. Decoding C2 0x1076b3ba/0x107159fb shows the merge conflict check is symmetric, so no source form has been found.
- Register picks: width/height/layer_count (January ecx/edx, ours edx/edi); the offset>0 index prologue (cx vs ax); the skinning pointer reload (ax vs cx); the stats block.
- Order differences: projection_scale multiply order (tan*far*aspect vs tan*aspect*far; declaration order and three spellings are inert); the green x87 term (2t computed first in January; five spellings inert); screen-constant zero-store interleave; point-setup store order; texture loop-bound load position; first-time add esp/fstp order.
Object-level blockers outside this function: the 7 SDK wrapper storage mismatches (the fix is proposed in the next unit), and the _real_local_random surplus COMDAT. That COMDAT differs from its NODUP provider effects.obj and fails the provider link with LNK2005, and it already fails on the production base.

## reopen_criteria
Reopen if any of these happen:
- Owner rulings on D and E (and R3, or an exact caller) arrive. Then resume from candidate_diag_DE_R3.c and attack the frame +4 first, using the frame-table instrument (ft.sh) plus conflict-set logging at C2 0x1076b564/0x107159fb.
- New first-party evidence (for example a later build that has a fog-screen body) shows the scope or form of vector, phase or point, the projection_scale expression, or the skinning store form.
- A decoded C2 rule explains January's vector/phase/point overlay alongside the separate double-temp entry.

## task notes
Tools written in the slug:
- probe.py (inline-budget probe) and costlab.py (helper-cost calibration).
- metric.py (slot-normalised distance) and lay.py (/FAsc named layout).
- ft.sh plus p2/ (live C2 frame table, retargeted to this worktree).
- xf.py (source transforms), object_audit_split.py (audit against a scratch split), and cfg/ + split/ (the scratch csplit).

I disassembled C2.Dll as data only, to read the slot-merge rule; nothing was patched. No tracked file was edited, and no ninja, configure or git-mutating command was run. The temporary git init used for patch-apply testing was inside a scratch subdirectory and has been deleted.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\rasterizer_xbox_environment_fog\LEDGER.md. Every probe is logged there (P0-P22 and FINAL): hypothesis, the January-byte evidence, the change and the measured result. Main result: the "alpha inline" owner blocker (ruling A) was a .c-local defect. The early return on the near==far density path cost one inline-budget step. Rewriting it as a single-exit if/else gives the same bytes on that path and lets real_alpha_to_pixel32 inline exactly as January does. Four more January-attested fixes landed in the admissible candidate:
- the flags word local;
- the decoded clear_z expression;
- the colour-arm pointer and time local;
- the texture transforms built by this TU's set_real_vector4d.

Also the first-time reseed order and the rotation sign form. Admissible candidate c8: the only relocation difference from January is +5 csmemcpy (ruling D), and slot-normalised unmatched instructions drop from 277 to 105. The diagnostic that adds rulings D and E and the set_real_point2d change reaches [sha]-only at size 3904==3904. Also proposed: an admission fix in symbols.json (7 SDK wrappers marked static), verified by a scratch csplit.
