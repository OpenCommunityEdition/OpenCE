# Adversarial review: source/structures/structures (proposed FUNCTIONS_EXACT_OBJECT_BLOCKED)

Reviewer lane claude/fifty-objects-20260925, 2026-09-23. Worker: scratch/w/structures/ (LEDGER.md read in full).

## R1 patch identity
- production source/structures/structures.c sha256 72611997... (unmodified, HEAD e9e62b78) copied to prod_structures.c.
- production.patch applied to a copy (applied/source/structures/structures.c) -> sha256 2fb89c42..., byte-identical to
  scratch/w/structures/final.c. Diff touches only the sphere_intersects_cluster_portal body (prod lines 331-403).

## R2 independent gate
- baseline (prod copy): exact 16 / residual 1 `_sphere_intersects_cluster_portal [size 416!=432, sha]` -- matches worker.
- candidate final.c -> cand.obj: exact 17 / residual 0 / unwritten 0 -- REPRODUCED.

## R3 whole-object audit (independent)
- relocdiff _sphere_intersects_cluster_portal cand.obj: 11/11, 0 differing rows, size 432/432, sha equal.
- object_audit cand.obj: PASS, 26 January symbols 0 differ. Surplus list IDENTICAL to (a) gate of prod copy and
  (b) build/base structures.obj (only the PASS/FAIL line differs; base audit FAIL(1) = the sphere section).
- surplus_identity against cand.obj (local copy surplus_identity_cand.py, CAND_OBJ env): 12 code COMDATs, 0 not identical.
- provider_link cand.obj: FAIL(2) `_project_point3d` (path_obstacles NODUP) + `_projection_sign_from_vector3d` (decals NODUP);
  production build/base structures.obj output IDENTICAL; `--baseline=build/base` -> no NEW surplus. Pre-existing, not introduced.
- pdb_storage: 26 split symbols, 0 disagreements. build/report.json data 2292/2292 (100%); code 3470/3888 (gap 418 = this fn).

## R4 strip test of the load-bearing construct (reviewer-run, not in worker ledger)
Worker TG2 only stripped the `(real)` cast. The decoration that carries the match is the `double` local itself:
- S1 `real t = -plane_distance;` (type stripped, s1_real_t.c): exact 16 / residual 1 `_sphere_intersects_cluster_portal [sha]`.
- S2 no `t`, `point_from_line3d(point, &plane->n, -plane_distance, &projected_centroid3d)` (s2_inline_neg.c = the /Od-attested
  spelling): exact 16 / residual 1 `[sha]`; alndiff = the same three 1-slot moves (movzx ax,al / push eax / lea eax,[ebp-0xc]).
=> ONLY the double-typed local matches. By brief section 2 ("if the plain spelling no longer matches, the decoration IS the fake")
   this fails the strip test.

## R5 /Od readout check (od_sphere_portal.txt, 0x8a0670)
- Confirmed: statement order, standalone point_from_line3d call, short projection (mov word), short sign (movzx ax,al; mov word),
  plane pointer local, RTC names, single shared false epilogue (jbe/jbe/je all -> 0x8a08d8 `xor al,al`; `mov al,1` true arm).
- CONTRARY evidence for `t`: the /Od build negates the float local inline at the call (`movss xmm0,[ebp-0xc]; xorps [0x93dda0];
  movss [esp],xmm0` at 0x8a07bb) and has NO 8-byte (or any) slot for a `t` local - every user-area slot is accounted for
  (portal -8, plane_distance -0xc, projected_points, plane -0x418, projection -0x41c, sign -0x420, centroids, vertex_index -0x448,
  call temps -0x44c/-0x450/-0x454, RTC guards between). The later first-party source therefore passed `-plane_distance` directly.
- Same-TU January counter-example: exact `_render_debug_fog_planes` passes `-render.fog.planar_maximum_distance` directly to
  point_from_line3d (final.c:607-608).
- docs/common_constants.md: "Use `double` only where the genuine API or default vararg promotion requires it." Not the case here.
- Precedents cited by worker verified: actor_moving.c:2136 `double t = -distance_along_step;` (ea507d5b, a zero-credit fidelity
  landing in a still-parked function, no stated ruling on the double) and units.c:11087 `double origin_scale` (tgr closure).
  Lane B manifest (_actor_action_handle_vehicle_entry) sets the reopen bar as a natural fact "without a double temp; none is
  /Od-attested here" - i.e. the campaign treats an unattested double temp as not self-admitting.

## R6 house-rule pass on the diff (production.patch)
- params one per line, explicit `return TRUE;` / single bottom `return FALSE;` (matches /Od shared false epilogue): OK.
- TAG_BLOCK_GET_ELEMENT typed access throughout: OK. No new prototype/header/config/symbols.json/parked.json change: OK
  (`git apply --check` clean at repo root; touches only source/structures/structures.c lines 328-406).
- Rule 6: the patch REMOVES production's hand expansion of point_from_line3d (prod rows `projected_position.x = n->i * -d + p->x`)
  and uses the genuine call; `_point_from_line3d` COMDAT already emitted+admitted in this TU (structures_fog_inline_closeout_20260923);
  no call reloc in the function (inlined), surplus unchanged: OK and an improvement.
- Names: `point` (HCEX), projected_points / projected_centroid3d / projected_centroid2d (RTC + HCEX): OK.
- `short projection_sign` from a boolean-returning helper: attested by January `movzx ax,al` (T+0xed) and /Od `mov word [ebp-0x420]`: OK.
- /W3 (reviewer re-run): final adds C4244 double->real (final.c:367, the `t` argument) + 2x C4244 short->boolean (372, 385);
  the plain S2 spelling adds only the two short->boolean ones. The double->real warning is produced solely by the held construct.
- Rule 19 / docs/common_constants.md `double` rule: VIOLATED by `double t` (not API- or vararg-required).
- Rule 20 / brief strip test: FAILS (R4). If the owner nevertheless admits it, the owner's 2026-09-20 inferred-construct precedent
  requires an in-source comment stating it is inferred from January's bytes and not attested (the /Od build contradicts it);
  the patch carries no such comment.

## VERDICT: approve = false (do not apply mechanically)
All worker measurements reproduce exactly (17/17, relocs 11/11, audit PASS, surplus identical, provider FAIL(2) pre-existing and
identical, pdb 0, data 100%). But the only thing that closes the function is a `double` local that (1) fails the strip test both ways
(S1 real t and S2 plain `-plane_distance` are residual `[sha]`), (2) is contradicted by the first-party /Od readout (later source passes
`-plane_distance` directly, no `t` slot) and by the same-TU January sibling `_render_debug_fog_planes`, and (3) breaks the documented
`double` rule. The worker itself marks it HOLD FOR RULING, which is incompatible with a mechanical "apply" instruction. Route to the owner
as an escalation (like the actor_perception inferred macro), not to the integrator. The label FUNCTIONS_EXACT is conditional on that ruling.
Fallback needing no ruling (zero credit, would need a park entry per rule 11): s2_inline_neg.c body = /Od-attested shape, 432/11 `[sha]`,
3 one-slot moves, removes the rule-6 hand expansion, surplus identical to production.
