# REVIEW: source/bink/bink_playback storage packet (round 2, wave 2)

Reviewer label: review-r2w2-bink_playback. Tree HEAD 434f0151 = 05255584 plus a docs-only ledger commit. Clean except the untracked `research/fifty_objects_r2_20260924/`, which is not mine.
Packet under review: `scratch/w/hygiene_r2w2/bink_playback/PACKET_bink_playback.patch`, sha256 417f72a2..., which is byte-equal to 01 + 02. Checksums match patches_sha256.txt.

## Verdict: APPROVE (zero credit, admission hygiene only)

This packet makes 4 .bss definitions static and adds 4 matching symbols.json `"static": true` rows. It changes storage class only. Bytes, relocations, credit and the gate are unchanged. It does not complete the object. The object stays blocked on the q3 owner row `_bink_query_analog_controller_buttons` (parked) and the `_code_001b5850` naming item.

## Independent checks (all reproduced here; nothing reused from the worker except reading its patch)

| check | result |
|---|---|
| `git apply --check` at HEAD (PACKET, 01, 02 separately) | rc 0 / 0 / 0 |
| round trip (apply then `-R` on LF HEAD blobs) | byte-identical to HEAD |
| rows edited | symbols.json lines 23308, 23309, 23310 and 23313 at HEAD. Edited in place, nothing re-serialised |
| control csplit (unmodified config copy) vs build/split | 833/833 identical, so build/split is current |
| emulated csplit (config + 02) vs build/split | 1/833 differs: bink_playback.obj. It differs in exactly 4 bytes, the storage-class byte of symbols 162, 163, 164 and 167 (2 to 3) |
| cachebeta publics | `_debug_bink` [004D2B74] and `_bink_globals` [004D2B78] are public. There is no public at 0x4D2AF0, 0x4D2B30, 0x4D2B60 or 0x4D2C50, which are the file_offsets of the four |
| UNDEF references (all 833 build/split objects and all build/base objects) | none to the four. `_debug_bink` has one (hs_globals_external), which is consistent with it staying external |
| other source/header naming the four | none. Only bink_playback.c, and no header extern, so there is no C2375 linkage conflict |
| HCEX.pdb (`DIA2Dump -sym`) | no entries, as expected: HCEA holds only bink stubs. Neither corroborates nor contradicts |
| gate prod vs split | 23/1 |
| gate cand vs split | 23/1, same row (`_bink_query_analog_controller_buttons` size 96!=80, parked) |
| cand.obj vs prod.obj (all 77 non-debug sections by section_infos_equal + full symbol table) | 0 section diffs. The only symbol diffs are the 4 storage bytes, 2 to 3 |
| object_audit prod vs split | FAIL(1): the q3 row. 59 symbols, 0 differ |
| object_audit cand vs emu split | FAIL(1): the q3 row. 59 symbols, 0 differ. `.bss` 356/356 ok, `.data` ok |
| object_audit mixed (cand vs old split / prod vs emu split) | FAIL(5) each, the 4 storage DIFFs, so both halves must land together |
| pdb_storage prod | 5 disagreements (the four + `_code_001b5850`) |
| pdb_storage cand vs emu | 1 (`_code_001b5850`) |
| surplus_identity | 0 candidate-only code COMDATs |
| provider_link cand.obj (both orders) | PASS: 3 SDK tables + 3 `__real` literals, same as prod |
| objdiff 3.3.1 (sha1 3130e428) mini report | prod, cand and both mixed pairs all give 3733/3803 code, 23/24 fn, 1701/1701 data (no credit change) |
| /Zs /W3 | 14 warnings, identical prod vs cand |
| fake_match_scan | 1 pre-existing lead (`__asm { int 3 }` at line 880), identical |
| board sweep | not needed. No header changed, and no other TU includes bink_playback.c. configure.py does not read symbols.json, and the emulated split changes only this object |

## Precedent check (special focus)

The packet relies on the canonical storage-truth rule: a symbol absent from cachebeta publics, with no UNDEF reference from any split object, was file-static. Canonical applied the same shape in cdc8ebd3 (Batch 5b). There, `collision_bsp_usage_times = { 0 }` became `static` and its symbols.json row became `"static": true`. That landing was zero credit, on an incomplete object, and atomic with its row. I read the original diff. Every condition holds here:
1. All four are absent from the publics. The same check puts the two external neighbours in the publics.
2. No split object references them (0 UNDEF).
3. The change is byte- and credit-neutral (sections identical, objdiff identical).
4. Source and symbols land atomically, with a csplit-only regeneration.
5. No header declares them.

The precedent is not stretched.

## Strip tests / house rules

- The only construct the packet adds is `static`. Stripping it keeps the bytes but brings back 4 storage DIFFs against the PDB-derived split. So `static` is required by first-party PDB evidence, and it is not decoration.
- The `= { 0 }` initialisers already exist; this packet does not add them. A lab strip removing them on the four statics moves the .bss layout (356 to 360, 6 symbol offsets shift). That follows the VC7 law that uninitialised statics are placed in name-hash order. January's .bss is in plain declaration order, and canonical precedent covers `static ... = { 0 }` (a595bbc2 models, cdc8ebd3 rasterizer_lights P1 and collision_bsp). No change is needed.
- No new scopes, locals, casts, pragmas or asm. /Od declaration order does not apply.
- The names were already canonical from the Fable reconciliation (bink_playback_obj_fable_graph_reconciliation_20260908) and are not altered here.

## Not covered by this packet (unchanged owner items)

- `_bink_query_analog_controller_buttons`: owner ruling on the q3 START/BACK split (parked).
- `_code_001b5850`: an empty, unreferenced, non-public 16-byte leaf. Absent from the publics, so it is consistent with a January static that was used but inlined at every call site (round-1 law 9). Making it static in our source drops the section, because our source never calls it. Its address-derived name is a house-rule naming item.

No amended patch is needed.
