# `cache_files_windows.obj` remaining-pair re-audit (2026-09-12)

## Scope and result

This was a read-only source re-audit of the two remaining non-exact owners in
`source/cache/cache_files_windows.c`.  The production source already contains
the most credible complete typed reconstructions, so it was left byte-for-byte
unchanged at Git blob `80827a9508bbfa781aec2d5538186d41e1b97b4f`.

The grouped VC7 gate remains:

```text
== exact 48  residual 2  unwritten 0  (of 50 listed)
```

All 48 inherited exact owners remain exact.  The emitted-symbol guard for
`_point_from_line3d` passes.  This re-audit claims **zero new exact functions
and zero meaningful/padded exact bytes**.

## Evidence reviewed

The January split object, current disassemblies, all Git revisions that changed
this translation unit, the earlier forwarding/public/register ledgers, and the
complete Opus cache reconstruction experiment corpus under
`C:/halo-worktrees/opus-small-families-30k-20260902/scratch` were checked.
That corpus includes the retained reconstruction and numerous declaration,
control-flow, volatile, initialization-position, and expression-shape probes.
No historical or uncommitted candidate closes either owner naturally.

The existing fuzzy records in `config/parked.json` still match the compiled
object exactly:

| Owner | Target / candidate padded bytes | Relocations | Current similarity | Disposition |
|---|---:|---:|---:|---|
| `_cached_map_file_read_header` | 400 / 400 | 26 / 26 | 98.70079% | retain fuzzy, zero credit |
| `_cache_files_open_cache_files` | 752 / 752 | 47 / 47 | 83.333336% | retain fuzzy, zero credit |

For `_cached_map_file_read_header`, the remaining differences are VC7 register
selection around the async-read argument block and one redundant completion
reload/control-flow scheduling choice.  The retained source expresses the real
FILETIME capture, asynchronous 2 KiB header read, alertable wait, verification,
clear-on-invalid behavior, and failure invalidation without steering devices.

For `_cache_files_open_cache_files`, every API/string relocation and the full
open/create/resize/header-validation behavior is present.  The residual begins
with VC7's placement of the cold `CreateFileA` failure arm and propagates through
the loop schedule.  Prior goto, volatile/register, declaration-order, and
expression-shape probes did not produce a natural exact body.

## House-rule disposition

Further tuning would repeat already exhausted byte-directed experiments.  No
goto reshaping, inert access, fake dependency, raw address/offset, pragma,
forced inline, assembly, or nonsensical success/failure path was admitted.
Both complete implementations remain explicitly fuzzy and receive no matching
credit.  Reopen only with authoritative January source/local-variable records
or a natural same-compiler donor that explains the remaining schedules.
