# review_critic_3_network_connection LEDGER (adversarial review of critic_3 patch 07_network_connection)

Worktree HEAD 5d3ca708 (task named 7b9de585; 7b9de585..5d3ca708 = one docs ledger only). git status clean except the
pre-existing untracked research/ at start and end. Claimed source/networking/network_connection as
review_critic_3_network_connection (verification compiles only). No tracked edits, no ninja/configure/git mutation.

## R1 name provenance (independent of critic_3's tools)
- Provenance of the CURRENT names: commit 3ad0011c (2026-09-02) replaced `_code_00117f30/118130/118ac0/118b40` with
  "restrained subsystem names" (docs/object_matching_logs/network_connection_obj_network_wave_20260902.md, "Names" section):
  invented descriptive names, no first-party source.
- Sept-2001 cachebeta.map "Static symbols": exactly 6 network_connection.obj statics; merged by address with the 17 publics
  the 23-function order and every gap size equal January's split (0x200, 0x440, 0x80, 0x1e0 for the four renamed rows).
  Aug-2001 map: same 6 static names, same order.
- rawcmp.py (rawcmp.txt): January split bytes vs Sept xbe bytes at the Sept map VA, masking ONLY January's relocation fields:
  0 differing bytes for ALL 23 functions of the object, including the 4 renamed rows (0x200/33 relocs, 0x440/81,
  0x80/10, 0x1e0/32). Note: critic_3's xcmp gave 0.957 for log_traffic_event; under raw January-reloc masking it is
  byte-identical (critic's figure was conservative). Control: read_reliable vs Sept idle_client_reliable_endpoint (same
  size) differs in 332/480 bytes -> the compare discriminates.
- Call edges: all 10 January E8 relocations into the renamed statics (delete, write x3, create_client_from_endpoint, new ->
  log_traffic_event; read -> unreliable x2 / reliable; idle_server_reliable_endpoint -> create_client_from_endpoint) decode
  in the Sept xbe to exactly the proposed Sept names: ok 10 bad 0. (The idle/idle_client log sites are partly inlined in
  January; no call relocation there.)
- Symbol atlas: the atlas keyed by JANUARY's cachebeta.exe hash (4cc87b45...) carries exactly these 4 names at exactly these
  4 addresses (off = file_offset + 0x400000), tier exact. The same 4 names appear in 4 atlas builds, including later
  fastcall-decorated builds (`@network_client_reliable_connection_read@16`, `@network_connection_create_client_from_endpoint@4`
  ...). The 4 CURRENT names appear in NO atlas build.
- cachebeta.pdb publics: no public at RVA 0x117F30/0x118130/0x118AC0/0x118B40 (RVA == file_offset, anchored by
  going_stale 0x117EF0) -> January statics; storage `static` correct and unchanged.
- HCEX.pdb 2011: no network_connection module (control `unit_update` resolves) -> neutral.
- January literals (jan_strings.txt): no literal names any old or new function. "_traffic_log.xls" and "!\"unknown traffic
  event\"" fit log_traffic_event; "serverside" occurs only in the `_connection_create_serverside_client_bit` flag name (the
  source of the invented `new_serverside_client`). Nothing contradicts a Sept name.

## R2 byte-inertness
- Patch = pure rename: reverse-mapping cand.c reproduces production (modulo CR); 4 symbols.json lines changed in place
  (4853,4854,4859,4860), `"static": true` kept, order untouched. New names collide with nothing in source/config/tools/tests.
- Own csplit: split_ctl == build/split 833/833; split_07 differs in source/networking/network_connection.obj ONLY;
  objcmp: 99 sections / 250 symbols identical modulo the 4-name map (bytes, relocation records, storage, section, value).
- gate: prod 21 EXACT / 2 residual (parked _network_connection_connect [size 272!=288], _network_server_close_client_connection
  [sha]); candidate vs split_07 21/2; per-function rows identical modulo map (symmetric diff empty).
- objcmp prod.obj vs cand.obj: 108 sections / 273 symbols identical modulo map (only .debug$S differs: names); renamed
  symbols keep section/index/storage 3.
- Battery prod vs cand (battery/): object_audit FAIL(2) both (the two parked residuals; 99 January symbols, 0 differ);
  pdb_storage 0/0 disagreements; surplus_identity 0 code COMDATs; provider_link PASS both orders (5 surplus data symbols);
  outputs multiset-identical modulo map (battery/compare.txt).
- objdiff 3.3.1 (sha1 3130e428) own mini report: 5558/7222 code, 20/23 fns, 3417/3417 data both sides; per-function
  rows identical modulo map.
- git apply --check: 07 alone OK at HEAD; series 01,03..21 together OK.
- config: parked.json network_connection entries are _network_connection_connect and _network_server_close_client_connection
  (not renamed); no other config/tools/tests reference the old names.

## R3 verdict
APPROVE, no amendment. Zero credit; byte-inert; names are first-party (Sept + Aug 2001 maps, January-hash atlas, later
retail builds) and replace invented ones (house rule 7 authentic-name preference). Integrator must regenerate csplit in
the same landing, else the 4 renamed rows go missing against the stale build/split names.
