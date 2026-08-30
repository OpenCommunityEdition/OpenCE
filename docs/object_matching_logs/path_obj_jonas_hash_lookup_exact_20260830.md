# `path.obj`: typed hash-table lookup leaf (2026-08-30)

## Result

The first code-producing candidate for
`source/ai/path.obj:_path_node_from_hash_table` is strict exact.  The object
advances from 8/32 to 9/32 exact functions, from 155 to 230 meaningful exact
bytes, and from 224 to 304 padded exact bytes.  The object remains incomplete.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_path_node_from_hash_table` | 75 / 80 | 0 | `a575b235961a456820ee5fa995ae69feb21e321e08049b5cdd4fd166242c0e03` |

The hardened comparator reports `all_equal: true`.  All 80 padded bytes and
the empty ordered relocation schedule agree.  The eight inherited exact Path
functions also remain strict exact.

## Selection and provenance

The lane starts from canonical commit
`3e919da7640e5ad3a990a19b27052c5612ee97b8`.  The requested first screen,
`cache_files_decompress_windows.obj:_cache_copy_end`, was rejected before any
source edit or compile because it is not link-closed.  Its January section is
76 meaningful / 80 padded bytes, eight relocations, and normalized SHA-256
`68d77c49087aee2d6f6efb9c87e5631193c3d0f69ea2fd273ddc7403ea20cab2`.
After the public HCEA wait/signal body, January tests
`_decompressor_print_timing` and tailcalls unwritten same-TU owner
`_code_001aa680`; the public donor omits that tail.  No Cache source or object
was changed.

The fallback Path leaf is independently supported by HCEA source
`src/path_node_from_hash_table.c`, Git blob
`08bb16d97b1a8cd79514b020c9fe08c8385179a9`.  A census of all 1,538
registered worktrees found zero prior definitions.  Local branch history and
both applicable Path ledgers contain the public declaration and typed layout,
but no implementation, attempt, rejection, park, or frozen boundary for this
function.

January disassembly fixes the natural open-addressing topology: mask the
surface index to nine bits, scale it by eight, read a signed-short hash slot,
increment and wrap the 12-bit slot, and stop on `NONE` or a node whose named
`surface_index` matches.  The existing owner header already proves the
`path_state.hash_table` offset (`+0x1208A`), the `path_node` stride (`0x44`),
and the full `path_state` size (`0x1408C`).  The source-local assertion added
by this lane proves `path_node.surface_index` at `+0x08`.

The retained implementation uses only those named fields and the public
`short path_node_from_hash_table(struct path_state *, long)` declaration.
The masked-before-shift spelling keeps the arithmetic defined while expressing
the same bounded hash directly.  A `/Zs /W3` parse produced only the existing
`cseries.h` and `real_math.h` warnings.  Exactly one normal XDK 3911 VC7
code-producing compile was then performed; it was byte-exact on first
emission.  No retry, sweep, compiler-option change, or body variant was used.

## Reproducibility evidence

- January `path.obj` SHA-256:
  `1b989e4dc3bf3d9623cec07fa19164794044eb0bf359cf08480cf0ea8b3a709c`;
- untouched candidate `path.obj` SHA-256:
  `597225fb5036c440035abfc54206682e574f912e82b9cdc53b3020bf49236aa2`;
- exact retained candidate `path.obj` SHA-256:
  `fdac95296d81c1c0ccffb14eb518d9066f4d6e05147bbf452c33f77f21b4c770`;
- `path.c` SHA-256 before / after:
  `8e085b37d5b8107e1c001125d11ba55e941b4c8d064bdcbc7fb62903bf36b2b5` /
  `b37b6803fbe543cb1ecc6d2b29898caea13513ec0e432911344cf199b59f3a21`;
- unchanged `path.h` SHA-256:
  `9697853ff7c3fa1b9763a034b91d821a430e73758dbf5f42441cdc7e0f0b638d`.

The protected Units sentinel
`_unit_preprocess_node_orientations` remains strict exact at 1,920 padded
bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.

## Validation

- Direct complete Path comparison: 9/9 present functions strict exact;
  `all_equal: true`.
- Full `halobetacache_build`, `libcmt_build`, semantic report, and progress:
  pass.
- Progress: 609,703/2,198,102 meaningful code bytes and 4,801/11,060
  functions overall; Halo contributes 596,789/1,770,166 bytes and
  4,634/7,574 functions.
- Semantic audit: 470 units, 4,947 functions evaluated, 4,830 semantic exact,
  4,840 accepted exact, and zero unit errors or ordinary rejections.
- Object admission: zero candidates, contradictions, and revocations.
- Park validation: 12 active, zero stale, and zero invalid; no park added.
- Tooling suite: 212/212 tests pass.
- `git diff --check`, tracked-path review, and protected-source review: pass.

## Policy boundary

The retained source is readable typed C89 with one parameter per line and an
explicit value return.  It contains no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, barrier, raw address or offset access,
pointer/integer reconstruction, union or aliasing pun, undefined behavior,
synthetic anchor, byte forcing, object patch, comparator exception, semantic
allowlist, or parked credit.  No shared header, configuration, protected
source, or prior ledger is changed.  No push is performed.
