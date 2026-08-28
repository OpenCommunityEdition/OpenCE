# `game_engine_ctf.obj` public-leaf wave (Jonas, 2026-08-28)

## Result

This bounded wave starts from integration commit
`734dd02e1fdb650169171048cc5e00955cb509af` and raises
`source/game/game_engine_ctf` from `13/39` to `17/39` strict-exact
functions. It retains four source-authentic public callbacks:

| January symbol | Recovered role | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_code_0009ecb0` | `ctf_engine_game_starting` | `9 / 16` | `1` | `92b040330e430d47c89a17a0381d3b3644b558ec5c17080372cd0efc40c80f4a` |
| `_code_0009ee80` | `ctf_engine_allow_pick_up` | `117 / 128` | `5` | `2ca9528b42e4f96877f9f8bafc5d574c580aa90dd07f7d6460a2361de0762e55` |
| `_code_0009f3d0` | `ctf_get_score_string` | `50 / 64` | `4` | `6d24cbc07d2fca9aea91b56ec9302a7b74772be336d0fce91fff482ccf367a61` |
| `_code_0009f470` | `ctf_get_team_score_string` | `37 / 48` | `3` | `7114bd24ec12efa5d7fb5cbe0ab7007c0d0d197d8742aea419d60e52abbdf61a` |

The improvement is `213` meaningful bytes in `256` target-owned padded
bytes. The retained-only replay is `17 exact / 0 residual / 22 unwritten`.
No object, data, or BSS completion is claimed. No commit or push is made.

## Authority and provenance

The January PC split object is the byte and ownership authority:

- path: `build/split/source/game/game_engine_ctf.obj`;
- size: `19,070` bytes;
- raw SHA-256:
  `7dad89f40b3c4206575064e98337ec7019fc243e2022e31f6585a4d6018b1baa`;
- compiler: Microsoft C/C++ `13.00.9254.1`, raw SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- unchanged edge: `/O2 /Oy- /DDEBUG /Dxbox` with the generated include set;
- acceptance oracle: hardened `tools/coff_compare.py` section equality via
  `tools/campaign/gate.py`.

The current and historical project guides were read before source emission.
The current authenticated Stian guide fingerprints are
`a5188f5a573ac3dcf077f50f49ffb99cfb69b1df138f414242207e53dc975b3d`
for `AGENTS.md` and
`116eda38343bf1a290f0b04797a74ab92c97dfa12eeb2a17f813480f5ec70bbb`
for `CLAUDE.md`. The authenticated Pastudan guide pair is byte-identical at
`39736825cebf6edd2d2d8e8f9481d36b36954227f00370fcd9c089b6e045c561`.
The applicable object history is
`docs/object_matching_logs/game_engine_player_added_family_jonas_20260826.md`;
there was no dedicated current `game_engine_ctf.obj` ledger before this wave.
The general `game_engine.obj` ledgers contain tooling policy but no CTF body
donor.

The cross-build semantic donor is the clean HCEA checkout:

- origin: `https://github.com/surreptitiousresearch/halocea.git`;
- commit: `570c83fd9c365dad6f2a3e7041705d5b84c7847c`;
- selected donor blobs:

| HCEA path | Git blob |
| --- | --- |
| `src/blam/game/ctf_engine_game_starting.c` | `7614ce7285094e671300df7fa34968e32f147a69` |
| `src/blam/game/ctf_engine_allow_pick_up.c` | `b13724c55e508073cf2f787cc3898ed30df55422` |
| `src/blam/game/ctf_get_score.c` | `6a788580fdcbf9ea62af77dddbcc7eee00b5fa1d` |
| `src/blam/game/ctf_test_flag.c` | `3e895306a1133427735d5a14d3f1c7a028decfe8` |
| `src/blam/game/ctf_get_score_string.c` | `2109060a9dab59113bb916a743637168ab56ad27` |
| `src/blam/game/ctf_get_score_header_string.c` | `a0d508c25d21ff8379dda022bd5ac1a023ad8d5d` |
| `src/blam/game/ctf_get_team_score_string.c` | `9ca074f519a804f846bd39338e81c9c9d98b40d9` |
| `src/data/ctf_engine.c` | `8b555ad0e05c3b0530ac746eef703cc292baa871` |
| `src/headers/ctf_globals.h` | `3b67a23e5b9713902aab02e757cb844a388bcd92` |

HCEA is a semantic and naming donor, not a byte authority. Its later engine
record is `156` bytes rather than January's `136`, and its CTF globals are
`52` bytes rather than January's `48`. It adds a player-removed slot, a
separate team-score callback, and three networking/team-change tail slots.
The wave therefore transfers no HCEA data owner and adapts bodies only where
the January call and relocation topology independently proves the older PC
shape.

Authenticated history audits were also performed at Stian commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf` and Pastudan commit
`918af885935ec470a31256ecce9a977b12b01f80`. Neither reachable tree contains
a tracked `game_engine_ctf.c` or CTF body donor. Pastudan's reachable deploy
history contains progress/report metadata for a later Xbox reconstruction,
not source bodies. The Stian clone is a promisor checkout; its reachable tree
and path history were empty for CTF, while a deeper content search could not
fetch missing promisor objects offline. No source or byte claim rests on that
negative result.

## January engine-table mapping

The target-owned `_ctf_engine` section is `136` bytes with `28` relocations
and normalized SHA-256
`45c41730d038fd3f985f7ccfd8a7e04aa02e2db549ac941386e7cd04c1e1d8c1`.
Its relocation slots, the current offset-asserted PC `struct game_engine`, and
the later HCEA callback sequence produce this mapping:

| PC offset | January owner | Target/HCEA role |
| ---: | --- | --- |
| `0x08` | `_code_0009ebc0` | dispose |
| `0x0C` | `_code_0009f4a0` | initialize for new map |
| `0x10` | `_code_0009ec70` | dispose from old map |
| `0x14` | `_code_0009ec80` | player added |
| `0x18` | `_code_0009eca0` | game ending |
| `0x1C` | `_code_0009ecb0` | game starting |
| `0x20` | `_code_0009ecc0` | statistics append |
| `0x24` | `_code_0009ecd0` | handle client message |
| `0x28` | `_code_0009ece0` | handle server message |
| `0x2C` | `_code_0009ecf0` | pregame post-rasterize |
| `0x30` | `_code_0009ed00` | post-rasterize |
| `0x34` | `_code_0009f9a0` | player update each tick |
| `0x38` | `_code_0009faf0` | objective weapon update |
| `0x3C` | `_code_0009fdb0` | weapon pickup |
| `0x40` | `_code_0009f340` | weapon drop |
| `0x44` | `_code_0009fd30` | engine update |
| `0x48` | `_code_0009f380` | combined player/team score query |
| `0x4C` | `_code_0009f3d0` | format player score |
| `0x50` | `_code_0009f410` | format score header/name |
| `0x54` | `_code_0009f470` | format team score |
| `0x58` | `_code_0009ee80` | allow pickup |
| `0x5C` | `_code_0009ef00` | player damaged player |
| `0x60` | `_code_0009ef10` | player killed player |
| `0x64` | `_code_0009ef20` | display/format score message |
| `0x68` | `_code_0009ff10` | starting-location rating |
| `0x6C` | `_code_0009f2c0` | prespawn player update |
| `0x70` | null | postspawn/player update |
| `0x74` | null | team-index override |
| `0x78` | null | goal matches player |
| `0x7C` | `_code_0009f3c0` | test flag |
| `0x80` | null | test trait |
| `0x84` | null | did player win |

The non-table relocation topology also identifies the small helper frontier:
`_code_0009eb60` constructs a flag object, `_code_0009ebd0` creates and owns a
team flag, `_code_0009ec30` emits the single-flag attack/defense messages,
`_code_0009ed10` awards a capture, `_code_0009edd0` throttles the flag-failure
sound, `_code_0009ee10` finds a weapon carrier, `_code_0009f2e0` updates a
warning sound, `_code_0009f320` sets a warning, `_code_0009f870` resets a
flag, and `_code_0009f8c0` drops/resets a player's flag. Several of these
receive live arguments in `EAX`, `ECX`, `ESI`, or `EDI`; they were deliberately
excluded as private-ABI work. `_code_0009f950` is a private flag-position
distance predicate and was likewise excluded.

## One frozen source batch

Exactly one code-producing candidate batch was emitted. It attempted seven
natural typed-C bodies: the four retained functions plus the combined score
query, `test_flag`, and score-header formatter. The source used the existing
`player_get` and `weapon_try_and_get` typed accessors, the real
`player_datum`/`weapon_datum` fields, and an offset-asserted `ctf_globals`
view. The older PC pickup rule is proven by target code: resolve the player's
unit, accept `NONE`, accept non-flags and already-handled flags, and deny only
the player's own-team flag. Unlike the later HCEA body, January has no
`game_connection` call or relocation, so none was invented.

The first-shot source Git blob is
`35f09493033f579405611804cec46ad020f15e77`. Its raw object is `5,696` bytes
with SHA-256
`166bb4c349953e4750d95d84492670a5b60ccb520f077cbb3ec48305c622d962`.
The frozen gate reported `17 exact / 3 residual / 19 unwritten`: all four
retained bodies were strict exact on their first compile.

The three rejects were not tuned:

| January symbol | Target size / reloc / hash | First-shot candidate | Disposition |
| --- | --- | --- | --- |
| `_code_0009f380` | `64 / 3 / 8550cc74ca7bb8e32d4888c108c8dd7d8ed8996e8965779c5bdad05c0bf32abe` | `64 / 3 / 05e857d3bc04ce0c1fd9b62ffd2b8e84265d43a3c5d1b3b0115ad4f4704d379e` | normalized-byte residual; removed |
| `_code_0009f3c0` | `16 / 0 / 618732076dca1b8401d28d6892404448514aa66623d95b8ebd9c23966a40f651` | `16 / 0 / e6e18aea5763ab7d4714af5291f172f0b5bbe1c74be48c96096d18f7f896379e` | normalized-byte residual; removed |
| `_code_0009f410` | `96 / 6 / 5634f4fc63ca196bd01bfedc309a0b89d402b324ab777ee1adbe7fcb8e37a19a` | `80 / 6 / 6642902d005f708954a7343d1efc251e6de94381ab033888f13bce26059ca79e` | size and normalized-byte residual; removed |

The rejected definitions and their unused tag/string-list includes and
constants were pruned in one cleanup edit. No alternative spelling, branch
shape, or compiler scheduling search followed.

## Retained-only replay and ownership

The final source blob is
`62cc6858524eac5ea0288e091de8ee72fc261f68`. The retained-only replay object
is `4,728` bytes with SHA-256
`9da85fa2945ada25ad0b7995536d386d149d96d6ae372c8a63045e34e7883f5b`.
It emits exactly the 13 inherited exact bodies and the four intended new
bodies. The replay gate reports `17 exact / 0 residual / 22 unwritten`; all
four selected section sizes, normalized bytes, relocation counts, ordered
relocation offsets, types, and identities equal the January owners.

The target BSS owner is `48` bytes, zero relocations, normalized SHA-256
`17b0761f87b081d5cf10757ccc89f12be355c70e2e29df288b65b30710dcbcd1`.
This wave does not define it. The candidate contains only an ordinary
undefined `_bss_0043e914` reference for the team-score formatter, whose
`DIR32 + 0x10` identity matches the target relocation. It does not emit or
claim `_ctf_engine`. There are no helper COMDATs, synthetic anchors, or new
data owners.

## Policy and scope

The retained source is ordinary C89 with berth formatting, own-line `void`,
one parameter per line, explicit terminal returns where applicable, and typed
tag/object access. It uses no assembly, `volatile`, `register`, pragma,
intrinsic, compiler barrier, attribute, raw address, raw-offset dereference,
pointer/integer conversion, union/type pun, undefined behavior, synthetic
caller, byte patch, compiler-flag change, or comparator exception.

Units, Vehicles, Claude-active source, configuration, semantic exceptions,
and unrelated paths were not modified. The worktree diff is limited to
`source/game/game_engine_ctf.c` and this dated ledger. Large initialize,
update, render/message, network/state-machine, objective-weapon, pickup, and
starting-location bodies remain unwritten. Private-register-ABI helpers remain
unwritten. Reopen the three rejected public leaves only with genuinely new
January PC source/topology evidence; do not retry the HCEA spelling or tune
the compiler output from this frozen batch.
