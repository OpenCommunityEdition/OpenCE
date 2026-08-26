# Game-engine player-added callback family (2026-08-26)

## Result

This bounded wave starts from cumulative commit
`87090a857c10c14a8751239e0051c6e3864e3a61` and retains one strict-exact
PC callback in each of three unprotected translation units:

| Unit | Function | Meaningful / padded | Relocations | Object progress |
| --- | --- | ---: | ---: | ---: |
| `source/game/game_engine_ctf` | `_code_0009ec80` | `24 / 32` | `2` | `12/39 -> 13/39` |
| `source/game/game_engine_king` | `_code_000a03c0` | `24 / 32` | `2` | `10/29 -> 11/29` |
| `source/game/game_engine_oddball` | `_code_000a1650` | `24 / 32` | `2` | `11/38 -> 12/38` |

All three sections have normalized SHA-256
`cc93a70dfca252fd3df5197977186fd70762b9ddb4a73e2d4aaf4bb7fc249eaa`.
Their ordered relocation identities are also identical: `DIR32` at `+0x08`
to `_player_data+0` and `REL32` at `+0x0F` to `_datum_get+0`.

The wave adds three exact functions and 72 meaningful code bytes. No object
becomes complete and no data credit changes. The three objects remain active
`NonMatching` units.

## PC semantic, ABI, and ownership proof

January's 136-byte `_ctf_engine`, `_king_engine`, and `_oddball_engine` data
records each relocate offset `+0x14` to the selected function. The current
PC `struct game_engine` independently types that slot as
`void (*player_added)(long player_index)`. The target code in every selected
section loads the one stack argument, loads `_player_data`, calls `_datum_get`,
discards the typed pointer, and returns through the ordinary C stack ABI.

The retained source expresses exactly that behavior through the established
subsystem accessor:

```c
void code_...(
    long player_index)
{
    player_get(player_index);

    return;
}
```

`source/game/players.h` defines `player_get` as the typed wrapper around
`datum_get(player_data, index)`. The source therefore follows the house rule
for typed data access and does not reproduce a raw `datum_get` cast. It uses
ordinary C89, one parameter per line, and an explicit terminal `return;`.
There is no assembly, `register`, `volatile`, pragma, barrier, intrinsic,
attribute, raw address or offset dereference, pointer/integer conversion,
type or union pun, undefined behavior, synthetic anchor, byte patch, compiler
flag change, or comparator exception.

The candidate defines only the three target-owned callback COMDATs and adds
ordinary undefined references to `_player_data` and `_datum_get`. It claims no
engine-table data owner. Oddball's pre-existing 260-byte `_bss_0043eba8`
section remains strict exact with zero relocations and normalized SHA-256
`9c0095c04ef53d9df41602f3783c90ef3c3e27cc9d0b38262d23930db6313f5a`.

## Claude and HCEA audit

All 27 reachable `claude/*` tips were searched before emission. Across those
tips, the CTF, King, and Oddball paths reduce to two historical blobs each:

| Path | Claude-reachable blobs |
| --- | --- |
| `game_engine_ctf.c` | `7d1824586451f6b05054582305ab28591bf6a8fe`, `9cc543cfb30ea3137cd1e23c98d7568dff3eea5f` |
| `game_engine_king.c` | `423293cbb9404d0fbd2c0f3cf31cebb63a4123d5`, `3f4f97142a64a75b3a6e7b32cc4c152a2f076890` |
| `game_engine_oddball.c` | `f667bbd365647db18e116152c40a9f96ce1078ec`, `92372f1750b71edff0c23d0700a97d4123911028` |

None defines the selected function. Claude's `residual_frontier.json` lists
all three only as `target_only`; it provides no residual recipe, experiment,
or positive body evidence. Reachable path history contains only the initial
shells, the already-integrated no-op callback waves, and the later Oddball
global-state recovery. The selected callbacks are therefore novel rather than
repeats of a Claude candidate.

The clean HCEA semantic repository is pinned at
`c168af2e747d3095d9a29418ae401f3a39544863`. Its game-engine data tables
corroborate the `player_added(int)` slot and signature. The relevant table
blobs are `8b555ad0e05c3b0530ac746eef703cc292baa871` (CTF),
`d143f1e8375ac611a38b54b262b90a092540214a` (King), and
`14eb6801dce93eb7ac4e4252f3e7779e51fe1902` (Oddball).

HCEA is explicitly **not** a body donor here. Its later-build body blobs are
`b8afe3b31cf75e187b28d143e1e4c4734a3045a6`,
`6929a2f4141031e98db07e87586a70d026aa901a`, and
`41c10d61e9f1a09ad04ab5fe224ac9651b6d3668`: CTF is a no-op, while King and
Oddball perform later network-score initialization. That cross-build semantic
divergence is recorded to prevent an invalid reverse transfer. January PC
COFF, the PC engine table, and the current typed PC header remain the byte,
ABI, and behavior authority for this wave.

## Frozen compile waves

The pre-wave regression manifest is
`build/audit/game_engine_player_added_baseline_20260826.json`, SHA-256
`b41ad1557ec219df4e3b40ee4c28c75ff58fda2cd684648718a2436bb03af143`.
The January split-object SHA-256 values are:

- CTF: `7dad89f40b3c4206575064e98337ec7019fc243e2022e31f6585a4d6018b1baa`
- King: `99ece249fe1ba736b7848c4b9f905e5330ef71d743828506b479ff356a9d6206`
- Oddball: `70aa145fd07f88ee32489a708e7b01f521555bb2751621bc19d4919373a856e1`

The XDK compiler is Microsoft C/C++ 13.00.9254.1, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
with the unchanged `/O2 /Oy- /DDEBUG /Dxbox` edge.

The first source shape passed `/Zs` after adding the normal core-type include
required by `players.h`. The first and only code-producing compile for that
frozen shape emitted all three target functions strict exact, but placing the
new definitions in address order renumbered later inherited COMDAT and
`.debug$F` sections. The first-shot raw object SHA-256 values are:

- CTF: `fded140b1ffe219a6cebd4a5b8261804848073652dad421c9927d52bf0d9442d`
- King: `fcc8e384669dd9e0f286e0b0087c637d5b6d66b0be477fdf894415f80af26c35`
- Oddball: `dec7447ada3dccdde76c27c1fee67c492068086df12f7d2303821ec9b9a0d396`

The fail-closed whole-TU gate supplied genuinely new evidence: although direct
comparison proved every inherited runtime section still exact, their physical
section identities had moved. A corrective evidence wave moved each new
definition after all inherited bodies. It passed `/Zs`, then one three-edge
XDK compile. All inherited section identities stabilized; the pre-wave gate
then reported all 33 inherited functions in `still_exact`, no changed nonexact
function, the three intended `newly_exact` functions, and only the expected
new symbol-set ownership. The corrective raw object SHA-256 values are:

- CTF: `1950f84329a55d47d29ab45c0f67fb9ae7fa045ca3220c774c237a88f9790fb6`
- King: `d16f99e18793f59495d2ee27884781b2f231e6bc9a51a3e9896603cea8ed4532`
- Oddball: `3f0ba3f897011d9d48090ea71821874e3d96966de0189a6881a6331b7eed9894`

No further spelling or scheduling search followed. Later compiles are only
committed-state reproducibility replays.

## Integration and validation

The implementation is cumulative commit
`80152bc9d53e9e52635f7641ed810368cd33c38f`. Its source blobs are
`16ce76f78b01e71cc6c2aab0d07724adcf252a80` (CTF),
`939eb5bfdc8fb394a38a933b2285abcd2cb0664f` (King), and
`97cbc5c455ea869be5dcb6d7783e15414506fec9` (Oddball).

After report regeneration:

- all campaign code is `508,369 / 2,198,102` meaningful bytes and
  `4,194 / 11,060` functions;
- Halo code is `495,455 / 1,770,166` bytes and `4,027 / 7,574` functions;
- data remains `1,836,756 / 4,176,062` campaign-wide and
  `1,831,564 / 3,923,451` for Halo;
- complete objects remain `377 / 833` campaign-wide and `275 / 468` for Halo.

The strict semantic audit scans 470 units and evaluates 4,302 functions. It
reports 4,159 semantic exact, 114 hidden exact / 64,705 hidden bytes, 4,220
accepted exact, and zero unit errors. Object admission reports zero candidates
and zero revocations, with only the inherited `source/shell/shell_xbox`
completion-label contradiction. Parked validation remains 3 active, 0 stale,
0 invalid. The complete tooling suite passes 179/179 tests. Both
`halobetacache_build` and `libcmt_build` report no work pending.

The committed manifest is
`build/audit/game_engine_player_added_committed_20260826.json`, SHA-256
`2d7cd9825338ee8ad40529e642b69a9a3d62f79ee2d53675816329afa1c665f3`.
All three base objects were then deleted by verified absolute path and rebuilt
through exactly three XDK edges. The replay raw SHA-256 values are:

- CTF: `46defe1afbfe34d52b512a6a969e39f9fbce89076b55544655b41cadf4429227`
- King: `f2a85af1f640e2f6fb270c66c7fcb980804d58d383de60c0908cbfe254a7639f`
- Oddball: `048c8cb2047b4edb52ec4fddedc8d6f4605813d0cdf0c3eaa380cdde2ee2cf7b`

The committed-manifest replay is fully clean: zero failures, zero warnings,
zero newly exact or changed-nonexact functions, and all 36 emitted functions
in `still_exact`. Direct hardened comparison again proves the three new
functions and Oddball BSS exact. Regenerated semantic results reproduce the
same 4,220 accepted functions and zero unit errors.

No protected Units source, header, configuration, semantic exception, parked
record, pre-existing Claude document, or unrelated path changes. No push is
performed.

## Do not repeat

Do not move these functions back into address order: that preserves code bytes
but churns inherited section identities. Do not import the later HCEA bodies;
their semantics are from a different build. Do not retry raw `datum_get`
casts, synthetic callers, compiler flags, assembly, volatility, register
hints, barriers, pragmas, intrinsics, or byte forcing. Reopen this family only
if the pinned January objects, compiler, typed PC engine-table evidence, or
strict comparator changes.
