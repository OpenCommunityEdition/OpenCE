# MANIFEST - review_critic_3_rasterizer_xbox_profile (adversarial review of critic_3 patch 09)

- Base/tip: worktree HEAD 5d3ca708 (task named 7b9de585, an ancestor; V1R profile packet already landed in 42125a52).
  No tracked file edited; no ninja/configure/git mutation. Claim label review_critic_3_rasterizer_xbox_profile (released at end).
- Reviewed: scratch/w/critic_3/patches/09_rasterizer_xbox_profile.patch (4 static renames in .c + symbols.json 5877-5880).
- Evidence per rename (LEDGER R1): Sept-2001 cachebeta.map statics + Sept-2001 retail cache.map statics (same 4, same order,
  same sizes; retail decorations @profile_assert@12 / @rasterizer_profile_enabled@0 match the parameter lists), Aug-2001 map,
  January atlas (3 of 4 exact), raw-byte identity vs Sept xbe (0 non-relocation byte diffs, callee names identical, data
  deltas identical). No contradicting January literal; HCEX silent.
- Target vs candidate per function: 16/16 EXACT both (gate_prod.txt, gate_cand.txt), identical padded sizes/relocs.
- Data sections: objcmp 0 differences over all non-debug sections (split and compiled object).
- Storage/owner: object_audit PASS (67 January symbols, 0 differ) both; pdb_storage 0 disagreements both.
- Surplus: surplus_identity 0 COMDATs; provider_link PASS both orders (unchanged).
- objdiff 3.3.1: identical measures (2511/2511 code, 16/16, data 2102/2234) -> zero credit.
- House rules: names only, authentic; strip test / /Od scope order N/A.
- Credit: none. Admission: unchanged (rejection keyed _rasterizer_profile_globals stays active).
- Optional: 09b_optional_rejection_prose.patch (rename the stale function name in the rejection prose).
- Follow-up (separate): January definition order (private statics first) reproduced byte-inertly in lab/lab_order.c.
- Files: septcmp.py, objcmp.py, with_split.py (copied), cfg_ctl/, cfg_09/, split_ctl/, split_09/, obj/, battery/, objdiff/, lab/.
