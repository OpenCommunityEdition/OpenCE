# `rasterizer_xbox_decals.obj` Jonas old-map disposal leaf

## Result

This bounded source-only wave starts from exact canonical commit
`fd4cb20f52fd9df0b66d07c0dee225475a40c5e3`. The sole requested body was
emitted in the first and only code-producing candidate compile and is
independently strict. `rasterizer_xbox_decals.obj` advances from 3/29 to 4/29
exact functions and from 3/4,333 to 66/4,333 meaningful code bytes. It
remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `__rasterizer_decals_dispose_from_old_map` | 63 | 64 | 8 | `1dfd9d05fa181ebf6dbcd54fae5e89c8f43075470bd6ac18bc76317714258c52` |

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, destinations, and addends:

| Address | Type | Destination | Addend |
| ---: | --- | --- | ---: |
| +1 | DIR32 | `_bss_0045e8e8` | 20 |
| +17 | DIR32 | target source-path string | 0 |
| +22 | DIR32 | target `local_vertex_cache` string | 0 |
| +27 | REL32 | `_display_assert` | 0 |
| +34 | REL32 | `_system_exit` | 0 |
| +44 | REL32 | `_decals_unlock` | 0 |
| +49 | DIR32 | `_bss_0045e8e8` | 20 |
| +55 | REL32 | `_lruv_flush` | 0 |

The other 25 target functions and all 819 target non-code bytes stay outside
this wave's credit.

## Provenance, line, and ABI

- Baseline `source/rasterizer/xbox/rasterizer_xbox_decals.c` blob:
  `0f7cd2588b8465ddf41d149eebe6fa6423ea78cb`.
- Retained source Git blob / SHA-256 of its committed Git-blob payload:
  `e5f8f853ce689f0f8c3870b08c7aeff1c421b3d1` /
  `149746f5520d42ab0fbbb9baa8c3c7c0ee8009b45dfb9068ea6e7db036ab1e3c`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `rasterizer_xbox_decals.obj` SHA-256:
  `bcbbf5e61571861c22ed4503fae05f8760652ba2d53f87e7e5d112c4db23c892`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its tree-reachable blobs are:
  exact-name internal body `93b05500bfc674d3aa89f513b19631678de3e824`,
  ordinary public wrapper `f6be9c9e080dc3b2372cd4491b541ddb51e8ca21`,
  typed `local_vertex_cache` owner `97920b1882a5441ead41df90262dc2b1b817816f`,
  `lruv_cache` declaration `e353d9d3dd83e11db462e5fd0bb3f817bf43fd92`,
  and typed `decals_unlock` body `d6f685991c316446b01795a8beefb0404230b12d`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

HCEA authenticates the leading-underscore internal name, `void(void)`
signature, permanent unlock, typed LRU flush, and typed cache owner. Its
public wrapper also calls the internal function with no arguments. January's
public PC wrapper is a five-byte tail jump to the internal COFF symbol with no
register setup or live-in contract, independently proving the ordinary
no-argument cdecl boundary despite the internal name.

January's internal disassembly pushes `0x83` for the assertion line and owns
the exact source-path and expression strings, so line 131 is target evidence,
not a physical-line guess. The retained function is placed naturally directly
after `_rasterizer_decals_initialize_for_new_map`, its target-order
predecessor. No blank lines, comments, declarations, or inactive source were
added to manipulate physical source line numbers.

```c
struct rasterizer_decals_globals_prefix
{
	byte reserved0000[0x14];
	struct lruv_cache *local_vertex_cache;
};

extern struct rasterizer_decals_globals_prefix bss_0045e8e8;

#define local_vertex_cache bss_0045e8e8.local_vertex_cache

void _rasterizer_decals_dispose_from_old_map(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_decals.c",
		131,
		local_vertex_cache);
	decals_unlock(TRUE);
	lruv_flush(local_vertex_cache);

	return;
}
```

No shared declaration, storage definition, or target layout changed.

## Defined-C and one-shot boundary

The first compiler invocation stopped in parsing because the semantic alias
macro was initially written before, and therefore expanded inside, the typed
structure field declaration. It emitted no OBJ. Moving the unchanged alias
below the unchanged typed external was a declaration-order-only parser
correction made before any code-producing candidate; the requested body,
function signature, assertion metadata, and codegen-relevant expressions did
not change. The next invocation produced the first and only candidate object,
and the leaf hit immediately. There was no source-shape retry, line-placement
retry, spelling retry, control-flow experiment, or tuning pass. Later
aggregate validation and the committed forced replay use the unchanged
retained source and are reproducibility checks, not new candidates.

The retained body is readable typed C with `void` on its own parameter line
and an explicit terminal return. It contains no assembly, volatile scheduling
device, force-inline annotation, optimizer pragma/barrier, raw-address
dereference, pointer/integer reconstruction, pointer or union pun,
inactive-union access, undefined overflow, synthetic anchor, line padding, or
object-byte forcing. The typed external prefix defines no storage. The two
target-owned assertion literal COMDATs are the natural dependencies of the
strict function body; they receive zero standalone data credit. The candidate
defines no `.data`, `.bss`, COMMON symbol, or other writable runtime storage,
and campaign matched data remains unchanged.

## Preserved exact bodies

Direct hardened comparison records all four accepted bodies independently:

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rasterizer_decal_vertices_end_update` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `__rasterizer_decal_vertices_unlock` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `__rasterizer_decals_initialize_for_new_map` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `__rasterizer_decals_dispose_from_old_map` | 63 | 64 | 8 | `1dfd9d05fa181ebf6dbcd54fae5e89c8f43075470bd6ac18bc76317714258c52` |
| **Final strict total** | **66** | **112** | **8** | |

The source edit does not alter any pre-existing exact body.

## Scope and validation

Only `source/rasterizer/xbox/rasterizer_xbox_decals.c` and this new
Jonas-owned ledger are changed. No header, storage definition, configuration,
data file, semantic exception, parked record, protected-five source,
pre-existing Markdown, Claude-owned file, or deletion is in scope.

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: the new leaf and all three inherited exact
  no-ops pass with exact padded bytes and relocation identities.
- Semantic audit: 470 units, 4,100 functions evaluated, 3,960 semantic exact,
  101 hidden exact, 36 ordinary-only, 4,021 accepted exact, and zero unit
  errors.
- Campaign progress: 375/833 complete objects, 4,008/11,060 exact functions,
  478,663/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- `rasterizer_xbox_decals.obj`: 4/29 exact functions and 66/4,333 meaningful
  code bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Park validation: three active, zero stale, and zero invalid records.
- Complete tooling suite: 179/179 tests pass.
- A clean committed regression snapshot/check and forced object deletion,
  rebuild, regression replay, and direct comparison preserve all four exact
  bodies, the eight new relocation identities, and the zero-data-credit
  boundary.

The five protected large objects (`vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`) are untouched. No tracked file is deleted, and no push
or history rewrite is performed.
