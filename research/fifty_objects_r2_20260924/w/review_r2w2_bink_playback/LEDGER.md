# Review ledger: bink_playback storage packet (r2w2)
Reviewer label: review-r2w2-bink_playback. Claimed 2026-09-24.
- apply --check PACKET/01/02 at HEAD 434f0151: rc 0; round trip OK; PACKET == 01+02; sha256 matches index.
- control csplit 833/833 == build/split; emu csplit (02 applied) -> only bink_playback.obj, 4 storage bytes (sym 162/163/164/167, 2->3).
- publics: only _debug_bink/_bink_globals of the six; 0 UNDEF refs to the four in build/split and build/base; no header extern.
- gate prod 23/1 == cand 23/1 (q3 parked row). cand.obj vs prod.obj: 0 section diffs, 4 storage diffs only.
- object_audit: prod/split FAIL(1), cand/emu FAIL(1) 0 sym diffs; mixed FAIL(5) -> atomic landing required.
- pdb_storage 5 -> 1 (_code_001b5850). surplus 0 code COMDATs; provider_link PASS both orders.
- objdiff 3.3.1 mini report: prod/cand/mixA/mixB identical 3733/3803, 23/24, 1701/1701.
- /W3 14 == 14; fake_match_scan 1 pre-existing int 3 lead in both.
- strip test: `static` is PDB-required (removal -> storage DIFFs). Stripping the pre-existing `= { 0 }` moves .bss (356->360), so the initialisers are load-bearing, covered by precedent and not introduced here.
- precedent cdc8ebd3 (collision_bsp_usage_times static) read: every condition holds.
VERDICT: APPROVE (zero credit). See REVIEW.md.
