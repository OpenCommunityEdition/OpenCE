# Bitmap small-family closeout freshness audit (2026-09-01)

## Scope and result

This isolated lane starts from canonical commit
`ecb25ee0cdaffe3bf8211a310b16edbfd066b366` and re-audits the two
smallest nearly complete bitmap-family objects:

1. `source/bitmaps/bitmaps_quantitize.obj`; and
2. `source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.obj`.

Both objects were already at their credible ordinary-C fixed points. No newer
original source, local-variable evidence, natural same-compiler donor, or
unintegrated exact candidate was found. The retained production sources are
therefore unchanged. Their two existing fuzzy parks remain active and valid;
neither park grants strict byte-match or whole-object completion credit.

| Object/function | Strict result | Padded bytes | Relocations | Park |
| --- | ---: | ---: | ---: | --- |
| `bitmaps_quantitize:_bitmap_quantitize_read_row` | exact | 32/32 | 0/0 | no |
| `bitmaps_quantitize:_bitmap_quantitize` | exact | 592/592 | 28/28 | no |
| `bitmaps_quantitize:_row_dither` | residual | 464/464 | 0/0 | instruction scheduling |
| `rasterizer_xbox_hardware_bitmaps` exact owners | 18/19 functions | 2,048/2,048 | exact per owner | no |
| `rasterizer_xbox_hardware_bitmaps:_rasterizer_bitmap_new` | residual | 400/400 | 26/26 | instruction scheduling |

The focused strict board remains `2/3` functions and `624/1,088` padded bytes
for quantization, and `18/19` functions and `2,048/2,448` padded bytes for
Xbox hardware bitmaps.

## Local-tree and history freshness census

The audit enumerated all 1,644 registered Git worktrees. Both source files
were readable in every tree, producing 3,288 source rows. After Git text
normalization these reduce to seven historical quantization identities and
five historical hardware-bitmap identities. The current source identities
are:

- `source/bitmaps/bitmaps_quantitize.c`:
  Git blob `29dda89fbf238ec94adaedfaef66857cdc1acebd`;
- `source/rasterizer/xbox/rasterizer_xbox_hardware_bitmaps.c`:
  Git blob `d1b7ae84f252730170c1eea111567be17d80c446`.

The wider workspace donor scan read 3,192 copies of the two exact filenames.
After CRLF/LF normalization those copies reduce to six quantization source
families and five hardware-bitmap source families. Every non-current family
is an already documented historical skeleton, naming cleanup, statement-order
probe, upload-cluster predecessor, or header-ownership predecessor. No new
implementation family exists.

Reachable history has no post-park quantization body. Its latest relevant
commit is `981e89a35574f544a74584a738aba2a21cbb1e0d`, which records the credible
`row_dither` park. The only later hardware-bitmap touch is
`f708e2e1837504210ec5daf8d618032b2d734fc5`, which moves the already exact
`bitmap_format_get_bits_per_pixel` declaration to `bitmaps.h`; it introduces
no alternate `rasterizer_bitmap_new` body.

The named closeout worktrees were inspected directly, including
`C:\tmp\bitmaps-quantitize-agent`, every registered
`bitmaps-quantitize*` tree, `root-hardware-bitmaps-20260830`,
`hardware-bitmaps-postcloseout-20260830`, and both historical
`work/lanes/hardware-bitmaps-*` trees. Their surviving sources and reports
reduce to the same measured families above.

## Donor and documentation audit

The complete ledgers were read before the source was touched:

- `docs/object_matching_logs/bitmaps_quantitize_obj.md`;
- `docs/object_matching_logs/bitmaps_quantitize_obj_jonas_credible_fuzzy_park_20260830.md`;
- `docs/object_matching_logs/rasterizer_transparent_geometry_bitmaps_quantitize_obj_jonas_closure_freshness_audit_20260830.md`;
- `C:\tmp\bitmaps-quantitize-agent\ROW_DITHER_PROVENANCE_REPORT.md`;
- `docs/object_matching_logs/rasterizer_xbox_hardware_bitmaps_obj_upload_helpers_blueprint_20260830.md`; and
- `docs/object_matching_logs/rasterizer_xbox_hardware_bitmaps_obj.md`.

The local Punpckhdq history still contains only an empty
`bitmaps_quantitize.c` symbol skeleton. Searches of the authenticated HCEA,
Stian, Pastudan, Demon, Nimbus, and Baboon caches found no independent
`row_dither` or `bitmap_quantitize` implementation.

For hardware bitmaps, the HCEA bitmap dispatcher, mipmap helpers, texture
upload code, and 16-bit cube-face table corroborate the retained semantics but
do not preserve January's `rasterizer_bitmap_new` source layout. Stian's
complete-history file contains the SDK wrapper/delete recovery but no create
routine donor. Those sources therefore support semantics and ownership only;
they do not reopen the final compiler-layout tie.

## Fixed-point evidence

### `row_dither`

Target and candidate each decode to 177 instructions in a 464-byte section.
They are identical except for three independent loop-maintenance operations:

```text
January: inc eax; add ebx,2; add ecx,2
current: add ebx,2; inc eax; add ecx,2
```

The documented reverse statement order emits
`inc eax; add ecx,2; add ebx,2`. Declaration, loop-tail, pointer, dependency,
barrier, pragma, and diagnostic assembly families have already been exhausted
and rejected. Repeating them would not be fresh evidence and would violate the
anti-fake-match boundary.

### `rasterizer_bitmap_new`

Target and candidate each decode to 150 instructions in a 400-byte section
and preserve all 26 relocation identities. They are instruction-identical
through `+0x159`. The only normalized difference is the branch destination:

```text
January +0x159: jne +0x171
current +0x159: jne +0x17e
```

Both destinations begin byte-identical `pop esi; mov al,bl; pop ebx; pop ebp;
ret` epilogues. More than 50 documented ordinary-C return, predicate, cleanup,
goto, and lifetime shapes failed to alter only this branch-threading choice.
The plain semantic source remains preferable to a code-generation control with
no source provenance.

## House-rule disposition

The retained implementations use semantic private/global names, project
types, typed bitmap fields, named enum values, project flag macros, explicit
returns, and natural XDK calls. They contain no address-derived identifier,
raw structure offset, volatile/register forcing, compiler barrier, pragma,
fake dependency, undefined behavior, byte patch, comparator exception,
nonsensical branch, or inline assembly. The January inline schedule is
unchanged and neither object emits a `point_from_line3d` COMDAT.

No production declaration or header was moved merely to perturb VC7. The
current owner-header cleanup for `bitmap_format_get_bits_per_pixel` is already
present and measurement-stable. Broader inherited interface cleanup belongs
to its defining translation units and must be attempted only with a
whole-includer regression manifest because VC7 is definition-position
sensitive.

## Verification

Fresh verification from the isolated worktree passes:

- full `ninja`: pass;
- semantic report: 472 units, 5,157 functions evaluated, zero unit errors;
- focused gates: quantization `2 exact / 1 residual / 0 unwritten`, hardware
  bitmaps `18 exact / 1 residual / 0 unwritten`;
- parked-function audit: 66 active, zero stale, zero invalid;
- protected `units.obj`: 189/189 strict exact;
- canonical strict board: 714,947/1,922,413 padded bytes,
  5,011/8,245 functions, 278/619 objects.

The remaining tooling suite and prohibited-source scans are recorded in the
admitting commit's verification output.

## Reopen criteria

Reopen either residual only for original Bungie source/local records, a
natural exact donor built by the same compiler and flags, or a genuinely new
lawful source-level compiler control outside the documented experiment
families. Do not reopen for a new permutation of already exhausted source
orders, inline assembly, fake aliasing, or machine-code transcription.
