# REVIEW3: rasterizer_xbox_dynavobgeom (claim: OBJECT_COMPLETE_CANDIDATE)

Reviewer slug: `scratch/w/review3_rasterizer_xbox_dynavobgeom/`. Worker slug: `scratch/w/dynavobgeom/`.
Tree: 954eebd2. Full probe log: `LEDGER.md` in this directory.

## Verdict

**APPROVE, using the amended packet** `admission_packet_amended.patch`. The worker's own packet is
also admissible. The amendment changes one pre-existing statement and is byte-inert: the whole object
is IDENTICAL, 51/51 sections.

The amendment is a single change: `largest_dynamic_unlit_triangle_count = MAX(triangle_count, largest_dynamic_unlit_triangle_count);`
replaces the hand-written `if`.
- **Evidence:** at /Od 0x7d8604..0x7d8623 the build computes `cmp tc,[largest]; jle; tmp=tc | tmp=largest; largest=tmp`.
  That is a `?:` temporary, which is the MAX expansion.
- **The worker's claim does not hold.** Disclosure (b) says only the `if` reproduces January's conditional store. The
  MAX spelling does too: 17/17, and the whole object is IDENTICAL. The bytes cannot tell the two apart, so the /Od
  shape and house rule 18 (cseries.h macros) decide it.

## Independent re-verification (every item re-run by the reviewer)

| Check | Result |
|---|---|
| All 4 patches and admission_packet.patch: `git apply --check` at the current tree | OK. Post-image .c == cand_final.c; symbols.json == worker copy (10 lines changed in place, CRLF kept) |
| gate.py vs the current split | 16/17. The only residual is the unlit draw's [reloc-identity], the latch rename. Expected |
| csplit regen (patched symbols.json + config.json) -> `split/` | 833 objects. ONLY rasterizer_xbox_dynavobgeom.obj differs from build/split. Control: csplit with the unmodified config == build/split |
| gate vs the regen split (worker cand and amended) | 17/17 EXACT |
| object_audit vs the regen split | 31/31 sections ok, 31 January symbols with 0 differences, OBJECT AUDIT: PASS |
| surplus (own script: every candidate-only external definition vs every January split definer; section_infos_equal + raw bytes + flags) | 19 surplus, 19 IDENTICAL, 0 problems |
| provider_link | 19/19 PASS in both orders; SELECTED-PROVIDER LINK: PASS |
| nodup_census | NODUP [] for the 4 code COMDATs and the 3 SDK tables |
| cachebeta publics, checked BY ADDRESS (pdb_storage.py matches names only) | No public in 0x465980..0x465B00. The latch is static. No public at any of the 9 wrapper offsets. Control: the unlit draw at 0x14EE80 is public |
| HCEX | The unlit draw has `zero_plane` (frame) and `Static Local unsigned char warned` |
| /Od 0x7d82d0 | vector_from_points3d(camera.position, centroid, &forward) comes before new_group. -dot_product3d(camera.forward, &forward). RTC records `forward` 12 and `zero_plane` 16. Frame declaration order: group, forward, zero_plane. The candidate agrees. No new block scope |
| /W3 /Zs | 0 warnings in the TU (12 in shared headers, the same as base) |
| fake_match_scan | 0 leads |
| objdiff 3.3.1 one-unit (amended vs regen split) | Data 525/525; 16/17 functions. The quad draw scores 97.74, the same under-credit as production. tools/audit_object_admission.py already gives function_gap 0 through the semantic pipeline |
| Admission audit simulation (complete=True, patched rejections/symbols) | 0 contradicted, 0 revoked. dynavobgeom leaves `rejected` |
| Layer-2 contingency (shadow real_math.h without the lock/unlock prototype move) | Still 17/17, so the result is NOT contingent on that ruling |

## House-rule / section-7 / section-8 review of the diff

Six things were removed:
- the opaque debug-options view;
- the window partial view;
- the local pixel_shader copy and its size assert;
- the shader-effect prefix view;
- the opaque `effect_type + reserved16[0x26]` span, which becomes the shared `struct render_model_effect`, as in the
  Matching active_camouflage copy;
- the hand-expanded vector_from_points3d / dot_product3d.

What remains after the diff:
- Every remaining TU-local struct is complete and has no opaque span: transparent_geometry_group has natural pads
  only, and rasterizer_meter_parameters is complete.
- No header was edited, so there is no consumer list.
- There is no float-for-real, no invented name, and no consumer-local function prototype.
- The two new helper COMDATs meet the rule-6 exception:
  - identical to January's selected copies (action_charge / action_alert);
  - the caller is strictly exact;
  - pair links pass in both orders;
  - the collision_debug / leaf_map / model_animations precedent covers it.

The latch storage and name come from first-party sources: HCEX gives the static local, and no cachebeta public exists
at that address. The mangled `?warned@?1??...@@9@9` follows 9 existing symbols.json rows.

Strip tests:
- `static boolean warned = FALSE;` spelled uninitialised, or declared first: IDENTICAL (worker ctl_uninit / cand2).
  Neither form is decoration.
- The helper calls are the plain spelling. Nothing in the diff depends on a decoration.

## Disclosed debt, advisory (not blockers)

- **The window global** is a consumer-local `extern struct rasterizer_window_begin_parameters global_window_parameters;`.
  It uses the genuine complete type and is the exact declaration the owner rasterizer_xbox.c uses (line 886).
  7 Matching objects declare it the same way, and no header declares it.
- **The quad-draw bare block** is pre-existing and not in this diff. It is load-bearing: hoisting its 5 locals gives
  size 2976 != 2960. The escape law justifies it: its arrays reach the out-of-line SetVertexShaderConstant wrapper,
  and map_color reaches real_argb_color_to_pixel32. The /Od quad draw is the dx9 body, so it cannot attest either way.
- **Header parameter names.** rasterizer_xbox_dynavobgeom.h names the unlit draw's parameters `bitmap`/`vertex_count`.
  The definition (HCEX) uses `primary_map`/`triangle_count`. This is cosmetic.
- **A worker ledger inaccuracy.** It says January's object UNDEF-references "every surplus literal". Only 2 of the 7
  literals are referenced, though all 5 `__real` constants are. The other 5 literals belong to the folded
  real_*_to_pixel32 COMDAT bodies. The conclusion is unchanged.

## Production changes for the integrator

Apply `scratch/w/review3_rasterizer_xbox_dynavobgeom/admission_packet_amended.patch`, which concatenates four patches:
- `rasterizer_xbox_dynavobgeom.c.patch`: the worker's .c patch plus MAX. Its post-image == `amended_final.c`.
- `symbols.json.patch`: unchanged from the worker. The full copy is `config/symbols.json` in this slug.
- `config.json.patch`: sets the unit to Matching.
- `object_admission_rejections.json.patch`: retires the entry.

The .c patch and symbols.json must land together. Then:
1. Rebuild. Ninja reruns csplit, and only this object changes.
2. Run the full ninja, progress, the stable sweep, the admission audit, parks, pytest and `git diff --check`.

The worker's `admission_packet.patch` is an equally valid fallback: its object is identical.
