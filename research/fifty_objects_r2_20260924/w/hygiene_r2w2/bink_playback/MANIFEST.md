# MANIFEST - source/bink/bink_playback: .bss storage packet (zero credit)

Base: 05255584 (clean). Claim: `r2w2-hygiene-bink_playback`. Verdict: **ADMISSION_FIX_PROPOSED** (zero credit, zero regressions).

## Packet (`PACKET_bink_playback.patch` = 01+02, `git apply --check` rc 0, round trip PASS)
1. `01_bink_playback_static_storage.patch`: adds `static` to `bink_pointer_blocks`, `bink_bitmap`, `bink_texture` and `bink_pointer_block_count`.
2. `02_symbols_json_static_storage.patch`: adds `"static": true` in place to rows 23308, 23309, 23310 and 23313. Run a csplit-only regeneration.
- Round-1 `storage.patch` was a plain `diff -u` whose symbols context no longer applies at 05255584. Both halves were regenerated from HEAD; 7979cf8f had touched bink_playback.c for bitmap_data field names. 01 and 02 must land together: the source half against the old split, or the reverse, gives 4 storage DIFFs.

## Evidence
- **cachebeta publics.** Of the six .bss rows, the publics list only `_debug_bink` and `_bink_globals`. The four objects are absent, so their January storage is file-static.
- **No outside references.** No other source file names them, and the emulated split changes only `bink_playback.obj`, so no split object references them.

## Target vs candidate
The target is `bk_emu/split`. Full table: `fntable.md`.
- Gate: 23/1 in production and 23/1 for the candidate. The single residual is the same in both: `_bink_query_analog_controller_buttons` (owner q3 ruling, excluded).

## Data, symbols and surplus
- **object_audit.** FAIL(1) before and after (that function); the candidate has 59 January symbols and 0 differ.
- **pdb_storage.** 5 disagreements before, 1 after. The one left is `_code_001b5850`, an address-derived name for an unreferenced leaf. It is a house-rule naming item for the owner, and making it static would drop the section.
- **surplus.** Unchanged: .drectve, 3 SDK tables and 3 `__real`. No code COMDATs. provider_link PASS.
- **objdiff 3.3.1.** Identical: 3733/3803 code, 23/24 fn, 1701/1701 data.

## Tests run
- Sweep: 611 identical, with only bink_playback (symtab storage) differing.
- /W3: 14 warnings in both.
- fake_match_scan: 1 pre-existing lead in both, `__asm { int 3 }` at line 880, unchanged.

## Credit
Zero. The object is still blocked by the q3 owner ruling and the `_code_001b5850` naming item.
