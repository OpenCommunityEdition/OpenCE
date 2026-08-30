# `glow.obj` constructor recovery (Jonas, 2026-08-29)

## Result

`_glow_new` is byte-exact and relocation-exact on the first frozen production
candidate:

- 240 padded code bytes;
- ten relocations;
- normalized SHA-256
  `66e5f0dc8e0024263c2bf6014d682230da039fcd397d9ae98efc211a3db05a5e`.

This bounded lane starts from canonical commit
`f968586d` (`Match glow delete`). `source/objects/widgets/glow` advances from
8/25 to 9/25 exact functions and from 720/7184 to 960/7184 strict padded code
bytes. The strict repository board advances to 4724/8246 functions and
654135/1922669 padded code bytes. No data credit is claimed.

## Source authority and reconstruction

The January split object is the byte, ABI, ownership, and relocation authority.
Its disassembly fixes the constructor's control flow: reject `NONE`, allocate a
glow datum, resolve the glow definition and referenced bitmap group, accept the
sprite-group case, resolve sequence zero and sprite zero, fetch the sprite's
bitmap, then store the definition index, particle count, and computed bitmap
dimension.

The independent HCEA donor at
`C:/Users/isabe/Documents/Codex/reference/halocea/src/glow_new.c` confirms that
behavior and the field meanings. Its associated `glow_datum`,
`glow_definition`, `bitmap_group`, sequence, sprite, and bitmap headers confirm
the typed layout. January additionally authenticates the nested tag-block
expression topology: the outer sprite lookup arguments are prepared before the
inner sequence lookup. The retained source expresses that topology directly
with nested, typed `TAG_BLOCK_GET_ELEMENT` calls.

Private typed layouts and compile-time checks establish the accessed fields:

- `glow_datum.definition_index` at `0x224`, `bitmap_dimension` at `0x228`,
  `number_of_particles` at `0x24C`, total size `0x25C`;
- `glow_definition.number_of_particles` at `0x20`, `texture` at `0x144`, total
  size `0x154`;
- `bitmap_group.sequences` at `0x54`;
- bitmap sprite size `0x20` and bitmap sequence size `0x40`.

The implementation is readable typed C89 with one parameter per line and an
explicit terminal return. It contains no assembly, intrinsic, pragma,
attribute, raw address or offset access, pointer/integer reconstruction, cast
or union pun, undefined behavior, volatile/register scheduling device,
optimizer barrier, synthetic anchor, byte forcing, object patch, or compiler
exception.

## Exact comparison

`python tools/coff_compare.py build/split/source/objects/widgets/glow.obj
build/base/source/objects/widgets/glow.obj _glow_new` reports
`all_equal: true`. The complete relocation schedule agrees with January:

| Address | Type | Identity |
| ---: | ---: | --- |
| `+0x17` | DIR32 | `_glow_globals+0` |
| `+0x1D` | REL32 | `_datum_new` |
| `+0x34` | DIR32 | `_glow_globals+0` |
| `+0x3B` | REL32 | `_datum_get` |
| `+0x48` | REL32 | `_tag_get` |
| `+0x5B` | REL32 | `_tag_get` |
| `+0x75` | REL32 | `_tag_block_get_element_with_size` |
| `+0x81` | REL32 | `_tag_block_get_element_with_size` |
| `+0x97` | REL32 | `_bitmap_group_get_bitmap_from_sequence` |
| `+0xC3` | REL32 | `__ftol2` |

The inherited `_glow_delete` remains strict-exact at 80 padded bytes, six
relocations, and normalized SHA-256
`cbe403b6bc67b7d867bee2c3dd8b6429d7e8eedaf0d2868f4089b57e350b202b`.
The board also confirms all seven earlier exact Glow owners remain exact.

## Provenance and one-shot boundary

- January split `glow.obj` SHA-256:
  `593db0e7fde065d7426231e9e08aaed6360b790033077481e1d70044e54a638a`.
- Candidate `glow.obj` container SHA-256 before committed replay:
  `662e7d63763a2754c44b5f22457ca71a25b82c0c36e495c3716b1bf7dd3e3371`.
- Clean committed-state replay `glow.obj` container SHA-256:
  `fcc4abc1bde9ec30b5e4034e5a936d5537b6c8ab6d194cf63a384a032295878a`.
  The expected container-hash difference is the VC7 COFF timestamp; normalized
  code, padding, relocation identity, and ownership remain exact.
- Retained `glow.c` Git blob before commit:
  `ce371a7eca69ff88124b32afd669e40ca985d83d`.
- Full report SHA-256:
  `df7831ce83ff0cbc524ba610782a5e3fd502f4feda4d4451f5be4ecfdb91e3a1`.
- Semantic report SHA-256:
  `c5763770e8dd726661be2800ff9ba224c4de7a37648b690075d343284bb057a2`.

Exactly one new object-producing production candidate covered `_glow_new`. It
passed the strict comparator immediately. There was no spelling sweep,
body-shape matrix, compiler-control experiment, or post-miss tuning. Subsequent
builds are validation replays of the unchanged retained source.

## Repository gates

- Full `halobetacache_build`, `libcmt_build`, report, semantic report, and
  progress build: pass.
- Semantic report: 470 units, 4869 evaluated functions, 4742 semantic-exact,
  129 hidden-exact / 76388 hidden code bytes, 4752 accepted-exact, and zero
  unit errors or ordinary rejections.
- Campaign progress: 382/833 files matched, 4713/11060 functions and
  597626/2198102 meaningful code bytes; Halo is 280/468 files, 4546/7574
  functions, and 584712/1770166 meaningful code bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked-function audit: 12 active, zero stale, and zero invalid.
- Pytest: 205 passed.
- `git diff --check`: pass.

After the source and ledger were committed, Ninja's normal clean operation
removed only `build/base/source/objects/widgets/glow.obj`. A dry run reported
exactly one `CL` edge, and the normal same-path rebuild reproduced `_glow_new`
and `_glow_delete` as strict-exact. The post-rebuild object board remained
9/25 and 960/7184 for Glow, and the worktree returned clean.

Only `source/objects/widgets/glow.c` and this additive Jonas ledger change. No
header, configuration, exception, parked record, completion label, existing
ledger, protected path, or tracked-path deletion is involved.
