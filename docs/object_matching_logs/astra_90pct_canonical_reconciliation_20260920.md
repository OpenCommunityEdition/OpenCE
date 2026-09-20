# Astra 90% campaign canonical reconciliation (2026-09-20)

## Final integrated result

This packet reconciles the clean Astra branch
`astra/halo-90pct-breakthrough-20260920` at
`27ed13595ac32f2570e038494d9b2f67a1c8d9f2` onto `jonas/exact-pilots`.
Both histories forked at `47d2e7b69f1c1277123e979296b6f0a7de00c80b`.
Canonical commit `5361eeae528b88a2697ed1b958a7c8d0ba5604f5` had already
integrated the first six admissions, so the old-base source patch and the
scratch-heavy Astra commit range were not replayed.

The second reconciliation adds four strict functions:

| Unit | Function | Meaningful | Padded |
| --- | --- | ---: | ---: |
| `source/rasterizer/xbox/rasterizer_xbox_hardware_geometry` | `_rasterizer_vertex_buffer_new` | 354 | 368 |
| `source/items/weapons` | `_weapon_update` | 2,788 | 2,800 |
| `source/ai/action_charge` | `_action_charge_setup` | 930 | 944 |
| `source/game/players` | `_player_examine_nearby_unit` | 1 | 16 |

Incremental accepted gain over `5361eeae` is **4,073 meaningful bytes / 4,128
padded bytes / four functions**, with zero inherited exact regressions. The
complete Astra campaign gain over `47d2e7b6` is **12,387 meaningful / 12,480
padded bytes / ten functions**.

Final accepted totals are:

- Halo-owned: **7,291 / 7,574 functions**, **1,443,979 / 1,770,166 meaningful
  bytes (81.573084%)**;
- overall: **7,558 accepted functions / 1,465,211 meaningful bytes**;
- parks: **184 active / 0 stale / 0 invalid**.

## Reconciliation boundary

The production/config delta from canonical was limited to:

- `source/ai/action_charge.c`;
- `source/game/players.c` and its owner header `players.h`;
- `source/items/weapons.c`;
- `source/rasterizer/xbox/rasterizer_xbox_hardware_geometry.c`;
- removal of the now-exact vertex-buffer entry from `config/parked.json`.

The final canonical `source/` and `config/` trees were compared directly with
Astra tip `27ed1359`; they are identical. The private player helper is restored
as a `static void(long, long)` owner in `players.c`, and its stale public
Boolean declaration is removed. The resulting header correction changes the
instruction schedule of the already-fuzzy
`_rasterizer_frame_statistics_draw` without changing its behavior; all 4,165
meaningful bytes remain at zero credit.

The vertex-buffer reconstruction restores the target-proven XDK Unlock/HRESULT
flow, genuine owner headers, lock-operation enum, and error enum while removing
the old barrier/partial-struct steering. Its inherited CreateVertexBuffer
failure-path bug is explicitly documented from January instructions rather
than invented for matching. Two stock unreferenced XDK SELECTANY wrappers
remain zero-credit ownership, not additional accepted bytes.

## Independent verification

The canonical integration passed:

- all four per-function strict gates;
- `ninja -j4 all_source progress semantic_progress`;
- stable diff: **four gains / 4,128 padded bytes / zero regressions**;
- accepted ledger: **+4,073 meaningful bytes**;
- parks: **184 active / 0 stale / 0 invalid**;
- admission audit: **19 candidates / 0 contradicted / 8 rejected / 0 revoked**;
- changed-source fake scan: **zero findings**;
- global fake scan: the same **22 inherited review leads**, none introduced by
  this packet;
- `_point_from_line3d` emitted-symbol guard on the changed rasterizer unit;
- **1,152 tests passed, 5 skipped, 26 subtests passed**;
- `git diff --check`.

## Evidence boundary

The refreshed master handoff, admission manifest, research atlas, rejected-
hypothesis ledger, and curated Markdown reviews are retained under
`docs/object_matching_logs/`. Generated scratch objects, raw evidence ZIPs,
the 13 MB generated function-atlas JSON/CSV, raw source patch, commit receipt,
and package checksum/README files are intentionally excluded from Git.

The complete immutable handoff package remains at:

`C:\Users\isabe\Documents\Codex\2026-09-20\files-pasted-by-the-user-halo\outputs`

Its package checksum audit passed 149/149 files. These records preserve
nonexact research and negative results without granting them matching credit.
