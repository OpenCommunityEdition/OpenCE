# MANIFEST - review of critic_3 patch 07_network_connection (source/networking/network_connection)
- Base: 5d3ca708. Candidate: scratch/w/critic_3/patches/07_network_connection.patch (network_connection.c + config/symbols.json
  lines 4853/4854/4859/4860). Applied CRLF-faithfully into patchroot/ -> cand.c, cfg_07/.
- Evidence per construct: 4 static renames only. Sept-2001 map static names at identical order/gap sizes; masked raw bytes
  identical for all 23 functions (rawcmp.txt); 10/10 call edges decode to the Sept names; January-hash atlas has the same 4
  names at the same addresses; names persist into later fastcall builds; current names occur in no first-party source.
- Target vs candidate: 23 rows identical to production (21 EXACT + parked connect [272 vs 288] and close_client [sha]).
- Data: all data sections identical (objcmp split and compiled); objdiff data 3417/3417 both.
- Storage: 4 renamed symbols static (3) in split_07 and candidate; pdb_storage 0 disagreements.
- Surplus/provider: 0 surplus code COMDATs; 5 surplus data symbols PASS both link orders (unchanged).
- House rules: identifiers/comments only; authentic names replace invented ones. No new construct -> no strip test or /Od
  scope question. Not a held class.
- Tests run: gate x2, csplit x2, objcmp x2, object_audit/pdb_storage/surplus_identity/provider_link prod+cand, objdiff 3.3.1,
  git apply --check (alone + series), HCEX.pdb query, January string scan, atlas scan. Not run: ninja, pytest, board sweep
  (integrator).
- Credit: zero. Landing requires csplit regen.
