# review source/objects/widgets/glow
approve=True

## per_object
[
 {
  "unit": "source/objects/widgets/glow (primary: production.patch / cand_final.c, owner-gated)",
  "approve": true,
  "issues": "Approved only as an owner-ruling packet (preserved uninitialised reads, brief s7); do not land without an explicit owner admission. Verified: 25/0/0 exact, object_audit PASS, surplus identical with provider link PASS, 0 PDB disagreements, data 100%, patch applies cleanly and byte-identically, diff --check clean. Casts, the PIN two-statement form and NUMBEROF change no bytes. The sine/cosine locals are load-bearing and attested by /Od. Nits: NUMBEROF against /Od's signed bound (`index < 4` preferred), sine/cosine shadow the real_math.h helpers, possible owner request for a layout assertion (N/A to uninitialised automatics), and the BUG comment range end is exclusive."
 },
 {
  "unit": "source/objects/widgets/glow (fallback: scratch/w/glow/cand_fixed.c zero-credit park re-baseline)",
  "approve": false,
  "issues": "The fallback routes the invented interior left-vector interpolation through point_from_parametric_line, whose y-base z quirk (result->z = forward->k*t + point->y) gives the .k components a .j base. That is new wrong behaviour absent from both January and /Od, and a regression against the current park body's correct .k interpolation. It is also 2544 bytes against the current 2528, at 24/1. Reject as written."
 }
]

## checks
All checks were run at 931ed8dc. Only scratch/w/review2_glow/ was written, plus two scratchpad copies used for the apply tests.

1. `git apply --check scratch/w/glow/production.patch` is clean.
   - A trial apply on fresh copies of the working-copy files gives glow.c, parked.json and config.json byte-identical (cmp) to cand_final.c and the new/config copies. Both JSON files parse.
   - I also ran `git diff --check` in an isolated scratchpad git repo seeded with the CRLF working-copy files, with autocrlf=true to match the project. It is clean: 3 files, +106/-164.
2. Gates (`--all --forbid-emitted-symbol _point_from_line3d`):
   - Baseline real file: 24/1/0, with residual `_get_particle_world_position [size 2528!=1744, relocs 55!=43, sha]`.
   - cand_final.c: 25/0/0. The guard passes and `_get_particle_world_position` is EXACT at 1744.
3. `object_audit` on my cand_final.obj: PASS. 36 January symbols, 0 differ, and every .text and .rdata section is ok. The candidate-only surplus block is identical to the surplus of build/base glow.obj; only the PASS/FAIL line differs.
4. `surplus_identity` (the tool with a one-line CAND_OBJ change) against the candidate object: 10 of 10 code COMDATs are IDENTICAL to their January definers. `provider_link cand_final.obj`: SELECTED-PROVIDER LINK PASS for all 20 surplus symbols, in both orders.
5. `pdb_storage`: 36 symbols, 0 disagreements. `_get_particle_world_position` is absent from cachebeta publics, so it is static, matching symbols.json line 5061 (`"static": true`). symbols.json does not change.
6. Data: build/report.json shows glow at 392/392 (100%), .rdata only. There is no object_admission_rejections entry. The only park reference is the parked.json entry being removed.
7. I disassembled January's function myself (jan_gpwp.txt):
   - `[ebp-0xc4..-0xa0]` is written only by the default-loop indexed stores (+0x21b, +0x23e, +0x259) and read by the spline-3 leas (+0x637..+0x64c).
   - On 3-marker case 1, `fld [esi+0x23c]` at +0x2ea stays on the x87 stack. The midpoint is stored with `fstp [ebp-8]` at +0x5c6. `[ebp-0xc]` is never stored on that path and is read at +0x5d2.
   - The z components use the points[0].y base (the y-base quirk).
   - Every offset in both BUG comments checks out.
8. /Od build (odbuild):
   - Callees: 0x42e3a0 is vector_from_points3d, 0x43e710 is cross_product3d, and 0x79ce50 is point_from_parametric_line with the y-base z.
   - Resolved through the IAT: 0x455320 chains to ucrt `sin` and runs first; 0x455220 chains to ucrt `cos`. Both results are homed and each is reused three times.
   - `[ebp-0xa8]` (traversal_left_vectors) appears only in the default-loop cross_product3d and the spline-3 leas.
   - Case 1 writes time_indices[2] twice and never writes [1].
   - The RTC descriptor names `forward` for the vector_from_points3d output.
   - HCEX: v shares slot 0xC0 with traversal_time_indices, so v is the spline-3 (left) result.
9. Strip tests, each built mechanically from cand_final.c:
   - All casts removed: 25/25. The plain form raises C4133 under /W3, so the casts are the type-correct spelling and change no bytes.
   - One-statement PIN: 25/25. The /Od two-statement form changes no bytes.
   - `index < 4`: 25/25. NUMBEROF changes no bytes.
   - Named sine/cosine removed: 24/1 [sha]. They are load-bearing, but /Od attests them.
   - Genuine `sine()`/`cosine()` helpers: the function is byte-identical, but it adds _sine/_cosine surplus COMDATs.
10. Warnings and scans: /W3 /Zs gives 0 TU-local warnings for both cand_final and the base. fake_match_scan: 0 leads.
11. Re-gated the worker's negative probe cand_order.c: 24/1 [sha], so that finding reproduces. Re-gated the fallback cand_fixed.c: 24/1, [size 2544!=1744, relocs 55!=43].

Notes: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_glow\REVIEW.md

## issues
CONFIRMED verdict: FUNCTIONS_EXACT_OBJECT_BLOCKED, blocked on an owner ruling. The primary packet (production.patch = cand_final body, parked.json entry removal, config.json NonMatching->Matching) is approved ONLY for the owner-ruling queue. It must not be applied until the owner explicitly admits, for this function only, the two preserved uninitialised reads from brief s7:
- traversal_left_vectors is never filled on the 2- and 3-marker paths;
- traversal_time_indices[1] is never set on 3-marker interval 1.

Both defects are proven three ways: January bytes, the later first-party /Od build, and HCEA. Every other element either changes no bytes or has attestation, and the house rules are met.

Non-blocking notes:
(1) The candidate replaced the base's `index < 4` with `index < NUMBEROF(traversal_time_indices)`. That changes no bytes, but /Od shows a SIGNED bound (`cmp ecx,4; jge`), so `4` is the /Od-faithful spelling. House rule 18 allows either.
(2) The local names `sine`/`cosine` shadow the real_math.h `sine()`/`cosine()` inlines. This is legal and warning-free. The /Od call chain has the shape of `sine(angle)`, so the later source must have given its locals other names. Renaming them changes no bytes. Spelling through the helpers adds _sine/_cosine surplus COMDATs, so keeping (real)sin/(real)cos is the safer choice.
(3) The lane-A owner precedent admitted an uninitialised/OOB BUG together with a compile-time layout assertion. Here the reads are of uninitialised automatic arrays with no struct-layout dependency, so no such assertion applies. The owner should be told in case they want one anyway.
(4) BUG comment 1 writes the range as "[ebp-0xc4..-0x94]", but the end is exclusive (-0x94 is `u`). This is cosmetic.

REJECTED: the fallback scratch/w/glow/cand_fixed.c. It fills the interior left vectors through point_from_parametric_line, whose body sets `result->z = forward->k * t + point->y` (glow.c:453). The left-vector .k components therefore get a .j base. That is newly invented wrong behaviour, present in neither January nor the /Od build, since neither ever interpolates left vectors. It is also a behaviour regression against the current parked body, which uses the correct .k base, and it is 16 bytes larger (2544 vs 2528). Do not adopt it as written. If a zero-credit re-baseline is wanted, fix the left-vector z base first, or use n3a v2.
