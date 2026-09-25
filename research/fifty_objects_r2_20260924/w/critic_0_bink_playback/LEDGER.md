# LEDGER critic_0_bink_playback (R2 completeness-critic route)

Tree: HEAD 5d3ca708 (task text said 7b9de585; HEAD advanced by one ledger-only commit). Claimed source/bink/bink_playback as critic_0_bink_playback.
Route: rename _code_001b5850 -> static bink_decompress_audio_frame (used, byte-inert empty stub), symbols.json row static.

## P0 premise verification (read-only, DATA only)
- Sept-2001 cachebeta.map (astra-inputs/earlier-map-archives/2001-09-25/cachebeta.map) line 19268 "Static symbols"; line 19359
  `0001:001b3090 _bink_decompress_audio_frame 005b3470 f bink_playback.obj`, between _bink_free@4 (001b2fd0) and _bink_draw_frame
  (001b30a0) -> 0x10 bytes. January symbols.json: _bink_free@4 0x1b5790, stub 0x1b5850 (0x10), _bink_draw_frame 0x1b5860. Same spacing.
- Sept retail cache.map line 13674 (Static symbols, header line 13583): `@bink_decompress_audio_frame@0 0054d870`.
- Atlas: 7eacac85 (Sept cachebeta xbe) line 5498 `_bink_decompress_audio_frame` off 0x1c4090 (map tier); 6455066470 (Sept retail exe)
  line 5992 `@bink_decompress_audio_frame@0` 0x54d870. January atlas 4cc87b45 has no record at 0x5b5850 (exact-tier size floor).
- Bytes (verify_bytes.py/.txt): Sept xbe 0x1c4090 = c3 90*15, 0 E8/E9, 0 abs refs; Sept retail 0x54d870 = c3 90*15, 0 E8/E9, 0 abs;
  January cachebeta.exe 0x5b5850 = c3 90*15, 0 E8/E9, 0 abs. Split section: 16 B, 0 relocs, sha 499f1f30.
  => never called out of line, never address-taken, yet emitted in all three builds => a USED file-static (static-COMDAT law).
- cachebeta.pdb publics: no symbol at RVA 001B5850 (nor 001B5E30) -> January storage static.
- /Od build (halo_cache_symbols.exe) is the PC binkw32 port (delay-import IAT 0xf5a200..; one monolithic caller 0x4d1a50..0x4d1efd):
  no Xbox bink_playback structure -> no /Od attestation of the call position. HCEX: bink stubs only (R2-2 review).
- Assert anchors (explicit literal line numbers; line-count edits are inert): render precedes the first anchor (jan 414) -> the
  call position cannot be localised by anchor drift either.

## P1 candidate + labs (mk_cand.py; all compiled with gate.py --out)
- cand.c = HEAD + rename code_001b5850 -> static bink_decompress_audio_frame (prototype + definition + header listing line) + one
  call before bink_decompress_video_frame() in bink_playback_render with a 2-line disclosure comment (hwgeom precedent wording).
- csplit emulation (mk_cfg.py): split_ctl == build/split 833/833; split_A (row 0x1b5850 -> _bink_decompress_audio_frame static)
  differs only in bink_playback.obj, only symbol 18: (_code_001b5850, ext 2) -> (_bink_decompress_audio_frame, static 3).
- gate vs split_A: cand 23/1 (only parked q3 row). production vs split_A 22/1/1 (stub UNWRITTEN) -> joint landing required.
- whole object cand.obj vs production orig.obj: 77/77 sections, NO non-debug section byte difference; only symbol 43 name+storage.
- Strip test (lab_strip_call): call removed -> stub NOT emitted (UNWRITTEN 16): the call is load-bearing through existence.
- Storage strip (lab_extern_call): 23/1 but object_audit symbol DIFF storage 3/2 -> `static` required (PDB + map).
- Position labs: call AFTER video (lab_call_after), second site in bink_playback_start (lab_two_sites): identical non-debug
  sections to cand (only .debug$S path length) -> position/site count byte-inert, disclosed in the comment.
- object_audit cand vs split_A FAIL(1) = q3 .text row only; 59 symbols 0 differ. Production vs build/split also FAIL(1) (q3).
  Mixed pairs FAIL(3) both ways (atomic landing).
- pdb_storage: production 1 (_code_001b5850 split 2 ours 2 PDB-public False) -> candidate 0.
- surplus_identity 0/0; provider_link 6 rows PASS both orders; --baseline=production: no new surplus.
- objdiff 3.3.1 (sha1 3130e428) mini report: prod/build-split 3733/3803 23/24 1701/1701 == cand/split_A; mixed 3732 22/24.
- /Zs /W3: 14 == 14 warnings (C4018 x2, C4146, C4244 x11), identical set. fake_match_scan: 1 pre-existing int 3 lead, both.
- .text emission order: cand == production order (stub keeps its slot); production order already differs from January's
  address order (pre-existing, not an object_audit criterion, unchanged by this packet).

## P2 optional hygiene: _bink_playback_idle (0x1b5e30, 0xa0) -> _bink_playback_update__internal
- Sept cachebeta.map static 0001:001b3670 _bink_playback_update__internal, next record _bink_playback_render 001b3710 -> 0xa0;
  January 0x1b5e30 0xa0, next _bink_playback_render 0x1b5ed0. Retail map @bink_playback_update__internal@0 0x14d780.
- verify_hygiene.py: Sept body vs January section: mnemonic sequences equal (52/52), 115/160 bytes equal (rest = addresses);
  Sept callers are tail jmps from bink_playback_render (0x1b3764) and bink_playback_update (0x1b3ba9) == January's two callers.
- cand_hyg vs split_H: 23/1 (q3 only), pdb_storage 0, non-debug sections identical to cand, only symbol 181 name.

## P3 lab (NOT LANDABLE; owner-gated q3 device) cand + round-1 L3 q3 else-if arms
- gate vs split_A 24/0; object_audit PASS; pdb_storage 0 => after this packet the q3 ruling is bink_playback's ONLY blocker.

## P4 packet + close
- patches/01 (source) + 02 (symbols.json row 7388) + optional 03 (hygiene, both files); git apply --check clean (worktree and
  --cached) singly, 01+02, 01+02+03 at 5d3ca708; index hashes == hash-object of cand.c / symbols_A_lf.json / cand_hyg.c / symbols_H_lf.json.
- MANIFEST.md written. VERDICT: ADMISSION_FIX_PROPOSED (zero credit; removes the naming + PDB-storage blocker; q3 ruling remains the only blocker).
