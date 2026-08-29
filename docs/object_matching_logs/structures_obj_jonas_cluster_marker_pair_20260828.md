# `structures.obj` Jonas cluster-marker pair (2026-08-28)

## Result

This bounded natural-C wave starts from pushed integration commit
`770f1c711344a4c0596678a9cddcc4c0a19c3f7e`. A frozen four-function batch
tested the public cluster-marker family once. The strict first gate retained
`_structure_cluster_marker_begin` and `_structure_cluster_marker_end`; the two
same-size residuals were immediately pruned without spelling, declaration,
control-flow, scheduling, or compiler-control retries.

`source/structures/structures.obj` advances from 4/17 to 6/17 exact functions
and remains `NonMatching`. The retained pair gains 104 meaningful and 128
padded code bytes, thirteen exact relocations, and no data credit.

| Retained owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_structure_cluster_marker_begin` | 55 | 64 | 7 | `17cec80ee833c5436c02106d4f25f452bdb57adee28fb4a2699317bce63fb0a9` |
| `_structure_cluster_marker_end` | 49 | 64 | 6 | `240d3c23649278abc41f449a37c057cdb3b40bf3cc3253ba0a3ab4d52fbfda7b` |
| **Wave gain** | **104** | **128** | **13** | |

The hardened COFF comparator proves equal padded bytes and exact relocation
address, type, destination, and addend for both retained owners.

## Provenance and ownership

The primary natural-C oracle is the clean HCEA history at commit
`c168af2e747d3095d9a29418ae401f3a39544863`. The relevant donor files and Git
blobs are:

- `src/structure_cluster_marker_begin.c`,
  `ee34c0faa49f771569152d5567ff9c6a1921a9cc`;
- `src/structure_cluster_marker_end.c`,
  `b831fead201252678c416cb352276cdda388dffe`;
- `src/structure_cluster_unmarked.c`,
  `e3d1c4c57541ebfc8ce899a6498962ded04b8d78`;
- `src/structure_cluster_mark.c`,
  `13fd3c99f5ebffbffbf94f73e2a5c0cc82e2aa84`;
- `headers/structure_globals.h`,
  `fe244a73205f2d5a1f9e9bc34b599e31a4cb76a4`.

The preserved complete source in the Pastudan history at commit
`918af885935ec470a31256ecce9a977b12b01f80` independently attests the family;
its `src/halo/structures/structures.c` Git blob is
`09b94a003c47c4339695dbe2621e30b1166862c7`. January disassembly and relocation
ownership were then used as the target authority for all four public owners.

Before editing, a read-only census of all 1,469 worktrees found no dirty
`source/structures/structures.c` except this isolated Jonas lane. The census
included the Claude worktrees and their documentation. Units, Vehicles, and
all Claude-active paths remain untouched.

## Typed state model

The donor and January accesses prove one natural runtime owner:
`struct structure_runtime_globals bss_004c1100`. Its independently checked
layout is:

| Offset | Field | Type |
| ---: | --- | --- |
| `0x000` | `initialized` | `boolean` |
| `0x001` | `cluster_marker_initialized` | `boolean` |
| `0x002` | reserved alignment | `byte[2]` |
| `0x004` | `cluster_marker` | `long` |
| `0x008` | `cluster_magic_numbers` | `long[MAXIMUM_CLUSTERS_PER_STRUCTURE]` |
| `0x808` | end of object | |

Compile-time `offsetof` and `sizeof` checks own these offsets and the `0x808`
extent. The implementation reaches the BSS object through the typed
`structure_globals` alias; it introduces no raw address or byte-offset access.

## Frozen batch and no-retry disposition

The single permitted candidate compile contained:

- `_structure_cluster_marker_begin` — strict on the first gate and retained;
- `_structure_cluster_marker_end` — strict on the first gate and retained;
- `_structure_cluster_unmarked` — 128 target/candidate padded bytes and eleven
  target/candidate relocations, but normalized bytes differed; pruned;
- `_structure_cluster_mark` — 128 target/candidate padded bytes and twelve
  target/candidate relocations, but normalized bytes differed; pruned.

The target normalized SHA-256 values for the rejected owners are
`98b5502fb6c8cb58f48c07e07ae161eb7434b246a299c669a8eb61d33efbed6a`
for `_structure_cluster_unmarked` and
`c426fbafcf634a63373bab944083c26aaebda5a181121a993ed0e2c0e644921a`
for `_structure_cluster_mark`. Their candidate hashes were not retained before
the mandated prune, so no unsupported fingerprint is claimed. Both owners are
documented no-retry residuals for this topology.

## Fingerprints and validation

- Baseline source Git blob:
  `6df8338b6db46ce5653aac68150a280bb4601220`.
- Retained source Git blob:
  `02bd525a880e1fecc82fb03a6f94ff7b25afcf13`.
- January split `structures.obj` SHA-256:
  `0e0875524b3580c272bb51b1d5630540d58ce9e1e1ae6c3ef52ca5f30669a4d7`.
- Retained candidate `structures.obj` SHA-256:
  `ed0d97ecb9ba77b661bd4adc3bc7e279c0be075ff3f896202166b68726c62516`.
- `tools/campaign/gate.py source/structures/structures --all`: 6 exact,
  zero residual, 11 unwritten.
- Direct retained-pair comparison: both owners strict; thirteen relocation
  records exact.
- Full `halobetacache_build`, `libcmt_build`, semantic report, and progress
  build: pass across 572 steps.
- Semantic report: 470 units, 4,684 functions evaluated, 4,555 semantic exact,
  4,565 accepted exact, and zero unit errors.

Only `source/structures/structures.c` and this Jonas-owned ledger are changed.
No header, configuration, semantic exception, parked record, completion label,
or other object is edited. The retained source is ordinary C89-compatible
typed C and contains no assembly, `volatile`, `register`, pragma, intrinsic,
optimizer barrier, undefined behavior, raw address, byte-offset access,
representation pun, private-ABI body, artificial anchor, or byte/code-generation
forcing.

No commit or push is performed in this isolated lane.
