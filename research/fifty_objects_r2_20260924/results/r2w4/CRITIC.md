# Completeness critic

I read everything the campaign produced: the brief, the R2 ledger, board/hints, the r2w1/r2w2/r2w3 results, claims and the round-1 owner queue. I then re-ran the admission battery myself (object_audit, pdb_storage, surplus_identity, provider_link) on all 83 incomplete source units at HEAD 5d3ca708, using the R2-2 build. Evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_r2\ (LEDGER.md C1-C9, MANIFEST.md, battery/, lab/). I made no tracked edits. The two lab probes were claimed and then released under the label critic_r2.

Most families have been triaged thoroughly. The gaps are in how evidence was used, not in how hard the residuals were worked:

(1) Sept-2001 first-party names were used shallowly. The raw linker maps (…/earlier-map-archives/2001-09-25/cachebeta.map, cache.map, 2001-08-15/cachebeta.map) were never read directly. R2 used the atlas copy, and that copy is lossy: 8,568 of 11,172 raw function records, and in section 1 only 1,975 of 3,115 statics and 4,655 of 5,629 publics. Even the atlas names that do exist were applied to only about 4 objects. My name alignment (anchors plus equal size) finds 72 first-party names in 21 incomplete units: 57 are in the atlas and 15 are only in the raw map. They include 20 `_code_` placeholders in ai_debug, and bink's `_code_001b5850`, which is one of bink's two completion blockers.

(2) A PDB storage check against the publics' addresses found hs_compile storage errors that no lane had flagged.

(3) A cross-build census of all 128 residuals against the Sept-2001 cachebeta.xbe and the Aug-2001 1749betaP.xbe, with bodies located through the raw maps and relocation-masked capstone streams (lab/xbuild.txt):
- 84 residuals are identical between Sept and January, so their source was stable from Sept 2001.
- For solo, the Sept build is identical to OUR output. That supports R2-3's reading of solo as a whole-TU declaration-count tie.
- `_sphere_intersects_cluster_portal` is new since Sept: the Sept frame is 0x408 against January's 0x414, and Sept has no 12-byte point local or fchs point computation.
- Beyond solo, this gives no closure lever.

(4) Negatives that strengthen existing holds:
- bitmap_drawing: the 16-byte prefix (ff×10, 00×6) is already present and unreferenced in both the Aug and Sept 2001 builds, so no user or name can be recovered.
- A Sept name can be stale. The Sept map calls the rasterizer_xbox function `rasterizer_filthy_bitmap_defaults_initialize`, but January's own error string says `..._default_initialize`. Any rename packet must check January's strings and asserts first.

(5) There is a metric/admission inconsistency. Of the 384 Matching objects:
- 74 are PHONY-only audit artifacts;
- 77 fail object_audit and also disagree with PDB storage;
- 71 pass the audit but disagree with PDB storage.
So the R2 admission battery is stricter than what about 148 already-counted objects meet.

Things I checked and found already covered, so no new route: periodic's .bss alignment (round-1 data_gap), interface's static storage (owner pick A/B), the hs, actions and projectiles data gaps (the '$' defect is known), the actor_combat invented inline macros (Codex packet), and the weather frame delta (/Od `unused_count`).

Codex-reserved units (e): the other four dependents fail only the `_cross_product2d` link and have 0 data gap, so they complete with the Codex packet. collision_features does not: it also carries an active admission rejection whose stated cause no longer reproduces.

## source/bink/bink_playback (p=0.4)

- route: Close bink's NAMING blocker with a first-party name. Rename `_code_001b5850` to `bink_decompress_audio_frame`, make it `static void bink_decompress_audio_frame(void){return;}`, and add one call next to `bink_decompress_video_frame()` in bink_playback_render. That call inlines to nothing (byte-inert), which follows the R2-1 hardware_geometry MoveResourceMemory precedent. Change symbols.json row 0x1b5850 to _bink_decompress_audio_frame with "static": true, then regenerate csplit. After that, the q3 ruling is bink's only blocker. Optional hygiene: rename _bink_playback_idle (0x1b5e30, 0xa0) to _bink_playback_update__internal.
- evidence: Name and storage:
- Sept-2001 cachebeta.map Static symbols: 0001:001b3090 _bink_decompress_audio_frame, 0x10 bytes, between _bink_free@4 (001b2fd0) and _bink_draw_frame (001b30a0). January 0x1b5850 is also 0x10 bytes, between _bink_free@4 (0x1b5790) and _bink_draw_frame (0x1b5860): the same spacing.
- Retail cache.map lists it as a static, @bink_decompress_audio_frame@0 at 0x54d870. The bytes are c3 90… with 0 E8 calls and 0 absolute refs. The Sept xbe copy at 0x1c4090 is also c3, with 0 refs.
- It is also in atlas 7eacac85.
- cachebeta PDB: not public, so January storage is static.

Lab (scratch/w/critic_r2/bink/):
- cand_static_used.c: _code_001b5850 is emitted EXACT, 23/24 EXACT (unchanged), provider_link PASS.
- The object audit leaves only the q3 .text row plus the storage row that the symbols.json flag fixes.
- cand_static_unused.c: the stub is not emitted, which confirms the static-COMDAT law.
- why untried: The R2-1 triage and the R2-2 review (REVIEW__source_bink_bink_playback.md:31) classed _code_001b5850 as an owner naming item. They checked neither the atlas record nor the raw map, and noted that making it static drops the section, without testing a static that is actually used.

## source/physics/collision_features (p=0.7)

- route: Codex-reserved; prepare this alongside the Codex packet. Retire or rewrite the stale config/object_admission_rejections.json entry (class candidate-only-comdat-owner, symbol _projection_from_vector3d). Without that, the Codex cross_product2d repair still leaves collision_features rejected by audit_object_admission.
- evidence: - scratch/campaign/gate_r2b2/admission.log reports collision_features as object-admission-rejected (rejected_count 2). The stated reason is that the surplus body reverses the x/y dominant-axis choice and differs from decals' copy.
- At HEAD, surplus_identity reports _projection_from_vector3d IDENTICAL to source/effects/decals, and provider_link reports it PASS in both orders.
- Everything else is clean: object_audit PASS, pdb_storage 0/30, data gap 0.
- The only remaining failure is the _cross_product2d link against actor_combat (battery/source__physics__collision_features.*.txt). The rejection's own reopen criteria appear to be met.
- why untried: The ledger's owner-escalation table says the Codex packet completes collision_features with 'none known' cost. No lane re-validated the standing rejection against the current surplus identity.

## source/hs/hs_compile (p=0.85)

- route: Zero-credit storage packet that no lane has prepared:
- Drop `static` from whitespace_characters, eol_characters and hs_type_primitive_parsers (hs_compile.c:634-636), and remove "static": true from symbols.json rows 16255-16257.
- Make hs_compile_finish static, with a TU-local prototype. Remove its prototype from hs_library_internal_compile.h:32 and add "static": true to its symbols.json row.
- Regenerate csplit.
- Because hs.c also includes that header, sweep the header for declaration-count effects.
- evidence: - cachebeta publics list _whitespace_characters [0x269F98], _eol_characters [0x269F9C] and _hs_type_primitive_parsers [0x269FA0] as PUBLIC. These addresses equal their symbols.json file_offsets 2531224/2531228/2531232.
- _hs_compile_finish is not public.
- No other split object references any of the four (storage_check.txt).
- Lab (scratch/w/critic_r2/hs_compile/): cand_extern_tables.c and cand_static_finish.c give 61/64 unchanged. Every section is section_infos_equal to the baseline compile; only the 4 storage rows change.
- why untried: The object_audit compares against the split, which inherits the wrong static flags, so it passes. The only pdb_storage disagreements with PDB-public True in the whole campaign are here and in interface, and no hs_compile ledger mentions them. This does not complete hs_compile: it still has 3 residuals, one of which (_hs_parse_boolean) is a held January bug.

## 21 incomplete units (ai_debug, transport_endpoint_winsock, transport_endpoint_set_winsock, cseries/profile, progress_bar, network_connection, saved_game_files, rasterizer_xbox_profile, xbox_sound_cache, king, players, hs, hs_runtime, main, decals, rasterizer_lights, dynavobgeom, environment_fog, hardware_bitmaps, render_cameras, periodic) (p=0.8)

- route: Apply first-party Sept-2001 function names in place of placeholder and invented names; this is byte-inert hygiene. Priority: the 20 house-rule-violating `_code_` placeholders in ai_debug. Examples:
- _code_00037890 -> _ai_debug_render_path_line (0x80)
- _code_00039990 -> _ai_debug_render_path_node (0x350)
- _code_0003a2e0 -> _ai_debug_render_vehicles_enterable (0x480)
Per-row, confirm that no January assert or string contradicts the Sept name (see the rasterizer_xbox caveat).
- evidence: - scratch/w/critic_r2/lab/map_align.py and map_align.txt give 72 unique-size alignments between raw-map names and January functions, using name anchors and equal padded size.
- 57 of these names are already in atlas 7eacac85; 15 exist only in the raw map (e.g. _profile_timesection_begin/_end/_begin_now/_end_now, _string_has_prefix, _script_error, _point3d_to_point2d, _submit_screen_vertex).
- Residual functions affected: _transport_endpoint_set_compare_entries = _poll_ep_array_compare_proc (0x30), _connect_endpoint_process@4 = _connect_async_thread_proc@4 (0x130), _draw_gravy_layer = _draw_layer_int (0x160).
- The atlas is lossy: 8,568 of 11,172 raw map function records.
- why untried: R2 treated the atlas map tier as the new source and applied it only to texture_cache, hardware_geometry, hardware_bitmaps and weapons. Nobody compared the raw maps with the atlas or swept names board-wide. This completes no object by itself, apart from bink (the separate route above).

## source/interface/ui_widget_game_data_input_functions (and census support for other tie rows) (p=0.1)

- route: Use the Sept-2001 cross-build census as a triage oracle before spending attack waves. Rows where the Sept build equals January (84 of 128) show that the source was stable from Sept, so there is no source-drift explanation. Rows where the Sept build equals OUR output (solo only) confirm a pure TU-context tie. Rows where Sept differs from January mark source changes made after Sept 2001.
- evidence: - scratch/w/critic_r2/lab/xbuild.txt: _solo_level_select_list_update_displayed_items is 203/203 instructions, with Sept equal to ours and January differing by a single `mov edi,[ebp+8]` placement. That supports R2-3's reading of it as a whole-TU count tie, and says its body is right.
- _sphere_intersects_cluster_portal: Sept has frame 0x408 and no point local; January has frame 0x414, a 12-byte point local and an fchs computation, so this code was added after Sept.
- _actor_combat_plan_grenade_trajectory: js 0.91.
- why untried: No R2 lane compared residual bodies against the Sept or Aug 2001 xbe bytes located through the raw maps. The xbe files were used only for names (hardware_geometry) and data (bitmap_drawing). This gives evidence for triage, not a closure.

## unverified claims

- Ledger owner-escalation row: the Codex cross_product2d packet 'completes ... collision_features' at 'none known' cost. This is false as stated. collision_features still has an active admission rejection (_projection_from_vector3d, candidate-only-comdat-owner), and gate_r2b2/admission.log reports it as rejected, although the surplus is now IDENTICAL to decals and provider_link passes.
- Ledger Wave R2-1: 'The key new evidence source was the Halo symbol atlas's 2001-09-25 linker-map tier ... It names static functions with January's exact spacing.' The atlas map tier is lossy: 8,568 of 11,172 raw cachebeta.map function records, with section-1 statics at 1,975 of 3,115. Of the 72 January-aligned first-party names in 21 incomplete units, 57 atlas names were never applied.
- Owner-escalation row 'bink_playback q3 else-if split | Completes bink_playback' leaves out the _code_001b5850 naming item. R2's own triage and review call it an owner naming item, and it is in fact resolvable from first-party evidence (Sept map name + lab-proven static-used emission).
- 'Halo objects 381 -> 384' / 'Objects complete' counts config status Matching. Of the 384, 77 fail object_audit and also disagree with PDB storage, and 71 more disagree with PDB storage only (74 others are PHONY audit artifacts). The R2 section-4 admission battery is therefore stricter than what about 148 already-counted objects satisfy (scratch/w/critic_r2/matching_audit.txt).
- Ledger/brief: storage fixes are complete once object_audit passes. hs_compile's audit passes only because the split inherits wrong static flags: three cachebeta-PUBLIC tables are static and one non-public function is external. Nobody recorded this.
- R2-1 hs triage: '3.6.0 recovers all 54,780 B'. I did not re-verify this; I only confirmed that object_audit finds no hs data-section mismatch.
- The R2-3 census says '126 residual functions'. At gate_r2b2 there are 128 non-exact rows in the 83 incomplete units; the difference is the 2 UNWRITTEN rows (_fast_ftol_C, _main_crash). I did not re-run the 5,504-compile census itself.