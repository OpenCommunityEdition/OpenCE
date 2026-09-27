# `_flag_update` first-party helper audit — 2026-09-26

**No new candidate, no production edit, no strict gain.** The proposed
missing-helper witness is absent from the later first-party `/Od` build.
This conclusion comes from fresh instruction inspection, not merely the
previous slot-only classification.

Base: `fdf76bd0caf859aa46e5a6ca41b33164635350d0`.
Fresh whole-TU gate: **15 exact / 1 residual / 0 unwritten**. `_flag_update`
remains 1184 padded bytes / 26 relocations, normalized SHA-256
`776ba88304402bf2ae995d27de02026f8a9a6655fe565c6385712bbf55c8d2ee`;
January SHA-256 is
`a8c30b7df9882ad4e154767a3ce63294ebae7d9b8bc5aa1671acc8e034efca6d`.
Fresh alignment: January 390 instructions, canonical 393. The diagnostics
compile reports zero warnings and its complete function sections match the
stock gate's. No ownership change. No full-board rebuild is needed for this
read-only audit and none was run.

## Direct `/Od` observations

Fresh `later_od_flag_update.txt` is function `0x797610..0x79802b`, extracted
using the inspected donor `scratch/orch/odbuild.py` from the primary supplied
`halo_cache_symbols.exe`. It is topology evidence from a later source
revision, not January byte proof. No executable was run.

| Site | Actual source-topology evidence |
| --- | --- |
| `0x797744..0x7977a0` | Forms the RTC-named 8-byte `parent_vector`, then genuine call at `0x797791` to magnitude2d (`0x42def0`). Already tested by Lane D S1/S8, inert. |
| `0x7979f8..0x797a38` | Turbulence scaling is three explicit component multiplies/stores, no scale helper call. Prior w2 had already tested this spelling inert. |
| `0x797bf3..0x797c3b` | Forms RTC-named 12-byte `parent` by explicit component differences. `normalize3d` is a real call at `0x797c4a` to `0x432980`. |
| `0x797c54..0x797cec` | Estimated positions are three separate component multiply/add/store sequences, **no point_from_line3d call**. The neighbor counter increments separately at `0x797cf5..0x797cff`. |
| `0x797dc3..0x797e4a` | Weighted components accumulate directly into `new_position`; the scalar weight sum is separately added. No vector helper call. |
| `0x797e63..0x797f04` | A real local weight initialized to 4, then explicit force-point component accumulations and scalar sum update. Already tested by Lane D S9/S15, inert. |
| `0x797f0c..0x797f64` | Weight sum reused as reciprocal, then three explicit component multiplies. No scaling-helper call; Lane D S10/S11 measured this 1168 bytes with wrong January x87 tail. |

RTC authenticates `attachment_location`, `attachment_points`,
`attachment_force_points`, `attachment_y`, `y_attachments`,
`weather_palette_index`, `parent_deltas`, `parent_distances`, `parent_vector`,
`turbulence`, `new_position`, `new_location`, `estimated_positions`, and
`parent`. Renaming the current corresponding local solely for compiler
identity would not supply a new semantic factor.

The late-build early `y_attachments[row]` load at `0x7978c2..0x7978c9`
precedes vertex lookup/physics; January's current ordering differs. Lane D
S5 already uses that witness and removes the row EDX web, leaving 28
displacement-only differences. Its useful fuzzy source is at
`C:/halo-worktrees/claude-lane-d-refresh-20260922/scratch/lane_d/w/objects__widgets__flags/v_s5.c`;
this is not a new result and is not replayed as one.

## Prior experiments checked

- Lane D `objects__widgets__flags/REPORT.md` S1–S26: parent-vector helper,
  direct new_position accumulation, reused reciprocal, early attachment
  index, separate counters, real force weight, division forms and scopes.
- `docs/object_matching_logs/flags_obj_flag_update_credible_fuzzy_park_20260903.md`.
- `flags_obj_opus5_150k_w2_20260914.md`, `flags_obj_opus5_250k_w3c_20260915.md`,
  and `flags_obj_opus5_next150_tierB_20260915.md`: width-by-owner and local
  scoping controls refute a demonstrated width defect; no fake slot control
  is licensed.
- `research/fifty_objects_r2_20260924/results/r2w1/TRIAGE__source_objects_widgets_flags.md`.
- Original Fable `C:/halo-worktrees/fable-small-families-20260901/scratch/workers/flags.log`
  and `flags_body_H3.c` / `flags_gate_H3.txt`: H3 specifically calls genuine
  `point_from_line3d` for each estimated position, and is already nonexact
  at 1184 bytes. It was formerly emission-barred; today any new emitted copy
  would still need the strict-exact caller and provider checks. The recorded
  nonexact body does not satisfy that class.

## Decision

There is no new `/Od`-attested helper restoration at the proposed sites.
Importing one would be an alternative mathematical spelling, not restoration
of the observed first-party call graph. The only genuinely attested missing
helper (magnitude2d) and the supported width/lifetime changes were already
measured in the cited independent packets. Thus no fresh bounded hypothesis
survives this evidence intake; no blind source permutations are made.

This does not prove January had no such helper or that the function is
impossible. Reopen on a new independently supported helper/API/source fact,
or a natural same-compiler witness for the reciprocal lifetime that preserves
January's tail. The source's hand-written arithmetic is not, by itself,
evidence that a genuine helper call was lost.

## Receipts

`audit.py` reruns only the unchanged source through reviewed
`gate.py --source --out`, captures a same-flags diagnostics compile, and
verifies all function sections agree. `results.json` binds HEAD, source,
compiler, gate, comparator, target, output and all 16 strict rows. Raw files
are `baseline.gate.txt`, `baseline.compile.txt`,
`baseline.flag_update.diff.txt`, and `later_od_flag_update.txt`.
Only scratch outputs were written. No git/config/header/production mutation,
ninja/configure, source-shape probes or held constructs were used.
