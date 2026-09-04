# `damage.obj` private-helper recovery (2026-09-04)

## Result

This isolated packet gives the two remaining small address-only Damage owners
their authenticated semantic names and reconstructs both as ordinary private C.
The effect wrapper is strict exact in the production translation unit. The
player-owner helper is strict exact when its real same-TU call relationship is
represented, but remains deliberately uncredited in the production build
because its only authentic caller, `object_cause_damage`, is still unwritten.

| January/source owner | Production result | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_damage_effect_new_on_object` | strict exact | `29 / 32` | `1` | `c579ae99c610b2026d2178ef3f5e4bbcf3f68f5f79a97d4fc117adacb8f6795c` |
| `_get_player_index_from_object_or_parents` | unwritten; exact in same-TU caller proof | `70 / 80` | `3` | `68bb4d24589f0217f74e7c24f53740122b1e22147474b62140143c7ac860db55` |

The credited campaign gain is therefore **29 meaningful bytes, 32 padded
bytes, and one exact function**. The focused Damage gate moves from **23 / 34**
to **24 / 34** strict-exact functions, with zero residuals. No fuzzy bytes are
claimed and no parked entry is needed.

The work was based on canonical
`08a8c8d1d74d0037b1724477eac87c353ab333d7` in the isolated branch
`agent/damage-private-helpers-20260904`. The canonical checkout and all donor
trees were read only, and this worker does not push.

## Name and behavior provenance

The semantic names and behavior are independently authenticated by the local
HaloCEA tree at `570c83fd9c365dad6f2a3e7041705d5b84c7847c`:

- `src/blam/game/get_player_index_from_object_or_parents.c` owns
  `get_player_index_from_object_or_parents`. It walks an object's parent chain,
  returns `player_index_from_unit_index` for the first unit, and returns `NONE`
  when no unit is found.
- `src/blam/objects/damage_effect_new_on_object.c` owns
  `damage_effect_new_on_object`. It calls `effect_new_from_object` with the
  affected object as both source and destination, no node, zero scale bounds,
  and null optional vectors.
- `src/blam/objects/object_cause_damage.c` calls the player-owner helper for
  the damaged object and the damage source. This authenticates the otherwise
  currently missing same-TU caller relationship.

Stian's local PC lift at `a613ab54a68366ad1053d1aa913ddf687a14df8e`
independently contains the same bodies as `FUN_00136890` and `FUN_001369e0`.
Its notes and call sites corroborate the private EAX entry convention visible
in January. Pastudan independently corroborates the parent-chain behavior.
Open Marathon was checked for Bungie source conventions; it contains no direct
Halo-specific implementation, but supports the use of semantic private
prototypes, typed access, and ordinary explicit-return source.

The pristine January split remains the authority for instructions,
relocations, ABI, and ownership. `config/symbols.json` now names both target
owners and marks them static. After target regeneration, each target owner has
COFF function type `0x20` and storage class 3, matching the private source.

## Recovered source and emission schedule

### `get_player_index_from_object_or_parents`

The retained function initializes a semantic `player_index` to `NONE`, walks
`object.parent_object_index` while an object exists, uses the typed
`unit_try_and_get` accessor to identify a unit, and converts that unit through
`player_index_from_unit_index`. There are no raw object casts or address-based
identifiers.

VC7 legitimately removes this unreferenced static definition while
`object_cause_damage` remains unwritten. The helper was therefore validated in
an ignored audit-only copy of the same translation unit with one natural
caller:

```c
long damage_helper_repro_caller(
	long object_index)
{
	return get_player_index_from_object_or_parents(object_index);
}
```

That relationship emits the private helper with the January EAX ABI and is
strict exact at 80 padded bytes. Target and audit candidate both have storage
class 3, three ordered relocations, and normalized SHA-256
`68bb4d24589f0217f74e7c24f53740122b1e22147474b62140143c7ac860db55`.
The production source does not retain the audit caller. Public linkage, an
assembly ABI bridge, an artificial emission anchor, and a fake keepalive were
all rejected. The current production gate consequently and honestly reports
this owner as unwritten; it can receive its 70 meaningful bytes naturally when
`object_cause_damage` is reconstructed.

### `damage_effect_new_on_object`

Three existing exact callers (`object_deplete_body`, `object_destroy`, and
`object_deplete_shield`) now express their shared effect construction through
the recovered static helper. VC7 naturally inlines the body at those call
sites, preserving all three inherited exact sections, and emits one out-of-line
copy for the private owner. The result is January's precise private ABI: object
index in EAX, effect definition index on the stack, eight semantic arguments to
`effect_new_from_object`, and a normal return.

The target and retained candidate are both 32 padded bytes with one REL32
relocation to `_effect_new_from_object`, identical normalized bytes, COFF
storage class 3, and function type `0x20`. The meaningful stream ends with
`ret` at section offset `+0x1C`; the final three bytes are alignment padding.
No inline annotation or compiler-control construct is needed.

## House-rule audit

Both prototypes and definitions are owner-local because the functions are
private. Every parameter is on its own line, the local declaration is
initialized where natural, and both functions end with an explicit `return`.
The source uses semantic identifiers, project constants, and typed object/unit
accessors. There is no address-derived live function or global name in either
owner.

No assembly, volatile/register steering, pragma, barrier, forced
inline/noinline, dummy dependency, raw offset, representation pun, undefined
behavior, fake branch, or nonsensical byte-matching construct is retained. The
rebuilt object contains no `point_from_line3d` symbol, so the protected January
inline and COMDAT schedule remains intact. No header was changed, avoiding the
known Units definition-position sensitivity.

## Verification

- Full `ninja halobetacache_build libcmt_build semantic_progress progress`:
  pass from final source.
- Focused Damage gate: **24 exact / 0 residual / 10 unwritten**; all 23
  inherited exact owners remain exact.
- Focused wrapper proof: **29 meaningful / 32 padded bytes**, one relocation,
  identical normalized SHA-256, and strict exact.
- Audit-only natural-caller proof for the player-owner helper: **70 meaningful
  / 80 padded bytes**, three relocations, identical normalized SHA-256, and
  strict exact. Production credit remains zero until the real caller exists.
- Rename-stable whole-tree snapshots: 8,245 functions, exact count 6,131 to
  6,132; **+1 function / +32 padded bytes / zero regressions**.
- Campaign progress: 886,097 to 886,126 meaningful code bytes and 6,083 to
  6,084 functions; **+29 meaningful bytes / +1 function**. Matched data remains
  2,018,514 bytes.
- Semantic report: 473 units scanned, 6,516 functions evaluated, 6,141
  semantic exact, 6,157 accepted exact, and zero unit errors.
- Units protected gate: **189 exact / 0 residual / 0 unwritten**.
- `python -B -m tools.parked_functions`: 232 active / 0 stale / 0 invalid.
- `python -B tools/audit_object_admission.py`: zero candidates,
  contradictions, or revocations; the two inherited reviewed rejections are
  unchanged.
- Changed-file fake-match scan: zero review leads.
- `python -B -m pytest tools -q -p no:cacheprovider` with a workspace-local
  `--basetemp`: **261 passed**.
- Rebuilt `damage.obj` symbol scan: no `point_from_line3d`.
- `git diff --check`: clean apart from Git's informational line-ending
  warnings.

Lane-local final fingerprints before the documentation-only commit are:

- regenerated semantic target object SHA-256:
  `DC026D27E460FCBC8C09D608D65712120F6AE85DD570FEC28D87F089905EA937`;
- rebuilt candidate object SHA-256:
  `FA87E8F7D38FB62569D9B19383549315901295F54FD6ADA00003AA6D10865079`;
- retained `damage.c` SHA-256:
  `DAA07DA2F81E01C9262F3BE2A01C3D1CCEA737E5EB04B37CD44FBE579BD6C32B`;
- stable snapshots: `scratch/damage-private-before.json` and
  `scratch/damage-private-after.json` (lane-local, intentionally uncommitted).
