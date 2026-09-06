# Weapons inventory pickup: owner-correct Fable reconciliation

Trial started 2026-09-05; integration review continued 2026-09-06.

## Result

The corrected `weapon_handle_potential_inventory_item` first candidate and
ordinary Ninja output reproduce January's **536 meaningful / 544 padded code
bytes**, all **19 ordered relocations**, and normalized SHA-256
`afa68283e01df84b8c6dc3feb6f010e45959c139a661b442af1dbc6d432b9ec2`.
Weapons advances **46 -> 47 / 79 strict functions**, with two inherited
residuals and 30 unwritten owners. The object remains `NonMatching`.

The full build reports **916,182 / 2,198,102 meaningful exact code bytes**,
**6,207 / 11,060 credited functions**, and **391 / 833 Matching objects**.
This is exactly **+536 meaningful bytes and one function**, not the donor's
entire six-function / 2,112-padded-byte claim. Matched data remains
2,061,020 / 4,176,062 bytes. The global strict census advances
6,252 -> 6,253 / 8,245 with zero inherited exact losses.

## Why this reopens the earlier rejection

Fable tip `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e` repeats an earlier
Claude body whose instructions already matched. That earlier body was rightly
held because it used a layout-only ammunition surrogate: a padding word,
two unused longs, and a tag field renamed `equipment`.

This packet instead defines the authentic complete tag subrecord in its
existing owner, `source/items/weapon_definitions.h`, immediately before the
magazine definition that consumes it:

```c
struct weapon_ammunition_object
{
	short rounds;
	word unused[5];
	struct tag_reference object;
};
```

The HCEA database-derived header has SHA-256
`42376e7a0a056d6e8c39201aeb8c2b9b989462a82fe3864ec229ccd2329a0580`
at the workspace research cache's
`halocea-full-blobs-20260830/src/headers/weapon_ammunition_object.h`.
January independently fixes the 28-byte tag-block stride, rounds at +0,
and the tag index at +24. The retained body uses the authentic structure
name and `ammunition_object->object.index`. It has no private schema copy.

`weapons.h` already owns the complete public handler prototype, and the
current Players implementation already calls it. `equipment.h` already owns
`equipment_definition_handle_pickup`; Weapons now directly includes that
header instead of inheriting Fable's implicit declaration. No new public
prototype, config alias, compiler control, or unrelated body was needed.

The two source changes are therefore a real schema/API correction plus the
matching implementation, not a retry of the rejected surrogate's field
spellings to steer code generation. There was one corrected-body scratch
compile followed by the normal Ninja build, with no source-shape variants.

## Source semantics and house rules

The body uses `weapon_get`, `item_get`, `weapon_definition_get`,
`weapon_magazine_get`, `TAG_BLOCK_GET_ELEMENT`, `MIN`, genuine `short` loop
indices, and the correct boolean return. All parameters have separate lines;
locals are initialized where the C89 block permits it. There is no assembly,
inline control, artificial caller, dead emission anchor, raw structure offset,
representation cast, alternate flag, fake global, or copied math helper.

For each non-full magazine, the routine either transfers rounds from the same
weapon type or searches its configured ammunition items. It preserves the
target's pickup-sound, equipment-pickup, deletion, magazine-update, and output
store order. In particular, the output is the last processed non-full
magazine's transfer, not an accumulated total; a matching same-type weapon
can be reported handled even with zero rounds transferred. The source does
not silently replace those January behaviors with later HCEA network logic.

The Fable worktree remains read-only. Its object was not copied into the build;
in particular its unrelated `_profile_global_enable` COMMON is not imported.
No Opus-reserved Rasterizer, Devices, or Widgets implementation was edited.

## Header and whole-runtime regression proof

Before any edit, root froze all actual Ninja consumers of `weapons.h`,
`weapon_definitions.h`, and `equipment.h`: **18 objects**, including all 11
weapon-definition consumers and the protected Units sentinel. The owning
schema and Equipment include were compiled before the handler body.

The prerequisite alone retains all 6,252 strict functions. Its complete
runtime comparison contains **2,235 unchanged sections and 40 compiler-local
label spelling changes**, with no actual runtime change, lost/added owner,
COMMON difference, or new `point_from_line3d` definition/reference.

The final packet retains that same inherited-runtime result and adds exactly
one ordinary external NODUP function owner: the handler. Weapons moves from
82 to 83 sections; all 82 inherited runtime sections are preserved, with only
the existing `weapon_set_state` compiler-local labels renumbered. There is no
new writable or read-only data owner, no helper COMDAT, no COMMON, and no
lost section. The three new undefined public function references are
`equipment_definition_handle_pickup`, `object_delete`, and
`unspatialized_impulse_sound_new`, each already genuinely owned elsewhere.

The frozen all-base census contains 572 objects. Only the 11 real consumers
rebuilt; the other 561 object hashes are identical. Units remains 189/189.
Source and metadata for all other Fable Weapons candidates remain untouched.

## Verification artifacts

All following paths are canonical-relative. Captures copy and hash immutable
objects and source/config inputs; the original Fable build is not a fallback.

| Artifact | SHA-256 |
| --- | --- |
| `scratch/weapons-fable-owner-before-20260905.json` | `db0c1efee14544d2850b8f9da9830e7e36586b18e56c06b2ab424b27e57bfa39` |
| `scratch/weapons-fable-owner-schema-20260905.json` | `31bee1ccadcb05ba373979c2de440a3ebbadbba3ee0817020e896e8883e898b2` |
| `scratch/weapons-fable-owner-inventory-20260905.json` | `21ee92deb887144dfb269541f90609728c112bd17fa827658a86e1688931dd96` |
| frozen January `weapons.obj` | `479772a8aa3ae2518cad52ec71c82faf44382a8808214546e52e2b7cf202ac11` |
| `scratch/weapons-fable-inventory-first-20260905.obj` | `0c824db65f9347bae031aada846eed12bad8edb45f088b65ae2115ef640bcc9a` |
| frozen actual Ninja `weapons.obj` | `015c3ff53cf778015aa666f68298ad011207aa155069658c5483a47d3c59a2a9` |
| `source/items/weapons.c` | `391f4f59f91828dfb283a5c7198f4e93d3d59e2481e6f218055e18126d3762fa` |
| `source/items/weapon_definitions.h` | `b79d200be38639991ca19162421ae293295285bea02dd6fce7e351269597d9bb` |

Capture tool: `scratch/weapons_fable_owner_capture_20260905.py`.
Stable snapshots: `scratch/weapons-fable-stable-{before,schema,inventory}-20260905.json`.
Runtime audit summaries: `scratch/weapons-fable-{schema,inventory}-runtime-20260905/summary.json`.
The generic frozen runtime comparator passes its nine invariance/negative
controls. Its intentional new-function review lead is adjudicated above,
not hidden with a comparator exception.

Full Ninja, the normal semantic report, and source-policy scan pass. Park
validation is **284 active, zero stale, zero invalid**. Object admission is
**zero candidates, zero contradictions, five existing rejections, zero
revocations**. The pre-oracle public tool suite passed **321 tests plus 26 subtests**.
With the 27 new oracle tests, the full suite passes **348 tests plus
26 subtests**. The subsequent two-function Geometry packet takes the combined
local total to 916,551 meaningful bytes and 6,209 credited functions; those
369 additional bytes are not Weapons credit.

## Independent runtime and owner review

The public, input-hash-pinned oracle is
`tools/audit/weapons_inventory_runtime_differential.py`, SHA-256
`3b59767afe002cdaf3cde8107ffe5b8b81b264ac8d9757e3587b0928f133a4e2`.
Root read the complete implementation and replayed the actual January,
first-candidate and ordinary Ninja function bytes under Unicorn. All three
pass **23 cases**, with zero semantic failures or differential mismatches;
all nine synthetic-outcome negative controls detect their intended changes.
The controls are not mutated-machine-code executions.

Root's report is
`scratch/weapons-inventory-public-runtime-root-20260906.json`, SHA-256
`f5bbff8222f4e50c76e75a4401269792af6dfb78949e9110887cb4d8a2246874`.
All three execution sets share trace SHA-256
`89b3add8e756eba4d68db6b1f129c60b2aee798f91ce5182e4277dd9bf670b57`.
The 27 source-independent tests in
`tools/test_weapons_inventory_runtime_differential.py` exercise case coverage,
explicit expected behavior, authentic contiguous magazine layout, packet
guards, ABI/state/event mutations, and the differential negative controls.

Before accepting this oracle, root caught two fidelity defects in the initial
scratch version: item and weapon access for one handle returned different
datum addresses, and magazine tag records used an artificial 0x100 stride.
Both were corrected to one actual datum and the authentic contiguous 0x70
stride. All 23 cases still pass. Historical pre-correction oracle hashes and
results are superseded, not additional independent evidence.

The harness executes the real 544-byte function, stubs only genuine external
APIs, poisons volatile registers across those calls, and compares all seeded
state regions, call arguments/order/depth, boolean return, stack, direction
flag, preserved registers and caller frame. Cases cover zero/one/two
magazines, same-weapon and equipment paths, empty/full/capped transfers,
nonmatching tags, pickup sounds and local-player bounds, zero/maximum rounds,
and both fatal assertion paths. Assertions model a definition changing
between lookups; they are not claimed to be ordinary tag behavior.

Deleted objects deliberately remain mapped: January can delete an item and
then access or delete it again while processing another magazine. The oracle
exposes and preserves this original ordering; it does not prove that such a
game-state scenario is safe, nor repair it to manufacture a nicer match.

Root also fully read and replayed the separate frozen owner comparison:
`scratch/weapons_fable_inventory_independent_review_20260906.py`.
Its root replay, `scratch/weapons-fable-inventory-independent-owner-root-20260906.json`,
has SHA-256 `74c5466af1fd4a4bec096aa7093199486b71830bdc81217adcf945e7860cb7a9`.
Target/first/actual packet metadata and all 19 relocations agree; all 50
inherited function owners and 18 named runtime-data owners are unchanged.
Only the reviewed handler and three genuine undefined APIs are added, with
no COMMON or point helper change. The generic all-section comparison above
also covers unnamed sections rather than relying solely on named owners.

## Remaining Weapons disposition

See `weapons_obj_fable_packet_intake_20260905.md` for the complete independent
intake. Keep the five other Fable strict claims / 1,568 padded bytes uncredited:
the charged/export pair needs its semantic enum/API owner packet; Aim remains
at the prior shared-header boundary; Animation Update's authentic return
type remains unresolved; and the matching long-return `weapon_place` form
contradicts the actual void placement callback. `weapon_can_be_fired` remains
an exhausted residual outside that exact-byte claim. This packet does not
certify the whole Weapons object or finish the entire Fable backlog.
