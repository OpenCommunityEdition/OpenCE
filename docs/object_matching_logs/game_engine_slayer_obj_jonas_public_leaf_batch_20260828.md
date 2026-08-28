# `game_engine_slayer.obj` Jonas public-leaf batch (2026-08-28)

This Jonas-owned ledger records one frozen natural typed-C batch for five
small public Slayer callbacks. Three bodies were strict on the first
code-producing compile and are retained. Two bodies were residual and were
removed with all candidate-only support, without a spelling, topology,
scheduling, ABI, or compiler retry. The object advances from 11/27 to 14/27
strict functions and remains `NonMatching`.

## Authority and research boundary

- Integration base: `c7193e03dfea49ee52123dc3c7dce850c17e32e1`.
- Isolated branch: `jonas/game-engine-slayer-typed-batch-20260828`.
- Input source blob: `1a096c4ca64ff4340bd525c205b816b91dead5d0`.
- January split-object SHA-256:
  `2430c01be21865b84d2b0ad32c69676745adb64f3c6f4de77792f07c7a989a88`.
- January `_slayer_engine`: 136 bytes, 24 relocations, normalized SHA-256
  `cd47b29508fea4e4c11451792c17f629a9ad240b3c3df5c889c0d416c378bc78`.
- January `_bss_0043ed80`: 128 zero bytes, zero relocations, normalized
  SHA-256
  `38723a2e5e8a17aa7950dc008209944e898f69a7bd10a23c839d341e935fd5ca`.

All unique applicable `AGENTS.md` and `CLAUDE.md` payloads in the current,
complete-history, full-history, cseries, and Pastudan research caches were
read; byte-identical copies were de-duplicated by SHA-256. The five existing
game-engine ledgers were also read in full. There was no dedicated Slayer
object ledger.

Every `slayer_*.c` body under the pinned HCEA c168
`research-cache/halocea-cseries-20260820/src/blam/game` tree was read. The
HCEA set supplies public names, signatures, field intent, and cross-build
behavior. It is not the PC byte oracle: the Xbox build has callbacks and
network behavior that are absent or differently shaped in January PC.
January's engine-table slots, call/relocation topology, instruction bodies,
and existing PC types decide the mappings and admission below.

Authenticated Stian/Pastudan trees and their reachable source history were
searched for every Slayer name. They provide no usable body for the selected
leaves. Pastudan contains only a speculative raw-layout lift for the much
larger display/update cluster. The reachable PC path history contains shells
and the already integrated eleven no-op callbacks, not a prior candidate for
this batch. The Punpckhdq copy is likewise an empty shell.

## Complete remaining-body map

The table maps all 16 functions that were unwritten before this wave. Padded
size, relocation count, and normalized SHA-256 are January authority.

| January function | Source-authentic identity | Target padded / relocs / normalized SHA-256 | Disposition |
| --- | --- | --- | --- |
| `_code_000a3cf0` | private player predicate; no authenticated one-to-one HCEA donor | `80 / 4 / 808067cc594efac201433065c1a8e5c5377ded0db898fe684eb4805623dbe6a1` | omitted: private ABI |
| `_code_000a3d50` | `slayer_engine_initialize_for_new_map` | `48 / 4 / 4cdb016e0e1e11cf93bcf595cf19d24fe3bd520053ee29f592c538cc413b55a7` | omitted: anonymous BSS owner |
| `_code_000a3d90` | `slayer_engine_player_added` | `48 / 2 / 036ea5d1290e59dfb98c99aab50075117de32320687b76adc4496a2cceda5230` | retained exact |
| `_code_000a3dd0` | `slayer_engine_game_starting` | `32 / 2 / 90948ccee91829f6761b47939eaf902a2a089387c3282528b19f24a4e098e2b9` | retained exact |
| `_code_000a3e50` | `slayer_engine_allow_pick_up` | `16 / 0 / 1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707` | retained exact |
| `_update_speed_for_score` | private score/momentum helper referenced by HCEA kill flow | `272 / 20 / 228c484a2793f87033fb85cdb3e1daf2dfc8d327dfd0012ff39e5bce72413c89` | omitted: private helper |
| `_code_000a3f80` | `slayer_engine_adjust_score` | `64 / 5 / f476166fccdd1e7cc8b108853660585a73250d0aea24085382d1399b5fef0045` | omitted: anonymous BSS owner/private ABI |
| `_code_000a3fd0` | `slayer_get_score` | `64 / 4 / 4d6aed81aac168da1427d10c8ba620b0db70bfbcfbbdfe4c0a6e80cf195560a9` | omitted: anonymous BSS owner |
| `_code_000a4010` | `slayer_test_flag` | `16 / 0 / 3bc62756da9998565ac11450532809cd4eabe78d41a29fb8043ff3230dbe9786` | first-shot reject |
| `_code_000a4020` | `slayer_get_score_string` | `48 / 3 / d7bf56e34d7291483a7a7c46f4749b2e08f932499e433b33b43347204fe7ff11` | omitted: anonymous BSS owner |
| `_code_000a4050` | `slayer_get_score_header_string` | `96 / 6 / 5634f4fc63ca196bd01bfedc309a0b89d402b324ab777ee1adbe7fcb8e37a19a` | first-shot reject |
| `_code_000a40b0` | `slayer_get_team_score_string` | `48 / 3 / 7114bd24ec12efa5d7fb5cbe0ab7007c0d0d197d8742aea419d60e52abbdf61a` | omitted: anonymous BSS owner |
| `_code_000a40e0` | private `find_next_target` helper referenced by HCEA kill/update flows | `400 / 25 / a775cfffd3d045ceb91b9a463ea4134d773771dcb810772d55e8526da290759f` | omitted: private ABI/state machine |
| `_code_000a4270` | `slayer_engine_player_killed_player` | `144 / 9 / a887215eb035c524402fd2e0a3ae58f8f90c95608ef6a00aa3a547155468a8a0` | omitted: coupled private helpers |
| `_code_000a4300` | `slayer_engine_display_score` | `640 / 52 / 0c232f29dab93c8f31dcea0fae6d71a6024bef79360a36634aedbeb8c8e97b08` | omitted: large formatter |
| `_code_000a4580` | `slayer_player_update` / engine `player_update_each_tick` slot | `400 / 31 / 33682f71bca6af3fb5abf7d08fc18dd076a4bf720e952b56ff3006ba451c8743` | omitted: large state/update body |

The HCEA-only team-score getter, team-change callback, and replicated-state
callback do not have separate one-to-one January functions. January folds
team score into `_code_000a3fd0`, has no matching engine-table team-change
slot, and uses no corresponding HCEA network-state callback here.

## Frozen batch and gate

The fixed batch appended five definitions after the eleven inherited exact
bodies, avoiding inherited COMDAT identity churn. It used the established
`player_get` typed accessor, the verified `player_datum.unknown88` field,
ordinary `game_engine_has_teams` and multiplayer-sound calls, named local
sound constants, tag/text APIs, and readable C control flow. A production-
argument `/Zs` pass succeeded before emission.

Exactly one code-producing production compile emitted a 4,580-byte first
artifact with SHA-256
`cb2c85124708a2f41035ba8b44d7cbc32806829d340604104292719d0212feeb`.
Its frozen gate was:

| Candidate | Target padded / relocs / hash | Candidate padded / relocs / hash | Result |
| --- | --- | --- | --- |
| `_code_000a3d90` | `48 / 2 / 036ea5d1290e59dfb98c99aab50075117de32320687b76adc4496a2cceda5230` | identical | exact |
| `_code_000a3dd0` | `32 / 2 / 90948ccee91829f6761b47939eaf902a2a089387c3282528b19f24a4e098e2b9` | identical | exact |
| `_code_000a3e50` | `16 / 0 / 1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707` | identical | exact |
| `_code_000a4010` | `16 / 0 / 3bc62756da9998565ac11450532809cd4eabe78d41a29fb8043ff3230dbe9786` | `16 / 0 / 1b22de215bf4081cc0932bfbcda377792a97a32759ffa6bee733b57bad919bd9` | reject |
| `_code_000a4050` | `96 / 6 / 5634f4fc63ca196bd01bfedc309a0b89d402b324ab777ee1adbe7fcb8e37a19a` | `80 / 6 / 6642902d005f708954a7343d1efc251e6de94381ab033888f13bce26059ca79e` | reject |

The natural `flag == 1` candidate lowered to `cmp/sete`; January uses a
decrement-and-branch byte result. The score-header candidate preserved the six
semantic calls/data references but was 16 bytes short and scheduled the same
references at different addresses. Both were removed immediately. No
alternative expression, branch form, local lifetime, declaration, ABI, or
code-generation control was tried.

The three accepted functions contribute 60 meaningful bytes, 96 padded bytes,
and four exact relocations. Player-added has the exact ordered `_player_data`
and `_datum_get` references and stores `NONE` through the named `unknown88`
field. Game-starting has exact calls to `_game_engine_has_teams` and
`_game_engine_play_multiplayer_sound`. Allow-pick-up owns no relocation.

## Ownership, policy, and replay

The retained object introduces only the three external code owners above and
their four ordinary undefined references. It owns no `.data`, `.bss`, COMMON,
string, tag, engine table, or other runtime data. The target's 136-byte
`_slayer_engine` and 128-byte `_bss_0043ed80` remain unclaimed. Bodies whose
natural reconstruction requires that anonymous address-derived BSS owner were
deliberately omitted; no raw owner spelling or address was introduced.

The final source is ordinary typed C89. It contains no assembly, `volatile`,
`register`, pragma, intrinsic, optimizer barrier, incompatible call, raw
address/offset dereference, representation pun, pointer/integer
reconstruction, undefined behavior, synthetic anchor, byte patch, or
byte-forcing expression. No shared header or configuration changed.

After pruning the two rejects and their text/tag support, one retained-only
production replay emitted a 3,816-byte object with phase-specific SHA-256
`2fdc50036339fe0a9f5ac6b971d36ce2fddbe944bf7555f9bcdc1b510121219e`.
Direct strict comparison reports:

```text
exact 14  residual 0  unwritten 13  total 27
```

All eleven inherited exact functions remain exact. The two rejected symbols
and the eleven deliberately omitted bodies are absent. Final tracked scope is
`source/game/game_engine_slayer.c` plus this additive ledger only. Units,
Vehicles, Claude-active work, existing ledgers, parks, completion labels, and
tooling are untouched. No commit or push was performed.
