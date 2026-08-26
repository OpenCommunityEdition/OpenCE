# Weapons animation-update first-shot record (2026-08-26)

## Disposition

`source/items/weapons.obj:_animation_update` emitted strict exact on its first
and only current-weapons-TU production compile. The append-only wave is still
**rejected** because VC7 grouped the new external definition before the
translation unit's static/private tail. That compiler grouping shifted eight
accepted full section fingerprints and both deliberately retained nonexact
callers. The frozen whole-TU boundary therefore failed even though direct
runtime code and data remained equal.

The candidate source was removed literally. No source relocation, declaration
change, return-type variant, scheduling change, or second production compile
was attempted. The exact first object remains preserved as ignored audit
evidence. No implementation from this wave is selected for integration, and
the approval-gated Units/shared/protected package remains unchanged.

## Immutable inputs and first artifact

- Cumulative base commit:
  `bcd6dd685cf432b55220fa7b0b6edf6423a558dc`.
- Baseline source blob:
  `f6abe256f6b4becb78c6db398fd577e98a8cc6e3`.
- Baseline object: 16,576 bytes, SHA-256
  `1712e72036c666f29c2148c01bd4abccdff51a9d5f8dec5656052320ad181693`.
- January split object: SHA-256
  `5dd414f3f77e67aa7b6609f9131debfa8a9b3b23afab6a57fe4e790410cff2fe`.
- Frozen no-build baseline manifest:
  `build/audit/weapons_animation_update_baseline_20260826.json`, SHA-256
  `d21c1b7501641f18ec4517348283f4e7b5372494016a87c0599344157e4b51fb`.
- Frozen candidate source blob:
  `1894091140cc67cbf45a48ebdbbff59f2763ba2c`.
- Preserved first and only candidate object:
  `build/audit/weapons_animation_update_first_current_tu_20260826.obj`,
  16,782 bytes, SHA-256
  `e5a70999cab6c5c2154a1d433847767c83dc248957618901191e387da074400d`.
- Compiler: Microsoft XDK 3911 CL 13.00.9254.1,
  `xbox/bin/vc7/CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

The cumulative source/object/report baseline at `bcd6dd68` is byte-identical
to the pre-ledger `061cb2ee` weapons baseline. The intervening cumulative
commit records only the rejected `_random` experiment.

## Claude, history, and executor provenance

The read-only pre-emission audit scanned all 32 preserved
`C:/halo-worktrees/claude-*` weapons sources, all 25 XDK/GLM weapons objects,
all five CMake weapons objects, the two worktrees without a weapons object,
the documented dirty `work/libcmt-stream` source and ignored object, and
path-limited `claude/*` history. No source or object defined
`_animation_update`. Claude's full
`near-complete-objects-20260816:docs/object_matching_logs/weapons_obj.md`
mentions only a future direct `_animation_update_internal` call inside
`weapon_update`; it never reconstructs this wrapper. The rejected `_random`
object added only `_random`, so it is not prior compilation evidence here.

Current `shape_atlas_current_20260826.json` contains no `_animation_update`
entry. Production `transfer_executor.py` therefore fails closed at
`pair-not-found` before its non-stub policy or prototype checks. HCEA is not an
atlas donor. This candidate was manually authenticated from January COFF,
typed current-tree contracts, Claude's Units carrier, the Units animation
closure ledgers, and the corresponding later-build Stian wrapper at commit
`237dbff18bef78c9a5a6785f7c6023ba1c6f79d8`.

Stian's translated sketch spells the outer wrapper `void`, but it is weaker
later-build evidence with stale neighboring return metadata. The frozen
current-tree spelling used `short`: `_animation_update_internal` is
independently authenticated as short-return, and the immediately adjacent
exact `_animation_choose_random_permutation` is the same fixed-kind forwarding
pattern with an untouched-EAX short return. January's bytes are compatible
with that typed passthrough. The target COFF external storage class overrides
HCEA's source-scope annotation.

## Exact target and typed packet

January requires:

- external type-`0x20`, storage-2, selection-1 `.text` owner;
- flags `0x60501020`, alignment shift 5;
- 27 meaningful / 32 padded bytes;
- raw bytes
  `55 8b ec 8b 45 10 8b 4d 0c 8b 55 08 50 51 52 6a 01 e8 00000000 83 c4 10 5d c3 90 90 90 90 90`;
- one zero-addend REL32 at `+0x12` to
  `_animation_update_internal`;
- normalized SHA-256
  `1bba085c2f0d1d5e282a9626f765732497b85eda67e5fd1b4888f575227bf500`.

The frozen candidate added only this translation-unit-local external
prototype in the existing prototype block:

```c
short animation_update_internal(
	short animation_type,
	long animation_graph_index,
	struct animation_state *state,
	long *sound_index);
```

and this ordinary definition at physical EOF, after every existing body:

```c
short animation_update(
	long animation_graph_index,
	struct animation_state *state,
	long *sound_index)
{
	return animation_update_internal(
		1,
		animation_graph_index,
		state,
		sound_index);
}
```

The prototype is already authenticated in the current Units source and its
Claude animation-closure carrier. `struct animation_state` is present in the
existing weapons include closure. No shared header, configuration, Units
source, or protected neighbor changed. The callee itself remains absent from
current `source/models/model_animations.obj` (January owns 300 meaningful /
304 padded bytes), but current Units and scenery objects already contain
undefined references to the same external owner. This wave recovers only the
forwarding COMDAT and claims no callee closure.

## One-shot result

The frozen source passed `git diff --check` and the exact production-flag XDK
`/Zs` parse-only pass; the untouched object hash remained `1712e720...`.
Ninja dry-run exposed exactly one
`CL build/base/source/items/weapons.obj` edge. Its single invocation reported
exactly `[1/1] CL build\base\source\items\weapons.obj`, and the resulting
object was copied immediately to the preserved audit path.

The emitted `_animation_update` is strict exact across all 32 padded bytes,
the relocation position/type/destination/addend, external owner metadata, code
flags, and selection 1. It is emitted at candidate section 58. The candidate
has 68 sections, 41 defined function owners, 3,872 padded code bytes, and 152
code relocations, versus the baseline's 67 sections, 40 function owners,
3,840 padded code bytes, and 151 code relocations. The 32-byte/one-relocation
delta is exactly the new wrapper.

Direct hardened comparison proves all 37 inherited accepted functions remain
strict equal to January: 3,237 meaningful / 3,520 padded bytes and 143
relocations. With the new wrapper, the direct strict set is 38 functions,
3,264 meaningful / 3,552 padded bytes and 144 relocations. All nine accepted
runtime-data owners remain equal under the hardened acceptance view, including
the 1,536-byte `_data_00307140` owner and its three DIR32 relocations. Runtime
section count, logical bytes, and relocations remain 13, 1,867, and 3. The
inherited `_profile_global_enable` COMMON record is unchanged; no new runtime
storage owner appears.

## Compiler grouping blocker and rejection

Appending an external definition at EOF did not append its COFF section after
baseline section 67. VC7 grouped it at section 58, between the existing
private-tail groups. Representative numeric transitions are:

| Owner | Baseline section | Candidate section |
| --- | ---: | ---: |
| `_weapon_reloading` | 58 | 59 |
| `_code_000ec1a0` | 59 | 60 |
| `_code_000ec960` | 60 | 61 |
| `_weapon_update` | 62 | 63 |
| `_code_000ec300` | 63 | 64 |
| `_weapon_reset` | 64 | 65 |
| `_weapon_ready` | 65 | 66 |
| `_weapon_put_away` | 66 | 67 |
| `_weapon_stop_reload` | 67 | 68 |

The pre-wave no-build manifest correctly identifies `_animation_update` as
`NEWLY_EXACT`, and 29 inherited accepted owners retain identical full
fingerprints. It also reports eight accepted full fingerprints changed,
`changed_nonexact: [_code_000ec300, _weapon_update]`, one changed associative
debug record, changed `.debug$S`, and the expected symbol-set addition. The
code comparator proves the inherited runtime bodies unchanged; that does not
satisfy the frozen full-object boundary. The gate has no lawful general recipe
for external-function grouping or numeric section renumbering, and adding a
comparator exception is banned.

The candidate was therefore rejected immediately. The prototype and body were
removed verbatim, restoring source blob `f6abe256...`. The live ignored object
was restored from the authenticated cumulative baseline without compiling.
A final no-build check then passed with all 37 accepted functions
`still_exact`, `changed_nonexact: []`, no data failure, and no warning. No
second production object exists.

## Source-policy record

The rejected packet was readable typed C with one parameter per line and an
explicit return. It used no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, forced inline, barrier, raw address or offset,
pointer/integer reconstruction, cast or union pun, inactive-union access,
undefined behavior, synthetic caller or anchor, object-byte patch, comparator
exception, alternate compiler, or alternate flag. No push, amend, rebase,
history rewrite, or worktree removal occurred.
