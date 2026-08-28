# `game_allegiance.obj` Jonas public continuation

This 2026-08-28 Jonas/Codex ledger records one second frozen natural typed-C
batch for `source/game/game_allegiance.c`. The four remaining HCEA-backed
public bodies and one independently authenticated public predicate were all
nonexact on their first code-producing gate. Every attempted body and all
batch-only support were removed immediately without tuning. The object
therefore remains 8/15 strict with zero residual functions.

## Scope and inherited state

- Clean pushed integration base:
  `d77b090a375439e0dce06640feea4b27d25047d8`.
- Branch: `jonas/game-allegiance-public-leaves-20260828`.
- Worktree:
  `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/game-allegiance-public-leaves-20260828`.
- Inherited retained source blob:
  `9509ddb334cf5c321d79de6ab9e6720363233aec`.
- The first-wave ledger is preserved byte-for-byte at blob
  `d434780cca2ba51f7703998e2a88bcc4b26cbab0`.
- January split-object SHA-256:
  `005dc3acdefcc3bbb2c041087494c54531400a18babab990fb405a27172ed9cf`.
- XDK `CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The inherited flags are `/O2 /Oy- /DDEBUG /Dxbox` with the pinned project
  include set.
- The exact gate, comparator, and COFF disassembly tool blobs are
  `37880af12887a6b4eb455ef800d23dc13a80c500`,
  `54fa2025044da29225bef633f33c0a9ec0ea603f`, and
  `2a3e0bd1b1406d966600e94271a00e511fa12b87`.
- The documented `initialize_for_new_map` residual was not retried. The
  private `_code_00096300` body was not emitted or attempted. No other object,
  header, build file, protected Units/Vehicles path, or Claude-active source
  was changed.

## Donor and target mapping

The complete pinned HCEA tree at
`c168af2e747d3095d9a29418ae401f3a39544863` was already audited in the first
wave. The four files selected for this continuation were read again against
the January disassembly and relocations:

| Target owner | HCEA donor blob |
| --- | --- |
| `_game_allegiance_update` | `4a7ed1058f4ff34c4f406eb7da96b95c5d1907fb` |
| `_game_allegiance_create` | `3de97ad70758ff0cf57a8627699cf109e5dc0cd9` |
| `_game_allegiance_remove` | `814148ff755aa7cef5060d6342e726cc4415bf78` |
| `_game_allegiance_incident` | `2ee88a1c159b0d496a7f63e95a011ecbd487f52d` |

The typed HCEA record and globals layout remain pinned by blobs
`62981f25f2c82ab2d7ef305ea486ac60b45241fe` and
`7d085987fbdc4415795ee76c7ec943d0c3f2db1a`. January topology confirms the
0x12-byte record stride and the exact `+0x04`, `+0x06`, `+0x08` through
`+0x0C`, `+0x0E`, and `+0x10` field uses in these bodies.

The conditional fifth candidate, `_game_team_ally_status_changed`, was
admitted to the frozen batch only after one-to-one evidence was found. The
clean Pastudan repository at
`918af885935ec470a31256ecce9a977b12b01f80` contains the same named body in
source blob `19602ba8282bd0156c2aa7872dde5f749d1885b2`; its symmetric record walk and
`status_changed` read at `+0x0B` map directly to the January 96-byte target.
The donor's raw offsets were used only as semantic evidence; emitted source
used the typed record.

January relocation and call topology also exposes a hard boundary shared by
all four HCEA bodies. `_code_00096300` is a translation-unit-private state
transition helper with a compiler-created ABI: the allegiance pointer arrives
in EAX, the new broken state in BL, and only the permanent-state byte is on the
stack. `update`, `create`, `remove`, and `incident` all use that private ABI.
The private body was explicitly out of scope, and no source-level typed
external declaration can recreate that register allocation.

## Frozen first gate

The batch contained exactly these five natural typed bodies:

- `_game_team_ally_status_changed`;
- `_game_allegiance_update`;
- `_game_allegiance_create`;
- `_game_allegiance_remove`;
- `_game_allegiance_incident`.

One compile preflight diagnosed a purely mechanical macro collision: the
existing `game_allegiance_globals` macro expanded inside a new struct-tag
spelling. It returned a compiler error and produced no `_gate_20404.obj`.
A preprocessor-safe typedef spelling was substituted without changing any
body or using code-generation feedback. The next invocation was the sole
code-producing frozen gate.

Frozen source artifact: `scratch/_gate_33956.c`, 12,499 bytes, Git blob
`918d9911ca937ab96746d5814dfbeb6913698f74`, SHA-256
`4692fc3780978e0bff7cedd05ffb547cbbd6d0a58b66eb84034d18924cf5c0e7`.
Frozen candidate object: `scratch/_gate_33956.obj`, 5,096 bytes, SHA-256
`1acca179435d58ef5b0f8ac5134a93514074bc7ea76f1670cf3d93f84a969c3f`.

The first gate measured 8 exact, 5 residual, and 2 unwritten functions:

| Attempted target | Meaningful | Target padded / relocs | Candidate padded / relocs | Target normalized SHA-256 | Candidate normalized SHA-256 |
| --- | ---: | ---: | ---: | --- | --- |
| `_game_team_ally_status_changed` | 84 | 96 / 1 | 96 / 1 | `919c8ed9d39352e7bf8b65fd8b7afdc567e372393e9126dc7a79d826ae1339c8` | `83915d083c4df2746a4d93a78733a9de101eb4e1e39a5d2d488f7a213166c5cf` |
| `_game_allegiance_update` | 128 | 128 / 7 | 128 / 7 | `dc653d14a5af72b2469b9e6240fe3186eaa6d86a0e08350f59fbd23072cd71c3` | `ea9ddc36db0f95cdfee566745fbc90b3711009542ca061592d1b326703be515e` |
| `_game_allegiance_create` | 217 | 224 / 5 | 224 / 5 | `47961c6c1d4c59a901156d907b22388d0b0a39a06f367b8403cd9e97ada8ed1f` | `445d265206cc0ce1c40c1ecfa40c63d2ba0b35cd2f8cb55c449cd8142fe10462` |
| `_game_allegiance_remove` | 167 | 176 / 3 | 160 / 3 | `b1a6048fb25c85a30a734472e7bb8753251763d14653ef2585c6a6d1f24c3830` | `510d142d35fe399c20213a0b483c4cbcc41dcc0a629f593d8f60a25f8ef204d1` |
| `_game_allegiance_incident` | 197 | 208 / 2 | 208 / 2 | `20caedf6368622cbfa9cd7c61d5e231223a3a3ed7f256cdad75eb79fdc895786` | `af2b3d8045e3a94dfb566ee2488ea4ab4f5a8d2d8d92f24ab5f71747ffc2ae97` |

No alternate expression, declaration order, loop form, switch shape, local
lifetime, cast, or private-call representation was compiled. Direct
post-freeze inspection was used only to document the fixed point: each
natural external helper call emitted three cdecl stack arguments, while the
January target loads EAX and BL and pushes one argument. This guarantees
nonidentity in all four helper-calling bodies under the stated policy. The
status predicate had equal padded size and relocation count but a different
normalized hash; it was not tuned.

## Prune, replay, and ownership

All five residual bodies were removed immediately. The batch-only errors
include, incident enum, maximum-count constant, globals typedef, and private
prototype were also removed. The resulting source is Git-blob-identical to
the inherited first-wave source at
`9509ddb334cf5c321d79de6ab9e6720363233aec`; no rejected support remains.

The retained-only replay again measures 8 exact, 0 residual, and 7 unwritten
functions. Replay object: `scratch/_gate_2300.obj`, 2,691 bytes, SHA-256
`65d72ad357a8977766d95b1a77e58d36950b0477097f0e7b0c6cfa3a91015025`.
The PID-dependent scratch-source name accounts for the one-byte raw-size
difference from earlier replay artifacts; strict admitted section identities
are unchanged.

Both the frozen candidate and retained replay declare `_bss_0043e490` only as
an undefined external with section zero, value zero, type zero, and external
storage class. Neither object defines `.bss` or `.data`. The retained replay's
only defined external non-code symbol is the inherited 24-byte allocation-name
literal COMDAT, so this continuation claims no target non-code owner or data
byte.

The frozen and retained sources use typed record/global access only. There is
no assembly, volatile/register scheduling device, pragma, intrinsic, barrier,
undefined behavior, raw address/offset access, representation trick, byte
forcing, or private-ABI workaround.

Validation results:

- Frozen gate: 8 exact, 5 residual, 2 unwritten.
- Retained-only replay: 8 exact, 0 residual, 7 unwritten.
- Strict target-owner COFF inspection: pass.
- Forbidden-token and raw-address audit: pass.
- `git diff --check`: pass.
- Net worktree scope remains the first-wave source modification plus the
  unchanged first ledger and this additive continuation ledger.

No commit or push is performed.
