# REVIEW source/items/projectiles

approve: True

amended_patch: 

## per_object
null

## checks
All run independently at HEAD 434f0151. Evidence is in C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/review_r2w2_projectiles/ (REVIEW.md plus raw outputs).
(1) gate.py --all on production source: 29 EXACT. The only residual is _projectile_new 432 [sha], with an active park. The fresh compile matches build/base in 0/121 non-debug sections.
(2) object_audit on base and on the fresh object: FAIL(1). The only DIFF is .text _projectile_new. .data ok 1548/1548, flags eq, align 4/4. 55 January symbols, 0 differ.
(3) pdb_storage: 0 disagreements.
(4) surplus_identity: 25 COMDATs, 0 not identical.
(5) provider_link: PASS in both orders for all 56 surplus names, including the normal (game) and gravity (particles) literals.
(6) Data section: split sec#54, base sec#3 and the fresh object all have 1548 B, flags 0xc0400040, 6 DIR32, sha 7bfaf5fb. Owners _projectile_update_section@0 and _effect_marker_names@1528 are static on both sides. section_info_resolved gives split==base==fresh, and section_infos_equal is True.
(7) The two literals January leaves UNDEF (@1528 normal, @1544 gravity) are select-any (selection 2) COMDATs in ours, with identical bytes and flags to January's game.obj and particles.obj providers.
(8) My own objdiff 3.3.1 (3130e428) mini report:
  - January vs itself: .data 99.7416.
  - Production and fresh: 99.096275, which equals report.json.
  - '$'->'_' in both objects: 100.0.
  - Negative control, rename plus one tampered payload byte: 99.93557.
  - The same objects under 3.6.0 (the configure.py pin) score production .data 100.0 natively, so the entry is a verified no-op under the pinned scorer and cannot break a fresh-worktree progress run.
(9) In-memory progress emulation with the real pass order on a patched copy of the manifest:
  - +1 data exception, +1548 (halobetacache 2,583,826 -> 2,585,374).
  - Code, functions and complete_units (502) unchanged.
  - Only projectiles changes, 0 revoked, parks 83/0/0, and the unit stays incomplete.
  - Admission audit is identical with and without the entry.
(10) Verifier negative controls: a wrong sha, relocation count, size, unit, or a missing allow_incomplete_unit each fail closed.
(11) Patch checks:
  - git apply --check passes on the worktree, with --cached, and with --whitespace=error-all.
  - The patch is ASCII and LF.
  - Applying it to the HEAD blob reproduces packet/semantic_data_matches.json byte for byte (ca9e110f).
(12) The round-1 rule-6 production.patch still gates 29/1, and its .data stays resolved-equal.
(13) Precedent audit, read at the original commits:
  - 84414a1a introduced the opt-in for exact data in incomplete units, first used for objects while that unit was incomplete.
  - fcd8e378 render_particles uses the identical profile-section plus UNDEF-literal mechanism.
  - 88d0bd31 race doc l.21-22 says data only, with no completion and no code credit.
  - 5019c186 editor_flying_camera is an owner commit made after the 3.6.0 retirement. It added a new allow_incomplete_unit entry while the unit was NonMatching with a parked code residual.
  - ae12a3c1 leaf_map: the quotes at ten_more l.38-45 and reconciliation l.55-56 are verbatim.
  - No rejection row and no existing entry for this unit. No source, header or symbols.json change, so the strip test, /Od declaration order and house rules do not apply.

## issues
Nothing blocks this. Every condition in the precedent holds, and the policy is not stretched. Four notes for the integrator:
(a) MANIFEST.md says projectiles is a 'stronger' case than leaf_map. That is imprecise. leaf_map had all 25/25 functions exact and was incomplete only because of a provider conflict; projectiles has a parked code residual. The owner's incomplete-unit data policy covers that case explicitly: 5019c186 editor_flying_camera (parked residual, data-only opt-in, added after the 3.6.0 retirement), 88d0bd31 race, and fcd8e378 render_particles, which uses the identical UNDEF-literal profile-section mechanism. The integration ledger should cite those as the closest precedents.
(b) docs/objdiff_data_relocation_defect_20260922.md says 'do not add new adjudications'. The owner's later practice supersedes that: 20+ entries were re-added on 09-23, including the new camera opt-in. Brief section 4.5 also prescribes this route under the frozen 3.3.1 scorer. Under the pinned 3.6.0 the entry is a no-op.
(c) The hunk context is the last array entry (units). If any other manifest append lands first, regenerate the patch from packet/semantic_data_entry.json. No other pending round-2 packet touches the file right now.
(d) The credit is +1,548 data bytes only. There is no code or function credit, and the object stays at 29/30. _projectile_new stays owner-gated.
The packet patch is approved as submitted: C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/r2w2_projectiles/packet/semantic_data_matches.patch (sha256 4384521a...). No amended patch is needed. The review is at C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/review_r2w2_projectiles/REVIEW.md.
