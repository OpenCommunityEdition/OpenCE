# `game_sound.obj` Jonas scripting leaf wave (2026-08-21)

## Boundary and immutable evidence

This wave started from canonical commit
`c8ab755c312be6e9c4eb3cd0d9e6c6aba643c45e` on the isolated branch
`jonas/game-sound-scripting-wave-20260821`. The immutable January PC target is
`build/split/source/sound/game_sound.obj`, raw SHA-256
`d69fd3772b48a1d483d0c80af8703210b85bc65d2dbdda1402c0e542ad54168`.
The January executable SHA-256 is
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Strict admission uses each function COMDAT's meaningful bytes, padded extent,
normalized bytes, and complete relocation schedule rather than the
timestamp-bearing candidate-object container digest.

The compiler is Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`
for 80x86. Its XDK 3911 `CL.Exe` SHA-256 is
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
No compiler flag, include order, configuration file, or build rule changed.

The clean HCEA semantic-oracle tree is commit
`c168af2e747d3095d9a29418ae401f3a39544863`. It supplied these independently
tree-reachable source-topology blobs:

| Candidate | HCEA tree blob |
| --- | --- |
| `scripted_sound_time` | `9ac6374ad268cabd696c9fa0a1620cd272c204d0` |
| `scripted_sound_stop` | `2144465330996f702bdbe485d0507d34ed6196e0` |
| `scripted_looping_sound_set_scale` | `81f0a97c5652e924d697ef0579e7bf0bf7f73c07` |
| `scripted_looping_sound_set_alternate` | `1a97636453ff46f66c5bbd7a685296f4bd683263` |
| `unattached_looping_sound_stop` | `6d794260298e6c4a34c6cc1f3e69af43405fa97b` |

HCEA establishes purpose and broad topology only. January PC COFF,
disassembly, symbols, callers, and the repository's typed declarations remain
authoritative for PC layout and ABI. In particular, HCEA casts its mistyped
looping-sound global; the canonical repository already declares
`game_looping_sound_data` as `struct data_array *`, so this wave carries no
cast or raw-address reconstruction.

The final staged source identities are:

| Path | Git blob | Stable payload SHA-256 |
| --- | --- | --- |
| `source/sound/game_sound.c` | `add1a0275589ce5599d6c4674c8f94f86e69c963` | `2c908fa59c9a94d9cb6d6b976498b91c782aa33e731daaf3cc2b8378e2bd5d3c` |
| `source/sound/game_sound.h` | `a2cdb61719f62f6b6cde922482ac712c78f8d976` | `c492fa66753d941523ad838a877752f22d3f28324e4ba0b8716a1d73ec0b0a3c` |
| `source/sound/sound_definitions.h` | `93e20ca2438f827b2c489d71d83e702e15843369` | `7ddbe46f1d71ca460df7b16e9b3f150c216b1188d6789a868e692104278a42fd` |

## One-shot candidate boundary

All five natural typed bodies were present in one code-producing compile.
Each was adjudicated independently. Four strict hits were retained and the
one miss was removed immediately. The rejected body was not respelled,
reordered, tuned, or compiled again.

| Candidate | Target RVA | Meaningful / padded | Relocations | Target normalized SHA-256 | One-shot result |
| --- | ---: | ---: | ---: | --- | --- |
| `scripted_sound_time` | `0x001B6FF0` | 75 / 80 | 2 | `b311cf680fca7012212c635bb5023141e459da44b0adafb0e0ef9c6025f26e25` | rejected; candidate hash `ba9cdbb0ab9bb1dd26e13526682eb7a5069a1b21646c2800e666f7b55ce106b4`, target relocation offsets `+0x16/+0x2B`, candidate `+0x17/+0x31` |
| `scripted_sound_stop` | `0x001B7040` | 71 / 80 | 2 | `52780e1efb4c3c4ceb3af7dc28ff32e089e57be67e55cb9e69d5b0a48927896f` | retained exact |
| `scripted_looping_sound_set_scale` | `0x001B7140` | 112 / 112 | 7 | `fa6856d6b0ce053c1e677fa1aa7f0979bbd3f5fb4511f3364c3bab5ae91228c1` | retained exact |
| `scripted_looping_sound_set_alternate` | `0x001B71B0` | 74 / 80 | 3 | `fcd390aac586d07238dfa2c8685ebf4db4f1eddf966aa5432c2a2c6689607fc5` | retained exact |
| `unattached_looping_sound_stop` | `0x001B7260` | 33 / 48 | 2 | `72f38fecd99b2db3361aec94f7258d264cbc3ed79151cf9955663269a59a93c0` | retained exact |

The wave therefore adds four functions and 290 meaningful code bytes. The
rejected function is absent from final source and the rebuilt object and
receives zero credit.

## Retained typed behavior and relocations

All four retained functions use ordinary public cdecl ABI and explicit
terminal returns. `scripted_sound_stop` resolves the sound definition, stops
its live scripted impulse, and clears both the scripting-sound and
scripting-time indices. `scripted_looping_sound_set_scale` resolves the
looping definition and datum and clamps the datum's typed `real scale` field
to `[0.0f, 1.0f]`. `scripted_looping_sound_set_alternate` updates bit 3 of the
typed looping-datum flags, and `unattached_looping_sound_stop` marks bit 1.

The shared looping-sound definition now records its authenticated PC
`scripting_sound_index` at `+0x1C`, guarded by a compile-time `offsetof`
assertion. `<StdDef.h>` supplies `offsetof` through `cseries.h`, and every
direct consumer includes that owner before `sound_definitions.h`.

Every relocation destination addend is zero:

| Function | Offset | Type | Destination |
| --- | ---: | --- | --- |
| `scripted_sound_stop` | `+0x13` | REL32 | `_tag_get` through `sound_definition_get` |
|  | `+0x29` | REL32 | `_sound_stop_impulse` |
| `scripted_looping_sound_set_scale` | `+0x12` | REL32 | `_tag_get` through `looping_sound_definition_get` |
|  | `+0x23` | DIR32 | `_game_looping_sound_data` |
|  | `+0x29` | REL32 | `_datum_get` |
|  | `+0x32` | DIR32 | natural `__real@00000000` literal |
|  | `+0x44` | DIR32 | natural `__real@00000000` literal |
|  | `+0x52` | DIR32 | natural `__real@3f800000` literal |
|  | `+0x5F` | DIR32 | natural `__real@3f800000` literal |
| `scripted_looping_sound_set_alternate` | `+0x12` | REL32 | `_tag_get` through `looping_sound_definition_get` |
|  | `+0x23` | DIR32 | `_game_looping_sound_data` |
|  | `+0x29` | REL32 | `_datum_get` |
| `unattached_looping_sound_stop` | `+0x08` | DIR32 | `_game_looping_sound_data` |
|  | `+0x0F` | REL32 | `_datum_get` |

Cross-translation-unit declarations were audited before admission. The HS
bindings independently declare `scripted_sound_stop(long)`,
`scripted_looping_sound_set_scale(long, real)`, and
`scripted_looping_sound_set_alternate(long, boolean)` with the same return
types. `hud_sounds.c` calls `unattached_looping_sound_stop(long)`, whose
existing public declaration is preserved. The three missing public
prototypes were added to `game_sound.h`. Exact-name source and history
searches found no duplicate definitions or incompatible declarations.

## Shared-header consumer safety

Because both modified declarations are shared, the complete non-`game_sound`
consumer union of `sound_definitions.h` and `game_sound.h` was compared
between exact clean canonical `c8ab755c` objects and the retained candidate.
The eight direct `sound_definitions.h` consumers are:

- `source/cache/xbox_sound_cache.c`
- `source/cache/predicted_resources.c`
- `source/items/weapons.c`
- `source/game/game_engine_multiplayer_sounds.c`
- `source/interface/hud_sounds.c`
- `source/objects/objects.c`
- `source/scenario/scenario.c`
- `source/sound/sound_definitions.c`

The five additional `game_sound.h` consumers are:

- `source/saved games/game_state.c`
- `source/physics/breakable_surfaces.c`
- `source/items/equipment.c`
- `source/effects/material_effects.c`
- `source/units/units.c`

`game_sound.c`, which includes both headers, was the fourteenth compiled
object. Both roots were up to date under their normal Ninja object edges. The
candidate object root was
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\game-sound-scripting-wave-20260821\build\base`;
the canonical baseline root was
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\campaign-integration-20260820\build\base`,
whose source tree is exact commit `c8ab755c` and whose XDK/compiler/config
identities are those recorded above. A prior clean detached `c8ab755c` build
gave the same runtime result; no deleted temporary pathname is an evidence
dependency.

Each object was built through its normal Ninja edge under the same generated
configuration and XDK. A Python COFF comparison paired objects by
translation unit, keyed externally owned code COMDATs by function symbol,
aligned compiler-local code COMDATs by stable section/owner topology, and
compared raw runtime payload plus normalized relocation tuples
`(offset, type, symbolic destination, addend)`. It separately compared
runtime non-code sections and external defined/undefined/COMMON ownership.
Across the thirteen non-`game_sound` objects, all 458 common runtime code
COMDATs are directly byte-and-relocation identical. The complete count adds
six `material_effects.obj` and 129 `units.obj` COMDATs to the original
323-COMDAT set. Runtime non-code sections and external-symbol ownership are
also identical. This includes every one of
`scenario.obj`'s 51 functions and the already accepted
`_scenario_location_deafening` body (203 meaningful / 208 padded bytes), the
sole external consumer that reads `looping_sound_definition.flags`. Its load
and normalized relocations are unchanged, so the wider authenticated
declaration is demonstrably runtime-neutral.

The only object drift outside those runtime identities is compiler-local and
debug path/ordinal metadata. It has no runtime bytes or relocations and
receives no credit. `game_sound.obj` itself gains only the four admitted code
COMDATs, their expected undefined externs, and two natural four-byte
float-literal COMDATs.

## Storage, rejection, and scope boundary

The rebuilt object defines no `.bss` or writable `.data`. The pre-existing
tentative COMMON symbols `_game_sound_globals` and
`_game_looping_sound_data` remain section-zero, four-byte declarations; this
wave neither creates nor claims their external storage. The compiler-owned
`0.0f` and `1.0f` literal COMDATs are required by the exact scale body but
receive zero matched-data credit. Campaign matched data remains exactly
1,835,088 bytes.

No private ABI was imported. Specifically,
`scripted_looping_sound_stop` depends on the private
`scripted_looping_sound_stop_internal`; `object_impulse_sound_new` crosses a
callback/private-cast boundary; `scripted_sound_new` carries union/decompiler
risk; and the HCEA `game_sound_set_mouth_aperture` donor omits a January PC
store. Those neighbors remain unimplemented. There is no shared-inline
substitution, callback pun, union pun, inactive-union access, raw address or
byte-offset dereference, pointer/integer reconstruction, undefined signed
overflow, volatile scheduling device, force-inline annotation, optimizer
pragma/barrier, assembly, synthetic anchor, or byte-forcing expression.

Tracked scope consists only of `game_sound.c`, its two required public/layout
headers, and this new Jonas-owned ledger. No configuration, semantic
exception, parked record, completion label, protected-five source,
pre-existing Markdown, runtime-data owner, or deleted path is changed.

## Pre-commit gates

- Complete `halobetacache_build` and `libcmt_build`: pass, 568 actions.
- Direct hardened comparison: all four retained functions reproduce exact
  padded extents, normalized hashes, and all 14 relocation
  identities/addends; the rejected `scripted_sound_time` symbol is absent.
- The four inherited game-sound lifecycle functions remain directly exact.
- Semantic audit: 470 units, 4,116 functions evaluated, 3,976 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,037 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,024/11,060 exact functions,
  479,838/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo progress: 273/468 complete objects, 3,857/7,574 exact functions,
  466,924/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes.
- `game_sound.obj`: 8/31 exact functions and 432 matched meaningful code
  bytes, comprising the four inherited lifecycle bodies and four new leaves.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, source/config/header/protected/deleted-path scope audit,
  storage audit, signature/collision audit, and shared-consumer comparison:
  pass.

Nothing is pushed or history-rewritten.

## Committed-state replay

Implementation commit `a6f4c45d4bda952844a76b607c1f9d7f306003db`
was clean before replay. Reading its three source/header blobs directly from
Git reproduced every blob identity and stable payload SHA-256 recorded above.

A one-unit regression snapshot was written from that commit and checked
immediately. All eight accepted `game_sound.obj` functions were
`still_exact`, with zero changed-nonexact or newly-exact entries, failures,
or warnings. After verifying its resolved absolute path remained inside this
worktree, only `build/base/source/sound/game_sound.obj` was deleted. Its
normal single-object Ninja edge rebuilt it successfully, and the committed
regression check returned the same clean result.

Direct hardened COFF comparison after the forced rebuild again reports all
eight accepted functions exact. In particular, the four new normalized
hashes and all 14 new relocation identities/addends reproduce exactly. The
rejected `_scripted_sound_time` symbol remains absent. The rebuilt object has
no `.bss` or writable `.data`; the two inherited four-byte COMMON declarations
remain externally owned, and the only new non-code definitions are the two
compiler-owned float-literal COMDATs. Tracked status is clean after this
ledger-only replay record. No worktree or junction was removed, and nothing
was pushed or history-rewritten.
