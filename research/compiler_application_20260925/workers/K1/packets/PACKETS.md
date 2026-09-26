# K1 packets for _ai_communication_event (owner ruling 2026-09-26) - zero strict credit, nothing landed/committed

Verified at HEAD a7e14ea4 (packets/VERIFY_a7e14ea4.txt) and re-verified at HEAD 4f57f30486691fe9995e0ec754af0dd73f4e0848 (packets/VERIFY_4f57f304.txt; identical results). The unit source is unchanged
since a8854940; build/base/source/ai/ai_communication.obj == a fresh HEAD compile (keyed_diff 0 changes).

| packet | file (sha256 prefix) | applies to | event | frame | normalised hunks | other sections |
|---|---|---|---|---|---|---|
| HEAD | - | - | [size 8128!=8064, sha] | 0x126c | 97 | - |
| P1 K1-E polarity | P1_polarity.patch (4c6a545aa0e85e8f) | HEAD | [size 8096!=8064, sha] | 0x126c | 96 | none |
| P2 structural | P2_structural_on_P1.patch (3cd25d7baebf356b); cumulative P1+P2_cumulative.patch (138c935cdaf3a1fa) | HEAD+P1 | [size 8096!=8064, sha] | 0x1270 | 72 | none |
| P3 K1-A flag | P3_false_flag_on_P2.patch (0e6f8243e9e92fe7); cumulative P1+P2+P3_cumulative.patch (1015151fe976d204) | HEAD+P1+P2 | [size 8096!=8064, sha] | 0x1270 | 67 | none |

Every packet: git apply --check OK (P1 at HEAD; P2 on HEAD+P1; P3 on HEAD+P1+P2; both cumulatives at HEAD); gate --all
46 exact / 2 residual with every non-event row identical to HEAD; keyed_diff vs build/base = only
_ai_communication_event changed, 0 added, 0 removed (so _ai_communication_actor_talk_weight is byte-identical);
review_patch 0 gains / 0 losses; /W3 identical to HEAD (C4146 x1, C4244 x11; no C4013); fake_match_scan 0 leads;
relocation multiset 336 == January (only jump-table entry positions differ).

P2 dead-initializer removal (owner item 2): "short protagonist_look_priority = 0;" -> "short protagonist_look_priority;"
is BYTE-INERT (P2a == the previous admissible candidate: 8096, 72 hunks). Not restored. The reply-branch
"near_player = FALSE;" that K1 had introduced (also dead) is removed, byte-inert. Production's INHERITED
"boolean near_player = FALSE;" is kept verbatim and disclosed: it is also dead (near_player is read only in the
non-reply branch after its assignment); removing it measures 74 hunks, 8096 (P2nb, not applied: owner decision).

P2 frame note: P2/P3 frame is 0x1270, 4 bytes over January (HEAD and P1 are 0x126c). January's frame returns only with
the held B (play_type) or C (recipient_look_data) forms (K1-21 add-one-in).
