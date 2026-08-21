# `leaf_map.obj` Jonas delete-leaf wave

## Result

This bounded source-only wave starts from canonical commit
`8e3d82ed442155edb9e6f5221661f2bde4d52135`. The one requested body was
emitted in one natural code-producing compile and is independently strict.
`leaf_map.obj` advances from 0/25 to 1/25 exact functions and from 0/6,582 to
35/6,582 meaningful code bytes. It remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_leaf_map_delete` | 35 | 48 | 2 | `1a3a1d1eed2871eaf03fd6cd0f1dfbe78da3c1c6e60d263fca2d9a266ef8b44f` |

The hardened COFF comparator proves identical padded bytes and identical
relocation addresses, types, destinations, and addends. Both relocations are
REL32 calls to `_tag_block_resize`, at +0x0E and +0x19. The other 24 target
functions and all target non-code bytes stay outside this wave's credit.

## Provenance and layout

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `leaf_map.obj` SHA-256:
  `03642b1a1367d576979c8c0fdae4208bf2ecfaa0b58f785c508817f8a989ba02`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its
  `src/leaf_map_delete.c` body blob is
  `797da0eb82af6cb867ff4d8b3aff5d363ebeb629`; its leaf-map layout blobs are
  `f2ca0cdaeb2ed6264cb6d752eb6c29dfd9d8ee0d` and
  `917eae0c40137673e1889d041e6a1129926bfba7`.
- January PDB labels `_leaf_map_delete` at RVA `0x1813F0`. January COFF reads
  one ordinary pointer argument, addresses the tag blocks at +0x04 and +0x10,
  and calls `tag_block_resize` with zero for each block in that order.
- The repository's existing `struct leaf_map` independently gives those
  natural offsets: the BSP pointer at +0x00, `leaves` at +0x04, and `portals`
  at +0x10. No layout or shared declaration changed.

## Defined-C and one-shot boundary

The retained body is the direct typed operation:

```c
void leaf_map_delete(
	struct leaf_map *leaf_map)
{
	tag_block_resize(&leaf_map->leaves, 0);
	tag_block_resize(&leaf_map->portals, 0);
	return;
}
```

The first compiler invocation stopped during prerequisite-header parsing
before the candidate function was parsed or any object was emitted because
the previously empty translation unit did not include the canonical scalar
type owner. Adding the ordinary `cseries.h` prerequisite changed no candidate
expression or declaration. The next invocation was the sole code-producing
candidate compile and was strict immediately. There were no rejected bodies,
spelling retries, declaration retries, control-flow experiments, or
compiler-control devices.

The source is readable typed C with an explicit terminal return. It contains
no assembly, volatile scheduling device, force-inline annotation, optimizer
pragma/barrier, raw address or byte-offset dereference, pointer/integer
reconstruction, pointer or union pun, inactive-union access, undefined
overflow, synthetic anchor, or object-byte forcing. The wave defines no
storage and claims no data credit.

## Scope and validation

Only `source/structures/leaf_map.c` and this new Jonas-owned ledger are
changed. The final source Git blob is
`182a6c5a6e8518aaf675b2e6bb134346bc6112a6`; its committed payload SHA-256 is
`a6d89af27e5693ce77fb76c856bd4442591deadd4fc2e76d135dc1d5d324a1d8`.
No header, configuration, frozen-five source, semantic exception, parked
record, pre-existing Markdown, Claude-owned file, or deletion is in scope.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: the new leaf passes.
- Semantic audit: 470 units, 4,067 functions evaluated, 3,927 semantic exact,
  3,988 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,975/11,060 exact functions,
  476,460/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Tooling suite: 179/179 tests pass; `git diff --check` is clean.
- The pre-wave regression gate reports exactly `_leaf_map_delete` as
  `NEWLY_EXACT`, `changed_nonexact: []`, and the expected symbol-set finding
  solely for the new external function owner.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `leaf_map.obj` is the final same-path reproducibility proof. No
push is performed.
