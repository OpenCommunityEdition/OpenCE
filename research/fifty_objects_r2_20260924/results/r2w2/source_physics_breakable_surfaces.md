# source/physics/breakable_surfaces (admit source/physics/breakable_surfaces)

## verdict
BLOCKED

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hygiene_r2w2\\breakable_surfaces\\PACKET_breakable_surfaces.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\hygiene_r2w2\\breakable_surfaces\\MANIFEST.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\review_r2w2_breakable_surfaces\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-r2-20260924\\scratch\\w\\r2w2_breakable_surfaces\\LEDGER.md"
]

## production_changes
None from this worker. The claim was refused, so I built no packet. The same sub-packet already exists, built by another worker: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hygiene_r2w2\breakable_surfaces\PACKET_breakable_surfaces.patch (= 01+02+03). (01) Delete the TU-local hand copy breakable_surface_get_plane_from_designator and call bsp3d_get_plane_from_designator. (02) At config.json:520, drop the per-TU options /Ow /QIfist. (03) Add "static": true in place to symbols.json rows 5362 (_breakable_surface_effect) and 23116 (_globals). This re-cuts the round-1 symbols patch, which failed because its context row _collision_bsp_usage_times is now static. Then run a csplit-only regen. That packet is currently under adversarial review by review-r2w2-breakable_surfaces.

## evidence
Claim: `claim.py claim source/physics/breakable_surfaces r2w2_breakable_surfaces` returned exit 3, HELD by review-r2w2-breakable_surfaces. claims.log:37/49 shows r2w2-hygiene-breakable_surfaces already built and released this exact packet as ADMISSION_FIX_PROPOSED (zero credit). My own check, which compiled nothing: `git apply --check` on PACKET_breakable_surfaces.patch returns rc 0 at 434f0151, and git status is unchanged. From the existing evidence files, all read-only: gate against the emulated split is 12/12 EXACT. The emulated csplit differs only in breakable_surfaces.obj (833 compared). pdb_storage finds 0 disagreements after the patch (2 before). surplus_identity has 1 of 26 not identical: _real_local_random vs effects, which is B1. _bsp3d_get_plane_from_designator is IDENTICAL to January's decals copy, and decals is January's only definer. provider_link has one FAIL, _real_local_random, which is pre-existing and the same as production. The flag-only sweep has 611 of 612 identical, with only this object differing. The reviewer's ledger independently reproduced three results: A==F (/QIfist is dead), B vs D differ only in the helper swap, and object_audit PASSes 26/26 against the emulated split. On the precedent: the rule-6 exception conditions hold, since the helper copy is byte-identical, the caller is EXACT, the sweep has zero regressions and the provider link passes for that row. Rule 22 also holds: /QIfist is dead and /Ow has no evidence, and dropping /Ow makes 6 helper COMDATs IDENTICAL. Storage matches cachebeta publics. Ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_breakable_surfaces\LEDGER.md

## blockers
(1) The claim is held by review-r2w2-breakable_surfaces; the brief section 7 says exit 3 means do not probe for landing. My task would duplicate an existing packet that is under active review. (2) Even after this packet lands, whole-object completion stays blocked. B1: _real_local_random is DIFFERENT and fails provider_link against effects. It needs the real_math.h named-local body, but real_math is RESERVED for the Codex packet, and that change costs _rasterizer_frame_statistics_draw and _bitmap_copy through declaration count. B2: the static surplus _breakable_surface_plane_distance (32 B) needs an owner ruling on the bsp3d precedent. The sub-packet earns zero credit.

## reopen_criteria
Reopen for this worker only if the reviewer rejects the hygiene packet and releases the claim; then rebuild from the review findings. For whole-object admission, use the round-1 criteria: B1 needs a Codex real_math.h outcome plus an attested declaration-count compensator or owner acceptance, and B2 needs an owner ruling on the bsp3d TU-local plane-distance precedent.

## manifest
No new MANIFEST: I produced no packet. The existing manifest is C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\hygiene_r2w2\breakable_surfaces\MANIFEST.md. It has no dedicated precedent section quoting the ruling. My LEDGER.md has a read-only precedent-condition cross-check, citing docs/campaign_house_rules.md:9, 13, 28-31 and 33-40, that the integrator or reviewer can attach.

ledger: C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\r2w2_breakable_surfaces\LEDGER.md
