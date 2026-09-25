# source/bink/bink_playback

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
Tree: HEAD 5d3ca708. The task named 7b9de585; the commits since are ledger-only and the unit's blobs are unchanged. The patches are in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_0_bink_playback\patches\. (1) 01_bink_playback.c.patch (aa25ad5a..94b281d9): changes `void code_001b5850(void)` to `static void bink_decompress_audio_frame(void)` in both the prototype and the definition; the body stays `return;` and the definition keeps its place just before bink_draw_frame. Adds one `bink_decompress_audio_frame();` right before `bink_decompress_video_frame();` in bink_playback_render, with a 2-line comment saying the position is unattested, as in the hwgeom precedent. The header listing line now uses the real name. (2) 02_symbols.json.patch (885b3b3d..dc1dc066): row 7388 (0x1b5850) is edited in place to `_bink_decompress_audio_frame` with `"static": true`. 01 and 02 must land together, followed by a csplit-only regen. (3) Optional 03_optional_hygiene_bink_playback_update__internal.patch (needs 01+02 first): renames bink_playback_idle to the first-party bink_playback_update__internal in source and in symbols.json row 7400. git apply --check is clean at 5d3ca708, against the worktree and --cached, for each patch alone, 01+02, and 01+02+03. config.json status stays NonMatching.

## evidence
Premise verified first, reading the binaries as data only.
- Name: the Sept-2001 cachebeta.map lists `0001:001b3090 _bink_decompress_audio_frame` under Static symbols (line 19359), between _bink_free@4 (001b2fd0) and _bink_draw_frame (001b30a0), so it is 0x10 bytes. January has the same spacing: 0x1b5790 / 0x1b5850 (0x10) / 0x1b5860.
- The retail cache.map lists the static @bink_decompress_audio_frame@0 at 0x54d870. The atlas has the name twice: 7eacac85 line 5498 at 0x1c4090, and 6455066470 line 5992.
- Body and use: the Sept xbe, the Sept retail exe and January cachebeta.exe (0x5b5850) all hold c3 plus 15 bytes of 90, with 0 E8/E9 calls and 0 absolute references. So the function is used and inlined, never called out of line or address-taken, and still emitted.
- Storage: no cachebeta.pdb public at RVA 001B5850, so it is static.
- Split emulation: the control split equals build/split on 833/833 objects. split_A differs only in bink_playback.obj, symbol 18, which goes from (_code_001b5850, 2) to (_bink_decompress_audio_frame, 3).
- Candidate vs split_A: gate 23 exact, 1 residual, 0 unwritten; the residual is the parked q3 row. cand.obj matches production on 77/77 sections with zero non-debug byte differences; only symbol 43's name and storage change.
- object_audit gives FAIL(1), the q3 .text row only; 59 symbols, 0 differ. pdb_storage goes from 1 to 0. surplus_identity finds 0. provider_link passes 6 rows in both orders, with no new surplus.
- objdiff 3.3.1: 3733/3803, 23/24, 1701/1701, the same as production. Mixed pairings score 22/24 and audit FAIL(3), which is why the landing must be atomic.
- /W3 gives the same 14 warnings in both. fake_match_scan shows only the pre-existing int 3 lead.
- Strip tests: removing the call means the stub is not emitted (UNWRITTEN). Removing `static` gives a storage DIFF of 3/2.
- Position labs: the call placed after the video decompress, or a second call in bink_playback_start, both give identical non-debug bytes, so the position is byte-inert. There is no /Od attestation because the /Od build is the PC binkw32 port.
- Hygiene 03: the Sept static _bink_playback_update__internal is 0xa0 at 001b3670, the same size as January's 0x1b5e30. Its mnemonics match 52/52, and its Sept callers are tail jumps from render and update, the same two callers as in January. Candidate vs split_H: 23/1, pdb 0.
- Lab, not landable: packet plus the round-1 L3 q3 else-if arms gives 24/0, OBJECT AUDIT PASS and pdb 0.
Artifacts: LEDGER.md, MANIFEST.md, cand.c, cand.obj, gate_*_vs_split*.txt, audit_*.txt, pdb_storage_*.txt, surplus_*.txt, provider_link_*.txt, objdiff_proj/report.json, verify_bytes.txt, verify_hygiene.txt, symdiff_*.txt, text_order.txt, fn_table.txt.

## blockers
1. The owner q3 ruling on _bink_query_analog_controller_buttons (parked; the two else-if arms are textually identical). This is now the only thing blocking bink_playback. 2. The call position of bink_decompress_audio_frame is not attested. It is byte-inert and disclosed in the source comment. The reviewer or owner should accept it on the hardware_geometry MoveResourceMemory precedent. 3. .text emission order already differs from January's address order in production. This packet does not change it, and object_audit does not check it.

## reopen_criteria
Comment-only change if a first-party January-dated source ever shows where bink_decompress_audio_frame is called. Object admission follows as soon as the owner approves the q3 device: the lab composition already gives 24/24 and OBJECT AUDIT PASS on top of this packet.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_0_bink_playback\MANIFEST.md