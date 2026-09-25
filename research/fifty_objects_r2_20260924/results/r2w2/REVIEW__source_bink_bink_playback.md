# REVIEW source/bink/bink_playback

approve: True

amended_patch: None

## per_object
null

## checks
All checks were run independently in C:/halo-worktrees/claude-fifty-objects-r2-20260924/scratch/w/review_r2w2_bink_playback/ (REVIEW.md, LEDGER.md) at HEAD 434f0151. That is 05255584 plus a docs-only commit; the tree is clean.

- Patch integrity: git apply --check at HEAD returns rc 0 for PACKET, 01 and 02 separately. Applying and then reversing on the LF HEAD blobs gives byte-identical files. PACKET is byte-equal to 01+02, and the sha256 values match patches_sha256.txt. The edited symbols.json rows are 23308, 23309, 23310 and 23313, changed in place.
- Split emulation: a control csplit from an unmodified config copy matches build/split on 833/833 objects. With 02 applied, only bink_playback.obj differs, in exactly 4 bytes: the storage class of symbols 162, 163, 164 and 167 goes from 2 to 3.
- PDB storage: cachebeta publics list _debug_bink [004D2B74] and _bink_globals [004D2B78]. Nothing public sits at 0x4D2AF0, 0x4D2B30, 0x4D2B60 or 0x4D2C50. No object in build/split or build/base has an UNDEF reference to any of the four; the only UNDEF among the six is _debug_bink in hs_globals_external. No other source file or header names the four. HCEX.pdb has no entries (bink stubs only).
- Gate: 23/1 for production and 23/1 for the candidate, with the same row (_bink_query_analog_controller_buttons, parked q3). Compared cand.obj with prod.obj on all 77 non-debug sections using section_infos_equal: 0 section diffs, and only the 4 storage bytes differ in the symbol table.
- object_audit: prod against split is FAIL(1), and cand against the emulated split is FAIL(1). Both have 59 symbols with 0 differing, and .bss (356/356) and .data are ok. The mixed pairs are FAIL(5) each, so the two halves must land together.
- pdb_storage goes from 5 to 1; the remaining one is _code_001b5850. surplus_identity finds 0 code COMDATs. provider_link passes in both orders for 3 SDK tables and 3 __real literals.
- objdiff 3.3.1 (sha1 3130e428) mini report: prod, cand and both mixed pairs are identical at 3733/3803 code, 23/24 functions and 1701/1701 data.
- /Zs /W3 gives 14 warnings for both. fake_match_scan reports the same pre-existing int 3 lead for both.
- No board sweep was needed: no header changed, no other translation unit includes bink_playback.c, configure.py does not read symbols.json, and the emulated split changes only this object.
- Strip tests: removing `static` leaves the bytes the same but brings back 4 storage DIFFs against the PDB-derived split. A lab strip of the `= { 0 }` initialisers (already in the file, not added by this packet) moves the .bss layout from 356 to 360 bytes and shifts 6 symbol offsets. That follows the VC7 initialised-statics law, and canonical already accepts it.
- Precedent: I read the original cdc8ebd3 diff (Batch 5b, collision_bsp_usage_times made static with its symbols.json row set static:true, zero credit). Every condition holds here: absent from publics, 0 UNDEF references, byte- and credit-neutral, landed atomically with a csplit-only regen, and no extern in any header.

## issues
None blocking. The packet is a correct zero-credit storage-class fix.

- It needs an atomic landing: source and symbols.json together, then a csplit-only regeneration. Landing either half alone gives 4 storage DIFFs.
- It does not complete the object. Two owner items remain unchanged:
  - _bink_query_analog_controller_buttons still waits on the q3 owner ruling (parked).
  - _code_001b5850 keeps its address-derived name, which is a house-rule naming item. It is non-public and unreferenced, which fits a January static that was used but inlined everywhere. Making it static in our source drops the section, because our source never calls it.
- The original scope asked me to gate every January function against the emulated split. gate.py has build/split hardcoded, so I compared the candidate with the emulated split through a copy of object_audit (every section and symbol). I ran gate.py only against build/split. That is equivalent here, because the split change is 4 symbol-table bytes and touches no section.
- The worker brief named base 05255584; HEAD is now 434f0151, which adds one docs-only commit, and the packet applies cleanly there.
- No amended patch is needed.
