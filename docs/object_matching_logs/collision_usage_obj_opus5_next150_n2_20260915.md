# `collision_usage.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

## Scope and provenance

- Translation unit `source/physics/collision_usage.c`. Target
  `build/split/source/physics/collision_usage.obj` (January 2002 Xbox debug build; XDK 3911 `CL.Exe`
  13.00.9254).
- Lane root `C:\halo-worktrees\opus5-150k-house-clean-20260914`, HEAD `3083f800b`, worker
  `n2:collision_usage+devices`. **No source, header, config, symbols, parks or tooling change.**
  `git diff --stat -- source/physics/collision_usage.c` is empty. Notes:
  `scratch/workers/n2_collision_usage_devices.md`.
- Read first: `scratch/opus5-next150/WORKER_BRIEF.md`, `scratch/w2/laws_w2.md` (A3(c)),
  `scratch/w3/laws_w3.md` (A29(f), A45), the six earlier ledgers via
  `collision_usage_obj_opus5_150k_w3_20260914.md`, `..._w2_20260914.md`,
  `..._small_family_completion_20260902.md`, `opus5_150k_house_clean_HANDOFF_20260914.md`, and the
  `config/parked.json` entry.

## Result

| gate (`--forbid-emitted-symbol _point_from_line3d`, guard passed) | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (HEAD) | 13 | 1 | 0 |
| final (real file, unchanged) | 13 | 1 | 0 |

`scratch/parkcheck.py`: 1 park, 0 drift. `tools/fake_match_scan.py`: 0 review leads. Zero bytes landed, and
that is the correct outcome for this object.

## `_collision_log_get_current_user` — SKIPPED-EXHAUSTED (park confirmed unreachable)

| | padded | meaningful | relocs | normalized sha256 |
| --- | ---: | ---: | ---: | --- |
| January | 256 | 243 | 23 | `fc370d7e130011908802a1d09d62cadaccb97dac976f7c5705341b7eb44d1cb3` |
| ours | 256 | 243 | 23 | `fc370d7e130011908802a1d09d62cadaccb97dac976f7c5705341b7eb44d1cb3` |

The bytes are **identical**. The park class is already `csplit-relocation-alias` with
`objdiff_percent` 100.0, landed as the w3 reclass (laws_w3 A29(f) + A45).

### Census

A frame census is vacuous here: the normalized instruction streams are byte-equal, so `sub esp,N` and every
`[ebp±N]` slot agree by construction.

Relocation-by-target census
(`python -B -m tools.campaign.relocdiff source/physics/collision_usage _collision_log_get_current_user
build/base/source/physics/collision_usage.obj --count-by-target --allow-structural`): 23 target, 23
candidate, **22 rows identical**, one differing row:

| addr | type | January | ours |
| --- | --- | --- | --- |
| `0x36` | `0x0006` | `_collision_usage_current + 2974` | `_global_current_collision_users - 2` |

`config/symbols.json`: `5832448 + 2974 = 5835424 = _global_current_collision_users - 2`. Both names denote
the **same image address**. Nothing is missing and nothing is added; no call and no global differs.

### Why there is no live path

- The source expression is `global_current_collision_users[global_current_collision_user_depth - 1]`. VC7
  folds the `-1` index into the relocation addend and necessarily names the array base symbol, so ours reads
  `_global_current_collision_users-2`.
- January's own bytes carry the identical folded address. `csplit`, resolving that address by containment,
  attributes it to the *previous* image symbol — the address is the last two bytes of
  `collision_usage_current` — and spells it `_collision_usage_current+2974`. This is the negative-index case
  of the `csplit-relocation-alias` class (memory note `csplit-relocation-alias`; laws_w3 A45, which records
  that this exact alias passes `tools/parked_functions.py` resolved-equal admission).
- Because the code is already byte-identical, **no source change can move this relocation without breaking
  the bytes**. Any spelling that reproduces January's instruction stream reproduces January's address; any
  spelling that would name `_collision_usage_current+2974` would have to index a different object out of
  bounds — undefined behaviour and a fake match (WORKER_BRIEF section 7).
- The w3 ledger's post-reclass reopen criterion reads: "none on the code side. It becomes strict EXACT only
  if csplit could spell the `global_current_collision_users-2` destination, which it cannot." This wave's
  relocdiff reproduces exactly that state, and no law in `laws_w2`/`laws_w3` bears on relocation *naming*.

### Duplicate prevention

- `git log --all --oneline -- source/physics/collision_usage.c`: 12 commits; the most recent two are
  `3e223c3ab` (1 function strict exact) and `8468de1ba` (retain verified credible source at zero credit).
  None closes this row.
- `python -B tools/campaign/branch_sweep.py source/physics/collision_usage`: 11 unique historical blobs;
  the best is the current `94797d48` at 13/1/0. No blob ever reached 14 exact.
- The w3 shape table (g1-g6) already spent the admissible body space: only the flat
  `if (...) user = NONE; else if (period == NONE) user = NONE; else { assert; user = ... }` chain is
  code-identical; merged `&&`, merged `||`, nested and result-initializer spellings collapse to 240 bytes or
  move the parameter to EBX and break both callers. Zero further shapes were spent here, deliberately.

### Disposition

Park unchanged, class `csplit-relocation-alias`, body unchanged, measurements unchanged. Reopen criterion is
unchanged and **not source-side**: only a `csplit` / symbol-attribution change able to spell a
negative-addend destination could close it, which is outside worker scope and would produce no new exact
bytes.
