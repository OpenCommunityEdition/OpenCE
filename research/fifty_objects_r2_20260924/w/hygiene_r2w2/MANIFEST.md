# MANIFEST (index) - r2w2 hygiene: zero-credit ownership packets re-verified at 05255584

Every packet below scores zero credit and is ADMISSION_FIX_PROPOSED. No tracked file was edited; `git status` shows only the untracked `research/fifty_objects_r2_20260924/`, which is not mine.

Each unit has its own MANIFEST, its combined packet, its individual patches, gate/audit outputs and fntable. Checksums are in `patches_sha256.txt`.

| unit | combined packet | strict effect | storage / audit effect |
|---|---|---|---|
| text/draw_string | draw_string/PACKET_draw_string.patch (src + symbols) | 27/1 unchanged (bug row owner-held) | pdb 1->0; 4 HCEX file statics |
| physics/breakable_surfaces | breakable_surfaces/PACKET_breakable_surfaces.patch (src + config + symbols) | 12/12 unchanged | audit FAIL(2)->PASS; pdb 2->0; surplus non-identical 7->1 (B1) |
| bink/bink_playback | bink_playback/PACKET_bink_playback.patch (src + symbols) | 23/1 unchanged (q3 owner row) | pdb 5->1 (_code_001b5850 naming item) |
| rasterizer/xbox/rasterizer_xbox_profile | profile/PACKET_rasterizer_xbox_profile_full.patch (10 files incl. 07R) OR profile/PACKET_..._V1_byte_inert_subset.patch | 16/16; all exact rows unchanged | window/API owner debt resolved (FULL) |
| game/players | none: the round-1 packet and its COMMON supplement already landed in 6e3e2d35 | - | pdb 2 (owner-gated header statics) |

## Composition, all four packets together
- **Sequential application.** The four combined packets apply in sequence onto HEAD blobs: draw_string, breakable, bink, then profile-full, all rc 0.
- **symbols.json.** The union of the edits is 7 rows removed and 10 added. The emulated split `all_emu` changes exactly 3 of 833 objects: draw_string, breakable_surfaces and bink_playback. Each of those objects is identical to its per-unit emulated split.
- **Full sweep.** The composed tree `t_all` (breakable flags override) against the control gives 608 identical and 4 differing: draw_string, breakable_surfaces, bink_playback (renames and storage) and rasterizer_xbox_models (the disclosed residual shift from profile-full). All 6 touched objects are canonically identical to their per-unit candidates.

## Integrator notes
- The three symbols.json packets need a csplit-only regeneration.
- The breakable config.json change removes a per-TU option, so rerun configure to rebuild build.ninja.
- Integrator commands (ninja, configure, commit) were NOT run here.

## Harness (slug-local, reusable)
- `rsweep.py` and `compose.py` (from review_r2_xbox_texture_cache): FLAGS_OVERRIDE added.
- `emu.py`: config copy, apply patches, csplit into the slug, diff against build/split.
- `mkpatch.py`: git-style LF patches from HEAD blobs; `symedit` does in-place row edits.
- `roundtrip.py`, `canon_cmp.py`, `fngate.py`, `fntable.py`, `closeness.py`, `w3.py`, `warncmp.py`.
- `*_emu.py` variants of gate, object_audit, pdb_storage and surplus_identity (SPLIT_ROOT / OURS_OBJ / FLAGS_FROM).
- Control: the tbase tree compiled 612/612 canonically identical to build/base.
