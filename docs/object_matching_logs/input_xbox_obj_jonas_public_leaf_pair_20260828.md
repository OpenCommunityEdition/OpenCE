# `input_xbox.obj` Jonas public leaf pair (2026-08-28)

## Scope and authority

- Integration base: `c76f7a2872374b7cea6c879895ab43ea4a610087`.
- Isolated branch: `jonas/campaign-public-donor-leaves2-20260828`.
- Isolated worktree: `work/campaign-public-donor-leaves2-20260828`.
- Sole source owner: `source/input/input_xbox.c` / `source/input/input_xbox.obj`.
- January target owner: `build/split/source/input/input_xbox.obj`, 15,987 bytes,
  raw SHA-256 `a956f90624ce49ef7ecbdaf6005c5f008695170ebbee05687ed0dd08e4316457`.
- Compiler: XDK 3911 Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
  `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

January split COFF is authoritative for function ownership, padded extent, normalized
instruction bytes, complete ordered relocations, symbolic destinations/addends, linkage,
assertion strings, and ABI. The applicable historical root `CLAUDE.md` and `AGENTS.md` were
read in full; at commit `76ed248678d92b401b8ff73913d756050129481f` both resolve to Git
blob `1dfb833ebea9fd0fe7dfd880e45e879cc395a41a`. Neither file is present at the
integration base. The Stian and Pastudan repository instructions were also read in full.

The two existing object ledgers were binding inputs:

- `input_xbox_obj_jonas_gamepad_state_leaf_20260821.md` (including the fixed rejection of
  `_input_get_raw_data_string` and the established typed `0x40C` COMMON boundary);
- `input_xbox_obj_jonas_inline_helper_pair_20260827.md` (including the fixed rejections of
  `_code_000be800` and `_code_000bebd0`).

Campaign and audit tool instructions were read before selection. Relevant tool Git blobs are
`tools/campaign/gate.py` `37880af12887a6b4eb455ef800d23dc13a80c500`,
`tools/campaign/board.py` `e1fd37aa55d9f27968a63d1db294d1965eb3ce75`,
`tools/coff_compare.py` `54fa2025044da29225bef633f33c0a9ec0ea603f`,
`tools/audit/dump_coff_disasm.py` `2a3e0bd1b1406d966600e94271a00e511fa12b87`, and
`tools/audit/rank_public_donor_residuals_20260827.py`
`8c847f69daf2f6bbca2b3aa81e009ec1e26f6ada`.

The baseline was **11/27 exact**, zero residuals, 16 unwritten owners, and 368/3,808
exact padded code bytes. Units, Vehicles, all game/rasterizer/decals/game-sound work,
`network_game_manager`, and concurrent Claude/agent paths were read-only.

## Authenticated donor audit

The pinned HCEA source tree is commit `c168af2e747d3095d9a29418ae401f3a39544863`
from `surreptitiousresearch/halocea`. Every same-name file in the frozen candidate set was
read before target emission:

| HCEA path | Git blob |
|---|---|
| `src/input_flush.c` | `ab7bc39df47d53ed07d61c6e64dc7c51cbbecf3b` |
| `src/input_key_is_down.c` | `3bad2563cf2edca80be738cbf5aeac70957827c4` |
| `src/input_get_key.c` | `94a12fbb53964d3185bd9dceded9939de11c1694` |
| `src/input_has_gamepad.c` | `c314ece00c54f1d851cb517622275a6087499552` |
| `src/input_update.c` | `882eda84154ff1acd0bec4a00c0b10fafc38df09` |

HCEA authenticates the public names and high-level ring/key/gamepad behavior, but its later
Xbox 360 bodies are not a January byte oracle: `input_flush` is a stub, its gamepad predicate
only range-checks, and its update body is a platform thunk. The complete preserved retail-Xbox
reconstructions supplied the closer independent semantic topology:

| Donor | Commit | `src/halo/input/input_xbox.c` blob | File SHA-256 |
|---|---|---|---|
| `stianeklund/halo` | `fc11d861865735877d5b8b4c408b1a0c06dde3cf` | `bfb2bc62bfe839fa401674852135b5af0355b449` | `549fd91bad6c17088eb8d8211a92467527a0c7a5841661cc72346b9aead9bb41` |
| `pastudan/halo` | `918af885935ec470a31256ecce9a977b12b01f80` | `a838e5b90cdb841216fd6efc5c2b574593293961` | `ada14a1ac44684e861f1e6d4bd9e46beb255ff8328b4915f2a655b91fefb0744` |

Those files independently corroborate clearing four 40-byte gamepad states, two 104-byte key
arrays, and the 64-entry key ring; dequeuing a four-byte keystroke; checking the typed gamepad
handle; and the update topology. Their raw addresses, pointer casts, platform declarations,
private helper guesses, and source spelling were not copied. January COFF fixed all target
details.

## Target mapping and frozen batch

January disassembly/relocations prove these typed offsets within `_bss_004536a0`:

| Field | Addend | Evidence |
|---|---:|---|
| `suppressed` | `+0x138` | key/gamepad accessors |
| `gamepad_handles[4]` | `+0x13C` | gamepad predicates/accessor |
| `gamepad_states[4]` | `+0x14C` | flush and accessor, 4 x 40 bytes |
| `frame_active` | `+0x22D` | inherited exact frame end |
| `key_ticks[104]` | `+0x238` | flush and key predicate |
| `key_latches[104]` | `+0x2A0` | flush |
| `buffered_key_read_index` | `+0x308` | flush and dequeue |
| `buffered_key_write_index` | `+0x30A` | flush and dequeue |
| `buffered_keys[64]` | `+0x30C` | flush and dequeue |

The aggregate remains naturally typed and exactly `0x40C` bytes. Compile-time `offsetof` and
`sizeof` checks establish every new boundary. `#define input_globals bss_004536a0` is only a
typed source-access spelling so the preserved assertion text remains natural; it emits no
alias symbol or metadata directive.

Exactly four public bodies entered the one code-producing batch. `_input_update` was excluded
before compilation because its January body directly calls unresolved private owner
`_code_000bf150` and then `HATRun`; admitting it would cross the explicit private-ABI boundary.

| Candidate | Meaningful / target padded | Target relocs | Target normalized SHA-256 | First gate |
|---|---:|---:|---|---|
| `_input_flush` | 84 / 96 | 10 | `9c35ae4b34076c4179dea45a47f1af9654e117c324b2da827460901fdfdccf67` | retained strict exact |
| `_input_has_gamepad` | 73 / 80 | 5 | `2a92d28a0cff1e44f794e91dd66959c8ce6db2ad173559bcc5a2d669f82470e9` | retained strict exact |
| `_input_get_key` | 97 / 112 | 9 | `a3b48c1ac1818bbd2c937a5f56fa988454af93d4a023201c03682be5fd73f436` | rejected; SHA/schedule residual |
| `_input_key_is_down` | 192 / 192 | 19 | `c1ab3b0043e411c7e6695fb129837501c3bca12a2477d9497b5123f3eaf17805` | rejected; candidate padded 208 |

The first candidate object is 5,534 bytes with raw SHA-256
`b7314205c30a81b6eef98b100518c2857c89b4f62824fd1aa92160f0e6dc7dcb`.
The frozen gate reported **13 exact, 2 residual, 12 unwritten**.

`_input_get_key` had the target's 112-byte boundary, nine relocation identities/addends, and
exact assertion literals, but its normalized SHA-256 was
`9a2e9d2e233debf323faba5cd18e649300ed944540e8a55f259f186439a6d8c9`.
Its first two global relocations appeared at `+0x05/+0x0C` rather than January
`+0x06/+0x0F`; the remaining schedule stayed correspondingly earlier. `_input_key_is_down`
kept 19 relocations and the same semantic global owners/addends but compiled to 208 padded
bytes with normalized SHA-256
`4acb7c6f4e39a8a6fed34a3c496b0d42b991b4575da0a4f48429bc2051bc0e08`.
Both bodies were removed immediately. No spelling, declaration, branch, local, expression,
cast, macro, or statement-order retry was attempted. The support-only generic-key constant was
also removed. Reopen either body only with new compiler-authentic source evidence.

## Retained exact relocation proof

All addends and relocation identities below equal January in the retained-only object:

| Owner | Offset | Type | Symbolic destination |
|---|---:|---|---|
| `_input_flush` | `+0x08` | DIR32 | `_bss_004536a0+0x14C` |
|  | `+0x0D` | REL32 | `_csmemset` |
|  | `+0x16` | DIR32 | `_bss_004536a0+0x238` |
|  | `+0x1B` | REL32 | `_csmemset` |
|  | `+0x24` | DIR32 | `_bss_004536a0+0x2A0` |
|  | `+0x29` | REL32 | `_csmemset` |
|  | `+0x35` | DIR32 | `_bss_004536a0+0x30C` |
|  | `+0x3C` | DIR32 | `_bss_004536a0+0x308` |
|  | `+0x45` | DIR32 | `_bss_004536a0+0x30A` |
|  | `+0x4C` | REL32 | `_csmemset` |
| `_input_has_gamepad` | `+0x1B` | DIR32 | exact source-path literal |
|  | `+0x20` | DIR32 | exact gamepad-index assertion literal |
|  | `+0x25` | REL32 | `_display_assert` |
|  | `+0x2C` | REL32 | `_system_exit` |
|  | `+0x39` | DIR32 | `_bss_004536a0+0x13C` |

The retained source Git blob is
`23f68d2bcba241c1a664792ef7bdf67eb4a56e5f`; the integration-base source blob was
`3ea85b75ee123849763c191767b3850a566d7a4e`. Its normalized CRLF working-tree
SHA-256 is
`eb81a2347eebc4b787ed7e5be2a839c6070d3a3cf2a78baa7330a102d5ead71f`.
The retained replay object is 4,162 bytes with raw SHA-256
`886fd0b0f038cc59fe552980508de633b8b4967b57ea5d9029fccd767154a834`.

## Complete final owner map

The retained-only result is **13/27 exact**, zero residuals, and 14 unwritten. Exact padded
coverage advances from 368 to **544/3,808 bytes**. On the established instruction-byte
measure, coverage advances from 247 to **404/3,557 bytes**. The gain is two functions,
176 padded bytes, 157 meaningful bytes, and 15 exact relocations.

| Exact owner | Padded / relocs | Normalized SHA-256 |
|---|---:|---|
| `_code_000bebb0` | 16 / 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_code_000bebc0` | 16 / 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_fix_dead_zone` | 80 / 0 | `9cb7f2e9518611e8eaf13b6f0ebd21cd4da1ddb71bc5485300354f8665ed758b` |
| `_input_activate` | 16 / 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_input_deactivate` | 16 / 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_input_flush` | 96 / 10 | `9c35ae4b34076c4179dea45a47f1af9654e117c324b2da827460901fdfdccf67` |
| `_input_frame_end` | 16 / 1 | `6d756ab123ca36d54d960697fa16bc6dc86933084aa5b92457f2b58c5174722d` |
| `_input_get_gamepad_state` | 112 / 8 | `eac1914f21723d418481596ddcec3df48ff3614a52836c12c85c86799705ab10` |
| `_input_get_mouse_state` | 16 / 0 | `33cb7c6b5695e070548b4e7f33d43173b10670c306067b684edf21470a662ef2` |
| `_input_has_gamepad` | 80 / 5 | `2a92d28a0cff1e44f794e91dd66959c8ce6db2ad173559bcc5a2d669f82470e9` |
| `_input_mouse_button_is_down` | 16 / 0 | `1c93847ee0f29c8102c08f41566abe2ae27a71ab314eec709e90e3a28353e543` |
| `_input_suppress` | 16 / 1 | `4eb29bc953165a2d51bef0bb3528cd233ddf8a287aaf2beb597b6c4e135976ce` |
| `_update_ticks` | 48 / 0 | `a3153c5a06cb67390d81d7112942a0a0a8f2e485c554823319d45848502b504a` |

| Unwritten owner | Padded / relocs | Normalized SHA-256 | Bounded-lane policy |
|---|---:|---|---|
| `_code_000be800` | 48 / 0 | `5850a17e99380c5499a719ba1667ef91f992a37284afe5049ba6e7f04e79c826` | Prior fixed private-helper rejection; no retry. |
| `_code_000bebd0` | 1232 / 28 | `56316233dc2ab683b304fa910b38ac5a82e29e8a13850a48cf31af300c81d580` | Prior fixed broad-poller rejection; no retry. |
| `_code_000bf0a0` | 176 / 8 | `f473cb2d504d90b7ac4014fa2d05f189e9ef27ace087fd7fb23d69a6c9c19949` | Anonymous stateful platform body; outside public-only lane. |
| `_code_000bf150` | 640 / 49 | `80bcd2f76c52c728ac2196960ec81c6f2364ab006debe2291d6405a3c7372d89` | Private keyboard/device poller; outside size and ABI boundary. |
| `_code_000bf540` | 32 / 3 | `bb9c454c6ca60f04e12b08b0ebe14f2a43c22eb08e8488d72cf2e13a9ab563d0` | Anonymous thread/bootstrap helper; no authenticated public owner. |
| `_input_dispose` | 80 / 6 | `337fdbf044e9061222d68c614d21aaa4d4a73730acf0fe818aec3c60fd5a317e` | Device/SDK cleanup loop; not in the frozen donor-backed set. |
| `_input_frame_begin` | 16 / 2 | `7f8a484a3e821ca5cd2e5ae3f709d8a1adcad776dc63b32059e7b44d3a6b0c3b` | Calls prior rejected broad private poller; not link-closed. |
| `_input_get_key` | 112 / 9 | `a3b48c1ac1818bbd2c937a5f56fa988454af93d4a023201c03682be5fd73f436` | This wave's first-shot residual; pruned, no tuning retry. |
| `_input_get_raw_data_string` | 240 / 14 | `3a95b06caa2d47b995d9c24ba0a7450d1ccbc091e6a4dc288cc6a986d8f76c5a` | Prior fixed public-body rejection; no retry. |
| `_input_initialize` | 256 / 22 | `cf5e2e92a28cbbf77bea5849fe3771d78c9a3aaebe152b3abac243e465870a4c` | Platform handle/thread initialization with private dependencies. |
| `_input_key_is_down` | 192 / 19 | `c1ab3b0043e411c7e6695fb129837501c3bca12a2477d9497b5123f3eaf17805` | This wave's first-shot size/SHA residual; pruned, no tuning retry. |
| `_input_set_gamepad_rumbler_state` | 96 / 7 | `a45df86e5e59c9a5ad6fede77c449adff25cc1ac4f04a0bb1aca04cb606ae514` | Stateful wrapper outside the frozen donor-backed set. |
| `_input_update` | 96 / 14 | `de67c361cd5d6ad0b7a5f1c2c7e6d9892c030b7269fa9dd6f5ebab4e324f4d85` | Pre-excluded: direct private `_code_000bf150` dependency. |
| `_input_vertical_blank_interrupt` | 48 / 5 | `b6ac182841427062516e4b85d708b17ec7b0018faa82af148acfc03fa1a7118a` | Event/synchronization SDK boundary; outside frozen set. |

## Ownership, policy, and validation

The candidate continues to emit the inherited `_bss_004536a0` as a 1,036-byte tentative
COMMON symbol and creates no writable `.data` or candidate-owned `.bss` section. January owns
the concrete `.bss` section. Strict target-owner comparison resolves every retained function's
reference to the same `_bss_004536a0` symbolic owner and exact addend; the incomplete object
claims no non-code or object-completion credit.

Final evidence:

- retained-only `tools/campaign/gate.py source/input/input_xbox --all`:
  **13 exact, 0 residual, 14 unwritten**;
- hardened direct COFF comparison: both new owners equal in padded bytes, normalized SHA-256,
  relocation count, order, offset, type, symbolic destination, and addend;
- pre/post exact-set comparison: `lost_exact: []`; newly exact only `_input_flush` and
  `_input_has_gamepad`;
- rejected-symbol check: `_input_get_key` and `_input_key_is_down` are absent from the retained
  object;
- all 13 emitted accepted function COMDATs remain independently strict; 25 exact relocations;
- `git diff --check`: pass.

The retained source is ordinary C89-compatible typed C with explicit `void`, one parameter per
line, typed arrays/structure fields, and terminal returns. It uses no assembly, `volatile`,
`register`, pragma, intrinsic, optimizer barrier, undefined behavior, raw address/offset
dereference, pointer/integer reconstruction, representation pun, inactive union, byte forcing,
private/register ABI forcing, metadata/COFF alias, compiler switch, config change, comparator
exception, or completion-label change. Only this source and this additive ledger are changed.
Nothing is committed or pushed.
