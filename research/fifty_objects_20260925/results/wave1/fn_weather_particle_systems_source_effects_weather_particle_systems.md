# source/effects/weather_particle_systems

## verdict
FUNCTIONS_EXACT_OBJECT_BLOCKED

## newly_exact_functions
[
 "_weather_particle_system_render"
]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weather_particle_systems\\production.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weather_particle_systems\\final_weather_particle_systems.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weather_particle_systems\\fin2.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weather_particle_systems\\fin2.obj",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weather_particle_systems\\body_fin.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weather_particle_systems\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\weather_particle_systems\\od_render.txt"
]

## production_changes
This changes one file only: source/effects/weather_particle_systems.c. Apply scratch/w/weather_particle_systems/production.patch with `git apply` in the worktree. The patch is LF and applies under core.autocrlf=true. As an alternative, copy scratch/w/weather_particle_systems/final_weather_particle_systems.c over the file; it is byte-identical to the patched result, and I checked this in an isolated autocrlf repo.

The patch makes three edits:
(a) Line 309: `static real const one_over_char_max = 1.f/255.f;` becomes `real const one_over_char_max = 1.f/255.f;`. January's split object has this symbol as storage 2 (EXTERNAL), and it is a public in the cachebeta PDB. Nothing else in the TU uses it, and no other source file or PDB public defines it.
(b) The body of weather_particle_system_transform_clip_planes_to_box becomes `plane_distances[plane_index] = dot_product3d(&planes[plane_index].n, (real_vector3d const *)box_position);`. The /Od build attests that this site is a real call to dot_product3d. The function stays EXACT.
(c) The body of weather_particle_system_render is rewritten to follow the first-party /Od topology (halo_cache_symbols.exe 0x57fb00):
  - Names come from the RTC descriptors and HCEX: structure, build_sprite_data, box_offsets, clip_planes, clip_plane_d_transforms, weather_polyhedra_indices/count, box_deltas, viewer_box, box_delta, box, plane_evaluations, position, viewer_to_particle.
  - Box 0 is handled as `box_count = 0`, then `box_offsets[box_count]`, then `box_count++`.
  - `real far_clip_distance` is assigned after weather_polyhedra_find.
  - viewer_box is set with a chained assignment followed by `+=`.
  - particle is declared inside a `while (particle_index!=NONE)` loop body.
  - The loop uses `box_offset = NULL`, with `box_index<box_count && !box_offset` plus a break.
  - Particle distances use plane3d_distance_to_point, with no hand expansion.
  - `real maximum_distance = MIN(...)` is declared after position, viewer_to_particle and depth.
  - position = particle->position + box_offset, particle operand first as in /Od.
  - The polyhedron test has no named plane local.
  - The visibility flag starts TRUE and is set FALSE.
  - The sprite scale is written `runtime_one_over_sprite_width*(2.f*particle->radius)`.
  - viewer_to_particle is computed with explicit statements, as /Od shows.
  - `short unused_count = 0;` is added at type scope after `long particle_index;`. It matches /Od dead slot ebp-0x24.

Nothing else changes: no header, config, symbols.json, parked.json, semantic override or admission row. The unit has no park to retire. Optionally, copy scratch/w/weather_particle_systems/LEDGER.md into docs/object_matching_logs/ as the unit ledger.

## evidence
Starting point: the production gate was 19 exact and 1 residual (render, [sha]). The prior-lane ra.c body, spliced onto the current tree as cand_ra.c, still had 5 x87 rows: 4 at the inner inline transform site and 1 at position.x. Those are the same rows the w1 and w3 lanes recorded.

New evidence from the /Od build (fn 0x57fb00, no prior lane had used it):
- The RTC descriptors plus a full frame census give January's local inventory and declaration topology. Every gap in the frame is an /RTCs guard.
- There is one extra dead `short` at ebp-0x24. It is initialised to 0 and never read.
- The build computes viewer_to_particle explicitly, although vector_from_points3d is a real function in that build (0x42e3a0, called by distance_squared3d 0x42dc80).
- It calls plane3d_distance_to_point and dot_product3d as real functions.

od2 (/Od-faithful body, without the dead short, still using vector_from_points3d):
- position.x, the j-term row and the sprite row all closed.
- 3 rows remained, all in the inner-site term order.

LAW M8 lab (dead-local instrument, lab only):
- On od2, D=2 closes the function and D=8 equals D=0.
- 2 dead locals anywhere in the function close it, so the key is a count over the whole function.
- Uninitialised, unused declarations are inert.
- With explicit viewer_to_particle (v1), D=5 or D=1 closes it. Removing the inline temps of the later site shifts the key, and the inner-site key has period 4.
- v1 plus the /Od dead short (`short X = 0;`) is STRICT EXACT. So the two /Od-attested inventory facts together reproduce January's count key.

Final fin2, whole-TU gate: exact 20, residual 0, unwritten 0. Render is EXACT at 1680 padded bytes, 47 relocations.

Controls, all whole-TU:
- Without unused_count: render residual.
- With production's hand-expanded transform body: render residual.
- Without the (real_vector3d const *) cast: still 20/20, so the cast is byte-inert and only avoids C4133.
- Without the redundant `&& !box_offset`: still 20/20 (inert); I kept the /Od form.

Other checks:
- CL /Zs /W3 gives the same warning set as production.
- fake_match_scan: 0 leads.
- object_audit on fin2: every January-owned .text and data section is ok, and 30/30 January symbols match (fin1 differed only in the one_over_char_max storage, which fin2 fixes).
- report.json data was already 949/949; code goes from 4114/5779 to 5779/5779, a gain of 1665 meaningful bytes.
- The surplus list is identical to the production base object's. provider_link with --baseline=build/base reports no new surplus.
- surplus_identity: 13 of 14 header COMDATs are IDENTICAL.

## blockers
1. Object blocker, already present in production and outside this unit:
   - The _real_local_random surplus COMDAT is DIFFERENT from January's selected copy. Ours is the header __inline body ending `pop ecx`; January's copy in effects.obj uses a named result and ends `add esp,4`.
   - provider_link FAILs with LNK2005 in both link orders, against effects.c's NODUP hand-written real_local_random.
   - The production base object fails the same way. new_particle's existing real_local_random() use causes it, not this candidate.
   - The fix belongs to the owner, in real_math.h and/or effects.c (the known NODUP provider class). A named-result header body was previously measured to regress units (lane w1, tree D).
2. Owner acceptance needed for three first-party-attested but unusual forms:
   - The DEAD declaration `short unused_count = 0;`. /Od-attested (slot ebp-0x24, init-only). It is load-bearing for the M8 count key. This is not an invented filler: it is the genuine declaration the M8 count oracle pointed to. The name is descriptive (the original name is unknown; renaming is byte-inert).
   - Explicit viewer_to_particle statements instead of vector_from_points3d, at this one site.
   - The redundant `&& !box_offset` loop condition next to a break (byte-inert, /Od form).

## reopen_criteria
Object completion reopens in either of two cases:
- The owner rules on the _real_local_random NODUP provider. For example: effects.c consumes the header inline, and the real_math.h body is a named result so the SELECT_ANY copy matches January's `add esp,4` body. This needs a full-board blast-radius sweep, because the earlier attempt broke units::_unit_preprocess_node_orientations.
- The lane admits already-present NODUP surplus as non-blocking.

If the owner rejects the dead /Od-attested declaration, the render function reverts to residual. The best compliant fallback is od2.c, with 3 inner-site rows left. In that case, reopen only with first-party evidence of a different January local inventory; one IL-visible local net, modulo 4, closes it.

## task notes
Nothing was lost from the earlier cut-off: the slug directory did not exist, and every prior-lane ledger was read before any probe. No tracked file was edited; `git status` on the production source is clean.

Measured result: _weather_particle_system_render is strict EXACT and the unit's gate is 20/20. I closed it by reading January's local inventory out of the first-party /Od build and using the M8 count law to check it. The object stays blocked by one issue that was already in production before this work: the _real_local_random NODUP provider-link conflict with effects.c.

Keep the credited bytes separate: the meaningful gain is +1665 code bytes (1680 padded). The one_over_char_max storage fix brings January symbol parity to 30/30.

Lab tools written: lab/mod8lab.py and lab/place.py (dead-local instrument, lab only), splice_body.py, nontriv.py, warn.py, cmp_comdat.py.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\weather_particle_systems\LEDGER.md
