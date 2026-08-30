# `rasterizer_xbox_decals.obj` Jonas decal-vertex delete leaf

## Result

This bounded lane starts from clean local commit
`4b451ed0edaefa19a98915da73fa762c2cca5766`. The first and only
code-producing production candidate is strict exact. The object advances from
6/29 to 7/29 exact functions, from 304 to 416 exact padded code bytes, and from
240 to 342 exact meaningful code bytes. It remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `__rasterizer_decal_vertices_delete` | 102 | 112 | 11 | `dc43a0b28fce663eb6b124cd81baab3272be3bbb254aaef2755ec150ee44d85b` |

The hardened comparator reports `all_equal: true` for every padded byte and
all ordered relocation identities. The eleven relocations are:

| Address | Type | Resolved destination |
| ---: | --- | --- |
| +20 | DIR32 | target source-path string |
| +25 | DIR32 | target `cache_index!=NONE` string |
| +30 | REL32 | `_display_assert` |
| +36 | REL32 | `_system_exit` |
| +44 | DIR32 | `_bss_0045e8e8` +20 |
| +60 | DIR32 | target source-path string |
| +65 | DIR32 | target `local_vertex_cache` string |
| +70 | REL32 | `_display_assert` |
| +77 | REL32 | `_system_exit` |
| +85 | DIR32 | `_bss_0045e8e8` +20 |
| +92 | REL32 | `_lruv_block_delete` |

## Prior-attempt and provenance census

- The January split object is authoritative. Its SHA-256 is
  `bcbbf5e61571861c22ed4503fae05f8760652ba2d53f87e7e5d112c4db23c892`.
- All three earlier Rasterizer Xbox Decals Jonas ledgers were read before the
  edit. None mentions or compiles this function.
- All 1,524 registered worktrees were scanned for a definition of
  `_rasterizer_decal_vertices_delete`; none contains one. A complete
  `git log --all -S` search reaches only the initial skeleton commit
  `e830efbd`. No ref-reachable prior candidate exists.
- HCEA's exact-name internal donor
  `reference/halocea/src/_rasterizer_decal_vertices_delete.c` calls
  `lruv_block_delete(local_vertex_cache, cache_index)`. Its public wrapper
  independently proves the ordinary one-argument cdecl boundary.
- The Stian reconstruction at
  `research-cache/stian-halo-full-history-20260828/src/halo/rasterizer/xbox/rasterizer_xbox_decals.c`
  independently preserves the same assertion pair and LRUV deletion call.
- January disassembly proves a full-width stack parameter in ESI, assertion
  lines `0x106` and `0x107`, the named cache owner at +20, ordinary cdecl
  pushes to `_lruv_block_delete`, and no hidden register live-in. The existing
  repository declarations already type that owner as `struct lruv_cache *`
  and the block index as `long`.

The retained source is therefore the single natural January-concordant shape:

```c
void _rasterizer_decal_vertices_delete(
	long cache_index)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_decals.c",
		0x106,
		cache_index!=NONE);
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_decals.c",
		0x107,
		local_vertex_cache);
	lruv_block_delete(local_vertex_cache, cache_index);

	return;
}
```

## One-shot and source-policy boundary

The baseline source Git blob was
`894db7547d8f8fb9d8c858754d9399f8e4efc50c`; the retained source blob is
`cb41a33e96d644f6d9b5cf195640417bb199b77c`. Before the production build, the
unchanged candidate passed VC7 `/Zs`; the existing object hash and timestamp
remained unchanged, proving that syntax check emitted no object. The next
invocation through the production Ninja edge emitted the only candidate
object, which matched immediately. There was no body, predicate, declaration,
placement, spelling, topology, or compiler-control retry.

Compilation used Microsoft 32-bit C/C++ Optimizing Compiler
`13.00.9254.1` with the repository's unchanged XDK 3911 flags. `CL.Exe` has
SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
The rebuilt object SHA-256 is
`f508f8afd40ba5dc99313fee3d1c066214857608593678eb711a7e5066e7ae29`.

The retained body is readable typed C89 with one parameter per line and an
explicit terminal `return;`. It contains no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, compiler barrier, raw address or offset access,
pointer/integer reconstruction, pointer or union punning, undefined behavior,
synthetic anchor, byte forcing, object patch, compiler/tool exception,
semantic allowlist, or parked entry. It defines no storage and changes no
header, configuration, compiler flag, symbol mapping, or protected source.

## Preserved exact siblings

Direct hardened comparison preserves all six inherited exact functions:

| Function | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rasterizer_decal_vertices_end_update` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `__rasterizer_decal_vertices_unlock` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `__rasterizer_decals_initialize_for_new_map` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `__rasterizer_decals_dispose_from_old_map` | 64 | 8 | `1dfd9d05fa181ebf6dbcd54fae5e89c8f43075470bd6ac18bc76317714258c52` |
| `_rasterizer_decal_vertices_begin_update` | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| `__rasterizer_decals_dispose` | 176 | 20 | `1326cad86d7849e2ef42d4f0e08fd073805be1d8dd57a42cd87c0a514efacdf6` |

## Validation and disposition

- Direct hardened comparison: the new function and all six inherited exact
  siblings pass complete padded-byte and resolved ordered-relocation identity.
- Whole-TU pre-edit regression manifest check: pass with no inherited loss.
- Object gate: 7 exact, 0 residual, 22 unwritten of 29 functions.
- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` targets: pass.
- Semantic audit: 470 units, 4,918 functions evaluated, 4,801 semantic exact,
  4,811 accepted exact, and zero unit errors or ordinary rejections.
- Strict campaign board: 4,783/8,246 exact functions and
  663,191/1,922,669 padded code bytes.
- Protected Units sentinel: 189/189 functions and 54,864/54,864 padded bytes
  strict exact.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 12 active, zero stale, and zero invalid; this lane adds no
  park.
- Tooling suite: 212/212 tests pass.
- `git diff --check`, changed-source policy scan, deleted-path scan, and
  retained-path review: pass.

The function is strict exact and retained. The object is not eligible for
`Matching` until its remaining 22 functions and target-owned data are closed.
