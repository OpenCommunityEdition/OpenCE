# `sound_dsound_xbox.obj` Opus 5 150K house-clean lane, wave w2 (2026-09-14)

## Scope and provenance

- **Translation unit:** `source/sound/sound_dsound_xbox.c`.
- **Target:** `build/split/source/sound/sound_dsound_xbox.obj`.
- **Build:** January 2002 Xbox debug, VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`.
- **Lane:** `opus/150k-house-clean-20260914`, wave w2 worker. No header, config, park or build edits.
- **Ledgers read first:**
  - `sound_dsound_xbox_obj_opus5_fresh_graphs_20260914`
  - `sound_dsound_xbox_obj_opus5_100k_20260914`
  - `sound_dsound_xbox_obj_opus5_100k_waveC_20260914`
  - `sound_dsound_xbox_obj_stian_pr1_evidence_audit_20260913`
  - `sound_dsound_xbox_obj_opus_reconciliation_20260903`
  - the four Jonas getter, angle, attenuation and frequency ledgers
- **Inherited functions, not claimed:** `_dsound_set_listener_properties` and `_channel_set_location`.
- **History checks:**
  - `git log --all` for the file: 2885ba82c, e481d5142, ...
  - `branch_sweep.py`: 7 blobs, none better than 37/2/0.

## Baseline and final

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (point guard passed) | 37 | 2 | 0 |
| final real file (unchanged, guard passed) | 37 | 2 | 0 |

- `git diff --stat` is empty.
- The unit has no parks.
- The owner census equals `build/base`.
- The fake-match scan reports 0 leads.

## Per-function outcomes

### `_dsound_initialize` (720): skipped, evidence exhausted

Re-read against this wave's levers (single exit, conditional initializer, evaluation-order binding). The only
difference is which of the four identical `dsound_error` / `dsound_dispose` failure tails survives cross-jumping:

- **January** keeps the rolloff arm's full tail. It places the `push str; jmp` stubs for distance, caps and create
  after that tail.
- **Ours** keeps the create arm's tail.

The four arms show no structural difference in January, so the recorded reopen criterion is not met. Earlier
do-not-repeat shapes were not retried: failure-first nesting, a shared dispose, per-arm `success` stores. No new shapes
were spent.

### `_channel_queue_packet` (640): skipped, evidence exhausted

- `queued` is a genuine single-exit result. It is not the defaulted-overwrite pattern of the new
  conditional-initializer law (see the bink_playback w2 ledger).
- What remains is the recorded EBX/EDI colour web after the fresh-graphs `q1` then-arm order.
- No new evidence and no new shapes.

## Reopen criteria (unchanged)

- **`_dsound_initialize`:** evidence that the four failure arms differed structurally in source.
- **`_channel_queue_packet`:** a use-weight lever with source evidence, starting from the fresh-graphs `q1` shape.
