# REVIEW r2w2: source/items/projectiles, semantic data entry (ADMISSION_FIX_PROPOSED)

Reviewer slug: `scratch/w/review_r2w2_projectiles/`. Tree: HEAD 434f0151 (05255584 + ledger). I made no tracked edits and ran no ninja, configure or git mutation. `git status` still shows only the pre-existing untracked `research/fifty_objects_r2_20260924/`.
No claim was taken, because I edited no candidate source. The worker's claim is released.

**Verdict: APPROVE.** The packet patch is correct as submitted, so no amended patch is needed.

## Independent checks (evidence files in this slug)

| Check | Result | File |
| --- | --- | --- |
| gate --all, production source (fresh compile) | 29 EXACT; `_projectile_new` 432 [sha] is the only residual, and its park is active | gate_prod.txt |
| fresh compile vs build/base | 0 of 121 non-debug sections differ (build is current) | verify_data.txt |
| object_audit (base and fresh) | FAIL(1). The only DIFF is `.text _projectile_new`. `.data` ok 1548/1548, flags eq, align 4/4. 55 January symbols, 0 differ. | object_audit_base.txt, object_audit_prod.txt |
| pdb_storage | 55 split symbols, 0 disagreements | pdb_storage.txt |
| surplus_identity | 25 candidate-only code COMDATs, 0 not identical | surplus_identity.txt |
| provider_link, both orders | PASS for all 56 surplus names, including the normal (game) and gravity (particles) literals | provider_link_base.txt |
| .data section | split sec#54 = base sec#3 = fresh: 1548 B, 0xc0400040, 6 DIR32, sha 7bfaf5fb. Owners `_projectile_update_section`@0 and `_effect_marker_names`@1528 are storage 3 in both. `section_info_resolved` split==base==fresh; `section_infos_equal` True. | verify_data.txt |
| UNDEF literals | @1528 normal and @1544 gravity are UNDEF in January's split. Ours are select-any (selection 2) COMDATs with the same bytes and flags as January's providers (game.obj, particles.obj); section_infos_equal True. | verify_literals.txt |
| Cause proof, objdiff 3.3.1 (sha1 3130e428) | jan_self .data 99.7416; production/fresh 99.096275 (= report.json); '$'->'_' in both: 100.0. Negative control: '$'-renamed plus one tampered payload byte gives 99.93557. Renaming only the two UNDEF literal names gives 99.35467, so the other ?$AA literals are also hit by the same defect. | mini/mini_result.txt |
| Same objects under objdiff 3.6.0 (the configure.py pin) | production .data **100.0 natively**; tampered 99.93557 | mini/mini_result_360.txt |
| Progress emulation (real pass order, patched manifest copy) | +1 data exception, +1548 (halobetacache matched_data 2,583,826 -> 2,585,374). Code, functions and complete_units (502) unchanged. Only projectiles changes. 0 revoked. Parks 83/0/0. Unit stays incomplete. | verify_data.txt |
| Verifier negative controls | sha, relocs, size, missing opt-in and wrong unit all FAIL CLOSED. The other owner of the same section is accepted, as expected. | negctl.txt |
| Admission audit, head vs patched | identical: 9 candidates, 0 contradicted, 2 rejected, 0 revoked; projectiles not a candidate | admission_audit.txt |
| Patch | `git apply --check` OK on the worktree and with `--cached`; `--whitespace=error-all` OK; ASCII, LF. Applying it to the HEAD blob reproduces packet/semantic_data_matches.json byte for byte (ca9e110f). | applied/ |
| Round-1 rule-6 production.patch compatibility | gate 29/1; .data resolved-equal True | gate_r1patch.txt |

## Precedent audit: read at the original commits

- **84414a1a** (owner, "Credit exact data in guarded incomplete units") introduced `allow_incomplete_unit` for `objects` while that unit was incomplete. The verifier docstring says the opt-in exists "so partial spans are never credited accidentally". The fail-closed coverage check (credited size == total_data - matched_data) holds here: 2092 - 544 = 1548.
- **fcd8e378** render_particles is the same mechanism: an incomplete object, a 1528-byte profile section, and csplit leaves the string literal UNDEF while MSVC emits a select-any copy.
- **88d0bd31** race, doc l.21-22 (quoted correctly in the manifest): "allow_incomplete_unit: true ... recovers exact data only and cannot mark Race complete or grant code credit."
- **5019c186** editor_flying_camera (owner, 2026-09-23). This came AFTER the 4f4c8502 retirement of entries under 3.6.0. It added a NEW `allow_incomplete_unit` entry while the unit was NonMatching and `_editor_camera_set_position_and_roll` stayed a parked residual. This is the closest current precedent to projectiles: parked code residual, data-only credit.
- **ae12a3c1** leaf_map is the precedent the worker cites. The quotes at ten_more l.38-45 and reconciliation l.55-56 are verbatim. Its conditions (exact payload, flags, owner, relocation locations/types, resolved destinations, sole unmatched section, no completion) all hold here.
  - Framing nit: the manifest calls projectiles a "stronger" case than leaf_map. That is imprecise. leaf_map had 25/25 functions exact and was incomplete only because of a provider conflict, while projectiles has a parked code residual.
  - This does not stretch the policy. Race and editor_flying_camera cover a parked-residual unit explicitly.
- docs/objdiff_data_relocation_defect_20260922.md says "Do not add new adjudications". Later practice supersedes that line:
  - the owner re-added 20+ entries from 09-23 onward, including the new camera opt-in;
  - brief section 4.5 prescribes this exact route under the frozen 3.3.1 scorer.
- Under the pinned 3.6.0 the single-section entry is a verified no-op: unmatched_data is 0, so the verifier `continue`s. Unlike the grouped camera entry, it cannot break a fresh-worktree `progress`.
- No rejection row and no existing entry for this unit. No source, header or symbols.json change, so the strip test, /Od declaration order and house rules do not apply.

## Notes for the integrator (non-blocking)

1. The hunk context is the final `units` entry. If another manifest append lands first, regenerate the patch. The entry JSON (packet/semantic_data_entry.json) is unchanged.
2. The ledger should cite 5019c186 (editor_flying_camera) and fcd8e378 (render_particles) as the closest precedents, alongside leaf_map.
3. The credit is +1,548 data bytes only. There is no code or function credit, and the object stays at 29/30. `_projectile_new` stays owner-gated (the paren/named-accessor question).
