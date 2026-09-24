# review source/interface/first_person_weapons
approve=True

## per_object
[
 {
  "unit": "source/interface/first_person_weapons :: production.patch (A, land now at zero credit)",
  "approve": true,
  "issues": "None blocking. Verified 33/1 with the patched header. Every non-update section is identical to production. Regeneration changes only the storage of message (2 -> 3), and cachebeta publics plus HCEX.pdb both attest it is static. Zero effect across the 10 header consumers. Park measurements validate (active). /W3 is clean. The one scanner lead (the empty then-arm) is attested by the /Od build and January's bytes. Apply the symbols.json edit, then regenerate the split with csplit only."
 },
 {
  "unit": "source/interface/first_person_weapons :: owner_gated_exact.patch (B)",
  "approve": false,
  "issues": "HOLD for an owner ruling; not landable. Technically verified: 34/34 EXACT, object_audit PASS against the regenerated split, 3 surplus COMDATs identical to January's selected providers, provider link PASS in both orders, no NODUP conflict, and dropping the park is correct if it is admitted. The blocker is that it reproduces the authentic unguarded NULL dereference, an owner-gated class under brief section 7. Its BUG comments are accurate."
 }
]

## checks
All checks were independent re-runs, recorded in C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\review2_first_person_weapons\REVIEW.md.

1. Patches apply. `git apply --check` is clean at 931ed8dc for production.patch (the same file as prodA.patch) and owner_gated_exact.patch (the same file as prodB.patch). I applied each patch to the HEAD blobs outside the repo. The results are byte-identical to the worker's final_landable.c, final_exact.c, the shadow header, config/symbols.json and the prodA/prodB copies of parked.json.

2. Gate. gate.py copies `--source` into scratch/, so a header placed next to the candidate is ignored. I therefore pointed the include at the patched header with `--edits`, and proved that redirect is live with an #error lab header.
   - A: 33 exact, 1 residual. `_first_person_weapon_update` is 1520 bytes / 60 relocs, sha ba39884c. Every other non-debug section matches a fresh production compile (section_infos_equal).
   - B: 34/0/0 EXACT.
   - The `_point_from_line3d` guard passed for both. My objects are section-identical to the worker's final_*_t.obj.

3. Split regeneration. I ran csplit into scratch with the base and the patched symbols.json.
   - Base regeneration: all 833 objects are byte-identical to build/split.
   - Patched regeneration: only first_person_weapons.obj changes, and only the storage class of `_first_person_weapon_message` (2 -> 3).
   - object_audit against the regenerated split: B passes (45/45 symbols, every January section). A differs only in update.

4. Storage. cachebeta publics do not list `_first_person_weapon_message`, and pdb_storage reports that as its only disagreement. HCEX.pdb also declares it `static void first_person_weapon_message(short, short)`. An exact-name scan of every object in build/split and build/base finds it only in this object. Its only callers in source are message_from_unit and message_from_weapon in the same TU.

5. Surplus in B. surplus_cand.py: all 3 COMDATs are IDENTICAL to the selected copies in action_charge / action_alert. provider_link.py: SELECTED-PROVIDER LINK PASS, both link orders. nodup_census: 0 NODUP definers of `_magnitude3d`, `_magnitude_squared3d` or `_square_root`.

6. Evidence re-verified from the bytes.
   - January @0x13c is `cmp ax,1; je 0x150`, which skips the next_state call. Production has `je 0x148`.
   - The /Od build at 0x61fb2e has the `jne; jmp end` sequence an empty then-arm produces.
   - /Od 0x61fb22 and 0x61fc59 both call 0x61c0b0, a 3-argument wrapper that pushes 0 and calls animation_update_internal.
   - The /Od RTC descriptor lists `turning` (8 bytes) and `sound_definition_index` (4 bytes). HCEX.pdb lists `sound_definition_index`.
   - January 0x23e..0x255 dereferences the possibly-NULL pointer (`xor eax,eax` path, then `cmp [eax+0x10],3`).
   - n1 (guards restored) re-measured: exactly 4 extra instructions, two `test eax,eax; je` pairs.

7. Strip and necessity probes, compiled on B:
   - Plain `== will_restart` condition: residual.
   - `switch` spelling: residual.
   - `!= key_frame && == will_restart`: EXACT, but /Od shows the empty-then shape, not this one.
   - State call reverted to animation_update_internal: residual. Moving call reverted: inert. Both reverted: residual.
   - Scalars instead of `turning`: frame `sub esp,0x10` against January's 0x14.

8. Header blast radius. I searched for includers: there are exactly 10 consumer .c files and no header includes this one. My own sweep compiled each with the production header and with patch A's header. Gate rows, every non-debug section and the symbol tables are identical for all 10.

9. objdiff 3.3.1 (build/tools):
   - A update 95.21506, which equals the new park value.
   - Production update 93.48387, which equals the current park value.
   - B update 100.
   - Data 536/536 in all three.
   - message and next_state are credited through the accepted_ledger in semantic_report.json (semantic-coff), so no new semantic_matches entries are needed.

10. Park validation. validate_parked_functions on a mini project (candidate A, regenerated split, A's park entry): 1 active, 0 stale, 0 invalid.

11. /W3 /Zs, with /showIncludes confirming the patched header: 0 warnings in the candidate file for A or B.

12. fake_match_scan: production has 0 leads. A and B each have 1 lead (empty-then-else), which the /Od build and January's bytes authenticate. This matches the Lane B actions precedent.

13. I read the brief, the house rules, the prior first_person_weapons ledgers, the park entry and the worker ledger. No recorded inert experiment was repeated.

## issues
Nothing blocks patch A, and the verdict FUNCTIONS_EXACT_OBJECT_BLOCKED holds.

The empty key-frame then-arm is the only unusual construct, and it fails the literal strip test: the plain `==2` spelling is residual. It is kept because it is independently attested. January's bytes contain the `cmp ax,1`, and the /Od build has the empty-then `jne; jmp` sequence. Its behaviour is identical to January's. An owner may still want to see it.

Patch B must NOT be landed without an owner ruling. It reproduces January's authentic unguarded NULL dereference (the brief section 7 owner-gated class). January, the /Od build and HCEA 2011 all share that dereference.

Minor, non-blocking:
- gate.py ignores a header placed next to `--source`. The worker's *_t.c variants handled this correctly.
- The park evidence cites untracked scratch/ paths. parked.json already contains precedents for this, but the integrator's docs ledger should carry the reference.
- Outside this patch: the `animation_update_render_only` body passes a literal 0 instead of `animation_update_kind_render_only` (house rule 16). This can be a separate byte-inert cleanup.
