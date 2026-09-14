# `lrar_cache.obj` — opus5 150K house-clean lane, wave w2 (2026-09-14)

## Baseline

Gate: 11 exact / 1 residual / 0 unwritten, with the point guard passing. The residual is the parked `_lrar_allocate`:

| | Padded bytes | Relocations | sha16 |
| --- | ---: | ---: | --- |
| January | 784 | 27 | `a34c14dad0afc2a3` |
| Real file | 1056 | 47 | — |

The real file is unchanged.

## New mechanism evidence for the `_lrar_allocate` park

The park is classed `tu-context-optimization`. The evidence below shows it is an **inline-frequency** boundary instead (brief section 9, INLINE FREQUENCY LAW).

- **What January does with the helper:** it calls `lrar_get_block` out of line at +0x83 and +0xCF, which are:
  - the last-block end, in the else arm;
  - the first eviction lookup.

  At the post-increment lookup inside the eviction `while` it fully inlines the helper (verify_cache + index assert + verify_block).
- **Why that means cold blocks:** January also inlines `lrar_get_block` partially at its non-loop sites (`lrar_block_address`, `lrar_deallocate`) and fully in `lrar_flush`'s loop. So January's two call sites are colder than an ordinary statement.
- **Mechanism probe** (scratch only; NOT admissible): wrapping the retry `for (;;)` in a condition that VC7 folds late gives **12/12 strict exact**, with alndiff reporting no normalized instruction differences. The probe condition is `if (size >= 0)`, redundant with the dominating size check. File: `scratch/workers/player_profile_files_windows_etc/lr_p8.c`.
- **Other wrappers:**
  - A wrapper that is not folded (`if (user_data)`, `if (cache->block_count > 0)`) also moves both lookups out of line: 784 sha-only / 800.
  - `else if (size >= 0)` around the else arm alone moves only that site (928/37). The effect is per-site condition depth.

The only exact form found uses a fixed redundant condition, which the prohibitions forbid. No natural spelling reproduces the extra level.

Natural shapes tried, all residual:

| Shape | Size / relocs |
| --- | --- |
| goto retry | 1056/47 |
| January section-order placement of the verify/get helpers | 1056/47 |
| do-while with the fit condition | 1072/47 |
| do-while with a found flag | 1056/47 |
| early `return NONE` size check | 1056/47 |
| nested size ifs | 1056/47 |
| `while (!found)` | 912/37 (both sites partial inline) |
| `while (TRUE)` | 1056/47 |
| `do { if (!fits) reset; } while (!fits)` | 1056/47 |

The earlier rejections still stand. Never restore `noinline`, hand-expanded helper copies, pragmas or dead code.

**Reopen criterion:** authentic or natural source that puts the retry loop's two lookups one conditional level deeper. Probe `lr_p8.c` provides the expected bytes for confirmation.

## Orchestrator proposal (names only, code-neutral)

The January map atlas (`scratch/atlas/lrar_cache.obj.txt`, exact tier) gives authentic names for three private helpers. They match the sibling `lra_cache.obj` names `verify_lra_cache_block` / `verify_lra_cache`.

| Address | Current symbols.json name | Atlas name | symbols.json line |
| --- | --- | --- | ---: |
| 0x0010bfe0 | `_lrar_verify_block` | `_verify_lrar_cache_block` | 4627 |
| 0x0010c040 | `_lrar_verify_cache` | `_verify_lrar_cache` | 4628 |
| 0x0010c280 | `_lrar_get_block` | `_get_lrar_cache_block` | 4631 |

Proposed change:
- **symbols.json:** edit the three lines in place and keep `"static": true`.
- **Source:** rename the identifiers in `source/memory/lrar_cache.c` in the same commit.

Verification: an alias gate over a renamed scratch copy (`lr_names.c`, three `--alias` flags) stays at 11/1. Consumers are `lrar_cache.c` only; the header does not declare them. parked.json mentions `lrar_get_block` in evidence text only.

Correction: January section order for statics is not source order here. The assert lines prove `get_block` (0x16E), `verify_block` (0x186) and `verify_cache` (0x199) are defined after `lrar_allocate` (0x111/0x11C), yet their sections come earlier.
