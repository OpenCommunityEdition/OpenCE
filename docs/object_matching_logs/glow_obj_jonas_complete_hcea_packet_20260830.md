# `glow.obj` complete HCEA packet boundary (Jonas, 2026-08-30)

## Result

The authenticated 16-owner HCEA packet recovered one new strict owner:
`_glow_render`.

- 192 padded code bytes;
- seven relocations;
- normalized SHA-256
  `04c4f7888c729acf94203b6afbd762c1564e13b54b7832a336d20dd2cea640b7`.

`source/objects/widgets/glow` advances from 9/25 to 10/25 exact functions
and from 960/7184 to 1152/7184 strict padded code bytes. All nine inherited
exact owners remain strict-exact. The other 15 packet bodies were rejected and
removed; no non-exact production body, support layout, or prototype remains.

## Authority and provenance

The January split object remains the byte, section, relocation, ABI, and
ownership authority. Its container SHA-256 is
`593db0e7fde065d7426231e9e08aaed6360b790033077481e1d70044e54a638a`.

The independent semantic/source-shape oracle is the clean HCEA checkout at
commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`, remote
`https://github.com/surreptitiousresearch/halocea.git`. The complete packet is
under:

`research-cache/halocea-cseries-20260820/src/blam/objects/widgets/`

It includes every `glow*.c` body plus `get_particle_world_position.c` and the
associated typed headers. The exact `glow_render.c` donor has SHA-256
`5b765faacf0b40830d5385d2432943f6c21a42fa005bdea110d33483e0c541eb`.

The retained render body is a direct, natural typed-C transcription of that
donor, reconciled against January's disassembly and relocation schedule. It
looks up the glow datum and definition, begins an additive sprite batch,
traverses the typed particle list, submits one sprite per particle, and ends
the batch. The private layouts already present in the TU provide the exact
named fields and offsets; only the render declarations and body were added.

## One-shot packet measurements

Exactly one object-producing production candidate represented the entire
authenticated packet. The table records January's strict owner measurements
and the atomic result. A rejection means section-aware strict comparison
failed; the body was restored to unwritten state immediately.

| Owner | HCEA role | Target bytes | Target relocs | Result |
| --- | --- | ---: | ---: | --- |
| `_code_00122940` | trailing-particle color update | 128 | 6 | rejected |
| `_code_001229c0` | trailing-particle size update | 96 | 4 | rejected |
| `_code_00122a20` | trailing-particle velocity update | 144 | 4 | rejected |
| `_code_00122ab0` | trailing-particle position update | 64 | 1 | rejected |
| `_code_00122af0` | trailing-particle aging/unlink | 96 | 3 | rejected |
| `_code_00122b50` | normal-particle color update | 416 | 8 | rejected |
| `_code_00122cf0` | normal-particle size copy | 16 | 0 | rejected |
| `_code_00122d00` | particle allocation | 64 | 4 | rejected |
| `_glow_render` | additive sprite submission | 192 | 7 | **exact** |
| `_code_00123010` | normal-particle creation | 480 | 18 | rejected |
| `_code_001231f0` | particle world-position spline | 1744 | 43 | rejected |
| `_code_001238c0` | normal-particle position wrapper | 560 | 15 | rejected |
| `_code_00123af0` | normal-particle initialization | 176 | 2 | rejected |
| `_code_00123ba0` | trailing-particle creation | 608 | 26 | rejected |
| `_code_00123e00` | glow update | 1328 | 33 | rejected |
| `_glow_submit` | render submission wrapper | 112 | 8 | rejected |

The HCEA packet is a later-build semantic oracle, not a byte oracle. For the
large spline owner, the HCEA generic-marker path also relies on uninitialized
control state and point/vector aliasing. The candidate used explicit typed
component copies and initialized control data instead, preserving defined C;
it still rejected. No raw address/offset access, cast or union pun, undefined
behavior, assembly, intrinsic, pragma, attribute, volatile/register lever,
barrier, anchor, or byte-forcing mechanism was admitted.

## Exact comparison

`tools/coff_compare.py` reports `all_equal: true` for `_glow_render` and all
nine inherited exact owners. The render relocation schedule agrees completely:

| Address | Type | Identity |
| ---: | ---: | --- |
| `+0x0E` | DIR32 | `_glow_globals+0` |
| `+0x17` | REL32 | `_datum_get` |
| `+0x2A` | REL32 | `_tag_get` |
| `+0x40` | DIR32 | `_global_shader_effect_additive` |
| `+0x4E` | REL32 | `_build_sprites_begin` |
| `+0x8E` | REL32 | `_build_sprite` |
| `+0xA4` | REL32 | `_build_sprites_end` |

The retained rebuilt `glow.obj` container SHA-256 is
`099d69b5007475853d0fa816c2f0b8bf037f7371d5d3f22b8e85fb3b124d3fae`;
the source blob before commit is
`568d537d3ae36d1b168f1628398168f582a6e83c`.

The baseline regression manifest reports `_glow_render` as the sole newly
exact owner, `changed_nonexact: []`, and every inherited exact owner in
`still_exact`. Its symbol-set warning is the expected consequence of adding
the newly recovered owner, not an ownership regression.

## Durable residual boundary

The remaining 15 owners should not receive another direct HCEA transcription,
minor declaration shuffle, or replay of this packet. Further work requires
fresh January-specific evidence: an authenticated January-adjacent source
artifact, PDB/local-variable record, compiler provenance result, or a donor
whose emitted topology explains a measured target boundary. In particular,
the 1744-byte world-position spline and 1328-byte update owner should be handed
to Claude/Fable as provenance and January-control-flow research problems, not
as spelling searches.

## Repository gates

- Complete `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass.
- Semantic report: 470 units, 4958 evaluated functions, 4842 semantic-exact,
  131 hidden-exact / 78940 hidden code bytes, 4852 accepted-exact, and zero
  unit errors or ordinary rejections.
- Strict progress: 384/833 files, 4813/11060 functions, and
  610823/2198102 meaningful code bytes; Halo is 282/468 files,
  4646/7574 functions, and 597909/1770166 meaningful code bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked-function audit: 12 active, zero stale, and zero invalid.
- Tooling tests: 212/212 pass with the cache provider disabled.
- Units sentinel `_unit_preprocess_node_orientations`: strict-exact at 1920
  padded bytes, 87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- `git diff --check`: pass.

