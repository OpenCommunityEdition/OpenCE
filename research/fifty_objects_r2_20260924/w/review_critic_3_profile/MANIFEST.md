# MANIFEST - review of critic_3 patch 05_profile (source/cseries/profile)
- Base: 5d3ca708. Candidate: scratch/w/critic_3/patches/05_profile.patch (profile.c + config/symbols.json lines 1618-1648).
- Evidence per construct: 11 static renames only; Sept-2001 map static names at identical order/size; masked bytes
  identical (rawcmp.txt); 6/6 call edges agree (callcheck.txt); January assert text "timesection".
- Target vs candidate: all 44 rows identical to production (43 EXACT + parked _compare_profile_sections 304 vs 288).
- Data: all data sections identical (objcmp); objdiff data 1131932/1131932 both.
- Storage: 11 renamed symbols static (3) in split_05 and candidate; pdb_storage 0 disagreements.
- Surplus/provider: 0 surplus code COMDATs; 3 __real literals PASS both orders (unchanged).
- House rules: identifiers/comments only; removes code_<addr> placeholders from the symbol-list comment.
- Tests run: gate x3, csplit x2, objcmp, object_audit/pdb_storage/surplus_identity/provider_link prod+cand, objdiff 3.3.1,
  git apply --check. Not run: ninja, pytest, board sweep (integrator).
- Credit: zero. Held: none new. Landing requires csplit regen.
