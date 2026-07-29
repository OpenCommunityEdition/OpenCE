# `game_engine_multiplayer_sounds.obj` exact-match record

## Scope and provenance

- Translation unit: `source/game/game_engine_multiplayer_sounds.c`.
- Target: January 2002 `cachebeta.exe`, regenerated with `csplit`.
- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1.
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`.
- The queue layout is proven directly by the target relocations: a four-byte
  count at `0x0043EB78`, followed by five eight-byte queue entries at
  `0x0043EB7C`, for a 44-byte `.bss` span.

## Validated final state

| Symbol | Size T/B | Relocs T/B | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_code_000a1460` | `0x60/0x60` | `5/5` | `35efa2a712635ba63cbcfad223f9f9fc59a1f7adb0981e68f79be0d5c3c3c59c` | exact |
| `_code_000a14c0` | `0x20/0x20` | `4/4` | `aaec35eff7ba9a47045f8b81d2a9e3361a9f4fef9653ef8d19eb74c085cd2782` | exact |
| `_game_engine_update_multiplayer_sound` | `0x50/0x50` | `7/7` | `545d5026ecf0b102b59c4b21a263a05959c79af9b9d75fadb4e907c737f9df3d` | exact |
| `_code_000a1530` | `0x80/0x80` | `5/5` | `865e33e437559e17720e2e5d8f37dfb4e0358aae7a5b2b4bc08e5fe8b16ea014` | exact |
| `_game_engine_play_multiplayer_sound` | `0x50/0x50` | `7/7` | `b8e9ab234a811ecab69c9770047f777fd309b902f0132176278c2c4f48da8660` | exact |
| `_game_engine_intialize_queued_sounds` | `0x30/0x30` | `5/5` | `0c010958b07fb19cc374e33db5b6a5fc4e5ae01bfdb6bf84ddeca587b169e483` | exact |
| `_data_002de530` | `43/43` | `0/0` | `6299be18ce0a7352f82be001637f101d51112c013d5be331393f08eb20455e8b` | exact |
| owned `.bss` | `44/44` | `0/0` | `85759b3811ff7dc47b03792ac85317be51431a3f9e01dcafce317ed736a391b0` | exact layout and ownership |

The target originally exposed the whole anonymous queue through the generated
label `_bss_0043eb78`. The rebuilt code proves two non-aliasing source objects:
the count at offset zero and the entries at offset four. A second csplit label,
`_bss_0043eb7c`, records that proven boundary. After regeneration, relocation
destinations and addends match directly rather than relying on a semantic
allowlist.

## Accepted controls

- Model the count as a one-field structure and the entries as a separate
  array. The field form preserves the decrement flags in
  `game_engine_update_multiplayer_sound`; the separate array gives VC7 the
  non-aliasing proof required for SIB-indexed entry stores.
- Keep both objects externally linked and explicitly zero-initialized. VC7
  then emits them in source order at offsets zero and four. Static linkage
  reordered them and did not reproduce the target layout.
- Retain scalar entry stores and the original queue-count increment. These
  produce the target private calling convention and instruction order in both
  the helper and its caller.
- `_code_000a1530` retains its two source return sites as a documented
  house-style exception: the January target itself has distinct failure and
  success `ret` instructions at offsets `+0x47` and `+0x72`. Converting it to
  a single source return would change the proven target topology.

## Experiment matrix

Hashes not preserved by older exploratory lanes are intentionally omitted.

| ID | Source shape | Size / relocs result | First meaningful result | Siblings | Decision |
|---|---|---|---|---|---|
| E01 | Aggregate queue baseline | update exact; helper `0x20`, `4`; play `0x50`, `8` | helper used `shl index,3`; play reloaded count | four exact | superseded |
| E02 | External scalar count plus static entries | all six code hashes exact | `.bss` emitted entries at `+0`, count at `+0x28` | code exact | rejected: wrong layout |
| E03 | Two static objects, both declaration orders and explicit-zero variants | helper/play exact; update gained `test eax,eax` | `.bss` still entries at `+0`, count at `+0x28` | mixed | rejected |
| E04 | External count plus external entries, scalar count | all code except update exact | update inserted `test eax,eax` between decrement and store | five exact | superseded |
| E05 | Duplicate count stores in the update's zero/nonzero branches | update `0x60`, `8` versus target `0x50`, `7` | separate zero path emitted immediate store | five exact | reverted |
| E06 | One aggregate object viewed through an offset-derived entries pointer | update exact | helper returned to `shl index,3`; play gained one relocation | four exact | reverted |
| E07 | Increment index between the two entry stores | helper `0x20`, `4` | SIB addressing appeared, but increment stayed between stores | other exact baseline functions retained | reverted |
| E08 | Increment index after both entry stores, then assign count | helper `0x30`, `4` | VC7 created a frame and a second live index register | regressed helper/play | reverted |
| E09 | One-field count structure plus separate external entries array | all six target sizes, hashes, relocation counts, addresses, types, destinations, and addends exact | none | `6/6` exact | retained |

## Do not repeat

- Do not combine the count and entries into one C aggregate: VC7 then treats
  their aliasing as possible and destroys the live index with `shl`.
- Do not use two source-static objects; this compiler emits the larger array
  before the count in `.bss`.
- Do not duplicate the count store across the zero/nonzero update branches.
- Do not force the result with `volatile`, assembly, undefined aliasing,
  linker tricks, byte patches, pragmas, or compiler-flag changes.

## Residual classification and disposition

There is no residual. The former mismatch was a source ownership/alias model,
not a scheduler or register-allocation tie. The complete translation unit is
eligible for `Matching` after the regenerated target, consolidated build,
semantic audit, and whole-object regression checks pass.
