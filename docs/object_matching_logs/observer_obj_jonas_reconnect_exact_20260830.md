# `observer.obj` structure-BSP reconnect exact leaf (2026-08-30)

## Result and scope

The first and only natural production candidate for
`source/camera/observer.obj::_observer_reconnect_to_structure_bsp` is strict
exact.  It contributes 99 meaningful / 112 padded code bytes, seven ordered
relocations, and one newly exact function.  The object advances from 2/26 to
3/26 exact functions and from 32/11,280 to 144/11,280 strict padded bytes.
It remains incomplete.

This lane changed only `source/camera/observer.c` and this additive ledger.
Vehicles, Units, Matrix Math, AI Debug, every standing frozen candidate, all
active agent reservations, configuration, tools, headers, target artifacts,
and compiler flags remained outside scope.  The target-owned 2,676-byte BSS
record remains externally referenced and receives no data credit.

## Authority and provenance

The January PC authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Its generated `source/camera/observer.obj` has raw SHA-256
`7352ECF0205252E3F145938A70CAB3E161EF8208D7B3F99A097925688D805B95`.
The unchanged repository compiler is Microsoft VC7 13.00.9254.1 at
`xbox/bin/vc7/CL.Exe`, SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`,
with the normal `/O2 /Oy- /DDEBUG /Dxbox` edge.

The typed HCEA donor is authenticated at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`.  Its
`src/blam/camera/observer_reconnect_to_structure_bsp.c` is Git blob
`76714c8c394e41f349c86225d7acbb4c61f72dc0`, payload SHA-256
`F2F3A47F872A67C0257AD91773BBB02852E4416066237852D883B76A7B2D6DF1`.
HCEA authenticates the public cdecl parameterless ABI, local-player test,
typed observer-result location/position pair, and `0x29C` observer stride.
Its two-player loop is from a different build and was not copied.

The independent read-only Pastudan tree is commit
`918af885935ec470a31256ecce9a977b12b01f80`.  Its observer translation unit
is Git blob `7b288fe7e6ef2d014ed1868da2c0d668f0114924`, payload SHA-256
`FF5C9C7C541FD8C9664D7E8FA17D511B68E09111F8AE5610C06A69753CC810B7`.
That lift corroborates January's four-player loop, exact assertion and line,
`0x29C` stride, and `location, position` call order.  Its raw-address form was
read-only evidence and was not copied.

The acceleration playbook, campaign and audit tooling READMEs, object-ledger
doctrine, and applicable Stian/Pastudan `CLAUDE.md` documentation were read
before editing.  There was no Observer object ledger.  `git log --all -S
observer_reconnect_to_structure_bsp -- source/camera/observer.c` reaches only
the initial symbol inventory, and the worktree census found only untouched
inventory stubs plus the two external donors.  This was a fresh campaign
definition rather than replayed work.

## Frozen ABI and typed layout

January consumes no arguments and returns with ordinary `ret`, matching the
canonical `void observer_reconnect_to_structure_bsp(void)` declaration used by
the scenario reconnect table.  Its only calls are ordinary cdecl calls to
`local_player_get_player_index`, the standard assertion pair, and
`scenario_location_from_point`; there is no private or register-ABI callee.

The retained private layout names the four-element `local_players` array and
its existing typed `struct observer_result`.  Three compile-time proofs fix
the result at `+0x74`, one observer at `0x29C`, and the complete global record
at `0xA74`.  `struct observer_result` already names `position` at `+0x00` and
`location` at `+0x0C`.  Therefore the first location is naturally
`bss_0031d4b8 + 0x04 + 0x74 + 0x0C = +0x84`, exactly January's relocation
addend, without a raw address, offset dereference, pointer/integer
reconstruction, cast overlay, or type pun.

The C89 loop uses a signed `short local_player_index`, the canonical
`MAXIMUM_NUMBER_OF_LOCAL_PLAYERS`, and the January assertion at line `0x72`.
It skips disconnected local players and otherwise recomputes the observer
location from the corresponding position.  All parameters are explicitly
spelled, and the void body has an explicit terminal `return;`.

## Strict packet

January and the retained candidate both emit:

- 99 meaningful / 112 padded bytes;
- seven ordered relocations;
- normalized SHA-256
  `9c6b24f6b84a9d1c6d87dd8453b8623007a150a64b9f17c8d57f874a81c002d4`.

The relocation schedule is:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `+0x05` | `DIR32` | `_bss_0031d4b8 + 0x84` |
| `+0x12` | `REL32` | `_local_player_get_player_index` |
| `+0x2E` | `DIR32` | exact observer source-path literal |
| `+0x33` | `DIR32` | exact local-player range literal |
| `+0x38` | `REL32` | `_display_assert` |
| `+0x3F` | `REL32` | `_system_exit` |
| `+0x4C` | `REL32` | `_scenario_location_from_point` |

The hardened comparator proves every padded byte and every relocation's
address, type, identity, addend, and order.  The candidate object has raw
SHA-256
`213F0CFE4D182292CAEFEDFD1CB4342563ECB152C67EDDF32454D5AB0EEE0C8A`.
The pre-lane source was Git blob
`0a329c704bf105aa4e9379385119a4a7554033d0`, working-tree SHA-256
`956E01F7DD3AD5EC799F7CC007E5270E501542A869755D9374F4CBB86887ED51`.
The retained source is Git blob
`3cb96bb6b9674e3f642a8eecb26d370c5d95e0d7`, working-tree SHA-256
`80257CAA7745DDEAEA72448E37720D1B2575D03708D9122B53046D59BD238CE4`.

Exactly one code-producing production compile followed a VC7 `/Zs`
syntax-only preflight.  No alternate body, declaration order, expression,
loop shape, lifetime, qualifier, cast, optimizer option, sweep, or tune was
compiled.  The retained body contains no assembly, `volatile`, `register`,
pragma, intrinsic, attribute, barrier, raw address/offset access,
pointer/integer reconstruction, pun, undefined behavior, byte forcing, or
compiler/tool exception.

## Regression gates

The full `ninja halobetacache_build libcmt_build progress semantic_progress`
graph passed.  Semantic progress reported 470 units, 4,920 evaluated
functions, 4,803 semantic-exact functions, 4,813 accepted-exact functions,
131 hidden-exact functions, and zero unit errors.  Overall strict progress is
384/833 objects, 4,774/11,060 functions, 606,974/2,198,102 meaningful code
bytes, and 1,856,050/4,176,062 data bytes.  Halo progress is 282/468 objects,
4,607/7,574 functions, and 594,060/1,770,166 meaningful code bytes.

Direct whole-TU comparison reproved the two inherited 16-byte zero-relocation
leaves, `_observer_initialize` and `_observer_dispose_from_old_map`, at
normalized SHA-256
`499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca`.
The new leaf is the sole strict gain.

Protected `_unit_preprocess_node_orientations` remains exact at 1,920 padded
bytes, 87 ordered relocations, and matching normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reported zero candidates, contradictions, and revocations.  Parked
state reported 12 active, zero stale, and zero invalid entries.  Pytest passed
212 tests, with only the environment's non-semantic cache-write warning.
`git diff --check` and the changed-line policy scan passed.

`build/report.json` is 1,599,118 bytes with SHA-256
`ED631DBD77FCD1E6519664E0B17198E681FB342EC438EA8A5AB488886D568AC8`.
`build/semantic_report.json` is 3,268,485 bytes with SHA-256
`D4C164F43CE6C8D277D9D2C2BAB5020B4A677EAB60E915FAC0C35009FC059F65`.
Nothing is pushed.
