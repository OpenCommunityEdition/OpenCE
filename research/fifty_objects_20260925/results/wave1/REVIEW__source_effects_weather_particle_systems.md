# review source/effects/weather_particle_systems
approve=False

## checks
I re-ran everything myself in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review_weather_particle_systems\. Notes are in REVIEW.md there.

(1) Patch: production.patch applies in an isolated core.autocrlf=true repo. The result is byte-identical to final_weather_particle_systems.c, which equals fin2.c.

(2) Gate: gate.py --source cand.c --all gives exact 20, residual 0, unwritten 0. Render is EXACT at 1680.

(3) Object audit: object_audit on cand.obj PASSES. Every January-owned section is ok and 30/30 symbols match. The production base object FAILS the same audit on 2 items: render, and one_over_char_max with storage 2 in January against 3 in ours.

(4) Surplus: the candidate's external sections are byte- and relocation-identical to the base object's. The only new external symbol is _one_over_char_max, which January owns.
- provider_link on the candidate fails only on _real_local_random (the effects.obj NODUP copy), in both orders. The base object fails the same way, so this predates the candidate. With --baseline there is no new surplus.
- surplus_identity: 13 of 14 IDENTICAL; _real_local_random is DIFFERENT, as it already is in production.

(5) PDB storage: pdb_storage shows one cachebeta public, _one_over_char_max at 0x25AAFC, and no other definer exists in source/. So January's storage for it is external.

(6) Data: report.json already has data at 949/949. Code goes from 4114/5779 to 5779/5779, a gain of 1665 meaningful bytes.

(7) /Od check: I dumped fn 0x57fb00 again and it matches the worker's od_render.txt exactly. I checked each shape claim against the dump myself:
- The only reference to [ebp-0x24] is `mov word ptr [ebp-0x24],cx` with cx=0, at 0x57fbb4.
- box_count=0 is stored before weather_polyhedra_find. The real at -0x218 is set to box_width after the find call.
- viewer_box uses the chained assignment and then +=. box_deltas and box_delta match, and the box store order is x0,y0,z0,x1,y1,z1.
- The particle loop is a while loop. The loop condition includes `&& !box_offset` and there is a break.
- position is built particle first. viewer_to_particle is computed with explicit subtractions, with no call to vector_from_points3d.
- The polyhedron plane test passes the element straight in as an argument, and the visible byte goes TRUE then FALSE.
- /Od transform 0x580c10 calls dot_product3d(&planes[i], box_position) as a real function, with the same argument order as the candidate.
- HCEX types box_offsets as union real_point3d[0x1A].

(8) Strip tests, each a whole-TU gate:
- Removing `short unused_count = 0;` leaves render residual [sha], 19/20. The dead local is LOAD-BEARING.
- Removing the (real_vector3d const *) cast still gives 20/20; the cast is byte-inert but its absence raises C4133.
- Removing `&& !box_offset` still gives 20/20, so it is inert.
- The production sprite-scale spelling `2.f*r*w` leaves render residual. The candidate's `w*(2.f*r)` keeps the grouping January uses (fld r; fadd st0,st0; fmul w, per the fresh-graphs ledger), so the parentheses are a real parse difference, not decoration.

(9) Warnings: CL /Zs /W3 gives the same warning set as production (15). fake_match_scan reports 0 leads, but it does not detect dead locals.

(10) Separable fix: dropping only `static` from one_over_char_max on production still gives 19/20 with render unchanged, and 30/30 symbols.

(11) House rules, reviewed across the whole diff: every change is value-equivalent to production. The patch removes a hand-expanded dot product. The only other checks are that no header or config file changes and that the prior ledgers and owner queues were read.

## issues
BLOCKING: the render match depends on `short unused_count = 0;`, a dead local with an invented name. The strip test fails without it: render goes residual.

- WORKER_BRIEF section 2 lists "filler declarations/dummy locals (they are the name-count ORACLE, never a landing)" as fake matching.
- This exact item is already waiting in the owner queue. docs/object_matching_logs/claude_lane_d_refresh_HANDOFF_20260922.md line 142 lists class "I placeholder-named dead local (_weather_particle_system_render) 1 / 1,680 exact-under-ruling". The manifest (claude_lane_d_refresh_manifest_20260922.json, around lines 1081-1084) describes the same /Od [ebp-0x24] short with the same placeholder name `unused_count`, and it is HELD.
- The only admission precedent is Lane C ruling R8 (2026-09-21). It admitted /Od dead arrays that the RTC descriptors NAME, and says "rule 8 is not weakened generally". It does not cover an unnamed dead scalar with a guessed name.
- I found no later owner ruling on class I in docs/ or in memory.
- Landing this now would pre-empt a pending owner decision.

The /Od evidence itself is strong and verified. The candidate's local inventory matches the /Od frame slot for slot, and the dead short really does exist in the first-party build. So this should go back to the owner as a ruling request, not be rejected as fake matching.

Recommendation for the integrator:
- Do NOT apply production.patch yet.
- Send this candidate to the owner as the updated class-I item. It supersedes Lane D's variant, which hand-expanded the particle plane distances. This one uses the real plane3d_distance_to_point and dot_product3d helpers, both attested as calls in /Od.
- If the owner admits /Od-attested dead scalars with invented names, apply scratch/w/weather_particle_systems/production.patch unchanged. I verified that it applies and reproduces 20/20, object audit PASS, and zero new surplus.

Minor notes:
- `unused_count` is an invented name, not an authentic one.
- The (real_vector3d const *) view cast is attested per site in /Od and is byte-inert. It only meets the view-cast rule once the caller (render) is exact, so it lands only together with the dead local.
- The one_over_char_max storage fix can land on its own now, without the rest of the patch. It is a one-line change on production: `static real const one_over_char_max` becomes `real const one_over_char_max`. The candidate file is scratch/w/review_weather_particle_systems/prod_storage_only.c. It keeps 19/20 with no row changes, brings symbols to 30/30, earns no code credit, and has a single cachebeta PUBLIC with no other definer.
- The object remains blocked regardless by the older _real_local_random surplus conflict with the effects.obj NODUP copy.
