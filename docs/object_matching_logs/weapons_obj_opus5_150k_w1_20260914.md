# weapons.obj — opus5 150K house-clean lane, wave w1 (2026-09-14)

## Disposition

- Landed in `source/items/weapons.c`: **`_animation_update` strict EXACT** (32 padded / 27 meaningful, 1 relocation
  REL32 @0x12 -> `_animation_update_internal`, normalized SHA-256 prefix `1bba085c2f0d1d5e`, equal to January).
  Diff: one 8-line definition beside the existing `animation_choose_random_permutation` wrapper.
- Real-file gate `== exact 74 residual 4 unwritten 1 (of 79)` (baseline `== exact 73 residual 4 unwritten 2`),
  `_point_from_line3d` guard passed, no baseline-EXACT row lost, every other function's size/relocations/normalized hash
  byte-identical to the HEAD object (residuals included), park `_weapon_can_be_fired` unchanged (144/5), owner census:
  only `_animation_update` new (January defines it), fake scan 0 leads.
- `_weapon_place`: **owner-blocked**. Complete void body measured (208/4, residual only from the return register);
  owner-header packet proposed with a consumer blast measurement. Not landed.
- Not touched (no new evidence / brief exclusions): `_weapon_update`, `_trigger_create_projectiles`,
  `_weapon_magazine_finish_reload`, `_weapon_can_be_fired` (parked).
- Object remains `NonMatching`.

## 1. Scope and provenance

- TU `source/items/weapons.c`; target `build/split/source/items/weapons.obj`; XDK VC7 CL 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`.
- Evidence: January COFF bytes/relocations (alndiff, tinfo), an xref census over every January split object, a
  build/base COMDAT-emission census, January object_type_place and sibling `*_place` disassembly, HCEX Jun-2011 SHIP PDB
  (DIA2Dump `-compiland` / `-type`, read-only: parameter and UDT/enum names and layouts), HCEA
  `blam/items/weapon_place.c`, `blam/models/animation_update.c`, Stian `projectiles.c` FUN_000face0 (Ghidra), prior
  ledgers listed below. Worker notes: `scratch/workers/weapons.md`.
- Ledgers read: weapons_obj_codex_checkpoint, export_owner_reconciliation/schema_rejected, fable_inventory_reconciliation,
  fable_packet_intake, historical_packet_reconciliation, jonas_animation_update_first_shot, interface_state, random (2),
  safe_trio, total_rounds (2), zoom_packet, opus5_100k (+waveC), weapons_owner_packet_source_park,
  first_person_weapons render_only_wrapper; the 100K consolidated handoff.

## 2. `_animation_update` — return-type evidence and landing

Prior holds were only about `void` versus `short`: the wrapper leaves EAX as callee residue and nothing references it.
New evidence closes that:

1. January xref census: `_animation_update` is defined only in weapons.obj and referenced by no object;
   `_animation_update_internal` is defined in model_animations.obj and called from weapons, units, scenery and
   first_person_weapons. A plain model-animation wrapper would live in model_animations.obj and be called out of line.
   Like its weapons.obj siblings (`animation_key_frame_index` = field accessor, `animation_convert_frame_to_pal` =
   identity) and the header-inline `_power`/`_random`, it is a header `__inline` COMDAT emitted by weapons.c.
2. VC7 emits a non-static `__inline` COMDAT only in TUs that use it (build/base: `_power` 7 objects, `_random` 2,
   `_magnitude_squared3d` 90 of ~560, although real_math.h is included nearly everywhere).
3. The only other `_animation_update_internal` call in January weapons.obj is weapon_update @0x8f with kind 1 (the
   wrapper's fixed kind), followed by `movsx eax, ax; dec eax; je; dec eax; jne` — the result is consumed by a switch.
   The wrapper therefore returns the internal's `short` result. Stian's `void FUN_000face0` is decompiler inference (it
   makes the already-admitted `short animation_update_render_only` sibling void as well). HCEA spells `int16_t`.

Landed form (first natural, plain definition as the repo already models the other three weapons.obj wrapper COMDATs;
internal prototype from the genuine owner `models/model_animation_definitions.h`, already included):

```c
short animation_update(
	long animation_graph_index,
	struct animation_state *state,
	long *sound_index)
{
	return animation_update_internal(TRUE, animation_graph_index, state, sound_index);
}
```

The 2026-08-26 rejection concerned whole-object COFF section renumbering under a then-frozen fingerprint boundary, not
bytes; the current per-function gate, census and park checks are clean.

## 3. `_weapon_place` — owner packet

- January: `ebx = weapon_index` stays live to `mov eax, ebx` before the final branch; the table caller
  `object_type_place` never reads EAX and every sibling placement callback leaves EAX untouched.
- HCEX PDB: `weapon_place(long weapon_index, struct scenario_weapon_datum *scenario_weapon)`;
  `scenario_weapon_datum` {object +0, permutation +0x28, short rounds_total +0x48, short rounds_loaded +0x4A,
  unsigned short flags +0x4C, unsigned short pad +0x4E, unsigned long unused[3] +0x50};
  `scenario_object_permutation` {unsigned long change_colors[4]; unsigned char region_permutations[8];
  unsigned long unused2[2]}; anonymous enum `_weapon_created_at_rest_bit = 0, _weapon_obsolete_bit,
  _weapon_does_accelerate_bit, NUMBER_OF_SCENARIO_WEAPON_FLAGS`. January corroborates the short 16-bit compares at
  +0x48/+0x4A and byte tests of bits 0 and 2 at +0x4C.
- Measured shapes (scratch, TU-local copies only for measurement):

| shape | result | kept |
| --- | --- | --- |
| void body: MIN, SET_FLAG/TEST_FLAG, weapon_get, weapon_definition_get, TAG_BLOCK_GET_ELEMENT | 208/4 sha `609b62aa405c9728`; every alndiff block is weapon_index's liveness (ebx reuse, prologue, missing `mov eax, ebx`) | candidate |
| DIAGNOSTIC: same body, `long` + `return weapon_index;` | strict EXACT | never (rejected ABI) |

- ORCHESTRATOR-PROPOSAL P2 (natural owner, next to `scenario_placeholder_datum`): add to
  `source/scenario/scenario_definitions.h`

```c
enum
{
	_weapon_created_at_rest_bit = 0,
	_weapon_obsolete_bit,
	_weapon_does_accelerate_bit,
	NUMBER_OF_SCENARIO_WEAPON_FLAGS,
};

struct scenario_object_permutation
{
	unsigned long change_colors[4];
	byte region_permutations[8];
	unsigned long unused2[2];
};

struct scenario_weapon_datum
{
	struct scenario_object_datum object;
	struct scenario_object_permutation permutation;
	short rounds_total;
	short rounds_loaded;
	word flags;
	word pad;
	unsigned long unused[3];
};
```

  Consumer blast (every transitive consumer compiled control vs shadow header, per-function size/relocs/normalized hash,
  against the working tree at measurement time; `scratch/workers/weapons/blast.py`, consumers in
  `scratch/workers/weapons/p2_consumers_lf.txt`): 55 units, 53 clean;
  `source/ai/encounters:_encounter_update_respawn` (**parked**, hash `647cc2e7…` -> `32d0a247…`) and
  `source/interface/hud_weapon:_render_weapon_hud` (non-exact; `9eb8bcb1…` -> `343910c8…`) change. Probes: structs
  only -> encounters only; permutation only -> both clean; enumerators appended to the existing anonymous enum -> both
  change; structs with the repo-added `player_starting_location_game_types_offset_assert` typedef removed plus the enum
  in `items/weapons.h` -> 0 deltas over 64 consumers (count compensation; owner judgment); merging the two
  player_starting_location asserts instead -> hud_weapon changes. Declaration-count oracle, as recorded before.
- Candidate for after the packet: `scratch/workers/weapons/candidate_place_void_ownerpacket.c` (current real file plus
  the void body; gated under the shadow headers: 74 exact + weapon_place residual, all other rows identical, park 0,
  guard pass, census only `_weapon_place` new). Zero credit (fuzzy).
- Follow-ups once the owner types exist: bipeds.c TU-local copies (member `unused` vs PDB `unused2`) and scenery.c's
  byte-offset permutation cast could use them (other owners' files).

## 4. Skipped residuals

- `_weapon_magazine_finish_reload` (224/6, sha): the recorded 16-bit add destination tie; HCEX lists only enregistered
  params and HCEA is the later networked body — no new local evidence; no shapes spent.
- `_weapon_can_be_fired` (parked): no new lever; untouched; no reopen proposal.
- `_trigger_create_projectiles`: point_from_line3d form forbidden; non-point forms on do-not-repeat lists.
- `_weapon_update`: brief do-not-spend list.

## 5. Do-not-repeat

- Do not re-debate `_animation_update`'s return type without contrary January evidence (section 2).
- weapon_place: no void spelling can keep weapon_index live; do not spend shapes on it; the long form stays rejected
  unless the owner rules otherwise.
- The P2 packet as written drifts the encounters park; do not land it without a re-baseline or a count arrangement.

## 6. Reopen criteria

- `_weapon_place`: owner packet accepted (land the void body fuzzy), or an owner ruling that January returned
  weapon_index (diagnostic form exact).
- `_weapon_magazine_finish_reload`, `_weapon_can_be_fired`, `_trigger_create_projectiles`, `_weapon_update`: unchanged
  from the 100K lane ledgers.
