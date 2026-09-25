# Review ledger: critic_0_bink_playback naming packet (review_critic_0_bink_playback)
Claimed source/bink/bink_playback as review_critic_0_bink_playback. HEAD 5d3ca708, tree clean (untracked research/ not mine).

## Independent reproduction (nothing reused from the worker except its patches, with_split.py and w3.py, both read first)
- git apply --check (worktree + --cached) rc 0 for 01, 02, 03 each and 01+02+03 together; --whitespace=error-all clean; sha256 file OK.
  GNU patch on LF HEAD blobs -> post-image blobs 94b281d9 / dc1dc066 (01+02) and cc27b290 / 65510792 (+03) = the declared index lines; == worker cand.c / cand_hyg.c.
- Premise (DATA only, own parser verify_bytes_indep.py): Sept cachebeta.map 19359 `0001:001b3090 _bink_decompress_audio_frame` (Static symbols, hdr 19268),
  between _bink_free@4 001b2fd0 and _bink_draw_frame 001b30a0 -> 0x10. Sept retail cache.map 13674 `@bink_decompress_audio_frame@0 0054d870`
  (fastcall @0 attests a void parameter list). Bytes c3+15x90 at Sept xbe 0x1c4090, Sept retail 0x54d870, January 0x5b5850; 0 E8/E9 rel32 and
  0 absolute refs in all three. cachebeta publics: none at RVA 001B5850 / 001B5E30 (the 9 bink publics all present).
- Retail map extra: order bink_free, audio_frame, video_frame, draw_frame (audio+video adjacent in definition order) - consistent with the pairing,
  does not attest a call site.
- /Od: 0x4d1a00..0x4d1a40 empty stubs are called only from OTHER modules (PC port public API stubs); no internal bink callers -> no attestation. Agrees with worker.
- csplit emulation (own cfg copies, CRLF-faithful, == worker cfg_A/cfg_H): ctl == build/split 833/833; A differs only bink_playback.obj,
  only symbol 18 (_code_001b5850 st2 -> _bink_decompress_audio_frame st3), 0 section byte diffs; H adds only symbol 42 rename.
- gate: orig vs ctl 23/1/0; t12 vs A 23/1/0 (q3 parked row only); t123 vs H 23/1/0. Mixed: orig vs A 22/1/1, t12 vs ctl 22/1/1 -> atomic.
- whole object (section bytes + relocation RECORDS + symbol table): t12 vs orig -> only .debug$S and symbol 43 name+storage; t123 vs t12 -> only symbol 181 name.
- object_audit: t12/A FAIL(1)=q3, 59 symbols 0 differ; orig/split FAIL(1) same (identical output modulo stub name); t123/H FAIL(1); mixed FAIL(3) both ways.
- pdb_storage: prod 1 (_code_001b5850) -> t12/A 0, t123/H 0. surplus_identity 0. provider_link PASS 6 rows both orders; --baseline=orig: no new surplus.
- 0 build/base or build/split objects reference _code_001b5850/_bink_playback_idle except bink itself (no UNDEF anywhere).
- objdiff 3.3.1 (sha1 3130e428): prod/split, t12/A, t123/H all 3733/3803 23/24 1701/1701; mixed 3732 22/24.
- /Zs /W3 14==14==14. fake_match_scan: 1 pre-existing int 3 lead in all three.
- Strip/position labs vs split_A: strip call -> UNWRITTEN 16 (call load-bearing by EXISTENCE, which January's emitted static attests);
  strip comment, call after video, call inside bink_decompress_video_frame, call inside idle, call only in start -> all non-debug sections and
  symbols identical to t12; nostatic -> storage 3->2 only. Position genuinely byte-inert, comment accurate.
- q3 lab (NOT landable) vs split_A: 24/0, OBJECT AUDIT PASS - confirms q3 is the only remaining blocker.
- Hygiene 03 (own capstone check): Sept 0x1c4670 vs January 0x5b5e30, 52/52 insns, mnemonics equal; callers are tail jmps from render+update in both builds.
- Precedent: hwgeom D3DVertexBuffer_MoveResourceMemory (05255584) landed with a disclosed byte-inert call position; brief s8 allows first-party
  name evidence to remove the ruling; brief s9 law (used static inlined at every site is still emitted); Jonas 20260830 prohibition on adding a
  call was conditioned on new identity evidence, which the 2001-09-25 maps now provide.
VERDICT: APPROVE 01+02 (atomic, csplit-only regen), 03 optional also approvable. Zero credit; object stays NonMatching (q3 owner ruling).
