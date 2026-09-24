# REVIEW weather_particle_systems (adversarial)
Started 2026-09-23. Candidate: scratch/w/weather_particle_systems/final_weather_particle_systems.c + production.patch

## 1. Reproduction
- production.patch applies (isolated autocrlf=true repo) and result is byte-identical to final_weather_particle_systems.c == fin2.c.
- gate --all on final (cand.c -> cand.obj): exact 20 / residual 0 / unwritten 0; render EXACT 1680. REPRODUCED.
- PRIOR HOLD FOUND: docs/object_matching_logs/claude_lane_d_refresh_HANDOFF_20260922.md queue class "I placeholder-named dead local
  (_weather_particle_system_render) 1 / 1,680 exact-under-ruling" and manifest line ~1081: same dead `short X = 0` at /Od ebp-0x24,
  placeholder `unused_count`, HELD for owner ruling. Lane C R8 admitted only /Od-RTC-NAMED dead arrays ("rule 8 not weakened generally").
## 2. Independent checks (reviewer re-runs)
- object_audit cand.obj: all January-owned sections ok, 30/30 symbols, OBJECT AUDIT: PASS. Base obj: FAIL(2) (render + one_over_char_max storage 2/3).
- surplus external sections byte+reloc identical base vs cand (cmp_surplus.py); only new external = _one_over_char_max (January-owned, storage 2).
- pdb_storage: cachebeta public _one_over_char_max (0x25AAFC), single definer in source/ -> dropping `static` is January storage truth.
- provider_link cand: all PASS except _real_local_random (effects.obj NODUP) FAIL both orders; SAME FAIL on base -> pre-existing. --baseline: no new surplus.
- surplus_identity: 13/14 IDENTICAL, _real_local_random DIFFERENT (pre-existing).
- /Od re-dump (odbuild fn 0x57fb00) identical to worker's od_render.txt. Verified personally:
  * [ebp-0x24] single reference `mov word ptr [ebp-0x24],cx` (cx=0) at 0x57fbb4, after type_definition, before `if particle_count`: dead short, init 0.
  * box_count=0 stored before weather_polyhedra_find; real [-0x218]=box_width after it; passed to build_clipping_planes; MIN(fadeout_end, it) later.
  * viewer_box chained 0.f / box_width then += box_offsets[0]; box_deltas -w,0,w; box_delta i/j/k; box x0,y0,z0,x1,y1,z1; transform; box_count++.
  * particle loop: while (no for-increment jmp), particle/box_offset=NULL/box_index/plane_index/plane_evaluations; `&& !box_offset` in loop cond + break.
  * position = particle->position + box_offset (particle first); explicit viewer_to_particle subtraction (no call); dot_product3d(&v2p,&forward).
  * polyhedron test plane3d_distance_to_point(TAG_BLOCK_GET_ELEMENT(...), &position) inline arg; visible byte TRUE->FALSE+break.
  * /Od transform 0x580c10: dot_product3d(&planes[i](.n), box_position) real call, arg order matches candidate.
  * HCEX: box_offsets is `union real_point3d[0x1A]` -> transform param point type + (real_vector3d const *) view at dot_product3d is consistent.
- STRIP TESTS (whole-TU gate):
  * remove `short unused_count = 0;` -> render residual [sha] (19/20). LOAD-BEARING dead local.
  * remove (real_vector3d const *) cast -> 20/20 (byte-inert; typing only, C4133 otherwise).
  * remove `&& !box_offset` -> 20/20 (inert; /Od-attested so keeping it is fine).
  * production sprite scale `2.f*r*w` -> render residual; `w*(2.f*r)` is January-attested grouping (fresh-graphs: fld r; fadd st0,st0; fmul w) - a real parse-tree difference, not decoration.
- CL /Zs /W3: cand == production warning set (15; 3 pre-existing C4244 in the .c + header warnings). strip_cast adds C4133 (so the cast is typing-only).
- fake_match_scan cand.c: 0 leads (the scanner does not detect dead locals).
- report.json: data 949/949 already; code 4114/5779 -> render adds 1665 meaningful. Claim consistent.
- Separable storage fix measured alone (prod_storage_only.c = production + drop `static` on one_over_char_max):
  gate 19/20 unchanged (render still residual), object_audit 30/30 symbols (only the render section differs). Clean, zero-credit fidelity fix.

## 3. House-rule review of the diff
- Semantics: every change is value-equivalent to production (0+x, commutative adds/muls, while==for without continue, box_offset ptr
  vs index test, visible flag vs index==count, explicit v2p == vector_from_points3d). No nonsensical logic.
- Names: RTC/HCEX names (structure, build_sprite_data, box_offsets, clip_planes, clip_plane_d_transforms, weather_polyhedra_*, box_deltas,
  viewer_box, box_delta, box, plane_evaluations, position, viewer_to_particle). `unused_count` is an INVENTED placeholder (HCEX cannot list it).
- Helpers: patch REMOVES a hand-expanded dot product (transform) and keeps plane3d_distance_to_point; explicit viewer_to_particle is
  per-site /Od-attested (vector_from_points3d is a real fn in that build yet not called here). OK.
- View cast (real_vector3d const *): /Od per-site attested, compatible layout, byte-inert strip, transform exact - acceptable only
  together with an exact caller (render), i.e. only with the dead local.
- `w*(2.f*r)`: January-byte-attested grouping (fld r; fadd; fmul w); /Od consistent; not decoration.

## 4. BLOCKING ISSUE
The render match is bought by `short unused_count = 0;` - a load-bearing (strip test FAILS), dead, placeholder-named local.
- WORKER_BRIEF s2: "filler declarations/dummy locals (they are the name-count ORACLE, never a landing)".
- This exact item is already in the owner queue: Lane D refresh HANDOFF 20260922 class "I placeholder-named dead local
  (_weather_particle_system_render) 1 / 1,680 exact-under-ruling" (manifest: same /Od [ebp-0x24] short, same placeholder name, HELD).
- The only admission precedent (Lane C R8, 20260921) covers /Od RTC-NAMED dead arrays and says "rule 8 is not weakened generally".
- No later owner ruling on class I found in docs/ or memory.
=> Cannot approve a production landing; it would pre-empt a pending owner decision. The /Od attestation is genuinely strong
   (candidate local inventory == /Od inventory slot-for-slot), so this is a good ruling candidate, not a fake-match rejection.

## 5. VERDICT: approve = false (hold as exact-under-ruling)
Recommend: integrator forwards this candidate to the owner as the UPDATED class-I item (supersedes Lane D's variant: no hand-expanded
particle plane distances, genuine dot_product3d/plane3d_distance_to_point helpers). If the owner admits /Od-attested placeholder-named
dead scalars, apply production.patch as-is (verified to apply and reproduce 20/20). Independently landable now: the one_over_char_max
storage fix alone (19/20 unchanged, 30/30 symbols) - optional, zero credit.
