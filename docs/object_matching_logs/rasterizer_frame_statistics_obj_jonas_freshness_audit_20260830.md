# `rasterizer_frame_statistics.obj` closure freshness audit (2026-08-30)

## Result

The refreshed strict board leaves this object at `5/10` functions and
`208/5168` padded code bytes.  The five residual owners are all unwritten:

| Owner | January padded bytes | Status |
| --- | ---: | --- |
| `_rasterizer_frame_statistics_draw` | 4176 | no January-version source donor |
| `_rasterizer_frame_statistics_get_fps` | 384 | one typed donor shot already rejected |
| `_rasterizer_frame_statistics_count_dynamic_vertices` | 304 | later-build donor is a retail stub |
| `_rasterizer_frame_statistics_count_static_vertices` | 64 | one natural January shot already rejected |
| `_code_0016e5c0` | 32 | three natural comparator forms already rejected |

No production source was edited and no candidate was compiled in this audit.
The evidence boundary below supplies no genuinely fresh whole-object premise.

## Evidence read before the verdict

The production source is
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\game-allegiance-closeout-20260830\source\rasterizer\rasterizer_frame_statistics.c`.
The complete existing ledgers are:

- `docs/object_matching_logs/rasterizer_frame_statistics_obj_jonas_lifecycle_first_shot_20260826.md`;
- `docs/object_matching_logs/rasterizer_frame_statistics_obj_jonas_get_fps_fixed_point_20260829.md`;
- `docs/object_matching_logs/rasterizer_frame_statistics_obj_jonas_fps_accumulate_20260829.md`.

The later HCEA bodies and layouts were read directly under
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research-cache\halocea-cseries-20260820\src\blam\rasterizer\`
and `...\src\headers\`.  The Fable correspondence records were read at
`C:\halo-worktrees\fable-crossbuild-bridge\research\crossbuild_bridge\hcea_january_correspondence.json`
and `...\exact_tier_ranking.json`.  The Claude repository at
`C:\halo-worktrees\claude-ai-conversation-2` has only the already integrated
dispose reconstruction in `src\halo\rasterizer\rasterizer_text.c` and a
declaration in `kb.json`; it has no definition for any current residual.

## Source and artifact census

All 1,577 registered worktrees contain the translation-unit source.  They
reduce to eight physical hashes caused by the known production generations,
line-ending state, and restored experiment worktrees.  Reachable Git history
contains only four production file blobs; none retains a current residual
definition.

A filesystem census found 3,654 path-matched COFF artifacts and 2,520 distinct
raw object hashes.  Exactly 1,103 paths are byte-identical copies of the
January target, SHA-256
`48452628b2525d642bacbb87d89e273e710734f1ca73a5be722e288b4aa4b795`.
After excluding those target oracles, only one distinct artifact emits any of
the five residual symbols:

`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\rasterizer-frame-statistics-static-count-20260826\build\first-shot\rasterizer_frame_statistics.obj`

It is the already documented nonexact static-count first shot, SHA-256
`086bf822e358868aa633e1cc76e76c82dd93d04063a972c13374acee74b8d757`.
No exact or unreviewed residual artifact exists in the registered corpus.

## Why the cross-build sources do not reopen the object

Fable classifies the four named residuals as `exact_unique`, meaning only that
their name, cdecl ABI, parameter count, and module correspondence are unique.
It does not assert code equivalence.  The measured version gap is decisive:

- HCEA `draw` is 876 bytes; January is 4,165 meaningful / 4,176 padded bytes.
  January owns the large debug-statistics overlay represented by the complete
  string cluster in the target COFF; the retail HCEA body omits that topology.
- HCEA static and dynamic count functions are unconditional 8-byte return-zero
  stubs.  January is 50 and 297 meaningful bytes respectively.
- HCEA `get_fps` is 588 bytes.  The already documented natural January subset
  compiled to 352 padded bytes with all 21 relocation identities, versus the
  384-byte target, but missed January's register and exit topology.
- `_code_0016e5c0` is January-only.  Direct, positive-guard, and inverse-guard
  unsigned-short comparator spellings have already been measured and rejected.

The 4,176-byte draw body also dominates object closure: reconstructing any
small residual cannot complete the object while its January-only debug report,
typed BSS/data layout, and call schedule remain unavailable.

## Claude/Fable handoff

Do not spend another spelling shot on `get_fps`, the private comparator, or
the prior static-count candidate.  The highest-leverage reopening task is to
recover an authentic October/January debug-build definition of
`rasterizer_frame_statistics_draw`, including the statistics-global and BSS
layouts that own every target relocation and every format-string consumer.
Start from the January split object at
`build\split\source\rasterizer\rasterizer_frame_statistics.obj`, use the HCEA
draw only as version-delta evidence, and first prove a complete ordered call,
string, data-owner, and field-offset map.  Reopen production C only if a source
blob or independently authenticated donor explains the entire 4,176-byte body;
an ABI/name correspondence or retail HCEA transcription is insufficient.

## Disposition

`NonMatching`, evidence-frozen pending an authentic January/debug donor or a
new complete typed layout.  The five inherited exact functions, all owned
sections, and production source remain unchanged.
