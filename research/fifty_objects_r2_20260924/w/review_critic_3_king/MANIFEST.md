# MANIFEST - review_critic_3_king (review of scratch/w/critic_3/patches/11_game_engine_king.patch)

- Base/tip: 5d3ca708 (task named 7b9de585; ancestor, docs-only delta). No tracked edit; git status clean except pre-existing research/ dir.
- Change under review: static `hill_points_to_2d` -> `point3d_to_point2d` (.c listing comment, prototype, definition, 1 call;
  config/symbols.json line 2303, "static": true kept). Verdict: APPROVE unchanged.
- Source evidence: Aug-2001 + Sept-2001 cachebeta.map static `_point3d_to_point2d` (0x20, same slot), Sept retail cache.map
  `@point3d_to_point2d@12`; body RAW-BYTE-IDENTICAL (32/32, no relocs) in Aug xbe, Sept xbe, Sept retail exe and January split.
  Old name invented (Fable King wave); no HCEX/atlas/PDB/string name exists or contradicts.
- Per function: gate 29/29 EXACT vs emulated split, rows identical to production (helper 32 B EXACT under both names).
- Data/symbols: objcmp name-mapped IDENTICAL (split and compiled); object_audit PASS; pdb_storage 23 pre-existing (held storage packet), unchanged.
- Surplus/provider: surplus_identity unchanged (10 COMDATs, 0 not identical); provider_link PASS both orders, unchanged.
- objdiff 3.3.1: 4865/4865 code, 29/29 fns, 740/876 data both sides.
- Credit: ZERO (names only). Integration: csplit regen required; held owner packet 02_king_symbols needs a one-line context
  rebase (held_king_02_symbols_rebased_after_p11.patch here).
- Not run: ninja, pytest, board sweep (integrator).
