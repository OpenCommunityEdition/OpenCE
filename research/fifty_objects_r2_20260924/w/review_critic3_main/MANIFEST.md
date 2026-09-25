# MANIFEST - review_critic3_main (source/main/main, critic_3 patches/15_main.patch)

- Base/tip: 5d3ca708 (clean apart from the pre-existing untracked research/). Reviewed patch applied unamended.
- Change: `sort_controllers_ascending` -> `sort_desired_local_player_controllers` (static qsort comparator, 0xef8a0, 0x40):
  main.c prototype, qsort argument, definition, and symbol-listing comment row, plus config/symbols.json line 4107 (in place,
  "static": true kept).
- Source evidence: the Aug-2001, Sept-2001 beta and Sept-2001 retail linker maps all give the new name (static, same slot,
  same size). The Sept xbe body is byte-identical to January's. The sole reference is create_local_players+0x157 in both
  builds. The current name is a C++-mangled cross-game atlas propagation (the January builds have no map; majority conflict
  rule). HCEX is neutral, and there is no January string for either name. See LEDGER R1.
- Target vs candidate: `_sort_desired_local_player_controllers` EXACT 64 B, 0 relocations. Object gate 92 exact /
  2 residual / 1 unwritten, the same rows as production.
- Data: identical (tinfo --data; objcmp). Symbols: storage 3 both, owner and offset unchanged (object_audit row ok).
  pdb_storage unchanged (1 pre-existing). Surplus: 2/0 not identical, provider_link PASS in both orders, unchanged.
- Credit: ZERO. objdiff 3.3.1 is unchanged (11128/13117 code, 92/95, 2659/4507 data).
- House rules: identifiers and comment only; nothing added. Tests run: git apply --check, csplit control and candidate,
  gate, objcmp, tinfo --data, object_audit, pdb_storage, surplus_identity, provider_link, and objdiff 3.3.1.
