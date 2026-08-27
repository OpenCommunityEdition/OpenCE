# `items.obj` inventory, position, and detonation recovery (Jonas, 2026-08-27)

## Result

This wave recovers three adjacent public owners under the typed-access and
explicit-return house rules:

- `_item_in_unit_inventory`: 159 meaningful / 160 padded bytes, four `REL32`
  relocations, normalized SHA-256
  `90a20f98567c9dcafe8b25d08bf32fdacb4c2ff2b3d0dd3cd1782619ef235ba9`;
- `_item_get_position_even_if_in_inventory`: 131 meaningful / 144 padded
  bytes, four relocations, normalized SHA-256
  `dd964a40757dc634a08d9e17b0bf8461f33018fff9b9554853dfd53fcad9bdec`;
- `_item_detonate`: 140 meaningful / 144 padded bytes, seven relocations,
  normalized SHA-256
  `921ef25c8a9b68d4adcf69128f437ff91b891b4460161bc9e6928cb3403d20fb`.

`items.obj` advances from 10/18 to 13/18 exact owners. The wave adds 430
meaningful code bytes and three functions without changing the complete-object
count. The containment census reports `lost_exact: []`.

## Frozen evidence

- Target object raw SHA-256:
  `D7C23F2AC171FBEDC9C2FBDFA3053D663145D69B3DC19D402BB8080379C4314C`.
- Pre-edit source blob: `1a666cd595726448cb8d57ddeed58d8cea2b9563`.
- Pre-edit source raw SHA-256:
  `CAE4A7CA55A3C88878F425AF9720761041FC1E4ABD6F0C376A745E340166F90E`.
- Pre-edit header blob: `9b4385ca01e06c0e151696cb336b9bbf8abeba7a`.
- Pre-edit header raw SHA-256:
  `0D1CA7FDA842A5B00D5791BD63F57B35C14EBA638A5D1B6220657D9B63B9C383`.
- Frozen baseline:
  `build/audit/items_inventory_position_detonate_baseline_20260827.obj`.
- Baseline raw SHA-256:
  `5C694F1E477211E83AB649D1E1E9A85BF5BEF494A2CFAB17D40903A0C92C467A`.
- First code-producing candidate:
  `build/audit/items_inventory_position_detonate_first_shot_20260827.obj`.
- First-candidate raw SHA-256:
  `5AA6513472B218EB0FED2D71AA9783E6E41973C7A29E3C12CF7218036EC88DE7`.
- Corrected candidate:
  `build/audit/items_inventory_position_detonate_cfg_corrected_20260827.obj`.
- Corrected-candidate raw SHA-256:
  `8850535BA0D629C35F80346022F0558572176D224F4C76E6D1778C2462DBF51D`.
- Retained source raw SHA-256:
  `DE474027A25B73F6A4B46DAF6AE1FD177CE15B495A77926D78158693CB083949`.
- Retained header raw SHA-256:
  `4A471E380ED81AF88DA095CE374F295550B0B309004FD1552543DD87B47650E7`.

## Source authority and topology

The January Xbox split object is the final byte and relocation authority. The
canonical bodies in `build/audit/refs/stian-halo/src/halo/items/items.c` and
the focused HCEA donors under `build/audit/refs/halocea/src/blam/items/`
establish the field semantics and call families. HCEA's later inventory
variant is not used where it conflicts with the January control flow.

The first code-producing candidate exactly recovered the position and
detonation owners. Its inventory body had the correct 160-byte extent and all
four relocation identities, but placed the detach case first, shifting the
last three relocation addresses from target `33, 93, 129` to
`52, 112, 148`. Target disassembly instead falls through the attached-unit
path, returns from it, and places detach cleanup at the late label. That new
frozen evidence justified one correction: a positive owner test with an
explicit return inside the attached path and a terminal return after detach.
The corrected candidate reproduces the target CFG and relocation addresses.

The retained implementation uses `item_get`, `item_try_and_get`, `unit_get`,
`player_get`, and `item_definition_get`. It contains no raw typed object/tag
casts, address reconstruction, compiler barriers, or byte-forcing construct.
All declarations and terminal returns follow the project house style.

## Strict comparison and containment

`python -m tools.coff_compare` reports `all_equal: true` for all three owners,
including padded section bytes and relocation address, type, identity, addend,
and ownership. The pre/post ordinary census is 10 to 13 exact owners with only
the three named additions and no loss.

## Repository gates

- `build\local-tools\ninja.exe`: pass.
  - 470 semantic units scanned;
  - 4,373 functions evaluated;
  - 4,228 semantic-exact and 4,288 accepted-exact functions;
  - zero unit errors;
  - Campaign: 377/833 complete objects, 4,262/11,060 exact functions,
    513,166/2,198,102 code bytes;
  - Halo: 275/468 complete objects, 4,095/7,574 exact functions,
    500,252/1,770,166 code bytes.
- `python -m tools.audit_object_admission`: pass, 0 candidates,
  0 contradicted, 0 revoked.
- `python -m tools.parked_functions`: pass, 3 active, 0 stale, 0 invalid.
- `python -m pytest -q`: 179 passed; only the existing cache-directory
  permission warning remains.
- `git diff --check`: pass.
