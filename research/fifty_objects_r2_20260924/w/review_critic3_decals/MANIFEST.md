# MANIFEST - review_critic3_decals (adversarial review of scratch/w/critic_3/patches/16_decals.patch)

- Base/tip 5d3ca708; git status clean apart from pre-existing untracked research/ (start and end). No tracked edits.
- Unit source/effects/decals: claimed/released as review_critic3_decals.
- Change reviewed: `_decal_verify_neighbors` -> `_decal_check` (static) in config/symbols.json line 1840 (in-place) and
  decals.c (symbol-listing comment, prototype, 2 calls in decals_reconnect_to_structure_bsp, definition).
- Name evidence: Sept-2001 + Aug-2001 cachebeta.map static `_decal_check` (first decals.obj function, 0x130); 32/33 aligned
  name+size anchors; January atlas 4cc87b45 exact `_decal_check` @0x486fe0; bytes identical to Sept xbe outside relocations
  (Aug: only assert line immediates); sole caller decals_reconnect_to_structure_bsp at identical call offsets; January strings
  do not contradict ("!layer_check || layer==decal->layer"); old name had no first-party source (4277f892).
- Target vs candidate: emulated csplit (control == build/split 833/833) differs only in decals.obj, identical mod rename.
  gate 31 exact / 2 residual both sides (rows identical mod rename; `EXACT 304 _decal_check`); prod vs cand objects identical
  except `.file`. object_audit FAIL(4) same (pre-existing), pdb_storage 0/0, surplus 21/0 same, provider_link FAIL(1)
  `_cross_product2d`/actor_combat same (pre-existing reserved class), data identical, objdiff 3.3.1 identical.
- Credit: ZERO (name only). Verdict: APPROVE unamended. Evidence files: LEDGER.md, bodyid.py, order_align.txt, objcmp.py,
  gate_*.txt, audit_*.txt, pdb_*.txt, surplus_*.txt, plink_*.txt, data_*.txt, objdiff/.
