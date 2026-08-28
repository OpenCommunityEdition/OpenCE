# `game_sound.obj` Jonas public leaf pair (2026-08-28)

## Scope and authority

- Integration base: `d77b090a375439e0dce06640feea4b27d25047d8`.
- Isolated branch: `jonas/game-sound-public-leaves-20260828`.
- Target owner: `build/split/source/sound/game_sound.obj`, 17,314 bytes, raw SHA-256
  `d69fd3772b48a1d483d0c80af8703210b85bc65d2dbdda1402c0e542ad54168`.
- Retained candidate owner: `build/base/source/sound/game_sound.obj`, 4,351 bytes, raw SHA-256
  `fa3bf486d7df04583f98a4baeccb8e01dc3e5d2c0e6d24da2b21b71a641b6d07`.
- Compiler: XDK 3911 Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
  `xbox/bin/vc7/CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

January PC COFF is authoritative for the body, padded extent, complete ordered relocation
schedule, symbolic destination/addend, ABI, and linkage. The applicable historical root
`CLAUDE.md` and `AGENTS.md` resolve to the same 53,972-byte blob
`1dfb833ebea9fd0fe7dfd880e45e879cc395a41a`; neither file is present at the integration base.
The existing `game_sound_obj_jonas_scripting_leaf_wave_20260821.md` ledger was read in full.
Its prior one-shot rejection and no-retry decisions remain binding.

The initial direct census was **8/31 exact**, zero emitted residuals, and 23 unwritten owners.
This lane changes only `source/sound/game_sound.c` and this ledger. Units, Vehicles, and all
Claude-active work were read-only.

## Donor audit

The pinned HCEA tree is commit `c168af2e747d3095d9a29418ae401f3a39544863`. The complete
`game_sound*.c` family and its data owner were read before editing:

| HCEA path | Git blob |
|---|---|
| `src/data/game_sound_globals.c` | `7d3ce1d31ed42d6704765449119cecf96e46ec60` |
| `src/game_sound_clear.c` | `c2cccbde0bfb28bbca1a3a87d3ef45d9cf654b3f` |
| `src/game_sound_dispose.c` | `79fdff97ec8da516bbb0a11dc4164819910e1d10` |
| `src/game_sound_dispose_from_old_map.c` | `2848eeafbbee862b5a027fe67b8c4ec8095b1f89` |
| `src/game_sound_initialize.c` | `34faae5b7de39c78f3e193d1b7081ac8b22c5837` |
| `src/game_sound_initialize_for_new_map.c` | `91521f18988ebbbe90cbbcd80330e4b4184fb75c` |
| `src/game_sound_restore.c` | `aca7e35ebf435801cfde94f781874402c1721f71` |
| `src/game_sound_set_mouth_aperture.c` | `3362dd38b8272fec56aa5fd3a0ef96af4b4d64df` |
| `src/game_sound_update.c` | `d5b65bc04fedd0ec474b9b62969142fa8c130e4b` |

The additional retained wrapper donor is
`src/unattached_looping_sound_start.c`, blob
`9c7400a935947d7022223703523b6755cf211eb6`. Relevant HCEA looping, scripting,
audibility, impulse, and tracking bodies were also read to map the anonymous January owners.
HCEA is semantic and topology evidence only: its later globals are 12 bytes rather than the
January PC 8-byte owner, its unattached impulse wrapper has an extra player argument, and its
mouth-aperture reconstruction explicitly omits the final store. January disassembly and the
repository's typed `unit_set_mouth_aperture(long, real)` declaration prove that omitted PC call.

## Frozen one-shot batch

Exactly two natural public C bodies entered the only code-producing batch. Both passed the
first strict target-owner function gate, so the first result was frozen without spelling,
statement-order, or scheduling experiments. There were no rejected batch bodies to retain or
prune.

| Owner | Meaningful / padded | Relocs | Target normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_game_sound_set_mouth_aperture` | 50 / 64 | 3 | `72da84019cfcee377a62b9200b6cdef1090a48c4add47c3b49008c6274de2dae` | retained exact |
| `_unattached_looping_sound_start` | 81 / 96 | 5 | `529b4262e939e325502fff81cbae346b9fb0c5d775046a80d159f8a1b0948412` | retained exact |

`game_sound_set_mouth_aperture` gates on the typed data array's `valid` byte, verifies the
object against `_object_mask_unit`, and calls the typed unit setter only for a live unit.
`unattached_looping_sound_start` performs the target's looping-definition validation, calls
`game_looping_sound_new(period, definition_index, "", NONE)`, and on success sets the typed
datum's unattached bit and scale before returning its handle.

Every addend below is zero:

| Owner | Offset | Type | Symbolic destination |
|---|---:|---|---|
| `_game_sound_set_mouth_aperture` | `+0x04` | DIR32 | `_game_looping_sound_data` |
|  | `+0x17` | REL32 | `_object_try_and_get_and_verify_type` |
|  | `+0x28` | REL32 | `_unit_set_mouth_aperture` |
| `_unattached_looping_sound_start` | `+0x0E` | REL32 | `_tag_get` through `looping_sound_definition_get` |
|  | `+0x18` | DIR32 | `??_C@_00CNPNBAHC@?$AA@` (natural empty string) |
|  | `+0x1F` | REL32 | `_game_looping_sound_new` |
|  | `+0x2F` | DIR32 | `_game_looping_sound_data` |
|  | `+0x36` | REL32 | `_datum_get` |

The retained source is Git blob `0d64d0e0e957db61fa3621d05fa252efafcd7b64`, stable
payload SHA-256 `2ec8c98c0981c64f344236074e03f4ae3ec39ce726ba424101b5ba90598e408b`.
The integration-base source blob was `add1a0275589ce5599d6c4674c8f94f86e69c963`.

## Complete January owner map

The retained-only census is **10/31 exact**, zero residuals, and 21 unwritten. Exact padded
coverage is 656/5,488 bytes. On the established instruction-byte measure, the pair contributes
131 meaningful bytes and advances exact meaningful coverage from 432 to **563 bytes**.

| Exact owner | Padded / relocs | Normalized SHA-256 |
|---|---:|---|
| `_game_sound_initialize` | 48 / 6 | `3b92776ef4273f11a60fc1934c1a625a93fe5be16252a9bfaec65dd74e391d7e` |
| `_game_sound_dispose` | 32 / 2 | `9f45cfb021d5ffe8cd8542ddf04458267cd6ec4acb16076bad891d40b79d86a8` |
| `_game_sound_initialize_for_new_map` | 48 / 4 | `97a9f46eea567eca337dcd49f810e577a412486df2a36619fd4acdc7441bd75c` |
| `_game_sound_dispose_from_old_map` | 48 / 4 | `9247d1f41f4f487362c1c26d6deb3fe66f3aec782389698eb8bc14b2263780ac` |
| `_scripted_sound_stop` | 80 / 2 | `52780e1efb4c3c4ceb3af7dc28ff32e089e57be67e55cb9e69d5b0a48927896f` |
| `_scripted_looping_sound_set_scale` | 112 / 7 | `fa6856d6b0ce053c1e677fa1aa7f0979bbd3f5fb4511f3364c3bab5ae91228c1` |
| `_scripted_looping_sound_set_alternate` | 80 / 3 | `fcd390aac586d07238dfa2c8685ebf4db4f1eddf966aa5432c2a2c6689607fc5` |
| `_unattached_looping_sound_stop` | 48 / 2 | `72f38fecd99b2db3361aec94f7258d264cbc3ed79151cf9955663269a59a93c0` |
| `_game_sound_set_mouth_aperture` | 64 / 3 | `72da84019cfcee377a62b9200b6cdef1090a48c4add47c3b49008c6274de2dae` |
| `_unattached_looping_sound_start` | 96 / 5 | `529b4262e939e325502fff81cbae346b9fb0c5d775046a80d159f8a1b0948412` |

All 21 unresolved owners are mapped below. The policy column is also the no-retry boundary for
this bounded leaf lane.

| Target owner | Padded / relocs | Normalized SHA-256 | Mapping and policy |
|---|---:|---|---|
| `_code_001b7290` | 624 / 25 | `8fe3a4a813d3a2a8e45a5ce10b8cbd71274a8553f11e8d43ca65fa95c63dd7dc` | `update_potentially_audible_looping_sound`; large state machine with transforms, velocities, refresh/delete paths, asserts, and global frame ownership. |
| `_code_001b7630` | 240 / 10 | `ecbde409a2d5415d4393356bfcfb672ac00268fd83c3ab10ce67ef80e46d97fb` | `compute_combined_pas`; owns/updates external `combined_pas` and observes BSP/player/camera state. |
| `_code_001b7720` | 112 / 6 | `2a9fb7234670d74e0373cb40f8370dbbcf7f369bc3310a24dd552f5adafe0885` | `location_potentially_audible`; depends on `combined_pas` storage ownership and an assertion path. |
| `_code_001b7790` | 112 / 5 | `4b07f0697590fd4256d4855f7eb9ef59d6f2517a904b6c0f1c699380a4762785` | `scripted_looping_sound_stop_internal`; January definition index arrives in `EAX` with a stack byte, a private ABI that ordinary public C must not force. |
| `_code_001b7800` | 96 / 3 | `af2569afae3a8287ecb8173844956172e074cacfcd45c84c48483a768dd9e7b1` | `looping_sound_definition_is_music`; private `EAX` entry plus unresolved January track layout. |
| `_code_001b7860` | 208 / 12 | `14fac40d64220ba7c1e08386be845e88dc0d781ea6a71d3ed740e0b368409c00` | `scripted_music_stop_all`; iterator/state body dependent on both private anonymous helpers. |
| `_compute_sound_obstruction` | 512 / 33 | `2040bfc2f77a485e4461abbeaa0b724c9d53feba4169d45f605c53ff3fd29dc5` | Large collision/PVS routine with collision-user global ownership and math/storage sensitivity. |
| `_game_looping_sound_delete` | 160 / 15 | `4c70667fcc6a9ea2126e74af05fdd61160ad78f4ed775ab6610aac4cc46ba955` | January state/assert/unlink behavior is materially richer than the HCEA datum-delete donor. |
| `_game_looping_sound_new` | 256 / 9 | `06177f333870e3c78b07329cc92cd24a2b45b624f36de6d7645097090904ea84` | Allocation/marker/state initialization at the batch ceiling; stateful and not a low-risk leaf. |
| `_game_sound_clear` | 176 / 13 | `5f0d2318a4ba045f8ee5aa090d9e01361891afedd6a2a6077d0c40fefbb20ac5` | Iterator, tag runtime-link, assertion, and datum state mutation. |
| `_game_sound_restore` | 208 / 13 | `564da8f217b03a2526989be7b079acd2bc27aa6b92cc308ba04ef79d8f27d4e7` | Two iterator families and runtime tag/datum relinking; later donor layouts diverge. |
| `_game_sound_update` | 464 / 31 | `d0c2dc1708f1dbee75a025bb0a2317afdcd65682faf8c301ceb879cdddeabf92` | Large frame/environment/background-loop state machine with anonymous dependencies. |
| `_object_impulse_sound_new` | 272 / 13 | `7e56d2de25b44ab99b2a6156c1a8e87facab541262d08058a1ab4ec35ed42e9d` | Over the bound and crosses the private sound-source/callback boundary. |
| `_scripted_foley_predict` | 176 / 6 | `9af3986f865faf1798043e1dd3e7165b25b16f741561760bc2b4399622dc45b6` | Public semantic leaf, but January looping-track/tag-block layouts are not typed in this repository. |
| `_scripted_looping_sound_start` | 144 / 10 | `f926ba0801df533399c0e6f0248d94645ce860fe48ada6432a41db9399f7e769` | Stateful wrapper dependent on private stop/music helpers and runtime scripting ownership. |
| `_scripted_looping_sound_stop` | 96 / 5 | `562bb32e0aed130524f454f6ec41225f380d1a3763fa8bd48fad9f220640d1ba` | January inlines private datum mutation; prior ledger excludes it from ordinary wrapper retry. |
| `_scripted_sound_new` | 352 / 12 | `4c5fbb78507d640978df233b21eeffce0c7dc3ee49f37bbc4a362055a140eb51` | Large branching object/unspatialized creation with union/decompiler and state risk. |
| `_scripted_sound_time` | 80 / 2 | `b311cf680fca7012212c635bb5023141e459da44b0adafb0e0ef9c6025f26e25` | Prior one-shot natural donor rejected (`ba9cdbb0...` candidate); do not retry without new authenticated source evidence. |
| `_track_object_impulse_sound` | 240 / 14 | `17b7fdeb84c738eb5e9a2bc9415742d42134e4b95f232032125c8fe8af1fb85d` | Private callback ABI with attachment/source layout, asserts, transforms, and velocity outputs. |
| `_unattached_impulse_sound_new` | 176 / 11 | `32e69b594ab844b2c84c8003e3c72c6458e96314f7f34595f1cac9388f15c598` | HCEA has a divergent fourth argument; local sound-source layout and `sound_new_impulse` callback prototype remain private. |
| `_unspatialized_impulse_sound_new` | 128 / 7 | `f4345f193ed2591ec9e3ca36e57faf2904262f694823c9f092a61f48ad2ed4e1` | Same unresolved private sound-source and mixer ABI despite simple surface semantics. |

## Retained-only replay, ownership, and gates

The first batch contained only the two retained bodies, so the normal rebuild is also the
retained-only replay. Direct comparison reconfirmed all ten accepted function COMDATs and all
38 ordered relocation records. The pre/post exact-set audit reports no lost exact owner and
exactly the two intended newly exact owners.

The candidate continues to declare only the inherited four-byte COMMON symbols
`_game_sound_globals` and `_game_looping_sound_data`; it defines no writable `.data` or `.bss`.
The natural empty literal is a one-byte select-any `.rdata` COMDAT. January relocates to the
same external literal symbol and addend; the candidate-local COMDAT receives zero target-data
or object-completion credit. The inherited `0.0f` and `1.0f` compiler COMDATs are unchanged.

Final checks:

- `tools/campaign/gate.py source/sound/game_sound --all`: 10 exact, 0 residual, 21 unwritten.
- Hardened direct COFF comparison: all ten accepted owner extents, normalized bytes, and
  symbolic relocation schedules exact.
- Pre/post exact-set comparison: `lost_exact: []`; newly exact only
  `_game_sound_set_mouth_aperture` and `_unattached_looping_sound_start`.
- Complete Ninja build: pass, 572 actions. Semantic report: 470 units, 4,645 functions
  evaluated, 4,514 semantic exact, 125 hidden exact / 72,363 hidden code bytes,
  4,541 accepted exact, zero unit errors.
- Whole-project progress/ownership validation: pass; the only reported strict ownership
  validation is the pre-existing `source/objects/objects:.bss` record.
- `git diff --check`: pass.

No assembly, `volatile`, `register`, pragma, intrinsic, optimizer barrier, undefined behavior,
raw address/offset, representation pun, byte forcing, private-ABI forcing, config change,
semantic exception, or completion-label change is used. Nothing is committed or pushed.
