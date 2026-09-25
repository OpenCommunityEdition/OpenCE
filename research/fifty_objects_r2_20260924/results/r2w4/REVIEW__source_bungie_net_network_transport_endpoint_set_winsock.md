# REVIEW source/bungie_net/network/transport_endpoint_set_winsock

approve: True

amended_patch: None

## checks
All checks ran at HEAD 5d3ca708. The task named 7b9de585, which is an ancestor. The tree stayed clean with no tracked edits. I claimed and then released the unit as review_critic_3_tesw04. All evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_tesw04\ (LEDGER.md, gate_prod.txt, gate_cand.txt, battery/, objdiff/, pv/, bodycmp.py, xbe.py, objcmp.py).

1. Name provenance, checked independently of critic_3's scripts:
- The raw Sept-2001 cachebeta.map static symbols are _get_next_available_set_array_index (0x6f980, 0x40 bytes, between _net_startup_debug and _transport_push_key) and _poll_ep_array_compare_proc (0x70060, 0x30 bytes, between _delete_endpoint_set and _poll_endpoint_set). These are the same anchor slots and sizes as January's 0x70430 and 0x70B10.
- The Aug-2001 cachebeta.map has the same names. The Sept-2001 retail cache.map has @get_next_available_set_array_index@4 (fastcall, one pointer parameter) and a cdecl _poll_ep_array_compare_proc, which is consistent with a qsort callback.
- Body identity: January's compare body (48 B, 0 relocations) is raw-byte identical to the Sept xbe at file 0x71060 and the Aug 1749betaP.xbe at 0x6a3c0. Both addresses are exactly the map addresses. get_next is identical to Sept with relocations masked, including assert line 0x39.
- The atlas name at January 0x470b10, ?poll_endpoint_array_compare_proc@@YAHPBX0@Z, is an exact-tier C++ propagation from the 2003 Halo 2 maps. The same propagation also labels Aug 1749betaP at 0x7a3c0, where that build's own map says _poll_ep_array_compare_proc. So it is not evidence of what the name was at the time.
- January strings: none of the old, new or Halo 2 names appear. January's assert text 'set && set->ep_array' keeps the ep_array vocabulary.
- cachebeta.pdb publics: neither address is public, so both are file statics.
- HCEX.pdb and HCEX_Release.pdb contain no endpoint_set symbols. The /Od build has no symbols.

2. Build and object measurements:
- git apply --check is clean. Applying 01..21 in sequence with GNU patch on copies works, and the unit-04 result equals 04 applied alone.
- csplit: the control run equals build/split (833/833). The patched run differs only in transport_endpoint_set_winsock.obj. With names mapped, all 47 sections, relocations and 132 symbols are identical, and storage stays 3.
- gate: 23 EXACT / 3 residual in both production and candidate, with the same rows.
- objcmp prod.obj vs cand.obj: all 63 sections are identical except .debug$S.
- objdiff 3.3.1 (sha1 3130e428): identical results (code 2610/3618, functions 23/26, data 452/452, 97.27595%; the comparator is 97.61905).

3. Parked entry: validate_parked_functions with the patched manifest gives 3 active, 0 stale, 0 invalid. As a negative control, the old manifest against the renamed build reports 'expected one report function, found 0'.

4. Section-4 battery:
- object_audit: FAIL(3) in both production and candidate. The 3 are the pre-existing residual .text sections, and all 49 January symbols match. The candidate against the old split gives FAIL(6), which proves source and symbols.json must land together.
- pdb_storage: the same pre-existing held _net_startup_debug row is the only one.
- surplus_identity: 0 candidate-only code COMDATs.
- provider_link: PASS in both orders.
- Data sections are identical.

5. Other checks: no header is touched, so there is nothing for consumers to pick up. There are no unusual constructs and no new scopes (the strip test does not apply to renames). House rules are met: authentic first-party static names, symbols.json edited in place, private prototypes kept in the .c.

## issues
Nothing blocks landing. Patch 04 only renames two statics, earns zero credit and changes no bytes. The Sept-2001 names are the nearest first-party evidence: two maps from the same lineage (Aug and Sept 2001, three map files) agree, and the bodies at those map addresses are byte-identical to January's. They replace names that the parked evidence itself calls inferred.

1. Framing correction: the production names (transport_endpoint_set_compare_entries and transport_endpoint_set_get_next_index, added in a5c2c50a) were invented. They did not come from the atlas. The atlas's January label, poll_endpoint_array_compare_proc, is a propagation of the 2003 Halo 2 name and was never used in production. This does not affect the patch.

2. Residual uncertainty: January's own name for these statics cannot be observed. It could in principle have changed between Sept 2001 and Jan 2002, and Halo 2 in 2003 did rename the comparator. Three things argue against that here: the names in the two 2001 maps agree, January keeps the 'set->ep_array' assert vocabulary, and the atlas label is shown to be non-contemporaneous (it also contradicts the Aug build's own map).

3. Landing constraints and integrator notes:
- The source, symbols.json and parked.json changes must land in one commit, followed by a csplit regen (verified above).
- Name-keyed caches will show _transport_endpoint_set_get_next_index (EXACT) disappearing and _get_next_available_set_array_index appearing with the same bytes. This affects regression_gate snapshots, stable_verdicts and board.json.
- docs/object_matching_logs/transport_endpoint_set_winsock_obj_set_operations_cluster_20260904.md still uses the old names. It is a history file, so a note there is optional.

4. Wording nit, not blocking and deliberately not amended: the parked evidence still says 'Private semantic name is inferred'. The appended rename note makes the history clear. It could also cite the Aug-2001 map, the Sept retail cache.map and the raw-byte body identity.

Reopen if a later first-party CE map or PDB (Oct 2001 to Jan 2002) names these statics differently, or if a January literal contradicts them.
