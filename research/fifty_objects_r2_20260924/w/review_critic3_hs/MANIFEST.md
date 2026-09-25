# MANIFEST review_critic3_hs: source/hs/hs, critic_3 patch 13 (radiosity_hack_* names)

- Base/tip 5d3ca708 (clean). Patch: scratch/w/critic_3/patches/13_hs.patch sha1 e2837739, approved AS SUBMITTED.
- Source evidence per changed construct: 3 identifier renames. Aug/Sept-2001 cachebeta maps + Sept retail cache.map name
  them radiosity_hack_{start,save,find_point}_evaluate; Aug/Sept XBE .rdata hs_function_definition structs for
  "radiosity_start"/"radiosity_save"/"radiosity_debug_point" point at exactly those functions; January's structs point at
  0xb14a0/c0/e0 (one reference each, no callers). 444/447 Sept hs.obj names already used by the tree; these 3 were the only
  gaps. No January string, PDB public, atlas or HCEX row contradicts.
- Per function: all 448 January functions gated; candidate 447/1 vs emulated split == production 447/1 (same rows). The three
  renamed functions EXACT 32/32 (relocs 1, normalized sha same as production).
- Data: all sections IDENTICAL (objcmp, rename-mapped); object_audit January-owned sections ok except pre-existing
  _real_random_range_evaluate.
- Symbols: 1830 January symbols 0 differ (audit); pdb_storage 0 disagreements; renamed symbols static (3) both sides.
- Surplus: 0 candidate-only code COMDATs; provider_link PASS (unchanged).
- House rules: names only; authentic first-party names; prototype order kept alphabetical; listing comment updated.
- Tests run: git apply --check; GNU patch combined with critic_2 packet both orders; csplit ctl/P; gate --all (prod, cand,
  negative control); objcmp (split, obj, +critic_2 header shadow with #error probe); object_audit; pdb_storage;
  surplus_identity; provider_link; objdiff 3.3.1 mini report. Not run: full ninja, pytest, board-wide stable_verdicts.
- Credit: ZERO (byte-inert). Held items: none.
