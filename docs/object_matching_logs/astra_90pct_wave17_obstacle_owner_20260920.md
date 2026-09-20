> Frozen evidence/probe report; later admission is recorded only in the completed wave17 summary. The private player helper adds one meaningful byte; all other lanes earn zero credit.

# New 2001 MAP evidence does not resolve obstacle snapshot ownership

**CLOSED — zero source shapes, zero C compiler calls, zero credit.**
`audit.py` reproduces the bounded read-only check; `audit.json` contains
verified input hashes, exact MAP lines/module names, January/current owner
records and fingerprints. No source, header, configuration, target or tool
was changed. No input binary was executed or decoded.

## New original linker records

| MAP | Static-symbol records | Static data records | Obstacle-module static functions |
| --- | ---: | ---: | ---: |
| 2001-08-15 `cachebeta.map` | 3,061 | 0 | 12 |
| 2001-09-25 `cachebeta.map` | 3,357 | 0 | 12 |
| 2001-09-25 `cache.map` | 3,216 | 0 | 10 |

Every parsed record under each **Static symbols** heading carries the `f`
function flag. None of the maps contains a symbol named `_debug_path`,
`_debug_obstacles` or `_current_traverse_index`. No **Line numbers** section
is present. The heading therefore does not establish private-data coverage;
absence here is not evidence that the original source lacked these objects.

Both beta maps explicitly assign `_debug_obstacle_path_on_failure` and
`_debug_obstacle_path` to `path_obstacle_avoidance.obj`:

| Build | Failure flag VA | Debug flag VA | MAP lines |
| --- | --- | --- | --- |
| August beta | `0x006DAE60` | `0x006DAE61` | 17,798–17,799 |
| September beta | `0x007253E8` | `0x007253E9` | 18,858–18,859 |
| January target | `0x0071BE48` | `0x0071BE49` | `config/symbols.json` |

In both beta maps, the failure flag is 8,524 bytes after
`ai_debug.obj::_actor_path_debug_array`, the same public-label spacing as
January. The flags are one byte apart. This independently corroborates the
nearby layout, **not** the beginning, size, type or name of either intervening
snapshot allocation. The maps give no internal label at the proposed split.
The September retail map omits these debug flags. The August
`1749betaP.xbe` patched-filename caveat is not used as evidence.

## January comparison and unchanged hold

January's 8,514-byte BSS contribution has `_current_traverse_index` at offset
zero and public flags at `0x2140` and `0x2141`. Current source's equally sized
section owns static `debug_path` at zero and `debug_obstacles` at `0x1538`.
The existing copy/use evidence supports path and obstacle snapshots but does
not independently authenticate their original ownership/declaration split.

The current `_render_debug_obstacle_path` remains 224 bytes / 28 relocations,
normalized-code and relocation-geometry equal but symbolically unequal.
Its pinned semantic-credit veto remains applicable. Current
`_path_avoid_obstacles` remains 1,312 padded bytes against January's
1,360 / 49. Historical v5 normalized equality was not replayed: the old
1,345 + 224 = **1,569 meaningful / 1,584 padded-byte** ceiling remains
conditional, not an available gain.

Read before inspecting the maps: Sept. 8 owner reconciliation, Sept. 13
unwritten-leaf reconciliation, Sept. 14 w1/w3 obstacle ledgers, the current
semantic-credit rejection, and campaign wave6/wave13 alias dispositions.
Reopening still needs independent original private-data identities and
boundaries/types, or equivalent source evidence. Names or public-label
spacing alone do not justify a target relabel, raw-offset source expression,
prototype inference or old source replay.

Evidence ownership: **RELEASED**.
