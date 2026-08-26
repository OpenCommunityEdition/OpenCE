# Weapons random wrapper first-shot record (2026-08-26)

## Disposition

The manually authenticated `source/items/weapons.obj:_random` candidate emitted
the exact January function on its first and only current-weapons-TU production
compile. The wave is nevertheless **rejected** because the frozen definition
was placed in the January-order gap before `weapon_delete`; that new COMDAT
renumbered later COFF sections and failed the pre-wave whole-translation-unit
regression manifest. The gate has no lawful recipe for accepting a general
numeric section-index transition. The candidate source was therefore deleted
verbatim, no placement or spelling tune was attempted, and no second
production object was emitted.

The first object remains preserved as ignored audit evidence. No implementation
from this wave is selected for integration. The approval-gated Units package,
its shared interface package, and every protected neighboring translation unit
remain unchanged.

## Immutable inputs

- Cumulative base commit:
  `061cb2eeb5b811e60f024057559f9431bd2ea5f8`.
- Baseline `source/items/weapons.c` blob:
  `f6abe256f6b4becb78c6db398fd577e98a8cc6e3`.
- Baseline generated object: 16,576 bytes, SHA-256
  `1712e72036c666f29c2148c01bd4abccdff51a9d5f8dec5656052320ad181693`.
- January split object: SHA-256
  `5dd414f3f77e67aa7b6609f9131debfa8a9b3b23afab6a57fe4e790410cff2fe`.
- Frozen no-build baseline manifest:
  `build/audit/weapons_random_baseline_20260826.json`, SHA-256
  `c77753ff5fbc47a20ae1679ec3a09b2b6babee48418bf27c0c77fbf7da62e45e`.
- Frozen candidate source blob:
  `f5d1a4297dd89f1c711fa04128fc2c7d8b8b2f61`.
- First and only candidate object:
  `build/audit/weapons_random_first_current_tu_20260826.obj`, 16,891 bytes,
  SHA-256
  `282d1f1e6a491c46e7fa4d74ec4ada5e3893836847de5e0508cc0792a14c381a`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1,
  `xbox/bin/vc7/CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

## Claude, history, and tooling provenance

The candidate was not treated as a shape-only transfer. Before emission, the
audit covered all 27 reachable `claude/*` branch tips, all 32 preserved
`C:/halo-worktrees/claude-*` worktrees, 30 preserved weapons objects, the
documented dirty `work/libcmt-stream` artifact, the Stian weapons source, and
path-limited repository history. None contained a weapons-TU `random`
definition or a compiled candidate `_random` owner. The three identical Claude
weapons logs have SHA-256
`27420a058ef67b509bbf02e271b1251aa184e98410f25cf86344a0343f0fc6a6`
and do not mention the wrapper. `claude/random-numbers-8` is unrelated.

The full documentation audit included Claude's
`near-complete-objects-20260816:docs/object_matching_logs/weapons_obj.md`, the
current `weapons_obj_codex_checkpoint.md`,
`effects_obj_jonas_random_wrappers_20260826.md`, and
`docs/shape_transfer_executor_strategy.md`. Repository history attributes the
canonical header signature/name to Berthalamew commit `369b71e72` and its body
to ficool2 commit `ef3598db7`; it is not Claude-authored.

The current shape atlas discovery entry groups exact donor
`source/cseries/debug_memory:_local_random` with recipient
`source/items/weapons:_random`. This does not authorize production transfer.
`tools/transfer_executor.py` must refuse the 15-meaningful-byte recipient as a
terminal `stub` because its size is at most 16. A diagnostic pass beyond that
boundary would also fail closed on textual prototype spelling,
`unsigned short()` versus typedef-equivalent `word()`. The executor therefore
produced no packet and granted no authority; this was a manually reviewed
typed-source candidate.

## Target and compiler proof

January requires an external type-`0x20`, storage-2, selection-1 code owner:

- 15 meaningful / 16 padded bytes;
- raw bytes
  `e8 00000000 50 e8 00000000 83 c4 04 c3 90`;
- zero-addend REL32 at `+0x01` to
  `_get_global_random_seed_address`;
- zero-addend REL32 at `+0x07` to `_seed_random`;
- normalized SHA-256
  `a8ad3cb30f329f39f3193d4112f79d5983d27e7d41b1c99b5e0b4cd98e555bd0`.

The canonical body is already present in `source/math/real_math.h`:

```c
word random(
	void)
{
	return seed_random(get_global_random_seed_address());
}
```

The exact compiler proof was stronger than the recent floating-return effects
miss. Current `source/cseries/debug_memory.obj:_local_random` is strict exact
with the same unsigned-16 return ABI, the same `seed_random` call, the same
15/16-byte normalized shape, and the same cleanup sequence. Only its first
relocation names the local rather than global seed getter. Both intended
dependencies are already strict in complete `source/math/random_math.obj`:

- `_get_global_random_seed_address`: 53 meaningful / 64 padded, seven
  relocations, normalized SHA-256
  `978f6fa37835b338c2088717e09aa32a8e80f074c0c52043c3ee74f9e0c3e9a1`;
- `_seed_random`: 26 meaningful / 32 padded, no relocations, normalized SHA-256
  `e5c8f44dbccf1be3251ae885c346b097f4d9642b0586721556d414ce40c882d8`.

## Frozen source and one-shot compile

The current include closure already reaches the inline header definition. The
candidate used only a translation-unit-local macro bridge around the existing
include block, followed by the ordinary external typed body in the January
order gap before `weapon_delete`:

```c
#define random random_inline
#include "cseries.h"
/* existing includes unchanged */
#include "units/units.h"
#undef random
```

No shared header, configuration, Units file, or protected neighbor changed.
The frozen source passed `git diff --check` and the production-flag XDK `/Zs`
parse-only check. The untouched base object remained at SHA-256 `1712e720...`.
A Ninja dry run exposed exactly one
`CL build/base/source/items/weapons.obj` edge. That edge was invoked once and
reported exactly `[1/1] CL build\base\source\items\weapons.obj`; its output
was copied immediately to the preserved audit path above. No source spelling,
declaration, scheduling, placement, include, or compiler flag changed before
adjudication.

The first object proves `_random` strict exact with the required raw bytes,
two relocation identities, external owner, code flags `0x60501020`, and
selection 1. It emits no defined `_random_inline` symbol. Direct hardened
comparison also proves all 37 inherited accepted functions remain identical
both to January and to the untouched baseline: 3,237 meaningful bytes, 3,520
padded bytes, and 143 code relocations. All nine accepted runtime-data owners
retain identical payload, relocation, selection, and ownership evidence,
including the 1,536-byte `_data_00307140` owner and its three DIR32
relocations. No new runtime data owner was introduced.

## Whole-TU rejection

The frozen baseline gate was intentionally run without an adjudication file.
It identified `_random` as `NEWLY_EXACT`, but its full object fingerprint also
reported:

- only 13 inherited owners as `still_exact` at the full fingerprint level;
- 24 accepted owner fingerprints changed because the inserted code section
  incremented their recorded numeric COFF section indices;
- the two deliberately nonexact callers listed as changed for the same
  object-order reason, despite unchanged normalized code evidence;
- associated `.debug$F`, `.debug$S`, non-code section-index, and symbol-set
  transitions.

Independent `coff_compare.section_info` comparisons prove that none of the 37
accepted code sections changed at runtime. An acceptance-view comparison that
removes only object-local numeric indices likewise proves all nine accepted
runtime-data sections unchanged. Those facts do not make the manifest pass.
The only function-transition recipe implemented by the gate is the narrowly
scoped stock-XDK selection-1 to selection-2 D3DINLINE recipe; it cannot waive
general section renumbering, and adding a new comparator exception is banned.

Accordingly, the exact function was not retained. The macro bridge and body
were removed literally, restoring source blob `f6abe256...`. The live ignored
object was restored from the authenticated untouched baseline without a
compile. A final no-build manifest check then passed with all 37 accepted
functions `still_exact`, `changed_nonexact: []`, no data failure, and no
warning. The first candidate object remains preserved; the rejected source
will not be repositioned or recompiled in this wave.

## Source-policy record

The rejected body was readable typed C with `void` on its own parameter line
and an explicit return. It used no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, forced inlining, barrier, raw address or offset,
pointer/integer reconstruction, cast or union pun, inactive-union access,
undefined behavior, synthetic caller or anchor, object-byte patch, comparator
exception, alternate compiler, or alternate flag. No push, amend, rebase,
history rewrite, or worktree removal occurred.
