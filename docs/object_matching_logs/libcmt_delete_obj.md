# `libs/libcmt/delete.obj` exact-match ledger

This atomic lane independently revalidated a completed historical result from
the current campaign tip `95908c0f`. The old `f902806d` result was treated only
as a hypothesis: the January object, source shape, flags, ownership, and every
acceptance gate were checked again in a fresh isolated worktree. The unit was
not parked, vendored assembly, or owned by another active lane.

## Plain-English behavior

This is the C++ scalar-delete entry point. It receives the address of a block
that is no longer needed and gives that address to the CRT `free` routine. The
compiler recognizes that there is no work after `free` and emits a tail jump,
so `free` returns directly to the original caller.

## January target inventory

- One external function: `??3@YAXPAX@Z`, meaning
  `void __cdecl operator delete(void *)`.
- One five-byte, one-byte-aligned, no-duplicates `.text` COMDAT.
- One `IMAGE_REL_I386_REL32` relocation at function offset `+0x1`, targeting
  `_free + 0`.
- No target-owned `.data`, `.rdata`, or `.bss`; no data aliases.

The target inventory was decoded directly from the current January split
object before source was admitted.

## Accepted representation and experiment ledger

Only the natural readable implementation was tested in this fresh lane:

```cpp
void operator delete(
	void *block)
{
	free(block);

	return;
}
```

It matched on the first build. The project retains the historical `.c` path,
so the object-specific `/TP` option selects C++ parsing; this is required by
the target's C++-mangled public symbol, not a code-generation experiment. The
optimization options remain libcmt's ordinary `/O1 /Gy`. No alternative
source shapes were needed or rejected.

## Strict proof

The candidate was compiled with XDK 3911 CL `13.00.9254.1` under
`/O1 /Gy /TP /I"xbox/include"`. The hardened comparator reported
`section_infos_equal` for the sole runtime COMDAT:

| evidence | January target | rebuilt candidate |
|---|---:|---:|
| code size | 5 bytes | 5 bytes |
| relocation count | 1 | 1 |
| normalized SHA-256 | `e8ebd827d1f36d7cfa5e5220610aa6370284d1589989363f48ac40166362d449` | `e8ebd827d1f36d7cfa5e5220610aa6370284d1589989363f48ac40166362d449` |
| relocation | `+0x1`, `REL32`, `_free + 0` | `+0x1`, `REL32`, `_free + 0` |

`llvm-readobj` independently confirmed identical external symbol linkage,
COMDAT selection, alignment, runtime size, and relocation identity. The
rebuilt object's `.drectve` and debug sections are compiler metadata and not
linked runtime ownership. There are no runtime data sections to approximate,
alias, or credit separately.

## House/Berth and original-bug audit

The source is ordinary typed C++ with the parameter on its own line, an
explicit `void` return type, and the customary explicit final `return;`. It
uses the standard declaration from `<stdlib.h>` rather than a guessed raw
prototype. It contains no assembly, `volatile`, forced inline, undefined
behavior, type-punning or codegen cast, byte patch, alignment trick, packed
surrogate, comparator exception, or per-function optimization override. The
symbol-list header is retained without affecting generated code.

No original behavioral bug is present: passing the block directly to `free`
is the intended scalar-delete behavior, and the C runtime defines `free(0)` as
safe. Therefore no bug-preservation or optional-fix comment is needed.

## Campaign gates

- The complete `halobetacache_build` and `libcmt_build` passed.
- Regenerated progress admitted the whole object. At this lane's snapshot,
  libcmt reports 24/212 objects, 25/476 functions, 1,822/55,015 code bytes,
  and 192/8,637 data bytes. Relative to the current-tip baseline, this object
  adds exactly one object, one function, and five code bytes; data is unchanged.
- The semantic audit evaluated 3,462 functions with zero unit errors and
  reported 3,378 accepted exact functions.
- The object-admission audit produced no candidates or revocations; its sole
  contradiction is the unrelated pre-existing `source/shell/shell_xbox` item.
- The complete tooling suite passed 179/179 tests.
- The fail-closed per-unit regression snapshot/check passed after the local
  atomic commit. It reports `??3@YAXPAX@Z` as `still_exact`, with no failures,
  warnings, changed non-exact siblings, unexpected new matches, or ownership
  changes.
- `git diff --check`, final scope review, and prohibited-construct audit are
  required clean before handoff.
