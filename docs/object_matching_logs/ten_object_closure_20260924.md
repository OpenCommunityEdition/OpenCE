# Ten verified object closures outside the active Claude lane

Baseline: `0d827851` on `jonas/exact-pilots`. The active Claude lane owns thirteen data/ownership contradictions; this batch changes none of those objects.

| Measure | Baseline | Verified batch | Change |
| --- | ---: | ---: | ---: |
| Conservatively complete objects | 449 / 833 | 459 / 833 | +10 |
| Halo complete objects | 339 / 468 | 341 / 468 | +2 |
| libcmt complete objects | 105 / 212 | 113 / 212 | +8 |
| Strict exact functions | 7,577 / 8,245 | 7,585 / 8,245 | +8, zero losses |
| All meaningful exact code | 1,614,503 | 1,615,221 | +718 |
| Halo meaningful exact code | 1,549,953 | 1,549,953 | 0 |
| Credited data | 2,561,415 | 2,564,059 | +2,644 |

The two Halo closures, `source/render/render_particles` and `source/bitmaps/bitmaps`, already had all their target functions strictly exact. The scorer undercounted otherwise byte-identical data sections. Pinned measurements in `config/semantic_data_matches.json` establish their respective 1,528-byte and 1,116-byte sections, including relocation destinations, flags and owner layout. The extra inline-provider copies have byte-identical selected providers and pass ordinary pair links. Neither closure claims new Halo code bytes.

The eight `libs/libcmt` closures are `ullshr`, `llmul`, `lldiv`, `strrchr`, `strncmp`, `ulldiv`, `ullrem` and `ulldvrm`. Their original XDK archive members identify MASM source units. Readable instruction-level reconstructions assemble to runtime sections strictly identical to both those members and January's split: 718 total bytes, zero relocations/data, the same `0x60500020` flags and a single correct external function owner each. Both candidate-first and archive-first link-only probes select all eight new providers. These are authenticated original-library assembly bytes, not recovered-C or Halo-owned code credit. The original archive and binary assets are not committed. See `libcmt_three_arithmetic_asm_20260923.md` and `libcmt_five_more_asm_20260923.md` for per-object hashes and provenance.

The regenerated full build passes. The stable strict diff gains exactly the eight CRT functions (718 bytes) with zero regressions. Parks remain 103 active, zero stale or invalid. The admission audit remains 17 candidates / 13 inherited contradictions / 9 rejections, with no new issue; those thirteen contradicted units are Claude's active scope. Tool tests pass: 1,154 passed, 5 skipped, 26 subtests. `git diff --check` is clean. No new source/header changes or point-from-line COMDAT rule changes occur in this batch.

Not counted: `source/effects/particles` has equal target function/data bytes but fails selected-provider links for four candidate-only helpers; `units`, `object_lights`, `game_engine_king`, and `path_structure_bsp` retain documented provider or data gaps. `_stricmp` and `_strnicm` MASM trials did not strictly match and were discarded. Status-only or fuzzy candidates receive no new code credit.

This local batch is below the standing 10,000-new-matched-code-byte push threshold. Keep it committed locally until a qualifying verified batch is ready or the user changes that instruction.
