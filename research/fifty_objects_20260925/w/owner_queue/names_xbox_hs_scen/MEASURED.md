# MEASURED - owner_queue/names_xbox_hs_scen (wave 5)

Base: cdc8ebd3 (Halo objects 379). During the run HEAD moved to eb33baec, a docs-only ledger commit.
Every patch was re-checked there and is still clean. Scorer: objdiff-cli 3.3.1 (frozen `build/tools`).

Nothing was landed. All work lives under this slug, and `git status` is clean. The full probe log is in `LEDGER.md`.

## Split emulation (both items)

| Config | csplit radius (833 objects vs build/split) |
|---|---|
| control (config/ copy) | 0 differ (build/split is current) |
| item 1 (`rx/config_P`) | only `source/rasterizer/xbox/rasterizer_xbox.obj` |
| item 2 (`hs/config_H`) | only `source/hs/hs_scenario_definitions.obj` |
| both (`hs/config_RH`) | exactly those 2; each byte-identical to its per-item split |
| independent (`rx/config_dbg`) | only `source/rasterizer/xbox/rasterizer_xbox_debug.obj` |

## Item 1 - rasterizer_xbox (patches/1_rasterizer_xbox/01-05)

| Check | Result | Evidence |
|---|---|---|
| gate --all, candidate + patched header (shadow; `#error` control hit) vs split_P, `--forbid-emitted-symbol _point_from_line3d` | **95/95 EXACT**, guard passed | `rx/gate_cand_vs_splitP.txt` |
| production gate vs build/split | 95/95 (function credit is already taken, so the gain is admission only) | `rx/gate_prod_vs_buildsplit.txt` |
| object_audit vs split_P | **PASS**: 188 sections ok (.bss 2230 8-align, blend .rdata 108 4-align, .data 6), 216/216 January symbols | `rx/audit_cand_vs_splitP.txt` |
| object_audit, production vs build/split | FAIL(22) | `rx/audit_prod.txt` |
| provider_link (both orders) | **PASS 15/15**: 3 code COMDATs `_dot_product3d`/`_plane3d_distance_to_point`/`_plane3d_from_point_and_normal`, 3 SDK tables, 9 literals | `rx/provider_link_cand.txt` |
| storage vs cachebeta publics | candidate 216/0 disagreements (production 25). None of the 9 names is public. | `rx/pdb_storage_cand.txt` |
| data (objdiff 3.3.1) | production == candidate: data 9048/9048 (.bss/.data/.rdata 100); code 91/95 raw, the same 4 pre-existing $L under-credits | `rx/objdiff_measures.txt` |
| header blast radius (7 includers) | 6 objects have every section identical. **rasterizer_xbox_models: one section moves, the PARKED `__rasterizer_model_draw`** (5168/348 both; sha 89b0d7ea -> cbfa8585; objdiff 95.08453 -> 95.07629). Strict status is unchanged (12/2). | `rx/blast_summary.txt` |
| park check (`tools.parked_functions.validate_parked_functions`, models manifest) | patched header + current parked.json: **STALE 1**. Patched header + `05_parked.json.patch`: active 2 / stale 0. | `rx/park_check.txt` |
| names load-bearing | Reverting only the 23 .bss rows to the aggregate row: gate 86/95 (9 reloc-identity residuals), audit FAIL(15). So the symbols.json rows are required. | LEDGER step 5 |
| /W3 /Zs | identical to production {C4090:1, C4146:1, C4244:15} | LEDGER step 5 |
| fake_match_scan | 0 leads | LEDGER step 4 |

**Rebase change at cdc8ebd3:** `05_parked.json.patch`. It is new and required. Since 466698b8/cdc8ebd3 dropped
`#include "interface/hud_draw.h"` from rasterizer_xbox_models.c, removing the one stale prototype in 02 flips a
declaration-count tie in that parked fuzzy body. The patch re-baselines the park: no source change and no strict
effect. Patches 01-04 are otherwise the wave-2 packet unchanged: `rasterizer_xbox.c` and the header did not change since
931ed8dc. They were regenerated with zero line offsets.

## Item 2 - hs_scenario_definitions (patches/2_hs_scenario_definitions)

| Check | Route A (A01-A04) | Route C (C01-C03 + A02-A04) |
|---|---|---|
| gate --all vs split_H | **EXACT 224 `_byte_swap_script_syntax_data` 1/1**, guard passed | **EXACT 1/1** (shadow header, `#error` control hit) |
| object_audit vs split_H | **PASS**: 23 sections ok incl. .data 628/628 align 4/4 (8-byte); 40/40 symbols | **PASS** (same) |
| production audit vs build/split | FAIL(1): .data align 4/3 (the recorded rejection) | |
| storage vs cachebeta | 40/0; none of the 9 names public | 40/0 |
| provider_link | no surplus externals: PASS | PASS |
| data (objdiff 3.3.1) | **1112/1112** native (production 484/1112, .data 92.773094): **+628 data bytes** | 1112/1112 |
| names load-bearing | candidate vs a split WITHOUT the 9 rows: .data 48.931007, 484/1112. So A02 is required. | |
| /W3 /Zs | +2 C4090 (hs.h `char const *[]` into tag_groups.h `char **names`), byte-inert | same |
| fake_match_scan | 0 | 0 |
| C1 consumer sweep (hs.c changed heavily in 6e3e2d35, so re-run) | n/a | hs 447/1 (1410 sections, 0 differ), hs_compile 61/3 (222, 0), hs_runtime 64/2 (172, 0; 64 internal `$L` names shift only). objdiff per-function maps mod == orig == build/base. |

**Rebase change:** 6e3e2d35 already landed the first-party static `byte_swap_script_syntax_data` (fallback F1+F2), so
the worker's A, B and C2 no longer applied. A01 and C03 are the worker's own final post-images (cand1.c / cand2.c,
CRLF) diffed against the tree. They carry only the data split. A02 is B minus the landed rename row. D is split into
A03 (config.json) + A04 (rejection retirement). C1 is reproduced as C01 (header) + C02 (hs.c).

## Objects that become COMPLETE on a yes

- Item 1 yes: `source/rasterizer/xbox/rasterizer_xbox` (+1 object; +0 functions, +0 data; the audit is the only gap).
- Item 2 yes: `source/hs/hs_scenario_definitions` (+1 object; +0 functions; **+628 data bytes**; the
  source-layout-incomplete rejection is retired).
- Both: Halo objects 379 -> 381.
- Would still NOT complete: `rasterizer_xbox_models`, whose 2 parked residuals are untouched (only one park hash is
  re-baselined). No other object is affected.
- Independent (not name-gated): the `rasterizer_xbox_debug` storage rows turn its object_audit FAIL(8) into PASS, with
  gate 18/18 and split radius 1. That object is already labelled Matching, so the count does not change.

## Apply readiness

The combined post-images were reproduced with `git apply` outside the repo. They are cmp-identical to the packet
trees: `rx/tree`, `hs/treeA`, `hs/treeC` and `hs/config_RH`. See `patches/APPLY_ORDER.txt`.
