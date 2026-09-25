# Fifty-object round-2 reconciliation (2026-09-24)

- Canonical before this reconciliation: `6e3465bc`; Claude tip: `69f30e19`.
- Common ancestor: `4d1ebf17`. Its two completed Halo objects, three strict functions (4,128 meaningful code bytes), and 3,089 verified data bytes were already canonical. **Do not credit them again.**
- Integrated the remaining map-backed private-function renames and `hs_compile` storage correction from `b9930521`, together with the research and handoff. These changes have zero new code, data, function, or object credit.
- Resolved the two sound-cache conflicts by retaining canonical's four January-static declarations while adopting the map-backed `cache_block_get_sound_permutation_name` name. Park-key renames from the same batch remain measurement-stable.
- Documented an attested original bug in `actors_spawn_from_unit`: when the swarm actor index is selected, January still passes the ordinary actor index to `actor_get`. The source comment replaces a blank line, preserving line numbering and exact code.

The `hardware_geometry` admission and the `bink_decompress_audio_frame` rename remain held: the proposed added calls have no first-party evidence for position and argument. Map-backed names alone do not authenticate the calls. No new whole-object admission was granted here. Existing static-storage gaps in `progress_bar` and `saved_game_files` remain open; the R2-3 naming changes do not close them.

Verification on the integrated tree: full `ninja` passes; the rename-stable whole-board sweep has 0 gained and 0 regressed functions against `6e3465bc`; Halo code remains 1,577,237 / 1,770,166 meaningful exact bytes and 7,445 / 7,574 functions; Halo data remains 2,587,011 / 3,923,451; whole Halo objects remain 383 / 468. Parks: 83 active, 0 stale, 0 invalid. Admission audit: 0 contradicted, 0 revoked. Fake-match scanner: the same 26 inherited review leads. Pytest: 1,161 passed, 5 skipped, 26 subtests.

Future work should use the detailed evidence and reopen criteria in `claude_fifty_objects_r2_20260924.md` and `research/fifty_objects_r2_20260924/`. Do not count scratch candidates or scorer-only changes as exact credit.
