# Fable `weapons.obj` packet: independent read-only intake

Date: 2026-09-05

## Subsequent canonical reconciliation

The inventory handler was subsequently rebuilt with the authentic shared
ammunition schema and the real Equipment API include. Its first scratch
candidate and actual Ninja output match January at 544 padded / 536 meaningful
bytes with all 19 relocations; the complete strict set gains one function
without losses. See `weapons_obj_fable_inventory_reconciliation_20260905.md`
for final admission and publication evidence. The other intake holds below
are unchanged. This addendum does not turn donor-wide claims into credit.

## Disposition

The Fable report's six exact code sections do total 2,112 padded bytes, but
they are not one admissible packet.  They have different provenance and owner
boundaries:

| Proposed owner | Padded / relocs | Fable result | Independent ruling |
| --- | ---: | --- | --- |
| `_animation_update` | 32 / 1 | strict exact | **hold**: old campaign exact, no caller, and `void` versus `short` return remains underdetermined |
| `_weapon_aim` | 272 / 14 | strict exact | **hold at owner API boundary**: credible first-natural body, but its real current caller is unwritten and the only recent coherent header trial lost protected Units |
| `_weapon_handle_potential_inventory_item` | 544 / 19 | strict exact | **go for one owner-correct current-context trial**; this is the smallest coherent packet |
| `_weapon_place` | 208 / 4 | strict exact only after changing `void` to `long` | **reject this exact form**: it conflicts with the genuine placement callback contract and is return-register retention, not authenticated ABI |
| `_weapon_export_function_values` | 912 / 31 | exact after semantic helper rename | **conditional atomic pair**, held until its semantic enum owner is authenticated and blast-tested |
| private `_weapon_trigger_get_charged_fraction` | 144 / 8 | strict exact | **conditional with export only**; never admit as an isolated private leaf |

The separately retained `_weapon_can_be_fired` is a 144-byte/5-relocation
residual after five recorded source shapes.  It is not part of the 2,112-byte
exact claim and must not be retried without new source evidence.

Thus the immediate recommendation is a **544-byte handler-only trial**, not a
2,112-byte replay.  A later 1,056-byte export/helper trial is plausible after
its schema prerequisite is resolved.  No donor object is safe to stage or
count directly.

## Inputs and scope

- Requested canonical baseline: `1200c46e9`; the working checkout advanced
  during this read-only audit to `e50f98a19` for the disjoint Bitmaps packet.
  The five Weapons-related paths inspected here have no diff from `1200c46e9`.
- Fable donor tip: `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`.
- Fable report: `scratch/reports/weapons.md`, SHA-256
  `11ed5412fe1d669c72f4ebd8cd40e927300f95cf8011c695699cfb7a977eee54`.
- Fable facts: `scratch/facts/weapons.md`, SHA-256
  `ea0201cea9251250c0ad92fb23970a7c600daaf5d71760d5b18cc852b7bfdd0f`.
- Fable header request: `scratch/hdr/weapons.md`, SHA-256
  `e650e81ef6b938a3183e3e8ddb732d20e678e658b863e23eb0c0ad2302440932`.
- January target `build/split/source/items/weapons.obj`, SHA-256
  `479772a8aa3ae2518cad52ec71c82faf44382a8808214546e52e2b7cf202ac11`.
- Current canonical `build/base/source/items/weapons.obj`, SHA-256
  `b1442b56699208e57acd941be6ea82dfea348aea1b7e46ecaa93791460c665aa`.
- Fable built object, SHA-256
  `8a033ac82e185932ed29a8252529b18c55da2669de9474c06064e7963ec74ddc`.
- Fable saved `ng_weapons.obj`, SHA-256
  `582bebe2a12024c3773f9d11dcda170fd194dd1222e9848c614ad8bb811ee15d`.
- Fable saved `w_weapons_exp1.obj`, SHA-256
  `2c88814127c4b16a0f123553f77f7bb27f1be88ea773de825e1f6dabd69dd143`.

The audit scans all 833 January split objects and all 572 current canonical
base objects for incoming references and pooled owners.  It does not compile,
gate, edit production/configuration, or inspect any active Opus-owned
Rasterizer/Devices/Widgets implementation.

## Provenance corrections

The Fable report uses "first compile" locally.  That is not campaign-wide
provenance for every body.

- `_weapon_aim` is the one genuinely new first-natural Fable exact in this
  group.
- `_weapon_handle_potential_inventory_item` had already emitted matching
  instructions in the earlier Claude reconciliation.  It was correctly
  rejected because that source used a layout-only ammunition record whose
  names and types contradicted the authenticated tag subrecord.  The Fable
  source repeats that surrogate.  A new owner-correct trial is justified, but
  the Fable compile is not itself admissible evidence.
- `_animation_update` was exact on its one-shot 2026-08-26 trial and remained
  rejected under the then-frozen whole-TU grouping boundary.  Later evidence
  favored `void` while the exact candidate and HCEA used `short`; January has
  no inbound relocation and leaves EAX as callee residue.  Fable did not
  resolve that ambiguity.
- The export/helper pair was already reconstructed exactly and rejected on
  2026-09-01 because the helper newly owned the `1/30` literal.  The current
  repository now has compatible pooled owners for that literal, so the old
  blocker is materially changed, but source/schema ownership still needs a
  fresh coherent trial.
- `_weapon_place` first emitted naturally as a `void` residual.  Fable then
  changed the function to return `long` and added `return weapon_index` to keep
  EAX live.  That is a return-recipe change, not first-natural exact evidence.
- `_weapon_can_be_fired` records five source-shape attempts and remains a
  block-placement residual.  Its exhaustion ruling stands.

## Smallest coherent packet: inventory handler

January and current canonical each have one genuine caller:
`players.obj::_player_examine_nearby_item`.  `weapons.h` already owns the exact
public declaration:

```c
boolean weapon_handle_potential_inventory_item(
    long weapon_index,
    long item_object_index,
    short local_player_index,
    short *rounds_picked_up);
```

The 544-byte January body independently proves:

- signed-short magazine and ammunition loops;
- the 0x1C-byte ammunition-object stride;
- `rounds` at +0 and a tag-reference index at +0x18;
- the same-weapon transfer, pickup sound, equipment pickup, deletion and
  per-magazine output-store order used by the readable body; and
- a boolean AL return.

The Fable-local `weapon_magazine_ammunition_object` is not acceptable:

```c
short rounds;
word pad;
long unused[2];
struct tag_reference equipment;
```

The authenticated tag schema instead supplies the 28-byte owner record:

```c
struct weapon_ammunition_object
{
    short rounds;                 /* +0x00 */
    word unused[5];               /* +0x02 */
    struct tag_reference object;  /* +0x0C; index at +0x18 */
};
```

Its natural owner is `source/items/weapon_definitions.h`, immediately before
`weapon_magazine_definition`, whose `ammunition_objects` block consumes it.
The handler should use `struct weapon_ammunition_object` and
`ammunition_object->object.index`; it should not preserve the donor's local
surrogate or rename the real field back to `equipment`.

Required trial scope:

1. Add only the complete authenticated subrecord (with size/offset checks as
   normal schema checks) to `weapon_definitions.h`.
2. Add only the natural handler body to `weapons.c`; its public prototype is
   already correctly owned.  Add a direct `items/equipment.h` include for
   `equipment_definition_handle_pickup`: the Fable source currently relies on
   an undeclared implicit-int call, which is not an acceptable owner boundary.
   `weapons.c` already directly includes `weapon_definitions.h`.
3. Remove no unrelated local declaration and add no public API.
4. Freeze and rebuild every actual Ninja dependent of
   `weapon_definitions.h`, not merely the eleven direct C includers.  Direct
   includers are ActorCombat, ActorPerception, Units, Cheats, GameEngine,
   Oddball, Players, HUD, FirstPersonWeapons, Weapons and
   `weapon_definitions.c`.  The protected Units 1,920-byte sentinel is a
   mandatory check.
5. Compare all inherited Weapons code/data/BSS/linkage/COMMON records and
   forbid surplus helper emission.  The Fable handler itself introduces no
   new literal or inline-helper owner in the saved object.

This trial corrects a genuine schema-owner defect; it is not a spelling or
placement retry of the rejected surrogate.

## Export/helper atomic pair

`object_types.obj` has the genuine current data-table reference to public
`weapon_export_function_values`, whose declaration already belongs to
`weapons.h`.  The export has two same-TU calls to the private helper.  January
names that helper only as `_code_000ead20`; the HCEA/PDB-backed Weapons ledger
supports:

```c
static real weapon_trigger_get_charged_fraction(
    long weapon_index,
    short trigger_index);
```

If admitted, the in-place target metadata entry at file offset 961824 must be
renamed to `_weapon_trigger_get_charged_fraction` and marked static.  Do not
export it or compile it without its real caller.

The January export switch performs `dec` followed by an unsigned compare with
15, proving explicit behavior for source values 1 through 16 and default-zero
behavior outside that range.  The published HCEA header supplies the familiar
function-mode names, but its provenance comments contradict each other: it
both calls a database enum exact and calls the reconstruction a guess, and it
adds values 17/18 that January does not handle specially.  Therefore Fable's
TU-local truncated anonymous enum must not be treated as a proved shared
owner.  Before the pair is compiled, root should authenticate the underlying
enum record or explicitly record values/names 0..16 as January behavior plus
cross-build naming evidence.  The semantic owner belongs in
`weapon_definitions.h`, not as a private duplicate in `weapons.c`.

The prior `__real@3d088889` blocker has changed.  The new helper's 4-byte
selection-2 owner has normalized hash
`c5fd575aaff8d1a1afc8a3410d0e195364aef9d29ec13f623d0c37506f183b2b`.
The January tree selects the same payload in `source/ai/actions.obj`, and the
current canonical tree contains 18 compatible selection-2 copies.  This
supports a fresh function-level trial; it does not make the donor object a
whole-object match or pre-credit its data.

The export body also calls `game_time_get`; its declaration belongs to
`source/game/game.h`, which the Fable Weapons TU does not include.  A future
pair must add that real owner include.  Keeping the implicit declaration or
copying a local prototype is not acceptable.

## Held owners

### `weapon_aim`

The 272-byte body is semantically credible.  January's two callers are both
inside the still-unwritten `_actor_combat_update`; both pass nine arguments,
clean 0x24 stack bytes and consume AL.  The recovered public signature is:

```c
boolean weapon_aim(
    long weapon_index,
    short trigger_index,
    real_point3d const *origin,
    real_point3d const *target_point,
    boolean lob,
    real_vector3d *result_aim_vector,
    real *result_ticks,
    real *result_distance,
    boolean *result_linear);
```

`projectile_aim` is already declared by its real owner.  The Fable body
correctly bounds the trigger, forwards the typed projectile definition,
validates the result vector, and returns true only for a valid trigger.

It still requires a `weapons.h` owner declaration.  The recent coherent
four-API ActorCombat prerequisite trial included this exact declaration and
regressed protected `_unit_preprocess_node_orientations` by 1,919 meaningful /
1,920 padded bytes.  Because that packet also contained three other APIs, the
trial does not isolate `weapon_aim` as the cause; it does prove that the shared
owner boundary is not currently cleared.  No bisection or declaration-order
experiment is warranted.  Reopen with the real ActorCombat caller closure or
new dependency evidence.

The natural body also emits selection-2 `_magnitude_squared3d` (48 bytes),
`_valid_real_normal3d` (96 bytes) and `_valid_realcmp` (64 bytes), plus its
assert strings/constants.  Every one has same-byte compatible selected owners
in the current canonical census; none is a NODUP conflict.  They nevertheless
remain additive candidate owners requiring a full owner audit and prevent a
whole-object-match claim.

### `weapon_place`

The actual object-type contract is
`void (*datum_place)(long, struct scenario_object_datum *)`, and
`object_types.c` retains the corresponding `void weapon_place(...)` declaration
and data-table reference.  HCEA also describes a `void` function.  January's
final `mov eax, weapon_index` is consistent with harmless register residue;
it does not authenticate a `long` API.  The Fable long-return edit is therefore
inadmissible even though its code is exact.

The source also substitutes `byte unused0[32]` for the real
`scenario_object_permutation` at +0x28 and uses renamed local flag constants.
The authentic 92-byte scenario record has rounds at +0x48/+0x4A, flags at
+0x4C, and the published bits `_weapon_created_at_rest_bit=0`,
`_weapon_obsolete_bit=1`, `_weapon_does_accelerate_bit=2`.  A future natural
`void` residual could be considered only with that complete record and the
generic placement callback ABI reconciled.  It receives no credit from this
Fable exact packet.

### `animation_update`

The body is an exact one-call wrapper to `animation_update_internal(TRUE, ...)`,
but January has no incoming reference.  The target leaves EAX untouched after
the call, so both `void` and passthrough-`short` source forms can emit the same
bytes.  Cross-build evidence is split.  Do not add an unused declaration to
`weapons.h`; an earlier unused Weapons API declaration already demonstrated
that this header can perturb protected Units.  Reopen only with an authentic
caller or stronger original type evidence.

## Whole-object and saved-artifact accounting

The Fable object has 60 function owners versus 50 current and 79 target.  Its
ten new code owners are the six exact claims, residual
`weapon_can_be_fired`, and the three natural Aim helper COMDATs.  All inherited
canonical code and runtime-data section contents/owner properties are
unchanged in the saved comparison.

It has five new local runtime-data owners (77 raw bytes total): two Aim assert
strings and constants `1/30`, `0.05f`, and the validation double.  All have
same-payload selection-2 owners elsewhere in current canonical.  They are
pooled compatibility evidence, not automatic data credit.

The saved Fable object also carries an unrelated candidate-only 1-byte COMMON
symbol `_profile_global_enable`, inherited from that donor worktree's stale
Profile header state.  Current canonical `profile.h` declares the real owner
externally and current Weapons has no such COMMON.  This alone makes the Fable
object unsuitable for staging; every accepted source subset must be rebuilt
under current headers and audited from scratch.

No claim here admits the whole Weapons object.  Exact function progress may be
retained only after current-context source compilation, complete inherited
owner preservation, pooled-owner checks, no surplus helpers/COMMON, the
protected Units sentinel, and the normal no-point guard.

## Durable evidence

- Full census script: `scratch/fable_weapons_packet_readonly_audit_20260905.py`,
  SHA-256 `0098f50fed46d1784b822004ee97cae111bd682df2dcac114b64cba548018f3f`.
- Full census JSON: `scratch/fable_weapons_packet_readonly_audit_20260905.json`,
  SHA-256 `14fb992d732bd7f0da21b4d06858ebefccb32adb9b5fcc06fd1bc5e187c2787b`.
- Independent linkage/provenance cross-check:
  `scratch/weapons_fable_owner_provenance_independent_audit_20260905.md`,
  SHA-256 `ad53f98aa00ea2ce63d887f35d5781232d17f8ce4a2a4375fde20abdc024d963`,
  with script `scratch/weapons_fable_owner_linkage_audit_20260905.py`,
  SHA-256 `ee560412cdcc67d4cd3ec619bf4136d4c161cca681b842e87b947c750e6602b7`.
  That cross-check independently found the missing Equipment owner include,
  caller graph, ABI holds and stale COMMON.  It proposed retaining the 28-byte
  record TU-locally; this review deliberately does not adopt that narrower
  shortcut because the earlier campaign rejection was specifically an
  unauthenticated layout-only surrogate and the complete tag subrecord now has
  a genuine `weapon_definitions.h` owner.
- Handler rejection history:
  `docs/object_matching_logs/claude_wave_small_exact_batch_20260831.md`
  (`1859a459...`) and
  `docs/object_matching_logs/opus_wave_canonical_reconciliation_20260902.md`
  (`92fd3e4e...`).
- Export/helper history:
  `docs/object_matching_logs/weapons_obj_jonas_safe_trio_and_charged_export_blocker_20260901.md`
  (`0632d696...`).
- Animation history:
  `docs/object_matching_logs/weapons_obj_jonas_animation_update_first_shot_20260826.md`
  (`11fc4c53...`).
- Aim/API boundary:
  `docs/object_matching_logs/actor_combat_obj_caller_closure_api_boundary_20260905.md`
  (`5e975a3e...`).
- Private helper/name boundary:
  `docs/object_matching_logs/weapons_obj_jonas_zoom_packet_20260831.md`
  (`68e1f22c...`).
- Authenticated ammunition record:
  `research-cache/halocea-full-blobs-20260830/src/headers/weapon_ammunition_object.h`,
  SHA-256 `42376e7a0a056d6e8c39201aeb8c2b9b989462a82fe3864ec229ccd2329a0580`.
- Cross-build function-mode header (provenance caveat above):
  `research-cache/halocea-full-blobs-20260830/src/headers/weapon_export_function_mode.h`,
  SHA-256 `61ddd54f8c7778fb203677b9d3cfb7ccf1834950a49c299c553feab052d1c3e2`.

This was a read-only reconciliation.  No canonical, donor, header,
configuration, build, or admission file was modified, and no source candidate
was compiled.
