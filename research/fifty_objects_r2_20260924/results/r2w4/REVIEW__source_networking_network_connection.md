# REVIEW source/networking/network_connection

approve: True

amended_patch: None

## checks
All checks ran at HEAD 5d3ca708. The task named 7b9de585; the only change between them is one docs ledger. The tree was clean at start and end, apart from the untracked research/ directory that was already there. I claimed and released the unit as review_critic_3_network_connection. Evidence is in C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\review_critic_3_network_connection\ (LEDGER.md, MANIFEST.md, rawcmp.py/.txt, objcmp.py, battery/, objdiff/, jan_strings.txt, gate_prod.txt, gate_cand.txt).

(1) Where the current names came from: commit 3ad0011c (2026-09-02) replaced _code_00117f30/118130/118ac0/118b40 with names it called "restrained subsystem names". They are invented, and no first-party source backs them.

(2) Sept-2001 cachebeta.map: it lists 6 network_connection.obj statics. Merged by address with the publics, the order and every gap size match January's 23-function split (0x200, 0x440, 0x80, 0x1e0 for the four renamed rows). The Aug-2001 map has the same names in the same order.

(3) My own raw-byte compare: I compared January split bytes with Sept xbe bytes at each Sept map address, masking only January's relocation fields. All 23 functions have 0 differing bytes, including all 4 renamed rows. So log_traffic_event (1088 bytes, 81 relocations) is byte-identical, not the 0.957 critic_3 reported; their figure was conservative. A control pair of the same size differs in 332 of 480 bytes, so the compare does tell functions apart.

(4) Call edges: all 10 January E8 calls into the renamed statics decode in the Sept xbe to exactly the proposed names (10 correct, 0 wrong).

(5) Symbol atlas: the file keyed by January's cachebeta.exe hash (4cc87b45) has exactly these 4 names at these 4 addresses (file_offset + 0x400000), at tier exact. The same names appear in 4 atlas builds, including later fastcall-decorated builds (@network_client_reliable_connection_read@16 and others). The current names appear in none.

(6) cachebeta.pdb publics: there is no public at RVA 0x117F30, 0x118130, 0x118AC0 or 0x118B40, so all four were January statics. HCEX.pdb has no network_connection module (a control lookup works), so it is neutral.

(7) January string scan: no literal names any old or new function. "_traffic_log.xls" and "unknown traffic event" fit log_traffic_event. "serverside" appears only in the flag name _connection_create_serverside_client_bit.

(8) The patch is a pure rename. Reverse-mapping the candidate reproduces production except for line endings. Four symbols.json lines change in place, keeping "static": true and the file order. The new names collide with nothing in source/, config/, tools/ or tests/.

(9) I ran csplit myself. My control split equals build/split (833 of 833 objects). The candidate split differs only in network_connection.obj, and objcmp shows its 99 sections and 250 symbols identical apart from the four names (bytes, relocation records, storage, section, value).

(10) gate: production is 21 EXACT and 2 residual; the two residuals are the parked _network_connection_connect [size 272!=288] and _network_server_close_client_connection [sha]. The candidate, gated against the emulated split, gives 21/2 with identical rows under the name map. Comparing the compiled production and candidate objects with objcmp: 108 sections identical apart from names; only .debug$S differs.

(11) Admission battery, production vs candidate: object_audit FAIL(2) on both sides, which are just the two parked residuals (99 January symbols, 0 differ). pdb_storage has 0 disagreements. surplus_identity finds 0 code COMDATs. provider_link passes in both link orders. The outputs are identical as multisets.

(12) objdiff 3.3.1 (sha1 3130e428): 5558/7222 code, 20/23 functions and 3417/3417 data on both sides, with per-function rows identical.

(13) git apply --check passes for 07 alone and for the series 01,03..21 together.

(14) parked.json: the only network_connection entries are connect and close_client_connection, which are not renamed. No other config, tools or tests reference the old names.

(15) Brief sections 2, 8 and 9: only identifiers and comments change, and invented names become first-party ones, as house rule 7 prefers. There is no new construct, so no strip test or /Od scope check applies. network_connection is not a held class.

Not run (left to the integrator): full ninja, pytest, board-wide sweep.

## issues
No blocking issues and no amendment needed. The patch is zero credit and byte-inert. Notes for the integrator:

(a) csplit must be regenerated in the same landing. If it isn't, the 4 renamed rows go missing against the stale build/split names and show up as regressions.

(b) The evidence is stronger than the manifest says: all 4 rows (log_traffic_event included) are byte-identical to Sept with only January's relocation fields masked, not "3 identical + 1 at 0.957". The atlas keyed by January's own exe hash also carries the same 4 names at the same addresses.

(c) The historical docs ledger (network_connection_obj_network_wave_20260902.md) still uses the old invented names. That is history and harmless, but a one-line provenance note in the landing commit would help.

(d) The unit stays incomplete because of the two pre-existing parked residuals (connect and close_client_connection); nothing here changes that.
