# King, PathObstacles and CacheFilesWindows leaf closeout (2026-09-19)

## Scope and result

This bounded scratch-first audit checked the sole remaining code owner in each
of these nearly exact translation units:

| unit | fresh gate | remaining owner | January | retained candidate |
| --- | --- | --- | --- | --- |
| `source/game/game_engine_king` | 28 exact / 1 residual / 0 unwritten | `_king_calculate_hill_state` | 448 padded, 32 relocations, `9c852f781c6a691a` | 448 padded, 30 relocations, `e9d7a30d3245b8ff` |
| `source/ai/path_obstacles` | 15 exact / 1 residual / 0 unwritten | `_obstacles_test_circle` | 160 padded, 4 relocations, `3a11e2b7f83efa43` | 160 padded, 4 relocations, `f07c634dda655b38` |
| `source/cache/cache_files_windows` | 49 exact / 1 residual / 0 unwritten | `_cache_files_open_cache_files` | 752 padded, 47 relocations, `f1c88310c585856c` | 752 padded, 47 relocations, `54553053bda7f884` |

Every gate passed the `_point_from_line3d` emitted-symbol guard. No function
became strict exact, so no production source, header, symbol, park or object
status was changed. The existing source remains the best truthful
house-rule-compliant reconstruction in all three units.

The audited source Git blobs are respectively
`c5691cb153c9d461d831a9f86b08bbff571acd05`,
`e38fe74528538bb275a692ba0cd4d84b0fd99178`, and
`4c8eadd2899939a8b40d4f2e516724fd11d61f3b`.

## Duplicate-prevention audit

The dedicated object ledgers and current park records were read before any
probe. In particular, the King LAW-Z/store-sink census, the PathObstacles
natural circle-query and wave-w2 matrices, and the CacheFilesWindows
remaining-pair plus waves-w2/w3 and wave-w19 matrices bind later work unless
new evidence satisfies their reopen criteria.

`tools/campaign/branch_sweep.py` found no better reachable source:

- King: 10 unique historical blobs; the current 28/1/0 source remains best.
- PathObstacles: 15 unique historical blobs; the current 15/1/0 source remains
  best.
- CacheFilesWindows: 11 unique historical blobs; the current 49/1/0 source
  remains best.

## Per-owner disposition

### `_king_calculate_hill_state`

January keeps `hill_controlled_count` in memory in its controlled-state tails:
it uses `inc [count]`, a separate reset store and a later reload. VC7 promotes
the value into the live zero pin and sinks the two arm stores to their join,
removing exactly two relocations. The prior real-file and standalone mutation
censuses already established that every measured way to defeat this transform
requires a redundant zero store, carrier, `volatile`, fake dependency or other
forbidden steering. The best scratch shape remains the recorded 448/30
`36aa8dce51998e31` variant, not strict exact and not admissible over the more
readable retained source. There is no new January source/local record or
same-compiler donor, so no prohibited shape was repeated.

### `_obstacles_test_circle`

The retained body differs only in the x87 evaluation order of the combined
radius at `[0x4d,0x5d)`: January materializes the disc pointer, loads
`disc->radius`, then adds the radius parameter; the retained ordinary inline
predicate loads the parameter first. Existing commutative spelling, local,
control-flow, section-order and inline-helper probes remain negative.

One genuinely new evidence-backed check compiled the later HCEA direct
geometry spelling, rather than tuning arbitrary declarations. Three natural
forms were measured to separate its evaluation-order effects:

| scratch form | padded / relocs | normalized hash | result |
| --- | --- | --- | --- |
| HCEA `dx`, `dy`, then `combined_radius` locals | 160 / 4 | `7ce062a869e90d2c` | residual |
| same values declared in January evaluation order | 160 / 4 | `68477f5ce4159958` | residual |
| direct distance/radius expression without the inline formal | 160 / 4 | `3c4ca82f0e3434a3` | residual |

An equivalent reversed relational spelling was also residual at 160/4
(`b58f1b21608b6b19`) and changed the ordered-comparison lowering. None equals
January's `3a11e2b7f83efa43`. The clean HCEA donor therefore does not satisfy
the recorded same-compiler/source-context reopen criterion. The original
typed helper spelling remains clearer and is preserved.

### `_cache_files_open_cache_files`

The current helper snapshot has already restored January's double volatile
completion-byte load and made `_cached_map_file_read_header` exact. The sole
remaining body still differs in cold-block placement: January outlines the
CreateFileA-failure formatted assertion after the function epilogue and jumps
back to the shared per-file join, while VC7 keeps the truthful `else` arm
inline. The previous 24-shape wave covered inverted conditions, materialized
validity, assertion predicates, flattened else-if forms, scopes, helper-loop
forms and the only source patterns known to move this block; those patterns
also moved the successful `valid = TRUE` store and did not close the body.
There is no new authoritative source or same-compiler donor, so those variants
were not repeated.

## Reopen criteria

- King: an authoritative source/local record or compatible-compiler donor
  explaining why the global counter is not promoted through the zero pin.
- PathObstacles: January-side evidence for a non-parameter radius operand, or
  a same-compiler source donor that explains the exact x87 order.
- CacheFilesWindows: authoritative source or a same-compiler donor explaining
  the cold CreateFileA-failure placement without moving the success store.

Absent one of those, further spelling sweeps would be fake-match pressure and
should not be scheduled.
