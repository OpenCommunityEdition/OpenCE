# MANIFEST - review of critic_3 patch 10 (source/cache/xbox_sound_cache)

- Base/tip: worktree HEAD 5d3ca708 (task text named 7b9de585). No tracked edits; no ninja/configure/git mutation.
  Label/slug: review_critic_3_xbox_sound_cache. Claimed/released source/cache/xbox_sound_cache.
- Reviewed: scratch/w/critic_3/patches/10_xbox_sound_cache.patch
  (`_sound_cache_debug_block_name` -> `_cache_block_get_sound_permutation_name`: symbols.json L7224, .c listing comment,
  definition, lruv_debug_to_file argument).

## Source evidence (the one changed construct: the function name)
- Sept-2001 cachebeta.map, Aug-2001 cachebeta.map, Sept-2001 retail cache.map: all list it under Static symbols, 0x40, between
  sound_cache_delete_block_proc and sound_cache_start_loading_sound; the other 17 names in the object are identical to ours.
- Sept and Aug XBE bytes equal January's modulo relocation fields (septcmp.txt); the Sept caller pushes its address at the same
  body offset (+249) where January's DIR32 relocation sits (+250).
- Halo CE atlas at the January address (4cc87b45, 0x5add60): same name, tier exact. cachebeta.pdb publics: absent (January static).
  HCEX.pdb: absent. No January string contradicts. Previous name was invented (docs closeout 20260901: descriptive).

## Target vs candidate (emulated split, csplit on a config copy)
| function | padded size | relocs | gate prod (build/split) | gate cand (split_P) |
|---|---|---|---|---|
| old/new name | 64 | 7 | EXACT 64 | EXACT 64 |
| all 18 rows | - | - | 17 EXACT / 1 residual | 17 EXACT / 1 residual (same rows) |
- split_ctl == build/split (833/833); split_P differs only in xbox_sound_cache.obj, IDENTICAL modulo rename (objcmp incl. .debug$S).
- prod.obj vs cand.obj: IDENTICAL modulo rename for all non-debug sections, relocations and the full symbol table.
- Data sections: unchanged (objcmp); objdiff 3.3.1 sha1 3130e428: 2208/2576 code, 17/18 fns, 1224/1224 data, 98.03722 both.
- object_audit FAIL(3) both; pdb_storage 6 both (same rows modulo rename); surplus_identity 0 not identical both;
  provider_link PASS both.
- Without csplit regen the candidate gates 15/2/1: the symbols.json edit and regen are mandatory (same patch; ninja regenerates).

## House rules / held classes
- Rename only; rule 7 improved (authentic name; 7979cf8f texture-cache sibling precedent). Strip test / /Od order: N/A.
- Not held, not reserved, no COMDAT, no header, no prototype elsewhere.

## Credit
- Zero (names are byte-inert). No object completes: _sound_cache_debug_render stays parked; storage 2/3 rows remain.

## Integration hazard + optional companion
- Pending round-1 storage packet A conflicts textually with patch 10 in both orders (same .c line 471, same symbols.json L7224),
  and its symbols.json copy is stale at HEAD. storageA_rebased_after_10.patch applies after 10 (post-image index hashes match) and
  measures byte-inert: storage-only flips, gate rows same, object_audit FAIL(3)->FAIL(1), pdb_storage 6->0, provider_link PASS,
  objdiff identical. It is offered as evidence, not as part of this verdict.

## Not run
- Full ninja, pytest, board-wide stable-verdicts sweep (integrator). The stable-verdicts diff should treat the rename as a
  same-address rename, as in the 7979cf8f precedent.
