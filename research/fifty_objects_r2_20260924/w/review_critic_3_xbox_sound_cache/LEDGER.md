# review_critic_3_xbox_sound_cache LEDGER (adversarial review of critic_3 patch 10)

Base: worktree HEAD 5d3ca708 (task text named 7b9de585), git status clean except pre-existing untracked research/.
Slug normalised to review_critic_3_xbox_sound_cache (orchestrator slug string garbled with a unit list).
Claim: review_critic_3_xbox_sound_cache on source/cache/xbox_sound_cache (claimed rc=0).

## R1 patch mechanics
- git apply --check scratch/w/critic_3/patches/10_xbox_sound_cache.patch at HEAD: OK (symbols.json + .c).
- No other critic_3 patch has a symbols.json hunk within 7150..7300 (no context overlap).
- patchroot/: HEAD blobs + GNU patch -p1: exactly 3 .c lines (listing comment, definition, lruv_debug_to_file arg) + symbols.json L7224.
- git grep old/new name outside symbols.json: only historical docs/research ledgers; no header prototype, no tools/tests/config use.

## R2 name provenance (independent)
- Sept-2001 cachebeta.map Static symbols: 0x1ab5a0 _sound_cache_delete_block_proc, 0x1ab660 _cache_block_get_sound_permutation_name (0x40), 0x1ab6a0 _sound_cache_start_loading_sound.
- Aug-2001 cachebeta.map Static symbols: 0x193350 _cache_block_get_sound_permutation_name (0x40) between delete_block_proc and start_loading_sound.
- Sept-2001 retail cache.map statics: @cache_block_get_sound_permutation_name@4 between @sound_cache_delete_block_proc@4 and @sound_cache_start_loading_sound@4.
- Halo CE atlas, January build 4cc87b45: off 0x5add60 = file_offset 1760608 -> _cache_block_get_sound_permutation_name tier exact.
- cachebeta.pdb publics: neither name public (January static, consistent with all three maps marking it static). HCEX.pdb: neither name.
- Only this row differs between Sept and January name sets for this object (17 anchors, all identical names).

## R3 emulated split
- csplit -p cfg_ctl (config copy) -> split_ctl == build/split 833/833 byte-identical.
- csplit -p cfg_P (only L7224 renamed, CRLF preserved) -> split_P differs from split_ctl ONLY in source/cache/xbox_sound_cache.obj;
  objcmp (all sections incl .debug$S, relocs by symbol name, full symbol table) IDENTICAL modulo the rename
  (the only raw differences: one DIR32 reloc name at .text sec12+0xfa and the symbol record name).

## R4 gate
- prod vs build/split: 17 EXACT / 1 residual (_sound_cache_debug_render [sha]).
- cand (patchroot .c) vs split_P: 17/1, identical rows; renamed row EXACT 64.
- cand vs UNREGENERATED build/split: 15/2/1 (renamed fn unwritten, start_loading_sound reloc-identity) => symbols.json edit and csplit regen are mandatory with the .c edit (same patch; ninja regenerates split from symbols.json).
- objcmp prod.obj vs cand.obj (non-debug sections, relocs, symbols, rename-mapped): IDENTICAL (.debug$S differs only by compile path).

## R5 section-4 battery (with_split.py redirect; tools unmodified)
- object_audit FAIL(3) both sides, same rows (debug_render DIFF; render_inverse_transform_screen_point + start_loading_sound storage 2/3).
- pdb_storage 6 disagreements both sides, same rows modulo the rename (renamed fn: split 2 ours 2 PDB-public False = pre-existing).
- surplus_identity 2 COMDATs, 0 not identical; provider_link PASS both sides.
- objdiff 3.3.1 (sha1 3130e428) mini project: prod, prod(gate obj) and cand all 2208/2576 code, 17/18 fns, 1224/1224 data, 98.03722 fuzzy.

## R6 byte-level name attestation (DATA only; no binary executed)
- septcmp.txt: January split _sound_cache_debug_block_name (64 B) vs Sept cachebeta.xbe VA 0x1bc660 (.text VA 0x11000 + map
  offset 0x1ab660): 23 differing bytes, ALL inside the 7 January relocation fields (0 outside) -> identical body.
- Aug-2001 1749betaP.xbe VA 0x1a4350: 0 differences outside relocation fields.
- Sept _sound_cache_start_loading_sound (VA 0x1bc6a0) pushes imm32 0x1bc660 (= &_cache_block_get_sound_permutation_name) at
  body offset 249; January's DIR32 relocation to the same function is at +250 (the imm32 of that push). Same caller, same site.
- cachebeta.exe strings: no 'block_name', 'permutation_name', 'sound_cache_debug' literal; no January literal contradicts.
- symscan.txt: no other object in build/split, build/base or split_P defines/references either name (no collision, no consumer).

## R7 house rules / strip test / held classes
- Pure identifier rename: no new construct, no cast, no scope, no declaration -> strip test and /Od declaration-order checks N/A.
- Rule 7: invented descriptive name -> authentic first-party name (precedent: 7979cf8f admitted xbox_texture_cache's sibling
  callback under its Sept name _texture_cache_name_block_proc, replacing invented _texture_cache_debug_block_name).
- Not a held class (not a January-bug candidate, no COMDAT, no header, not decals/physics, not RESERVED).
- Pre-existing, NOT introduced: the function stays external in source and split while January had it static (PDB not public;
  all three 2001 maps list it under Static symbols). pdb_storage row count unchanged (6 -> 6).

## R8 integration hazard: textual conflict with pending round-1 storage packet A
- research/fifty_objects_20260925/w/xbox_sound_cache/storage.patch (A; R2 triage: "still-pending zero-credit hygiene") and
  patch 10 edit the same definition line (471) and symbols.json L7224. Dry runs: A after 10 -> hunk 5 FAILS; 10 after A ->
  .c hunk 2 + symbols.json hunk FAIL. A's symbols.json side was a full-file copy whose line numbers are stale at HEAD
  (_xbox_sound_cache_globals is now L23298, not 23289).
- Companion (optional, NOT part of the verdict): storageA_rebased_after_10.patch (index 30961b42/642529f7 = patch 10 post-images).
  Measured A+10: split_PA differs from split_ctl only in xbox_sound_cache.obj; vs split_P only 6 storage 2->3 symbols;
  cand_PA vs cand: only 4 storage flips (globals, locked/delete procs, renamed fn); gate 17/1 rows SAME; object_audit
  FAIL(3) -> FAIL(1) (only the parked _sound_cache_debug_render residual); pdb_storage 6 -> 0; surplus 0 not identical;
  provider_link PASS; objdiff 3.3.1 identical (2208/2576, 17/18, 1224/1224). No other source names any of the 6 symbols.

## R9 verdict
- APPROVE patch 10 unchanged: byte-inert, zero credit, authentic name triple-attested (Aug/Sept beta/Sept retail maps) with
  relocation-masked body identity and the same caller push site; symbols.json + .c must land together and csplit must be
  regenerated (unregenerated split drops the gate to 15/2/1).
- Integrator note: land A only as the rebased companion (or re-derive it) after 10; never apply the round-1 A files as-is.
