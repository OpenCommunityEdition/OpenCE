# `game_engine_oddball.obj` public-leaf batch (Jonas, 2026-08-28)

## Result

This bounded wave starts from clean integration commit
`c7193e03dfea49ee52123dc3c7dce850c17e32e1` and raises
`source/game/game_engine_oddball` from `12/38` to `13/38` strict-exact
functions. One target-backed public callback is retained:

| January symbol | Recovered role | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_code_000a1b90` | oddball weapon-dropped assertion callback | `53 / 64` | `5` | `96dd97e88e6ea8b00682ab8c06e865a8cb7767da2ae64954643bc56b1a2409ab` |

The retained-only replay is `13 exact / 0 residual / 25 unwritten`.
All 12 inherited exact functions remain exact. The wave adds 53 meaningful
bytes in 64 target-owned padded bytes. No object, data, or BSS completion is
claimed, and no commit or push is made.

## Authority, guides, and provenance

The January PC split object is the byte and ownership authority:

- path: `build/split/source/game/game_engine_oddball.obj`;
- raw SHA-256:
  `70aa145fd07f88ee32489a708e7b01f521555bb2751621bc19d4919373a856e1`;
- compiler: Microsoft C/C++ `13.00.9254.1`, raw SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- unchanged edge: `/O2 /Oy- /DDEBUG /Dxbox` with the generated include set;
- acceptance oracle: hardened `tools/coff_compare.py` section equality via
  `tools/campaign/gate.py`.

The current repository methodology, acceleration playbook, campaign-tool
README, prior Oddball predicate ledger, and game-engine player-added family
ledger were read before emission. The authenticated current Stian guide at
commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf` has Git blobs
`4996babaf5cea48916feb580df9a4e91bb9d779e` (`CLAUDE.md`) and
`7ade6a3a9a0632bca117bb8a5e4a2376a36f484a` (`AGENTS.md`). Their content
SHA-256 values are respectively
`116eda38343bf1a290f0b04797a74ab92c97dfa12eeb2a17f813480f5ec70bbb`
and
`a5188f5a573ac3dcf077f50f49ffb99cfb69b1df138f414242207e53dc975b3d`.
The Pastudan guide pair at commit
`918af885935ec470a31256ecce9a977b12b01f80` is byte-identical at content
SHA-256
`39736825cebf6edd2d2d8e8f9481d36b36954227f00370fcd9c089b6e045c561`.

The cross-build semantic donor is the clean HCEA checkout at
`c168af2e747d3095d9a29418ae401f3a39544863`, origin
`https://github.com/surreptitiousresearch/halocea.git`. The selected donor
blobs are:

| HCEA path | Git blob |
| --- | --- |
| `src/data/oddball_engine.c` | `14eb6801dce93eb7ac4e4252f3e7779e51fe1902` |
| `src/oddball_weapon_drop.c` | `918cb7334334e82726f799e9556a3536c4d634ff` |
| `src/oddball_get_score.c` | `3d9e63191ef0043c5880a2f7f88798f4733b24a0` |
| `src/oddball_get_score_string.c` | `dfd34fb1bb317982193b1aea9b177a438ff562d7` |
| `src/oddball_get_score_header_string.c` | `a0fb2e227722dcc4dbe51ec01b3f6adb4b0a11ec` |
| `src/oddball_get_team_score_string.c` | `49146dc78f401d27420a6f6c8d7274a3e1d5dbb3` |
| `src/oddball_test_flag.c` | `76e09f1e8f9070dd09466c0e9e9ac0e6940ad636` |
| `src/oddball_test_trait.c` | `3748b761fdda33c71a989e56f0fd8d98a29b1e2c` |
| `src/oddball_weapon_pickup.c` | `3502e07f7b78e14eaf2cbea05cb332ed423fd56b` |
| `src/player_has_ball.c` | `4a01b3cd200aef5c52d41b47f060cf083b872f7f` |
| `src/ball_available.c` | `ce7ee614e721b2ba644629571a3fda34225257f3` |

HCEA supplies names, typed behavior, and table topology only. January PC
bytes, relocations, call order, assertions, field offsets, and section
ownership remain authoritative. This distinction matters for the sole exact
win: HCEA's later `oddball_weapon_drop` is a no-op, whereas January PC proves
the source assertion described below.

## January engine-table mapping

The target-owned `_oddball_engine` section is 136 bytes with 27 relocations
and normalized SHA-256
`ffe1ef0aa827017276299f3a497655019d91f94374129c31634061500e5ae4b9`.
Its relocation slots, the offset-asserted PC `struct game_engine`, and HCEA's
later table establish the public callback frontier:

| PC offset | January owner | Recovered role | Disposition in this wave |
| ---: | --- | --- | --- |
| `0x08` | `_code_000a1630` | dispose | inherited exact no-op |
| `0x0C` | `_code_000a1fc0` | initialize for new map | stateful 288-byte body; excluded |
| `0x10` | `_code_000a1640` | dispose from old map | inherited exact no-op |
| `0x14` | `_code_000a1650` | player added | inherited exact |
| `0x18` | `_code_000a1670` | game ending | inherited exact no-op |
| `0x1C` | `_code_000a1680` | game starting | inherited exact no-op |
| `0x20` | `_code_000a1690` | statistics append | inherited exact no-op |
| `0x24` | `_code_000a16a0` | handle client message | inherited exact no-op |
| `0x28` | `_code_000a16b0` | handle server message | inherited exact no-op |
| `0x2C` | `_code_000a16c0` | pregame post-rasterize | inherited exact no-op |
| `0x30` | `_code_000a16d0` | post-rasterize | inherited exact no-op |
| `0x34` | `_code_000a21e0` | player update each tick | large 448-byte body; excluded |
| `0x38` | `_code_000a23a0` | objective weapon update | stateful non-leaf; excluded |
| `0x3C` | `_code_000a26f0` | weapon pickup | first-shot reject; pruned |
| `0x40` | `_code_000a1b90` | weapon dropped | retained exact |
| `0x44` | `_code_000a2460` | engine update | stateful non-leaf; excluded |
| `0x48` | `_code_000a1bd0` | combined player/team score | first-shot reject; pruned |
| `0x4C` | `_code_000a1ce0` | format player score | first-shot reject; pruned |
| `0x50` | `_code_000a1d30` | format score header/name | first-shot reject; pruned |
| `0x54` | `_code_000a1da0` | format team score | first-shot reject; pruned |
| `0x58` | null | allow pickup | absent |
| `0x5C` | `_code_000a1820` | player damaged player | inherited exact no-op |
| `0x60` | `_code_000a2530` | player killed player | large 448-byte body; excluded |
| `0x64` | `_code_000a18a0` | display/format score message | large 736-byte body; excluded |
| `0x68` | null | starting-location rating | absent |
| `0x6C` | `_code_000a1b80` | prespawn player update | inherited exact no-op |
| `0x70..0x78` | null | postspawn/team/goal hooks | absent |
| `0x7C` | `_code_000a1c70` | test flag | first-shot reject; pruned |
| `0x80` | `_code_000a1c90` | test trait | first-shot reject; pruned |
| `0x84` | null | did player win | absent |

## One frozen source batch

Exactly one code-producing batch was emitted. It attempted eight natural,
typed-C public callbacks: the retained weapon-drop assertion plus the score
query, three score formatters, two predicates, and weapon-pickup callback.
The source used `player_get` and `weapon_get` for typed datum/object access,
the typed Oddball globals, the current offset-proven `game_variant` fields,
and the HCEA behavioral topology only where January disassembly independently
confirmed it.

The first-shot source Git blob is
`08c3fabd7f6866af7a5927679860ad6f4aba6103`. Its raw candidate object has
SHA-256
`18011aae979d35412feb8f55d4372717ce691519b584a430129909b206e7d931`.
The frozen gate reported `13 exact / 7 residual / 18 unwritten`.

The exact callback is target-specific and source-authentic. January calls
`weapon_is_flag(weapon_index)`, asserts it at source line `0x3C8` using the
real file and expression strings, and follows the repository's natural
`display_assert`/`system_exit` lowering. Its five ordered relocations are
`REL32 +0x08` to `_weapon_is_flag`, `DIR32 +0x1B` to the file string,
`DIR32 +0x20` to the expression string, `REL32 +0x25` to
`_display_assert`, and `REL32 +0x2C` to `_system_exit`.

The seven nonexact sections were pruned immediately without source-shape or
scheduling tuning:

| January symbol | Target padded / relocs / hash | First-shot candidate padded / relocs / hash | Disposition |
| --- | --- | --- | --- |
| `_code_000a1bd0` | `64 / 4 / 4d6aed81aac168da1427d10c8ba620b0db70bfbcfbbdfe4c0a6e80cf195560a9` | `64 / 4 / 8aa7b75c23c6d27a2ae1cf425de7f0267d3d6b1e7d9410d5f07fb2d3c613cc0c` | byte residual; removed |
| `_code_000a1c70` | `32 / 1 / de0ce82e465fbc78669633e280fc3e8d28a78ac4115beb6c829c9d719a40828e` | `32 / 1 / f52f0238bf95b8527e5b60ea23aacc280c9d3d8003db4e790ca394c7b1276edf` | byte residual; removed |
| `_code_000a1c90` | `80 / 4 / b77e4d47ea5689700a1341e08e939c723b83dcab33da43f75ba65bcb2299c294` | `96 / 4 / 24983b01ca711b3cdc389e2b754ef2ae094e0526752db41ec0f2c3af722acb0e` | size and byte residual; removed |
| `_code_000a1ce0` | `80 / 5 / 549cac99e889407ee34639a9c15525203d66ff8788bd7a8931deac935850f214` | `80 / 5 / 295cad37b7d28889629a9184c1dea79a864eaa5d386214f6a944a65b119d9413` | byte residual; removed |
| `_code_000a1d30` | `112 / 7 / 02ad29005b73cf247bebad214da4b0154c333a597bea246f789ecd84f8417535` | `112 / 7 / 33547ccc2f79ebe1bee1d55edcffdd363d98f95ea4309f1383219ebac1e1070c` | byte residual; removed |
| `_code_000a1da0` | `80 / 5 / 8c1aa2c3a35d10a2429567e40d796f2ef09347165927d4a31c9d30c521a2574d` | `80 / 5 / a344a18dad853374a87ed6a38c0056ca74c02bb1c95c6e50f63a004a842d0f98` | byte residual; removed |
| `_code_000a26f0` | `176 / 11 / 5a02cde3ffb5b1a1cf8cd8966b04c53a8afd3bf02ab271a4586dd8c019b7d5b4` | `176 / 11 / b7f811fbdfc705c6635e1059d4989eaa6091b87a35b1510bbf0982ba6d6ba523` | byte residual; removed |

The earlier documented `_player_has_ball` and `_ball_available` natural
candidates were not retried. Their previous ledger already records correct
sizes and relocation identities but nonexact loop/prologue shapes. Private
helpers `_code_000a16e0`, `_code_000a17b0`, `_code_000a17f0`,
`_code_000a1c10`, `_code_000a1c30`, `_code_000a1c50`,
`_code_000a1df0`, `_code_000a1f30`, `_code_000a20e0`, and
`_code_000a2150` were excluded as private-ABI, state-machine, or caller-cluster
work rather than public leaves.

## Retained-only replay and ownership

The final source Git blob is
`9216eab9cdf76d3683172b05b74388ecebc7fb55`. Its raw replay object has
SHA-256
`65caf589533db073498956bd88f2c5461d361fa07c6b8de2d5c3af1705b5ea50`.
The replay emits exactly the 12 inherited exact bodies and the intended new
callback. The strict gate reports `13 exact / 0 residual / 25 unwritten`.

The pre-existing 260-byte Oddball BSS remains strict exact, with zero
relocations and normalized SHA-256
`9c0095c04ef53d9df41602f3783c90ef3c3e27cc9d0b38262d23930db6313f5a`.
This wave does not emit or claim `_oddball_engine`, add a data owner, or emit
helper COMDATs. The source diff is limited to the typed weapons include and
the exact callback appended after every inherited body, preserving inherited
section identities.

## Policy and do-not-repeat

The retained source is ordinary C89 with berth formatting, one parameter per
line, and an explicit terminal return. It uses the typed `weapon_get` family
indirectly through `weapon_is_flag` and adds no raw object/tag access. It uses
no assembly, `volatile`, `register`, pragma, intrinsic, barrier, forced inline,
raw address, raw-offset dereference, pointer/integer conversion, union/type
pun, undefined behavior, synthetic caller, byte patch, compiler-flag change,
or comparator exception.

Units, Vehicles, Claude-active source, configuration, semantic exceptions,
and unrelated paths are untouched. Reopen a rejected public leaf only with
new January PC source/topology evidence; do not retry the frozen HCEA spelling
or tune its compiler output. Do not import HCEA's no-op weapon-drop body into
January PC. Leave the large/stateful public bodies and private helper clusters
for separately scoped work.
