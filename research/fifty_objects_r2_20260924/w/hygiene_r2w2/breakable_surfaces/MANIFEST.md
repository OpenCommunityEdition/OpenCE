# MANIFEST - source/physics/breakable_surfaces: helper, flag and storage sub-packet (zero credit; B1/B2 excluded)

Base: 05255584 (clean). Claim: `r2w2-hygiene-breakable_surfaces`. Verdict: **ADMISSION_FIX_PROPOSED** (zero credit, zero regressions).

## Packet (in order; `PACKET_breakable_surfaces.patch` = 01+02+03, `git apply --check` rc 0, round trip PASS)
1. `01_breakable_surfaces_designator_helper.patch`: deletes the TU-local hand copy `breakable_surface_get_plane_from_designator` (a copy of a bsp3d.h helper, rule 6) and calls the genuine `bsp3d_get_plane_from_designator`. It is identical to the round-1 patch.
2. `02_config_drop_ow_qifist_override.patch`: config.json line 520 drops `"options": { "cflags": [... "/Ow", "/QIfist"] }`, so the unit gets the project default flags. It is tab-indented like its neighbours and still parses as JSON.
3. `03_symbols_json_static_storage.patch`: adds `"static": true` in place to row 5362 (`_breakable_surface_effect`) and row 23116 (`_globals`). Run a csplit-only regeneration.
   - Round-1 `symbols.json.patch` fails at 05255584 because its context row `_collision_bsp_usage_times` is now static. This file regenerates it.

## Evidence
- **Rule-22 flag proof, re-measured.** With production source and flags only, the 12 functions and the data are identical. Only 7 helper COMDAT sections change. The sweep label is `bs_flagonly`, with `cmp_ctl_vs_flagonly.txt` beside it.
  - With the flags dropped, 6 external helpers become IDENTICAL to January's selected copies.
  - Surplus that differs from January: 7 in production, 1 in the candidate.
  - /Ow appears nowhere else in config.json.
- **Static storage.** Neither symbol is a cachebeta public. Production already defines both as static (storage 3), so the split was the side that was wrong.

## Target vs candidate
The target is the emulated split with 03 applied (`bs_emu/split`); the candidate is compiled with the project default flags. Full table: `fntable.md`.
- Gate: 12/12 in production and 12/12 for the candidate, also 12/12 against the old split.

## Data, symbols and surplus
- **object_audit.** Production: FAIL(2), both storage rows. Candidate against the emulated split: **PASS**, 26 January symbols and 0 differ.
- **Emulated split.** Only `breakable_surfaces.obj` differs out of 833.
- **pdb_storage.** 2 disagreements before, 0 after.
- **surplus_identity.** 7 not identical before, 1 after: `_real_local_random` against effects, which is B1 and excluded. The new surplus `_bsp3d_get_plane_from_designator` is IDENTICAL to the decals copy, and provider_link `--baseline` PASSes in both orders.
  - The full provider_link result is FAIL(1) on `_real_local_random`. That failure is pre-existing and the same as production.
- **objdiff 3.3.1 mini report.** Identical: 5331/5331 code, 12/12 fn, 685/685 data.

## Tests run
- Full sweep with the flag override against the control: 611 identical, with only breakable_surfaces differing. Warnings are identical.
- `CL /Zs /W3`: 17 warnings in both. fake_match_scan: 0 leads in both.

## Held (excluded)
- B1 (`real_local_random` named-local body in real_math.h, plus the effects NODUP copy): real_math is RESERVED for the Codex packet. It also costs `_rasterizer_frame_statistics_draw` and `_bitmap_copy` through declaration count.
- B2 (static `_breakable_surface_plane_distance`, 32 B): needs the owner's ruling on the bsp3d precedent.
- Reopen criteria are unchanged from round-1 `breakable_surfaces_audit2/LEDGER.md`.

## Credit
Zero. After this packet the object is blocked only by B1 and B2.
