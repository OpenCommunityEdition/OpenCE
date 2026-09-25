# REVIEW source/bink/bink_playback

approve: True

amended_patch: None

## checks
All checks were done independently in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_0_bink_playback\ at HEAD 5d3ca708. The tree is clean apart from the untracked research/ directory, which is not mine. The claim was taken and then released.

Patches:
- git apply --check passes (rc 0) on the worktree and --cached for 01, 02 and 03 each, and for all three together. --whitespace=error-all is clean and the sha256 file verifies.
- Applying the patches to LF HEAD blobs gives exactly the declared post-image blobs: 94b281d9 and dc1dc066 for 01+02, and cc27b290 and 65510792 with 03 added. These are identical to cand.c and cand_hyg.c.

Premise (binaries read as data only, with my own XBE/PE parser):
- Sept cachebeta.map line 19359 lists `_bink_decompress_audio_frame` under Static symbols, between _bink_free@4 and _bink_draw_frame, so it is 0x10 bytes.
- Sept retail cache.map line 13674 has `@bink_decompress_audio_frame@0`. The @0 decoration confirms a (void) parameter list.
- The Sept xbe (0x1c4090), the Sept retail exe (0x54d870) and January (0x5b5850) all hold c3 followed by 15 bytes of 90. None has an E8/E9 rel32 reference or an absolute reference.
- cachebeta publics have no symbol at RVA 001B5850 or 001B5E30. All 9 bink publics are present.
- The /Od PC-port stubs at 0x4d1a00-0x4d1a40 are called only from other modules, so there is no /Od attestation of the call site.

Split emulation:
- I made my own config copies with CRLF line endings preserved; they equal the worker's cfg_A and cfg_H.
- The control split equals build/split on 833/833 objects.
- split_A differs only in bink_playback.obj, symbol 18: (_code_001b5850, storage 2) becomes (_bink_decompress_audio_frame, storage 3). No section bytes differ.
- split_H adds only the symbol-42 rename.

Gate and whole-object checks:
- Gate results:
  - production vs split: 23/1/0
  - candidate vs split_A: 23/1/0
  - candidate + 03 vs split_H: 23/1/0
  - mixed states: 22/1/1
- In every case the residual is the parked q3 row.
- Whole-object comparison (section bytes, relocation records and the symbol table): candidate vs production differs only in .debug$S and in symbol 43's name and storage. Patch 03 changes only symbol 181's name.
- object_audit:
  - candidate vs split_A: FAIL(1), the q3 row; 59 symbols, 0 differ
  - production: the same FAIL(1)
  - mixed states: FAIL(3) in both directions
- pdb_storage: 1 disagreement in production, 0 in the candidate, 0 with 03.
- surplus_identity: 0.
- provider_link: all 6 rows pass in both orders; with --baseline there is no new surplus.
- Scanning every build/base and build/split object finds no reference to _code_001b5850 or _bink_playback_idle outside bink.
- objdiff 3.3.1 (sha1 3130e428) is unchanged at 3733/3803, 23/24 and 1701/1701; mixed pairings score 3732 and 22/24.
- /Zs /W3 gives 14 warnings in production, candidate and candidate + 03.
- fake_match_scan finds only the pre-existing `__asm { int 3 }` lead in all three.

Strip and position labs:
- Removing the call leaves the stub UNWRITTEN.
- Removing `static` changes only the storage class (3 to 2).
- The comment has no effect on the bytes.
- I placed the call five ways: after the video call; inside bink_decompress_video_frame; inside the idle function; only in bink_playback_start; and in its proposed position. All give identical non-debug sections and an identical symbol table. So the position really is byte-inert, and the disclosure comment is accurate.

Hygiene (patch 03):
- My own capstone check of Sept 0x1c4670 against January 0x5b5e30 finds 52/52 instructions with the same mnemonics.
- In both builds the callers are tail jmps from render and update.

q3 lab (not landable):
- It gives 24/0 against split_A and OBJECT AUDIT PASS.
- So q3 is the only blocker left.

House rules:
- void on its own line, explicit `return;`, and the static prototype stays in the file's own block.
- The definition stays before bink_draw_frame, as in the Sept map order.
- No new declarations, scopes, casts, pragmas or asm, so /Od declaration order does not apply.
- No held class is touched.

Precedent:
- The hwgeom D3DVertexBuffer_MoveResourceMemory landing (05255584) used a disclosed, byte-inert call position.
- Brief section 8 says first-party name evidence can remove the need for an owner ruling.
- Brief section 9: a used static that is inlined at every call site is still emitted.
- The Jonas 20260830 ledger said "do not add a call" only while there was no new identity evidence; the 2001-09-25 maps are that evidence.

## issues
No blocking issues. APPROVE patches 01 and 02 together, followed by a csplit-only regeneration. Patch 03 is also approvable as optional hygiene. The object gets zero credit and stays NonMatching; the parked q3 owner ruling is its only remaining blocker.

Non-blocking notes:
1. The call site of bink_decompress_audio_frame is not attested by first-party evidence. It is disclosed in a source comment, and I measured it as byte-inert at 5 placements. This is the same class as the landed hwgeom precedent.
2. The "symbols in this file" header comment still lists `_code_*` for most functions that already have real names. Only the touched rows are updated. This is pre-existing, cosmetic and not introduced by the packet.
3. Patch 03 says it "needs 01+02 first", but it also applies cleanly on its own. Either way it is internally atomic, because it carries both its source and symbols.json halves.
4. The Sept retail map places bink_decompress_video_frame right after bink_decompress_audio_frame. That hints the two were defined next to each other in the original source; the current file defines video_frame later. This is informational and outside this packet: it only affects emission order, which object_audit does not check.
5. The integrator still has to run a full ninja build, the stable sweep, parks/admission checks and pytest after applying.
