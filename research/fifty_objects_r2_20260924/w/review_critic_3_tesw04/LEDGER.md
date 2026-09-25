# review_critic_3_tesw04 LEDGER - adversarial review of critic_3 patch 04 (transport_endpoint_set_winsock names)

HEAD 5d3ca708 (task named 7b9de585, an ancestor). Claimed source/bungie_net/network/transport_endpoint_set_winsock as
review_critic_3_tesw04 (garbled orchestrator slug normalised).

## R1 name provenance (read-only, independent of critic_3's scripts)
- Raw maps grep: Sept-2001 cachebeta.map Static symbols: `_get_next_available_set_array_index` 0x6f980 (next 0x6f9c0 -> 0x40,
  between _net_startup_debug and _transport_push_key) and `_poll_ep_array_compare_proc` 0x70060 (next 0x70090 -> 0x30, between
  _delete_endpoint_set and _poll_endpoint_set) = January 0x70430/0x40 and 0x70B10/0x30 in the same anchor slots.
  Aug-2001 cachebeta.map: same two names/sizes/slots. Sept-2001 RETAIL cache.map: `@get_next_available_set_array_index@4`
  (fastcall, one 4-byte param = the set pointer) and `_poll_ep_array_compare_proc` (cdecl, as a qsort callback must be).
- Bodies (bodycmp.py, xbe.py): January compare body (48 B, 0 relocs) is RAW-byte identical to Sept xbe file 0x71060 and Aug
  1749betaP.xbe 0x6a3c0, which are exactly the map addresses (.text raw 0x1000 / VA 0x11000). get_next: relocation-masked
  identical to Sept incl. the assert line 0x39; Aug differs only in that assert line (0x30).
- Atlas: January 4cc87b45 @0x470b10 = `?poll_endpoint_array_compare_proc@@YAHPBX0@Z` tier EXACT (C++ mangled => propagated from
  the 2003 Halo 2 maps, a later C++ fork). The same propagation labels Aug 1749betaP @0x7a3c0, where that build's OWN map says
  `_poll_ep_array_compare_proc`: the propagated name is provably not contemporaneous evidence. No atlas name for 0x70430.
- The current production names are invented ("Private semantic name is inferred", parked evidence; a5c2c50a 2026-09-04),
  not the atlas name (the task summary's framing is loose; the patch is unaffected).
- January cachebeta.exe strings: 0 hits for poll_ep / poll_endpoint_array / compare_proc / get_next_available / current names;
  January keeps the assert text "set && set->ep_array" => January still uses the `ep_array` vocabulary the Sept name is built on.
- cachebeta publics: neither 0x70430 nor 0x70B10 is public (neighbours push_key/pop_key/poll_endpoint_set are) => statics.
- HCEX.pdb / HCEX_Release.pdb: no endpoint_set symbols at all (360 network layer replaced) - no later-lineage CE evidence.
- /Od build has no symbols (names unavailable there).

## R2 measurements (slug-local)
- git apply --check patch 04 at 5d3ca708: clean. Sequential GNU patch of 01..21 on copies: all OK; unit-04 result identical to 04 alone.
- csplit (build/tools/csplit.exe -> slug dirs): control split_ctl == build/split 833/833; split_p04 vs ctl: ONLY
  transport_endpoint_set_winsock.obj differs; objcmp (names mapped): all 47 sections + relocations + 132 symbols IDENTICAL,
  renamed symbols keep storage 3.
- gate prod (build/split) vs cand (with_split -> split_p04): 23 EXACT / 3 residual both; same rows
  (get_next EXACT 64; compare residual 48 [sha]; poll_endpoint_set 560 [sha]; transport_initialize 416 [size 400!=416]).
- objcmp prod.obj vs cand.obj: 63 sections, 152 symbols IDENTICAL (names mapped) except .debug$S (CodeView names, unscored).
- objdiff 3.3.1 (sha1 3130e428): prod vs cand identical: code 2610/3618, fns 23/26, data 452/452, 97.27595%;
  compare 97.61905, get_next 100.
- parked validation (tools.parked_functions.validate_parked_functions on slug project): patched manifest => 3 active, 0 stale,
  0 invalid. Negative control: OLD manifest vs renamed build => invalid "expected one report function, found 0" => parked.json
  rename MUST land in the same commit as symbols.json (as the manifest says).
- object_audit: prod FAIL(3) == cand-vs-split_p04 FAIL(3) (the 3 residual .text; 49 January symbols, 0 differ). cand vs OLD
  split: FAIL(6) (2 MISSING + 2 surplus) => source + symbols.json atomic.
- pdb_storage: 1 disagreement both (pre-existing `_net_startup_debug` 2/2, owner-held in round 1); renamed rows agree (static).
- surplus_identity: 0 candidate-only code COMDATs both. provider_link (both orders): PASS both (3 literals + 3 D3D tables).
- No header touched, statics only => no consumer can change. No unusual construct (strip test N/A: names only), no new scope.

## R3 verdict
APPROVE (zero credit, names-only, byte-inert). Land 04 atomically (source + symbols.json + parked.json), then csplit regen.
Integrator notes: name-keyed caches (regression_gate snapshots, stable_verdicts, board.json) will see the EXACT
`_transport_endpoint_set_get_next_index` vanish and `_get_next_available_set_array_index` appear (same bytes); the docs ledger
transport_endpoint_set_winsock_obj_set_operations_cluster_20260904.md still names the old inferred names (history, optional note).
Wording nit (not blocking): the parked evidence keeps "Private semantic name is inferred"; the appended rename note makes the
history clear, and could also cite the Aug-2001 map, the Sept retail cache.map and the raw-byte identity.
Reopen: a later first-party CE map/PDB (Oct 2001-Jan 2002) naming these statics differently, or a January literal contradicting them.
