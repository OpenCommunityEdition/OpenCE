# `files_windows.obj` — opus5 150K house-clean lane, wave w2 (2026-09-14)

## Baseline

`python -B tools/campaign/gate.py source/tag_files/files_windows --all --forbid-emitted-symbol _point_from_line3d`

| Measure | Value |
| --- | --- |
| Gate | 23 exact, 4 residual, 0 unwritten |
| Point guard | passes |
| Parked residuals | `_file_open`, `_file_read`, `_file_write`, `_file_get_size` |

The four parked residuals, with January padded bytes / relocations / normalized sha16:

| Function | January target | Real file |
| --- | --- | --- |
| `_file_open` | 400 / 23 / `b91ff127948eb458` | 384 / 23 |
| `_file_read` | 176 / 13 / `8442e02641914ce5` | 176 / 13 |
| `_file_write` | 176 / 12 / `f1fb6f5c8738ecea` | 176 / 12 |
| `_file_get_size` | 224 / 13 / `3e237cfe3e76e06d` | 224 / 13 |

The real file is unchanged: every candidate is a park, so all work stayed in scratch.

## Outcome: park-reopen proposal for three functions

Candidate: `scratch/workers/player_profile_files_windows_etc/fw_reopen.c`, SHA-256 `01d5ba3850c8f78f2120012e2d10a1211c88cd5095254ca23d12a3c83f4b9d93`. It is the current real file with only the three reopened bodies changed. Its whole-TU gate is **26 exact / 1 residual / 0 unwritten**:

- all 23 baseline EXACT rows are still EXACT (checked row by row);
- `_file_open` 400/23, `_file_read` 176/13 and `_file_write` 176/12 are strict EXACT with January's sha;
- the `_point_from_line3d` guard passes;
- `scratch/parkcheck.py`: `_file_get_size` shows no drift, and only the three reopened parks changed;
- owner census against build/base and build/split: no new owners;
- `tools/fake_match_scan.py`: 0 leads.

| Function | Meaningful bytes | Laws |
| --- | ---: | --- |
| `_file_open` | 391 | SINGLE-EXIT LAW |
| `_file_read` | 168 | house rule 17 initializer + SINGLE-EXIT LAW |
| `_file_write` | 161 | house rule 17 initializer + SINGLE-EXIT LAW |

**`_file_open`.**
- *Park criterion:* the park wants authentic source or family-level evidence. Its recorded premise is "return/control topology", and the 2026-08-30 early-return donor was rejected.
- *Evidence:* January materializes `mov cl,1` at +0x10E and has three epilogues (`xor al,al`, `mov al,cl`, `mov al,1`). That is a boolean result declared FALSE, set TRUE once the handle opens, and cleared when the append seek fails. VC7 tail-duplicates the result constant on each path, and the flags stay in EBX.
- *Candidate source:* this is the measured SINGLE-EXIT LAW (brief section 9). The new body is exactly that natural form: `if (!result) file_error(...)` followed by one `return result;`. Behaviour is identical to the parked body.

**`_file_read` and `_file_write`.**
- *Park criterion:* same as `_file_open`.
- *Evidence:* January keeps `file` in EBX and reloads `count`.
- *Measured levers:* two ordinary rules are needed together:
  - declaring `info` with its `file_reference_get_const_info` initializer, as the exact siblings `file_create`/`file_exists` do (house rule 17);
  - a single-exit boolean result.

  Either lever alone stays residual (scratch `fw_v7.c` has only the single exit; `fw_v8.c` has only the initializer). The same family law closed `_file_open`, and behaviour is unchanged.

## `_file_get_size`: not landed

The initializer form fixes a structural statement-order gap: January calls get_info before the `full_path` rep-stos. What remains is a pure callee-saved permutation: January has file in ESI and info in EBX; ours has file in EBX and info in ESI.

Five shapes were tried:
1. initializer;
2. result/else single exit;
3. `if (!result)` single exit;
4. partial January section order;
5. full January definition order for the whole TU.

Every shape after the first leaves the same swap. Reopen criterion: authentic evidence for the file/info register assignment.

Note: in `lrar_cache.obj`, section order proved not to equal source order for statics. The definition-order trials here are a closed negative, not an authoritative ordering.

## Rejected / do not repeat

- Definition-order moves for `_file_get_size` are code-neutral.
- The single-exit result without the declaration initializer (read/write) is residual.
- The initializer without the single exit is residual.
