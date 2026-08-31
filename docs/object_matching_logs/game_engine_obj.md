# `game_engine.obj` exact-match ledger

## Scope and provenance

- Translation unit: `source/game/game_engine.c`
- Target object: `build/split/source/game/game_engine.obj`
- Candidate object: `build/base/source/game/game_engine.obj`
- Target executable: PAL debug `cachebeta.exe`, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`
- Target object SHA-256:
  `2c74631e4a056a3d8a94c30d6b4d79ba17971cf4b1fc5d97690fb6be1d0e654b`
- Compiler: XDK 3911 Microsoft 32-bit C/C++ 13.00.9254.1 for 80x86.
- Repository flags: `/O2 /Oy- /DDEBUG /Dxbox` and the checked-in include
  paths. No lane-specific flag changes are permitted.
- Canonical source baseline: `e54ead79c643862cab1bbcb5330fcb2084eadb84`.
- Frozen canonical candidate SHA-256:
  `3a184a158241bc52c54685ac0f9d5a969661ad1009cc2b519edbabf83343482d`.
- Strict acceptance oracle: `tools/coff_compare.py`,
  `section_infos_equal`, including padded bytes and semantic relocation
  identities. Every retained batch also checks the complete canonical exact
  set for regressions.

## Validated baseline and current state

Canonical rebuilt baseline:

- Functions: `125/180` strict exact.
- Padded code: `13,904/33,760` target bytes.
- Fifty-three target functions were not emitted by the candidate.
- Two emitted residuals had target-equal size and relocation count, but failed
  because the candidate did not own `_game_engine` and `_global_variant`.

After retained batch GE01:

- Functions: `130/180` strict exact.
- Padded code: `14,304/33,760` target bytes.
- Candidate SHA-256:
  `bae168dc91f6ce452f4ce9d68ac34b27dcaf5e601beca56c99321b88c5568e63`.
- Canonical regressions: none.

After retained batch GE02:

- Functions: `133/180` strict exact.
- Padded code: `14,896/33,760` target bytes.
- Candidate SHA-256:
  `f19b78c05a4b9986cefed66ca1397de4de9dff629365e206ca86c8ad3e831b0e`.
- Canonical regressions: none.

After retained batch GE03:

- Functions: `137/180` strict exact.
- Padded code: `16,496/33,760` target bytes.
- Candidate SHA-256:
  `73519e75b279ec453784dc0ea880561a7e461f15b4bd4804e0b41882e861689d`.
- Canonical regressions: none.

After retained batch GE04:

- Functions: `139/180` strict exact.
- Padded code: `16,816/33,760` target bytes.
- Candidate SHA-256:
  `2b08ed82d77ac468e4530bbfb4ce76726111d94b370f4a3a24dcfbc1b3b5c369`.
- Canonical regressions: none.

After retained batch GE05:

- Functions: `144/180` strict exact.
- Padded code: `17,904/33,760` target bytes.
- Candidate SHA-256:
  `fcfda7a08e0928432e8e9c747774ae3cefa461f164a28d06e5d8fcaf834349d6`.
- Canonical regressions: none.

After retained batch GE06:

- Functions: `144/180` strict exact.
- Padded code: `17,904/33,760` target bytes.
- Candidate SHA-256:
  `1b8182d561b2909de59a43cbcdb081ffa96830b8c0359e96c574f707a39ad4e8`.
- `.bss`: strict exact, `1,148/1,148` bytes, zero relocations, target-equal
  normalized hash.
- Canonical regressions: none.

After retained batch GE07:

- Functions: `152/180` strict exact.
- Padded code: `21,136/33,760` target bytes.
- Candidate SHA-256:
  `293feb40ebe414da8932b76f17f87d79ad17be4a0b54fc55c6deff0c7ab3392f`.
- `.bss`: remains strict exact.
- Canonical regressions: none.

After retained batch GE08:

- Functions: `156/180` strict exact.
- Padded code: `22,256/33,760` target bytes.
- Candidate SHA-256:
  `a9aa7ce0a0b067003b2b63b78f1559d038307a1124d0b4fe6135b45e5e2f9342`.
- `.bss`: remains strict exact.
- Canonical regressions: none.

After retained batch GE09:

- Functions: `156/180` strict exact.
- Padded code: `22,256/33,760` target bytes.
- Candidate SHA-256:
  `ddda326a95808c5a544ebc92886c6ef8526fc1e68f3289c50829984d4df9e1cf`.
- `.data`: strict exact, `32/32` bytes, zero relocations, target-equal
  normalized hash.
- `.bss`: remains strict exact.
- Canonical regressions: none.

After retained batch GE10:

- Functions: `158/180` strict exact.
- Padded code: `22,512/33,760` target bytes.
- Candidate SHA-256:
  `b1c099e6b4152671d352e1d9560e904eb344b2a263422593d4a45f5b36eae9fa`.
- Newly exact: `_code_00097020` and `_code_00096b50`.
- `.data`: remains strict exact at `32/32` bytes with zero relocations.
- `.bss`: remains strict exact at `1,148/1,148` bytes with zero
  relocations.
- Canonical regressions: none.
- `_code_0009d140` is now a policy-clean emitted structural checkpoint at
  `1088/1152` bytes and `70/73` relocations; it receives no exact credit.

After retained batch GE11:

- Functions: `159/180` strict exact.
- Padded code: `22,640/33,760` target bytes.
- Candidate SHA-256:
  `4b669ac72186e8cbbb4b45663254cb5734d23cdf3dccd89df14b3a06e70d7f39`.
- Newly exact: `_code_0009cb60`.
- `_populate_statistic_buffer` remains strict exact at `560/560` bytes and
  `27/27` semantic relocations.
- `.data`: remains strict exact at `32/32` bytes with zero relocations.
- `.bss`: remains strict exact at `1,148/1,148` bytes with zero
  relocations.
- Canonical regressions: none.
- Five policy-clean postgame functions are now emitted as structural
  checkpoints. They receive no exact credit.

After retained batch GE12:

- Functions: `159/180` strict exact.
- Padded code: `22,640/33,760` target bytes.
- Candidate emission: `180/180` target functions; no candidate symbol is
  absent.
- Candidate SHA-256:
  `421a662a208d69aabf33cebbab2e7067540e093b0b37d93434e514fe78a11ae0`.
- Six ordinary-C target-backed bodies are restored as nonexact structural
  checkpoints: `_code_0009a490`, `_code_0009b3a0`, `_code_0009b4f0`,
  `_code_0009bdf0`, `_game_engine_update`, and
  `_game_engine_player_killed`.
- `.data`: remains strict exact at `32/32` bytes with zero relocations.
- `.bss`: remains strict exact at `1,148/1,148` bytes with zero
  relocations.
- Canonical regressions: none.
- This is an emission-completeness checkpoint only. It receives no new
  function credit and the object remains active with 21 strict residuals.

The residual table is measured after GE12. Hashes are normalized SHA-256
prefixes; `-` means the candidate symbol is absent.

| Function | Size T/C | Relocs T/C | Normalized hash T/C | First meaningful divergence |
|---|---:|---:|---|---|
| `_code_00096ba0` | `112/112` | `7/7` | `4ac1ff18279a/9db784535924` | Policy-clean body emitted; register scheduling differs |
| `_game_engine_should_spawn_player` | `256/256` | `9/9` | `8259180402f7/c0782ea3a816` | Control-flow register allocation differs |
| `_game_engine_get_place` | `368/368` | `12/12` | `91161c382c55/bcf2be6ca02e` | Placement loop register allocation differs |
| `_code_000994f0` | `416/416` | `18/18` | `211627377053/10c5400d228a` | Policy-clean body emitted; register scheduling differs |
| `_code_0009a490` | `496/480` | `16/16` | `f30da54283e6/01537066ba3b` | Complete ordinary-C body emitted; local scheduling is short by 16 bytes |
| `_code_0009a680` | `448/448` | `18/18` | `4c90ec184c40/41e698125368` | Policy-clean body emitted; compiler shape differs |
| `_code_0009b3a0` | `336/336` | `11/11` | `7e6f5319e160/dbb1e45ecac4` | Complete ordinary-C body emitted; register scheduling differs |
| `_code_0009b4f0` | `112/96` | `5/5` | `048cb68bcbf1/3285bf46e3e7` | Complete ordinary-C body emitted; target is 16 bytes longer |
| `_code_0009b6e0` | `144/144` | `5/5` | `675e96da950f/bb509add4c20` | One independent terminator store is scheduled before, rather than between, the two HUD call argument pushes |
| `_game_engine_postspawn_player_update` | `336/336` | `15/15` | `4bf0dbc3a0ce/643ef05eed91` | Grenade setup local scheduling differs |
| `_code_0009bdf0` | `992/1008` | `46/46` | `2874ef26f3de/dd76f756771b` | Complete ordinary-C body emitted; policy-clean scratch clearing is 16 bytes longer |
| `_code_0009c460` | `176/176` | `10/10` | `cb6f30979095/a7e62792e4b5` | Register allocation differs despite target-equal relocation identities |
| `_game_engine_get_starting_location_rating` | `96/96` | `6/6` | `9b229c3bccdf/601000c33610` | Register allocation differs despite target-equal relocation identities |
| `_game_engine_remap_equipment` | `304/304` | `15/15` | `b315e0bf2bb5/5fd2bf5a01d1` | Policy-clean body emitted; compiler shape differs |
| `_game_engine_get_state_message` | `320/320` | `10/10` | `20ff545777ef/63ba933b746c` | Respawn-state local scheduling differs |
| `_netgame_verify_spawn_points` | `96/112` | `4/4` | `43a24b160b72/35ac6d2c5836` | Loop-invariant game type occupies the target's counter register |
| `_code_0009d140` | `1152/1088` | `73/70` | `e261315dfb4e/1a8d99b44cb1` | Policy-clean title topology is short by 64 bytes and three relocations |
| `_game_engine_post_rasterize_post_game` | `2528/2528` | `148/148` | `5d9500c231fe/a8bfb78acd0f` | Policy-clean body emitted; compiler shape differs |
| `_game_engine_update` | `752/752` | `67/67` | `a83ee62930df/1ae46e8434e6` | Complete ordinary-C body emitted; compiler scheduling differs |
| `_game_engine_player_killed` | `832/832` | `39/39` | `4d4a8b42b57d/d8d21f2ca76f` | Complete ordinary-C body emitted; compiler scheduling differs |
| `_code_0009e670` | `848/848` | `41/41` | `28483cea9891/7bacec539a12` | Policy-clean body emitted; compiler shape differs |

## Accepted controls

- GE01 retains five ordinary C functions recovered from the legal source state
  at `527fc167705b11887e0aee123bdc9cb91d1ffb15`:
  `_code_00096890`, `_code_00096b30`, `_code_00096b40`,
  `_code_00096c10`, and `_code_00096d00`.
- Exact callers were expressed through those recovered helpers:
  `game_engine_did_player_win_default`, `get_postgame_hilite_colors`, and
  `_code_00096d30`. The whole-TU sweep proves they remain exact.
- These are source/topology recoveries supported by target symbols and strict
  object equality. They are not byte-forcing controls.
- GE02 retains the typed `game_engine` callback at offset `0x38`, its ordinary
  C objective-weapon scan helpers `_code_00096e30` and `_code_00096ed0`, and
  the recovered goal-position getter. The callback type changes no structure
  size or offset; the target symbols and strict function comparisons support
  the recovered topology.
- GE03 retains the legal C vehicle/weapon prediction and remap flow plus the
  local HUD fade/target-name helpers. `_code_0009a840` currently calls the
  still-active `_code_0009a680` through an ordinary declaration; no match
  credit is given to that missing dependency.
- GE04 retains the recovered ranking comparator and the measured
  first-eligible-player team scan. Both are strict exact ordinary C.
- GE05 recovered the five map validators. The first build proved four leaf
  validators exact but expanded `game_engine_verify_current_map` to `1104` bytes and `60`
  relocations because `find_netgame_flags` was inlined through
  `_netgame_flag_verify_team_exists`. The target dispatcher is `640` bytes and `52` relocations
  and calls `find_netgame_flags`. GE05 historically restored the donor's
  `__declspec(noinline)` and made the caller strict exact. The 2026-08-31
  credibility audit supersedes that decision: the attribute has no original-
  source provenance, is rejected as compiler steering, and the natural caller
  is now honestly fuzzy-parked.
- GE06 replaces imported/Common storage with ordinary typed zero-initialized
  definitions. The resulting `.bss` has the target's exact `1,148`-byte
  layout: `global_goal[32]` at `0`, `global_variant` at `1024`,
  `game_engine` at `1128`, typed private teleporter state at `1132` and
  `1136`, and the autogenerate pointer/count at `1140` and `1144`. The strict
  section comparator proves the complete uninitialized-data section exact.
  This batch changes no function credit; renewed measurements show that the
  two emitted residuals also have register-allocation differences and were
  therefore correctly left unmatched.
- GE07 recovers the HUD score-message topology: the player-state initializer,
  the full message formatter, two small dispatch helpers, both public score
  wrappers, the ally/enemy dispatcher, and the player-added path. All eight
  recovered functions are strict exact. The ally/enemy dispatcher uses an
  ordinary two-arm C control flow that produces the target's `208` bytes
  without the earlier volatile parameter workaround. `_code_0009b6e0` is
  emitted with target-equal size and ordered relocations, but remains
  unmatched because its independent final terminator store is scheduled at a
  different instruction boundary; no assembly or byte forcing was retained.
- GE08 recovers the statistic score encoder and full ranking buffer plus the
  weighted item-selection and starting-equipment inventory helpers. All four
  functions are strict exact ordinary C. The typed callback at
  `game_engine + 0x70` is restored without changing the verified structure
  layout. Legal source bodies are also retained for their five coupled
  callers; those callers have target-equal relocation counts but remain
  unmatched and receive no credit.
- GE09 recovers the complete typed `debug_player_color` initializer, including
  the adjacent teleporter flash type, intensity, color, and duration fields.
  The resulting `32`-byte `.data` section is strict exact and the complete
  prior function set remains exact.
- GE10 restores the natural weighted-permutation `for` loop independently
  preserved at `b9aa157a957ac54e58a915bf58a2f690179caf7a`. XDK 3911 performs
  the target's four-way loop transformation by itself; no handwritten
  unrolling or scheduling control is retained. The result is the target's
  exact `176` bytes and five semantic relocations for `_code_00097020`.
- GE10 also restores the policy-clean postgame-title source topology. The
  named outcome, team-name, and player-entry branches are corroborated by the
  documented October decomp at
  `C:/Users/isabe/Downloads/halo-main (1)/halo-main/src/halo/game/game_engine.c`
  and by January strings/calls. Referencing `_code_00096b50` from that natural
  private caller restores the target's private ABI and makes the helper strict
  exact. `_code_0009d140` itself remains short by `64` bytes and three
  relocations and is explicitly parked without credit.
- GE10 removes three raw structural accesses without changing any admitted
  bytes: player offset `0xC0` is the offset-asserted
  `statistics.multiplayer_statistics` member, and the HUD font fields at
  `0x54`/`0x64` are exposed through an offset-asserted
  `game_engine_hud_globals` view. The three formatter callbacks at
  `game_engine + 0x4C/+0x50/+0x54` are likewise named and offset-asserted.
  The former definition-local non-inline boundary on `find_netgame_flags`
  was measured in GE05a to prevent nested inlining into the map validator,
  but that code-generation effect is not source provenance. It is removed;
  the resulting `640/1104` target/base discrepancy is recorded in
  `game_engine_obj_jonas_no_fake_inline_cleanup_20260831.md` without exact
  credit.
- GE11 restores the natural postgame renderer and its private display
  topology. The renderer's ordinary call to `_code_0009cb60` recovers that
  helper's target-private emission boundary and makes its complete `128`
  padded bytes and six semantic relocations strict exact. Typed,
  offset-asserted views replace the HUD definition's bitmap-group offset and
  the player statistic accesses; the postgame display helpers themselves are
  retained only as nonexact structural checkpoints.
- The postgame cluster exposed an XDK 3911 whole-translation-unit scheduling
  interaction in `_populate_statistic_buffer`: defining the already-exact
  function after the large renderer reverses two independent local loads.
  Moving the unchanged natural definition before the postgame functions
  restores the target order. The retained source has one ordinary definition,
  no excluded duplicate, and no scheduling primitive. Rebuilt comparison
  proves both `_populate_statistic_buffer` and `_code_0009cb60` exact
  simultaneously.
- GE12 restores the final six target-backed function bodies and their typed
  support surfaces using ordinary C. `_code_0009b4f0` keeps external linkage
  because the target both emits that symbol and inlines its behavior into
  `_game_engine_update`; the rebuilt candidate likewise emits both functions
  at their target relocation counts. `_code_0009bdf0` clears its scratch
  storage with an ordinary byte loop. No assembly, volatile forcing,
  scheduling barrier, pragma, excluded duplicate, or object patch is retained.
  A whole-TU sweep proves all 180 target functions now have candidate symbols,
  the protected 159-function exact set remains exact, and both owned data
  sections remain strict exact. The six restored bodies remain residuals and
  receive no exact credit.

## Experiment matrix

| ID | Source shape | Size T/C | Relocs T/C | Hash / first divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|
| GE00 | Rebuild canonical `e54ead79c` unchanged | `13,904/33,760` exact bytes | `125/180` functions | Baseline object `3a184a158241...` | Frozen exact set | retained baseline |
| GE01 | Recover five small adapters and route their existing callers through them | `400/400` | target-equal per function | All five strict exact | `125/125` canonical exact | retained |
| GE02 | Recover typed objective-weapon callback scan and goal getter | `592/592` | target-equal per function | All three strict exact | `130/130` prior exact | retained |
| GE03 | Recover remap/prediction and local HUD fade helpers | `1,600/1,600` | target-equal per function | All four strict exact | `133/133` prior exact | retained |
| GE04 | Recover ranking comparator and team-presence scan | `320/320` | target-equal per function | Both strict exact | `137/137` prior exact | retained |
| GE05a | Recover five map validators without the donor `noinline` declaration | `game_engine_verify_current_map 640/1104` | `52/60` | `find_netgame_flags` body inlined into dispatcher | `139/139` prior exact | retained after 2026-08-31 credibility review |
| GE05b | Restore donor `find_netgame_flags` non-inline boundary | `1,088/1,088` | target-equal per function | All five strict exact | `139/139` prior exact | superseded; fake exactness rejected |
| GE06 | Own the complete typed zero-initialized data layout | `.bss 1,148/1,148` | `0/0` | Strict section info exact | `144/144` prior exact | retained |
| GE07 | Recover HUD formatting, dispatch, score wrappers, and player-added topology | `3,232/3,232` exact bytes | target-equal per exact function | Eight strict-exact functions; one `144`-byte helper parked | `144/144` prior exact | retained |
| GE08 | Recover statistic ranking and weighted starting-equipment helpers | `1,120/1,120` exact bytes | target-equal per exact function | Four strict-exact functions; five coupled callers remain parked | `152/152` prior exact | retained |
| GE09 | Recover the typed debug/teleporter flash initializer | `.data 32/32` | `0/0` | Strict section info exact | `156/156` prior exact | retained |
| GE10 | Restore the natural item-weight loop, postgame title topology, and named offset-asserted structural views | `256/256` newly exact bytes | target-equal per newly exact function | Two strict-exact functions; `_code_0009d140` parked at `1088/1152`, `70/73` | `156/156` prior exact | retained |
| GE11 | Restore the policy-clean postgame renderer topology and place the unchanged ranking-buffer definition before that cluster | `128/128` newly exact bytes | `6/6` for `_code_0009cb60` | `_code_0009cb60` strict exact; `_populate_statistic_buffer` remains exact at `560/560`, `27/27` | `158/158` prior exact | retained |
| GE12 | Restore the final six target-backed bodies with ordinary C | `180/180` target symbols emitted | Restored bodies have `16/16`, `11/11`, `5/5`, `46/46`, `67/67`, and `39/39` relocations | No new exact credit; all six bodies remain measured structural residuals | `159/159` prior exact | retained emission checkpoint |

## Post-GE12 retained work

The active lane is
`codex/game-engine-full-admission-20260729`. Its clean restart checkpoint is
`e311fa706078abeb2ed8cc7b556909cc1de68c51`.

After retained batch GE18:

- Functions: `165/180` strict exact.
- Padded code: `25,232/33,760` target bytes.
- Candidate SHA-256:
  `ba69073dfd624a6744e183497ed6e76c4daf642b77e31389d74dca3f723f6c73`.
- Candidate emission: `180/180` target functions.
- `.data` and `.bss` remain strict exact.
- Canonical regressions: none.

The six retained post-GE12 functions are:

| ID | Commit | Exact function | Recovered source topology | Result |
|---|---|---|---|---|
| GE13 | `e808f2bde7747b02a4c637e19aeba4fa1c03036f` | `_code_0009bdf0` | Order the independent teleporter-flash stores as fade function, type, duration, priority, intensity, scale, and color; the unrelated historical `find_netgame_flag` non-inline attribute was removed by the later credibility audit without changing this function | `160/180` exact |
| GE14 | `bca69538a4d344230a8290331407ef4351ebc8ba` | `_code_0009b4f0` | Use the existing `game_engine_has_shield(player_index)` predicate and private linkage, then zero both shield fields only for a live unit | `161/180` exact |
| GE15 | `519f5d2e4db98b26bb2238d745eed52209c1d014` | `_game_engine_postspawn_player_update` | Use the exact `code_00097c00()` infinite-grenade predicate before restoring both starting grenade counts | `162/180` exact |
| GE16 | `0594405e063dde6a9d29e7ceccc8f5167b2f7c29` | `_netgame_verify_spawn_points` | Recover the private prototype's second parameter as `short`, not `long` | `163/180` exact |
| GE17 | `1e22811771f276868d0e497f0a99755b850382a5` | `_game_engine_remap_equipment` | Name the grenade remap predicate, reuse `code_00097c00()`, and funnel remapped list indices through `list_index_to_weapon_definition_index()` | `164/180` exact |
| GE18 | `e311fa706078abeb2ed8cc7b556909cc1de68c51` | `_game_engine_update` | Iterate directly on `data_iterator_next`, materialize `always_invisible` through flag/callback arms, and pass `iterator.datum_index` at the two tail calls | `165/180` exact |

Each retained step passed a full 180-function
`section_infos_equal` sweep, the policy scan, and a source diff check before
commit.

### Active residual measurements after GE18

Hashes are normalized SHA-256 prefixes from the hardened comparator.

| Function | Size T/C | Relocs T/C | Normalized hash T/C | Current classification |
|---|---:|---:|---|---|
| `_code_00096ba0` | `112/112` | `7/7` | `4ac1ff18279a/9db784535924` | draw-line arithmetic CSE/scheduling |
| `_game_engine_should_spawn_player` | `256/256` | `9/9` | `8259180402f7/c0782ea3a816` | boolean/control-flow allocation |
| `_game_engine_get_place` | `368/368` | `12/12` | `91161c382c55/bcf2be6ca02e` | tied/all-tied local allocation |
| `_code_000994f0` | `416/416` | `18/18` | `211627377053/10c5400d228a` | private draw-line ABI/register allocation |
| `_code_0009a490` | `496/480` | `16/16` | `f30da54283e6/01537066ba3b` | select-players topology, short by 16 bytes |
| `_code_0009a680` | `448/448` | `18/18` | `4c90ec184c40/41e698125368` | closest-player tail block layout |
| `_code_0009b3a0` | `336/336` | `11/11` | `7e6f5319e160/dbb1e45ecac4` | prologue/tail scheduling |
| `_code_0009b6e0` | `144/144` | `5/5` | `675e96da950f/bb509add4c20` | multiplayer-message push/store scheduling |
| `_code_0009c460` | `176/176` | `10/10` | `cb6f30979095/a7e62792e4b5` | private callback ABI, EBX/EDI mirror |
| `_game_engine_get_starting_location_rating` | `96/96` | `6/6` | `9b229c3bccdf/601000c33610` | caller half of EBX/EDI private ABI pair |
| `_game_engine_get_state_message` | `320/320` | `10/10` | `20ff545777ef/63ba933b746c` | target shrink-wrapped prologue |
| `_code_0009d140` | `1152/1088` | `73/70` | `e261315dfb4e/1a8d99b44cb1` | postgame title topology, short by 64 bytes/3 calls |
| `_game_engine_post_rasterize_post_game` | `2528/2528` | `148/148` | `5d9500c231fe/a8bfb78acd0f` | large postgame renderer compiler shape |
| `_game_engine_player_killed` | `832/832` | `39/39` | `4d4a8b42b57d/d8d21f2ca76f` | event-block ordering |
| `_code_0009e670` | `848/848` | `41/41` | `28483cea9891/7bacec539a12` | score-row private ABI/register allocation |

### Provenance recovered after GE18

- The January `cachebeta.exe` symbol atlas identifies
  `_code_00096ba0` as `_drawline`, `_code_000994f0` as
  `_rasterize_in_game_score_draw_line`, `_code_0009a490` as
  `_select_players_to_display` (fuzzy tier), `_code_0009a680` as
  `_find_closest_player_index`, `_code_0009b6e0` as
  `void multiplayer_message(long,long,long)`, and `_code_0009c460` as
  `_default_starting_location_rate_function`.
- Public PC 1.10 maps corroborate the sequence
  `find_closest_player_callback`, `find_closest_player_index`,
  `internal_rasterize_target_name`: `_code_00096c80`,
  `_code_0009a680`, `_code_0009a840`.
- Tempera and OpenSauce contain call wrappers and symbol maps for these
  routines, but no public function body suitable as a donor.
- A direct January PDB module-stream audit found stream 316 contains only
  `S_OBJNAME`, `S_COMPILE2`, and no procedure, local, register, block, or
  line records. Original local names/order and prototypes cannot be recovered
  from that PDB. Evidence is recorded in
  `outputs/jan-pdb-game-engine-locals/evidence.md`.

### Rejected post-GE18 experiments

All entries below were rebuilt with XDK 3911 and restored because they did not
produce a strict improvement.

| ID | Function/cluster | Source shape | Exact measurement / rejection |
|---|---|---|---|
| GE19a | `_code_000994f0` / `_code_0009e670` | Alias the color local; alias the row local; move `row_color` to function scope; mark formal declarations `register`; swap the last two formal parameters and all calls | Color/row/scope/register spellings produced no object change. Parameter swapping also preserved the compiler's existing private ABI. Baseline hashes remain `10c5400d228a` and `7bacec539a12`. |
| GE19b | `_code_00096ba0` | Spell the row ordinate as `2 * (row * 9) + 26`, unsigned multiply variants, and separately recomputed expressions | No object change. Target recomputes the two ordinates; candidate CSEs the first value and adds `0x1a`. Baseline hash remains `9db784535924`. |
| GE19c | `_game_engine_player_killed` | Positive `unknown_d1` early arm; negative guard without `else`; merge event 5/event 4 as `if`/`else if` | Positive early arm shrank/reordered the body to `816` bytes. The two other forms produced no useful change. Target is `832/832`, `39/39`, with the `unknown_d1` loop at `0x1d7`, event-5 fallback at `0x226`, and event-4 block at `0x276`; candidate orders the same blocks at `0x2e7`, `0x1d3`, and `0x226`. |
| GE19d | `_code_0009a680` | Put the failure return inside the `best == NONE` block and make the outer conversion explicit | `448/448`, `18/18`, candidate hash `5e0ed18322b9`; wrong failure/fast-conversion block order. |
| GE19e | `_code_0009a680` | Explicit positive `object_count` guard with `do` loop | `448/448`, `18/18`, candidate hash `601dd2a2e76d`; popped EBX too early and inserted `mov edx,eax` before the inner conversion. |
| GE19f | `_code_0009a680` | Explicit positive `object_count` guard plus ordinary `for` loop | `448/464`, `18/18`, candidate hash `564a3c340805`; redundant entry test expanded the body by 16 bytes. |
| GE19g | `_code_0009a680` | Direct early conversion return followed by an unwrapped search | `448/448`, `18/18`, candidate hash `83138e3c2851`; moved the search body and changed register allocation well before the target tail. |
| GE19h | `_code_0009a680` | Explicit final `if (best == NONE) return best;` followed by conversion | `448/448`, `18/18`, candidate hash `5e0ed18322b9`; same rejected block order as GE19d. |
| GE19i | `_code_0009a680` | Read the ternary false arm through a same-type pointer alias | `448/448`, `18/18`, unchanged candidate hash `41e698125368`; optimizer sees through the alias. |
| GE20a | `_code_0009b6e0` | Introduce a block-local `wchar_t *message_pointer` alias and pass it to `hud_print_message` | No object change: `144/144`, `5/5`, candidate hash `bb509add4c20`. The optimizer eliminates the alias and still schedules the terminator store at `0x6c` instead of target `0x77`. |
| GE20b | `_code_0009b6e0` | Materialize `player->local_player_index` as a block-local `word` before the terminator store | `144/144`, `5/5`, candidate hash `c47997f24cc2`. The load moves before the store, but becomes a shorter `movzx`; the store moves after both argument pushes and the real body ends two bytes early. |
| GE20c | `_code_0009b6e0` | Embed the terminator assignment in the message argument with a comma expression | No object change: `144/144`, `5/5`, candidate hash `bb509add4c20`; XDK hoists the independent store to the same `0x6c` position. |
| GE21a | `_game_engine_get_state_message` | Replace the outer `if (game_engine)` with an early `if (!game_engine) return FALSE` guard and an unwrapped body | Rejected at `320/304`, `10/9`, candidate hash `0f94227ea43a`. It split the false epilogue, shortened the function by 16 bytes, and disturbed allocation beyond the prologue. |
| GE21b | `_game_engine_get_state_message` | Route the false-engine path to an explicit shared `exit:` label | No object change: `320/320`, `10/10`, candidate hash `63ba933b746c`; XDK canonicalizes the label to the existing outer-if CFG. |
| GE21c | `_game_engine_get_state_message` | Move the initialized `result` declaration ahead of the four other locals | No object change: `320/320`, `10/10`, candidate hash `63ba933b746c`; declaration order does not affect the shrink-wrapped saves. |
| GE22a | `_code_00096ba0` | Spell the second ordinate directly as `row_index * 18 + 26` | No object change: `112/112`, `7/7`, candidate hash `9db784535924`; XDK CSEs the first ordinate. |
| GE22b | `_code_00096ba0` | Use a differently factored second ordinate, `row_index * 9 * 2 + 26` | No object change: `112/112`, `7/7`, candidate hash `9db784535924`; reassociation canonicalizes to the same CSE. |
| GE22c | `_code_00096ba0` | Materialize the cast `justification` as a scoped `short` between the two ordinate statements | No object change: `112/112`, `7/7`, candidate hash `9db784535924`; the local is eliminated and the justification load remains scheduled before the first ordinate calculation. |
| GE23a | `_code_0009b3a0` | Initialize `equipment_index` before the empty-block early return | `336/336`, `11/11`, candidate hash `0246a597921d`. It moves the zero store before the branch as suggested by target, but changes the entire 16-byte prologue schedule without fixing the tail. |
| GE23b | `_code_0009b3a0` | Add an explicit `scenario` local before deriving the equipment-block pointer | No additional object change from GE23a; XDK eliminates the alias and retains hash `0246a597921d`. |
| GE23c | `_code_0009b3a0` | Increment `equipment_index` before refreshing `equipment_count` at the loop tail | No object change from GE23a; store/sign-extension order remains candidate-style. |
| GE23d | `_code_0009b3a0` | Remove `equipment_count` and compare the cast index directly with `equipment_block->count` | No object change from GE23a; direct condition canonicalizes to the same tail. |
| GE24a | `_game_engine_should_spawn_player` | Recover `should_spawn` as `boolean` instead of `long` | Much closer at `256/256`, `9/9`, candidate hash `031050d5b5fa`, but the real body is two bytes shorter. It emits the target byte initialization/update/test, except `TRUE` is written with `mov bl,1` rather than target `mov ebx,1`; the later `respawn_timer == 1` therefore uses an immediate instead of EBX. |
| GE24b | `_game_engine_should_spawn_player` | In the boolean variant, compare `respawn_timer` with `should_spawn` | No object change from GE24a; SSA constant-folds the known-true boolean back to immediate `1`. |
| GE24c | `_game_engine_should_spawn_player` | Keep `should_spawn` as `long` but cast the timer-zero assignment to `boolean` | `256/256`, `9/9`, candidate hash `ccda5c6da927`; XDK decrements memory directly, sets CL, then zero-extends into EBX, diverging more broadly. |
| GE24d | `_game_engine_should_spawn_player` | Keep the long assignment and cast only the subsequent false test to `boolean` | `256/256`, `9/9`, candidate hash `88d4d379707c`; the test narrows to BL, but XDK still clears full EBX at initialization and before `sete`. |
| GE24e | `_game_engine_should_spawn_player` | Add a long `true_value`, copy it to the boolean result, and reuse it for the one-tick comparison | Optimizer eliminates the auxiliary value; identical to GE24a (`031050d5b5fa`). |
| GE24f | `_game_engine_should_spawn_player` | Keep the long result but stage the timer-zero expression through a block-local boolean | `256/256`, `9/9`, candidate hash `ff0a5a6c9795`; result moves through AL and a later `movzx`, changing the update tail and remaining two bytes too long/short in different places. |
| GE25a | `_game_engine_get_place` | Reverse the declarations of `tied` and `all_tied` | No object change: `368/368`, `12/12`, candidate hash `bcf2be6ca02e`; stack-slot allocation follows use/lifetime, not declaration order. |
| GE25b | `_game_engine_get_place` | Initialize `tied` before `all_tied` | `368/368`, `12/12`, candidate hash `f3df41c318b9`; store order changes, but tied remains at `[ebp-1]` and all-tied at `[ebp-2]`, opposite the target. |
| GE25c | `_game_engine_get_place` | Materialize `all_tied & tied` as a block-local boolean before building result flags | Rejected at `368/384`, `12/12`, candidate hash `8bd0c13d5f01`; it adds a separate AL value and expands the flag-builder by 16 bytes. |
| GE26 | `_game_engine_get_starting_location_rating` / `_code_0009c460` | Prior bounded families: split or initialize the `player` local separately, reverse formal parameter order and calls, inline `player_get`, and introduce aliases around `player_index`/`starting_location` | No family changed the coupled private ABI mirror. Metrics remain `96/96`, `6/6`, hash `601000c33610` for the caller and `176/176`, `10/10`, hash `a7e62792e4b5` for the callee. |
| GE27a | `_code_000994f0` / `_code_0009e670` | Move the font-header pointer declaration to function scope and assign it inside the font block | No object change in either function; hashes remain `10c5400d228a` and `7bacec539a12`. |
| GE27b | `_code_000994f0` / `_code_0009e670` | Move the caller's `color` declaration ahead of `text_color` and `team_colors` | No object change; stack allocation and the private call ABI ignore declaration order. |
| GE27c | `_code_000994f0` / `_code_0009e670` | Change private `row_index` formal from `long` to `short` | Caller remains unchanged and the callee stays `416/416`, `18/18`, but changes to nonexact hash `cd51e44cd367` through a word compare at the row test. |
| GE28a | `_code_0009a490` | Rewrite only the move length from `maximum_count * 28 - entry_index * 28 - 28` to `(maximum_count - entry_index - 1) * 28` | Still `496/480`, `16/16`; candidate hash `ce681b033294`. First divergence remains the frame immediate at instruction `0x03` (target `0x244`, candidate `0x248`). The long-lived temporary changes from `maximum_count * 28` to the first discarded-entry pointer, but the candidate still removes the target's second comparison and remains 16 bytes short. Full-TU sweep remains the same 165 exact functions and 15 named residuals; rejected and restored. |
| GE28b | `_code_0009a490` | Spell only the outer guard as `player_count > maximum_count`, leaving the nested `maximum_count < player_count` test unchanged | Still `496/480`, `16/16`; candidate hash `62f46588e0c2`. First divergence remains the frame immediate at `0x03`; XDK still eliminates the nested comparison and merely changes the discarded-entry pointer register schedule. Full-TU sweep remains the same 165 exact functions and 15 named residuals; rejected and restored. |
| GE28c | `_code_0009a490` | Express the nested gather guard as a one-iteration `while (maximum_count < player_count) { ...; break; }` | Canonicalizes completely to baseline: `496/480`, `16/16`, candidate hash `01537066ba3b`; first divergence is still the frame immediate at `0x03`, and only one comparison remains at `0x7e`. Full-TU sweep remains 165 exact with the unchanged 15-residual set; rejected and restored. |
| GE29a | `_code_0009d140` | Give each successful postgame outcome branch a block-local `outcome_string` before its `ustrncpy`, matching the one already present in the winning-team arm | Regressed from `1152/1088`, `73/70`, `1a8d99b44cb1` to `1152/1024`, `73/66`, candidate hash `02877d10d508`. First divergence remains the branch at `0x112` (target destination `0x2a7`, candidate `0x223`). XDK tail-merges even more outcome arms, leaving only two `ustrncpy` relocations at `0x1f0` and `0x20e`; full-TU sweep remains the same 165 exact/15 residuals. Rejected and restored. |
| GE29b | `_code_0009d140` | Reorder the semantic outcome chain from `NONE`, false, true to false, true, `NONE`, preserving every branch body | `1152/1088`, `73/70`, candidate hash `7fca1b217d7d`. First divergence is again the branch at `0x112` (target destination `0x2a7`, candidate `0x25d`). The compare order follows source order, but XDK still merges the same three `ustrncpy` sites and emits only two at `0x1dd` and `0x248`; full-TU sweep remains 165 exact with the unchanged residual set. Rejected and restored. |
| GE29c | `_code_0009d140` | Replace the `NONE`/false/true `if` chain with a semantically equivalent `switch` containing explicit breaks | Regressed to `1152/1072`, `73/70`, candidate hash `9402ff95adec`; first divergence remains the branch at `0x112` (target destination `0x2a7`, candidate `0x257`). The switch retains only two `ustrncpy` sites at `0x198` and `0x242` and changes compare/layout shape without recovering any missing relocation. Full-TU sweep remains 165 exact/15 residuals; rejected and restored. |
| GE29d | `_code_0009d140` | Add a block-local `outcome_string` only to the false/team arm (`0x38`), leaving the other four outcome arms at baseline | Regressed to `1152/1072`, `73/69`, candidate hash `cd14c1e44f9f`; first divergence remains the `0x112` branch. XDK coalesces more of the tag/string retrieval and still leaves only two `ustrncpy` sites at `0x1b9` and `0x239`, so the selective-local hypothesis does not incrementally recover a target call. Full-TU sweep remains 165 exact/15 residuals; rejected and restored. |
| GE30a | `_game_engine_post_rasterize_post_game` | Move only the `code_0009cb60(0)` team-order decision before loading the two team-format strings, as ordered in the target relocations | Remains `2528/2528`, `148/148`; candidate hash changes from `a8bfb78acd0f` to `2452c802321f`. First divergence remains the frame immediate at `0x03` (target `0x670`, candidate `0x674`). This exactly restores the early semantic call order (`code_0009cb60` at candidate `0x173`, target `0x173`) but 125 relocation addresses and later player/prompt blocks remain displaced. Full-TU sweep remains 165 exact with the unchanged 15 residuals; nonexact source change rejected. |
| GE30b | `_game_engine_post_rasterize_post_game` | On top of GE30a, declare `team_order[2]` before `team_tab_stops[6]` | Regressed to `2528/2528`, `148/146`, candidate hash `a0960d2334c3`; first divergence remains `0x03`. The stack allocator does not adopt the target slots and two later string-fetch relocations disappear through changed whole-function optimization. Full-TU exact set remains 165/180; rejected and restored. |
| GE30c | `_game_engine_post_rasterize_post_game` | On GE30a, leave `place`, `string_list_index`, and `place_string` uninitialized at declaration and assign them after the first row color/tab calls, matching target relocation order | Improves semantic ordering but shrinks to `2528/2512`, `148/148`, candidate hash `fdab273b3ace`. First divergence is still the frame immediate at `0x03`; the complete relocation-target sequence then agrees except for the final prompt-bounds cluster, but 126 relocation addresses differ. Full-TU sweep stays 165 exact/15 residuals; rejected. |
| GE30d | `_game_engine_post_rasterize_post_game` | On GE30c, copy `bounds = render.camera.window_bounds` before assigning `prompt_color.alpha` | `2528/2512`, `148/148`, candidate hash `d63305c7dc08`. First divergence remains `0x03`. The only remaining semantic relocation-order delta is the placement of the `render + 0x3E` viewport-word load inside the prompt cluster; full-TU sweep stays 165/180. Rejected. |
| GE30e | `_game_engine_post_rasterize_post_game` | On GE30d, remove explicit prompt `bounds.x1 = 640` / `bounds.y1 = 480`, retaining the copied window bounds as the target disassembly does | Still `2528/2512`, `148/148`, candidate hash `1b8c8c531b65`. The two redundant stores disappear without changing padded size; only the `render + 0x3E` load order remains semantically displaced, the frame is still `0x674` versus target `0x670`, and 130 relocation addresses differ. Full-TU sweep remains 165 exact/15 residuals; rejected. |
| GE30f | `_game_engine_post_rasterize_post_game` | On GE30e, remove the block-local `draw_row` alias and pass `entry_index` directly to all six row draws | No object change from GE30e: `2528/2512`, `148/148`, candidate hash `1b8c8c531b65`; XDK had already coalesced the alias. Full-TU sweep remains unchanged; rejected and restored. |
| GE31a | `_game_engine_player_killed` | Add an explicit positive `unknown_d1` jump to a `player_quit:` label while retaining the negative normal-event guard | Canonicalizes exactly to baseline: `832/832`, `39/39`, candidate hash `d8d21f2ca76f`. Target and candidate remain byte-identical through the conditional branch instruction at `0x13b`; only its destination differs (`0x1d7` target versus `0x2e7` candidate), because the quitting-player broadcast block stays at the tail. Full-TU sweep remains 165 exact with the unchanged 15 residuals; rejected and restored. |

The target and GE18 candidate for `_code_0009a680` are byte-identical through
offset `0x190`. The remaining target tail uses EAX for both conversion
arguments and reloads `best_object_index` on the zero-count return path.
The candidate uses EDX in the outer conversion and constant-folds the failure
to `-1`. Further work on this function requires a genuinely new control-flow
family, not a spelling variant of GE19d-i.

`_code_0009b6e0` park packet (GE20): target and baseline are identical through
offset `0x6c` and again from the call at `0x7e` through the padded end. Both
are `144` bytes with five ordered semantic relocations. The only difference is
the legal scheduling of the independent
`message[NUMBEROF(message) - 1] = 0` store: target places it after the message
pointer push and before the player-index push; the candidate places it before
the local-player-index reload. Alias, explicit-local, and comma-expression
families are exhausted. Reopen only with donor source or a new compiler
scheduling dependency that does not use volatile/barriers.

`_game_engine_get_state_message` park packet (GE21): both objects are
`320` bytes with ten ordered semantic relocations, and offsets `0x26` through
the padded end are byte-identical. The sole difference is the first
`0x26` bytes. Target saves EBX/ESI before the `game_engine` test and EDI
between the test and branch; candidate shrink-wraps all three saves after the
false branch. Early-return, explicit-shared-label, and declaration-order
families are exhausted. Reopen only with a new target-backed lifetime/CFG lead
that explains the callee-save placement.

`_code_00096ba0` park packet (GE22): both sections are `112` bytes with seven
ordered semantic relocations. Offsets `0x00..0x34` are exact. Target computes
`row_index * 18` independently into EDX for `bounds.y0`, then into EAX for
`bounds.y1`, ending its real body at `0x70`. Candidate computes it once in EAX,
adds 26 for `y1`, and ends at `0x6c` with four padding NOPs. Direct,
differently-factored, unsigned variants from GE19b, and a scoped
justification-local family all canonicalize to the same candidate. Reopen only
with donor topology or a legal alias/lifetime reason for the target's
recomputation.

`_code_0009b3a0` park packet (GE23): target/candidate are `336/336` with
`11/11` ordered semantic relocations. The central body from `0x30` through
`0x127` is byte-identical. Baseline differences are confined to pointer/count
setup before `0x30` and loop-index/count scheduling at `0x127..0x135`.
Moving the zero initialization produces a distinct but non-improving
`0246a597921d` candidate; scenario aliases, source-order swapping, and a direct
loop condition do not refine it. The committed `dbb1e45ecac4` baseline is
restored. Reopen only with original topology that jointly explains target's
`lea ebx,[eax+0x384]` setup and EAX/ECX tail allocation.

`_game_engine_should_spawn_player` park packet (GE24): target/baseline are
`256/256` with nine ordered semantic relocations. The target exhibits a mixed
width lifetime in EBX: initial false and the timer-zero result use BL, the
intermediate true uses full EBX and is reused for the one-tick comparison.
The natural boolean source is the closest candidate (`031050d5b5fa`) and
matches that behavior except for a two-byte-short `mov bl,1`/immediate compare
pair. Natural long, boolean, cast-at-assignment, cast-at-test, auxiliary
constant, and boolean-temporary families are exhausted. The committed long
baseline is restored; do not synthesize mixed-width storage through union or
pointer byte forcing. Reopen only with donor source or a target-backed local
type/lifetime explanation.

`_game_engine_get_place` park packet (GE25): target/baseline are `368/368`
with `12/12` ordered semantic relocations. Target assigns tied to `[ebp-2]`
and all-tied to `[ebp-1]`, then loads both before its flag builder and retains
their AND in DL. Baseline assigns the logical locals to the opposite byte
slots and uses `test cl,dl` later. Declaration order is ignored, assignment
order changes only store scheduling, and materializing the AND expands the
body. A previous packed-struct family also worsened allocation. Baseline is
restored; reopen only with original local topology or a target-backed lifetime
lead, not manual storage overlays.

Starting-location rating park packet (GE26): the caller is exact through
`0x33`, then target saves player index in EBX while candidate uses EDI. The
private callee receives starting location in EAX in both objects; target's
hidden player index is EBX and its player pointer is EDI, while candidate
mirrors those roles as EDI/EBX. All instruction sizes, control flow, and
ordered semantic relocations otherwise agree (`96/96`, `6/6` and `176/176`,
`10/10`). The January atlas confirms `_code_0009c460` is
`_default_starting_location_rate_function`. Local splitting, aliasing,
parameter order, and inline-get families are exhausted. Reopen only with a
new whole-TU/private-ABI allocation lead; do not add register-forcing syntax.

Score-row draw park packet (GE27): `_code_000994f0` and its large caller
`_code_0009e670` are respectively `416/416`, `18/18` and `848/848`, `41/41`.
Target's private helper passes only string/brighten on the stack, row in EAX,
and color in ESI; candidate passes row on the stack and color in EDI. Target
therefore retains `font_index` on the helper's stack and uses EAX for the font
pointer, while candidate caches them through ESI. In the caller this is visible
at the first call: target pushes two arguments, zeros EAX, and computes color
in ESI; candidate pushes three arguments and computes color in EDI.
Color/row aliases, formal reordering, register keywords, declaration scope and
order, and a short-row prototype are exhausted. Baseline is restored. Reopen
only with original private-call topology or a whole-TU allocation lead.

`_code_0009a490` bounded structural park packet (GE28): target/baseline are
`496/480` with `16/16` ordered semantic relocations. The first divergence is
the prologue frame at instruction `0x03`: target allocates `0x244`, candidate
`0x248`. At `0x7E`, target retains two consecutive
`maximum_count < player_count` checks around the zero initialization and
recomputes `maximum_count * 28` at `0x168`; candidate removes the second check
and preserves the product in an extra stack temporary. Consequently target
ends its real body at `0x1EE` and candidate at `0x1E0`. Equivalent move-length
algebra, reversed outer comparison spelling, and a one-iteration while/break
guard either produce distinct non-improving hashes or canonicalize to the
`01537066ba3b` baseline. Every trial preserved the 165-function exact set.
No source/PDB donor contains the original locals. This packet does not grant
target-equal size or exact credit; reopen only with original
`select_players_to_display` topology or a measured reason for both the
redundant comparison and absent long-lived product.

`_code_0009d140` bounded structural park packet (GE29): target/baseline are
`1152/1088` with `73/70` semantic relocations. Instructions `0x00..0x112`
agree; the first differing instruction is the `0x112` postgame-state branch,
whose destination is target `0x2A7` versus candidate `0x25D`. In the outcome
cluster target emits five distinct `ustrncpy` calls, while XDK tail-merges the
natural source arms into two, exactly accounting for the three missing call
relocations. Homogeneous and selective outcome-pointer locals, semantic arm
reordering, and a switch form produce `1024`, `1072`, or `1088`-byte
nonmatches and never recover a third call site. The January PDB has no local,
block, or line records, and the October decomp corroborates behavior but not
the target's source topology. This packet does not grant target-equal size or
relocation credit; reopen only with donor source or a new CFG family that
explains five unmerged return/copy epilogues without forcing code generation.

`_game_engine_post_rasterize_post_game` bounded compiler-shape park packet
(GE30): target/baseline are `2528/2528` with `148/148` semantic relocations.
The first divergence is the prologue frame at `0x03` (target `0x670`,
candidate `0x674`) followed by different callee-save and local-slot
allocation. Target orders the team-order predicate before the two team-format
lookups, and orders the first row color/tab calls before the place-string
lookup; baseline source order is the reverse. GE30a and GE30c recover those
semantic call orders, and GE30d/e recover the prompt bounds/progress order and
remove two target-absent bound stores. The best combined form has all
`148` relocations and only one repeated `render + 0x3E` relocation-order
delta, but shrinks the candidate to `2512` bytes, retains the `0x674` frame,
and has hash `1b8c8c531b65`. Reordering the team locals instead drops two
relocations; removing the draw-row alias is a no-op. All trials preserve the
165 exact functions. Baseline is restored because no partial structural
change receives admission. Reopen only with original local lifetime/order
evidence capable of jointly explaining the four-byte frame difference and
whole-function register allocation.

`_game_engine_player_killed` compiler-layout park packet (GE19c/GE31):
target/baseline are `832/832` with `39/39` semantic relocations and are
byte-identical through the conditional instruction at `0x13B`. Only its
destination differs: target places the quitting-player iterator/broadcast
block at `0x1D7`, candidate at `0x2E7`. Target then places the event-5 fallback
at `0x226` and event-4 medal block at `0x276`; candidate places them at
`0x1D3` and `0x226`. A positive early quitting arm shrinks the body to
`816` bytes; a negative guard without `else`, event 5/4 `else if`, and an
explicit positive jump label either preserve the `d8d21f2ca76f` baseline or
worsen layout. Every form keeps the same 39 call/data relocations and the
165-function exact set. Reopen only with a genuinely new target-backed CFG
that orders the three event blocks without changing their emitted bodies.

## Do-not-repeat list

- Do not import `.agent-ge-wave1`, raw `.obj` checkpoints, or their text
  inventories.
- Do not cherry-pick either artifact-bearing checkpoint commit.
- Do not use inline assembly, volatile byte forcing, pointer-past-object
  storage, `_ReadWriteBarrier`, compiler pragmas/flags, object patches, or
  comparator exceptions.
- Equal size or equal relocation count without ordered semantic relocation
  identity is not a match and is not sufficient parking evidence.
- Do not repeat GE19a-i or the older cosmetic spelling trials for
  `_code_0009b6e0`, `_code_0009c460`, and
  `_game_engine_get_starting_location_rating`; reopen them only with a new ABI
  or source-topology lead.
- Do not repeat GE28a-c, GE29a-d, GE30a-f, or GE31a. Those families cover the
  remaining arithmetic/guard, postgame-outcome, renderer-order, and
  kill-event-layout hypotheses and preserve no strict-exact improvement.

## Residual classification

- No candidate symbol is absent; all 180 target functions are emitted.
- Thirteen target-equal-size residuals are evidence-exhausted compiler
  scheduling, register-allocation, private-ABI, or basic-block-layout shapes:
  `_code_00096ba0`, `_game_engine_should_spawn_player`,
  `_game_engine_get_place`, `_code_000994f0`, `_code_0009a680`,
  `_code_0009b3a0`, `_code_0009b6e0`, `_code_0009c460`,
  `_game_engine_get_starting_location_rating`,
  `_game_engine_get_state_message`,
  `_game_engine_post_rasterize_post_game`,
  `_game_engine_player_killed`, and `_code_0009e670`.
- Two structural residuals are evidence-exhausted but do not meet
  target-equal size/relocation gates: `_code_0009a490` is 16 bytes short with
  equal relocation count, and `_code_0009d140` is 64 bytes/three calls short.
- Every one of the 15 residuals has a reproducible bounded packet above. A
  packet records why ordinary source families are exhausted; it grants no
  exact or semantic-match credit.
- No residual is currently granted exact credit or marked complete.

## Final clean audit after GE31

- Clean XDK 3911 rebuild from source checkpoint `e311fa706078...`.
- Functions: `165/180` strict exact.
- Padded exact code: `25,232/33,760` target bytes.
- Candidate emission: `180/180` target functions.
- Final rebuilt candidate SHA-256:
  `eeadadb05d04c74aad18e31636fe52b9ab111cdbf768b4f78a609484b582194b`.
- `.data`: strict exact at `32/32`, `0/0` relocations, normalized hash
  `daa87eb1c49a` on both sides.
- `.bss`: strict exact at `1,148/1,148`, `0/0` relocations, normalized hash
  `509607d81ec2` on both sides.
- The residual names and all 15 baseline metrics reproduce the post-GE18
  table exactly.
- Full policy scan has no hits for inline assembly, `volatile`,
  `_ReadWriteBarrier`, intrinsic pragmas, pointer-past-object markers,
  `UB_` markers, or `#if 0`.
- `git diff --check` passes, and source/header diff is empty. Only this ledger
  is changed by the bounded-exhaustion pass.

## Reopen criteria

A residual may be retained only after an XDK 3911 rebuild proves its complete
padded section and semantic relocations exact, with no loss from the frozen
canonical exact set. Formal parked-registry admission additionally needs
target-equal size and ordered semantic relocation identities; the two bounded
structural packets that fail those gates are documentation only. New
PDB/source provenance, donor source, or a measured compiler-shape rule is
required to reopen any exhausted spelling.

## Disposition

Evidence-exhausted at `165/180`, with no legal source change retained after
GE18. `game_engine.obj` remains active and must not be marked `Matching` until
all 180 target functions and every owned data/BSS section pass the strict
gates.
