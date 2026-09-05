# `leaf_map.obj` Opus private-helper emission audit (2026-09-04)

## Result

Two old Opus bodies are credible, byte-exact private helper reconstructions,
but they are **not independently admissible** on the current canonical source.
Both helpers' real same-translation-unit callers are still unwritten, so VC7
correctly discards the unused static definitions. Retaining them alone would
add source without emitting either January owner or earning any exact credit.
Changing linkage or adding a dormant reference merely to emit them would be
fake matching and was not attempted.

| January target | Proposed semantic name | Size | Relocs | Normalized SHA-256 | Current disposition |
| --- | --- | ---: | ---: | --- | --- |
| `_code_001814c0` | `leaf_face_index_from_node` | 64 | 1 | `35fb97c5039e2c3249d786e46a4f47ca0cafb97618606d5bf7215482ba05729e` | Preserve for its real portal-building caller cluster. |
| `_code_00182400` | `normalize_three_dee` | 128 | 3 | `33f9c068af4dd7fcba5d26455f5db493b3a7cd1dd82f322ae7edf159a672370b` | Preserve for the real `project_plane3d` caller cluster. |

No production C/header/config/park file changed. The natural bodies are
preserved as an in-memory gate edit at
`scratch/leaf-map-two-helper-edits-20260904.json`.

## Provenance and source credibility

The frozen read-only donor is
`C:\halo-worktrees\opus-small-families-30k-20260902` at
`eeedd72e5ba0fc1761519f68cad241fe606320e7`. Its complete candidate object is
`scratch/w6_leaf_map.obj`, SHA-256
`C997026A4DBA1ABBA0B1EA89A3DEA4CBC1EAC10D40494041C07A3F5B28B34A88`.
Direct target/candidate inspection confirms the two table rows above,
including relocation identities:

- `leaf_face_index_from_node` has one REL32 call to
  `tag_block_get_element_with_size` at `+0x15`;
- `normalize_three_dee` has the `1.0f` data relocation at `+0x2B`, the REL32
  call to `valid_real_normal3d` at `+0x5C`, and the `0.0f` data relocation at
  `+0x72`.

The first body performs an ordinary typed linear search over
`map_leaf::faces`, returns the matching signed-short index, and returns
`NONE` on failure. January's function accepts three arguments and never reads
the first `leaf_map` argument. Its recovered callers nevertheless pass that
argument, so the unused parameter is authenticated ABI/evolution debt rather
than padding source. HCEA independently reconstructs the same operation and
three-argument boundary as `map_leaf_find_face_on_node`; therefore
`leaf_face_index_from_node` is a convention-consistent descriptive private
name, not claimed PDB-authenticated spelling.

The second body computes a vector's length, rejects an invalid reciprocal,
normalizes all three named components, validates the resulting normal, and
returns either the original length or `0.0f`. HCEA independently names and
reconstructs `normalize_three_dee` in
`_research_cache/halocea/src/blam/structures/normalize_three_dee.c` at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` (blob
`509f7cd31c6ce121640cbfb60c6aa1320c82c73e`). January remains the byte and ABI
authority.

Both bodies use project `real`, `real_vector3d`, `NONE`, named fields,
`TAG_BLOCK_GET_ELEMENT`, `square_root`, `valid_real`, and
`valid_real_normal3d`. Every value path returns explicitly. They introduce no
raw address, offset-derived identifier, forced inline/noinline annotation,
volatile/register coercion, barrier, pragma, assembly, representation cast,
dummy dependency, or nonsensical branch.

## Authentic emission topology

The source and ownership evidence supports private linkage, and the preserved
donor compiler emits both helpers with COFF storage class 3. The current
generated target placeholders are still storage class 2 because their
anonymous `config/symbols.json` entries have no reviewed `static: true`
metadata. That generated placeholder linkage is not treated as proof of the
original source linkage. Root should apply the semantic names and reviewed
static metadata atomically before a future caller-cluster gate or admission.

Current canonical has none of the helpers' genuine callers:

- `_code_001818a0` (the 832-byte
  `leaf_map_build_portal_from_leaves`) has two January relocations to
  `_code_001814c0`, at `+0x43` and `+0x4F`;
- `_code_001825f0` (the 480-byte
  `leaf_map_build_portals_from_leaf`) contains one additional inlined copy of
  the face lookup rather than a third relocation; and
- `_code_00182480` (the 368-byte `project_plane3d`) calls
  `_code_00182400` at `+0x104`.

Accordingly, the narrow current-canonical replay emits neither static helper.
This is the correct compiler result. The safe continuation is to reconstruct
and review the real caller clusters: face lookup with the portal-builder
family, and normalization with `project_plane3d`. The latter caller is already
known to remain a natural 368-byte residual around the inherited
`projection_from_vector3d` header behavior; it may still legitimately emit and
credit the exact 128-byte helper when audited as one topology. Do not export
either helper or invent an anchor.

## Gate and complete-owner checks

Baseline command:

```text
python tools/campaign/gate.py source/structures/leaf_map --all --forbid-emitted-symbol _point_from_line3d --out scratch/leaf-map-two-helper-before-20260904.obj
```

Natural replay command:

```text
python tools/campaign/gate.py source/structures/leaf_map --edits scratch/leaf-map-two-helper-edits-20260904.json --alias leaf_face_index_from_node=code_001814c0 --alias normalize_three_dee=code_00182400 --all --forbid-emitted-symbol _point_from_line3d --out scratch/leaf-map-two-helper-natural-20260904.obj
```

Both runs report **8 exact / 3 residual / 14 unwritten** and an explicit
emitted-symbol guard pass. A complete `tinfo.py` inventory comparison reports
no difference between their 21 emitted code owners: all eight inherited exact
functions and all three existing residuals retain their hashes, and neither
new helper is present. The scratch artifact hashes are:

- baseline object:
  `672FDB0922B7D5EA8E904B1349289241097E9049C30CA254637AA9410AFAAEF7`;
- natural two-helper object:
  `65F17CE5CEEB54CD6537BDD261B3C6BAECCC376C4C1ED9D87E26E0C5CC444FF6`;
- in-memory edit manifest:
  `C8A78B5F0A54B1EA47F05E067DE39C99F049C33171D47E3D6305061FD496B6E5`.

The complete donor object does contain a forbidden 48-byte
`_point_from_line3d` definition, normalized SHA-256
`9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`.
That confirms why the donor translation unit must never be transplanted
wholesale. The selective current-canonical replay emits no such owner and does
not waive the January inline-schedule rule.
