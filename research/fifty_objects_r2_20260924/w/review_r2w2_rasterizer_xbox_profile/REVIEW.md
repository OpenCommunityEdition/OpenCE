# Adversarial review: rasterizer_xbox_profile (r2w2 hygiene, ADMISSION_FIX_PROPOSED, zero credit)

**Verdict: FULL rejected as submitted, and V1 rejected as submitted. Approve only my amended V1R:
`scratch/w/review_r2w2_rasterizer_xbox_profile/PACKET_rasterizer_xbox_profile_V1R_reviewed.patch`**
(sha256 0a5885fd..., see `patches_sha256.txt`). It scores zero credit, and the unit stays NonMatching with no data entry.

- Base: HEAD 434f0151, which is 05255584 plus a ledger document.
- Claim: `review-r2w2-profile-adv`.
- Full probe log: `LEDGER.md`, R1 to R6.

## Blocking findings

### F1. FULL fails the ninja progress step, and the packet does not disclose it
`config/parked.json:1166` parks `rasterizer_xbox_models:__rasterizer_model_draw` with these pinned values:
- base `normalized_sha256` 89b0d7ea...;
- `objdiff_percent` 95.08453.

What happens under FULL:
- The body changes to `cbfa85852c836af123fe93a3f15f22f2f43ba574fbff9add3c28f0e0275ed4d4`, at 5168 bytes with 348 relocations.
- The worker's own objdiff 3.3.1 mini report puts the function at 95.07629.
- `tools/parked_functions.validate_parked_functions` therefore marks the park **stale**.
- `tools/project_x86.py:717`, `require_valid_parked_functions`, then exits with "Parked-function verification failed" in the progress step.

The FULL packet has no `parked.json` hunk, and neither the MANIFEST nor the ledger mentions the park.

I confirmed the cause independently:
- Moving models.c's own extern earlier, to just after either the internal.h include or the models.h include, gives an object canonically identical to the FULL object.
- So the change comes purely from the declaration position of `global_window_parameters`. The mechanism is correctly identified, but its consequences are not handled.

FULL could land only with all of the following:
- a park re-baseline hunk (sha above and 95.07629, measured with 3.3.1 by the integrator);
- explicit acceptance that a parked residual moves one instruction away from January;
- a fix for its owner file: rasterizer_xbox.c would keep its own local extern (line 886) next to the new header declaration, a verbatim duplicate.

Also, FULL moves only profile's extern and 5 consumer externs into the header. Six other full-type externs and nine partial views remain, so the dynavobgeom reopen text ("reconcile the partial consumer views") is still not met.

### F2. 07R and 07R_V1 stretch the collision_debug SDK-table precedent (special focus)
Both rewrites say that the three stock D3D8.h select-any tables "are now accounted for as authentic topology", and both drop the tables from the reopen criteria. I read the original record:

- **Canonical reconciliation.** `docs/object_matching_logs/claude_fifty_objects_canonical_reconciliation.md` (641e1466) says the stock SDK-table coalescing criterion "conflicts with canonical's existing ruling" and grants "no whole-object credit ... pending a uniform policy/owner audit".
- **Lane ledger.** `claude_fifty_objects_20260925.md:518-541` records the same open policy conflict.
- **Brief.** Section 8 says the dynavobgeom hold reasons still apply.
- **Earlier profile evidence.** `rasterizer_xbox_profile_obj_shared_owner_reconciliation_20260906.md#remaining-whole-object-boundaries` also records that linking against January's split owners gives LNK2005 in both orders. `provider_link.py` tests only our base providers.

So collision_debug's selected-provider standard is the lane's standard. It is not a canonical ruling that settles profile's SDK-table item.

My fix, 07R_V1R:
- It keeps every verified fact:
  - the tables are byte- and flag-identical to January's definers in headers, progress_bar and decals (3/3);
  - pair links PASS in both orders;
  - January's object imports `_D3DDevice_InsertCallback@12` and `QueryPerformance*`.
- It records that the owner policy decision is still open and adds it to reopen.
- It adds the park re-baseline to the future window-move reopen clause.

## What I verified independently (V1 and V1R)

| Check | Result |
|---|---|
| `git apply --check` at HEAD | FULL rc 0, V1 rc 0, V1R rc 0; V1R round-trip is byte-equal to the tested tree |
| Consumer sweep (own `sweep.py` + `canon.py`: every non-debug section's bytes and relocations by name, plus the symbol table) over all 10 includers of the changed files | control 10/10 IDENT (harness valid); FULL 9/10 (models `__rasterizer_model_draw` only); V1 10/10 and V1R 10/10 IDENT |
| gate (every January function) | profile 16/16 EXACT; detail_objects 20/20 EXACT |
| object_audit (no symbols.json change, so build/split is the emulated split) | profile PASS (67 symbols, 0 differ; .data 132/132, .bss 1122/1122). Surplus is the same as production: .drectve, 4 literals, 3 SDK tables, 3 `__real`. detail_objects FAIL(14) is identical to the production audit (pre-existing storage packet) |
| pdb_storage / surplus_identity | 0 disagreements / 0 candidate-only code COMDATs |
| provider_link | 10/10 PASS in both orders |
| data | .data and .bss coff-equal; report .data 68.69% is the known '$' defect; no data entry in the packet (correct) |
| `CL /Zs /W3` | profile 12 and detail_objects 12 warnings, identical in ctl, V1, FULL and V1R |
| fake_match_scan | 0 leads |
| admission audit | 9/0/2/0 with the production, FULL and V1 rejection files; the edited entry loads through `rejection_index` |

## Per-construct review (V1R contents)

- **04, `rasterizer_profile_check(condition, profile, message)`.**
  - The /Od build at 0x8004c0 (rasterizer_dx9_profile.c, a later revision) has condition at [ebp+8], profile at [ebp+0xc] and message at [ebp+0x10].
  - January's private ABI (esi = message, di = profile, stack = condition) does not depend on parameter order. Measured IDENT.
  - This is an attested, byte-inert declaration fix.
- **06, detail_objects.**
  - The file now uses rasterizer_xbox.h for rasterizer_error, profile_begin/end, set_pixel_shader, set_texture, set_vertex_shader_permutation, global_d3d_device and pixel_shader.
  - The removed consumer-local `rasterizer_set_texture` prototype had the wrong return type (void instead of `union point2d *`).
  - `_rasterizer_profile_detail_objects` is rasterizer.h:70 (value 21; names[21] = "detail objects").
  - Byte-inert.
  - The pre-existing `main_get_window_count` local prototype and the `detail_objects_debug_options` view remain as disclosed debt in an already-Matching object.
- **08, BUG comment.**
  - It uses the standard "BUG (preserved for exact matching)" marker on the already-landed NONE-branch defect.
  - I re-read the claims: January pushes esi (the message) before `push -1`, and /Od pushes the message and then the profile.
  - It does not admit B2.
- **03 listing hunk (added by me; byte-inert).**
  - The header's "symbols in this file" block was stale: it still showed `_rasterizer_profile_globals (0000)` and `_bss_00465e28`.
  - `config/symbols.json` already carries the static rows `_rasterizer_profile_performance_counter_frequency` at 0x30CF00 and eight .bss statics from 0x465E28.
  - The hunk syncs the listing to those rows. I checked every offset against symbols.json.
  - It is identical to FULL's 03 listing text.
- **No new scopes, locals, casts, qualifiers or directives.** A strip test has nothing to act on. Held items B1, B2 and B3 are untouched.

## Production changes (integrator)
1. Apply `scratch/w/review_r2w2_rasterizer_xbox_profile/PACKET_rasterizer_xbox_profile_V1R_reviewed.patch`. It touches 3 files: profile.c, detail_objects.c and `config/object_admission_rejections.json`.
   - The patch cites the tracked evidence path (review3 REVIEW.md) and the tracked reconciliation document.
   - Expect a full ninja build with 0 moved rows and valid parks.
2. Do not apply FULL unless the owner accepts the models park re-baseline (F1).
3. Owner questions carried forward: B1 (aggregates and the +0x45c object), B2 (the varargs bug), B3 (volatile), and the uniform stock SDK-table policy.
