# `rasterizer_xbox_decals.obj` Jonas flush leaf

## Result

This bounded lane starts from clean commit
`cbfaa281303f14b2749df1e35397f744548434e3`. The first and only
code-producing production candidate is strict exact. The object advances from
7/29 to 8/29 exact functions, from 416 to 480 exact padded code bytes, and
from 342 to 405 exact meaningful code bytes. It remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `__rasterizer_decals_flush` | 63 | 64 | 8 | `804761b776bce9f614f4773dd0027cb311818eea35413f4e269a0070ec332c44` |

The hardened comparator reports `all_equal: true` for every padded byte and
all ordered relocation identities:

| Address | Type | Resolved destination |
| ---: | --- | --- |
| +1 | DIR32 | `_bss_0045e8e8` +20 |
| +17 | DIR32 | target source-path string |
| +22 | DIR32 | target `local_vertex_cache` string |
| +27 | REL32 | `_display_assert` |
| +34 | REL32 | `_system_exit` |
| +44 | REL32 | `_decals_unlock` |
| +49 | DIR32 | `_bss_0045e8e8` +20 |
| +55 | REL32 | `_lruv_flush` |

## Census and provenance

- The January split object's SHA-256 is
  `bcbbf5e61571861c22ed4503fae05f8760652ba2d53f87e7e5d112c4db23c892`.
  Its disassembly proves assertion line `0x8E`, the typed cache owner at +20,
  `decals_unlock(FALSE)`, and `lruv_flush(local_vertex_cache)`.
- Every applicable campaign/tooling document and Rasterizer Xbox Decals ledger
  was read before the edit. None records a prior attempt at this function.
- All 1,524 registered worktrees were scanned for a definition of
  `_rasterizer_decals_flush`; none contains one. `git log --all -S` reaches
  only the initial skeleton commit `e830efbd`.
- HCEA's exact-name donor
  `reference/halocea/src/_rasterizer_decals_flush.c` independently preserves
  the same two typed calls, while its public wrapper proves the ordinary
  no-argument cdecl boundary.
- The already exact same-TU old-map disposer authenticates the cache assertion,
  `decals_unlock`, and `lruv_flush` compiler surface under the January flags.

The frozen candidate therefore uses the sole natural evidenced form:

```c
void _rasterizer_decals_flush(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\rasterizer\\xbox\\rasterizer_xbox_decals.c",
		0x8E,
		local_vertex_cache);
	decals_unlock(FALSE);
	lruv_flush(local_vertex_cache);

	return;
}
```

## One-shot and policy boundary

The baseline source Git blob was
`cb41a33e96d644f6d9b5cf195640417bb199b77c`; the retained source blob is
`8375270a7468f61ea0c6bce0c798fd85ef533729`. VC7 `/Zs` first accepted the
frozen source without changing the existing object hash or timestamp. The
next invocation through the production Ninja edge emitted the only candidate
object, and it matched immediately. There was no body, predicate, declaration,
placement, spelling, topology, or compiler-control retry.

Compilation used Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`
with the unchanged XDK 3911 flags. `CL.Exe` has SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
the rebuilt object has SHA-256
`6e34672caca085a396647cef22d388de2a003e4f546ee494515d9e6dfda39bf6`.

The retained function is readable typed C89, spells `void` on its own
parameter line, and ends with an explicit `return;`. It contains no assembly,
`volatile`, `register`, pragma, intrinsic, attribute, compiler barrier, raw
address or offset, pointer/integer reconstruction, pointer or union punning,
undefined behavior, synthetic anchor, byte forcing, object patch, compiler or
tool exception, semantic allowlist, or parked entry. It defines no storage and
changes no header, configuration, symbol map, compiler flag, or protected
source.

## Regression and validation

- Direct comparison preserves all seven inherited exact functions, including
  the immediately preceding decal-vertex delete leaf.
- The pre-edit whole-TU regression check reports no changed nonexact function
  and all seven inherited exact functions in `still_exact`. Its only findings
  are the reviewed new exact function, its new symbol-set entry, and the
  ordinary new anonymous `.debug$F` compiler metadata section. No target-owned
  data or prior ownership changes.
- Object gate: 8 exact, 0 residual, 21 unwritten of 29 functions.
- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` targets: pass.
- Semantic audit: 470 units, 4,919 functions evaluated, 4,802 semantic exact,
  4,812 accepted exact, and zero unit errors or ordinary rejections.
- Strict board: 4,784/8,246 exact functions and 663,255/1,922,669 exact padded
  code bytes.
- Protected Units sentinel: 189/189 functions and 54,864/54,864 padded bytes
  strict exact.
- Admission audit: zero candidates, contradictions, and revocations.
- Park validation: 12 active, zero stale, and zero invalid; this lane adds no
  park.
- Tooling suite: 212/212 tests pass.
- `git diff --check`, changed-source policy scan, deleted-path scan, and
  retained-path review: pass.

The function is strict exact and retained. The object is not eligible for
`Matching` until its remaining 21 functions and target-owned data are closed.
