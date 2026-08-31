# `observer.obj` Claude/Fable semantic eight-function cluster (2026-08-31)

## Result and retained scope

An atomic, naturally typed Observer reconstruction closes eight January
function packets and preserves all three inherited exact functions.  The
translation unit advances from 3/26 to 11/26 strict exact functions and from
144/11,280 to 2,032/11,280 strict padded code bytes: a gain of eight functions
and 1,888 padded bytes.  The retained cluster is:

- private `observer_get` and `observer_clear` helpers;
- `observer_initialize_for_new_map`;
- `observer_get_camera`;
- `observer_command_has_finished`;
- `observer_obsolete_position`;
- `observer_up_from_forward`;
- `observer_set_camera`.

This is the owner-topology reopening anticipated by the prior
`observer_command_has_finished` and `observer_obsolete_position` boundary
ledgers.  The two private helper aliases, typed private global, and complete
private Observer layout are retained atomically with their callers.  No
unrelated collision helper or other Claude campaign function is included.

## January authority and compiler

The PC authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Its extracted `source/camera/observer.obj` has raw SHA-256
`7352ecf0205252e3f145938a70cab3e161ef8208d7b3f99a097925688d805b95`.
The repository compiler remains Microsoft VC7 13.00.9254.1 at
`xbox/bin/vc7/CL.Exe`, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
with the normal `/O2 /Oy- /DDEBUG /Dxbox` edge.

## Independent provenance and semantic-name audit

The executable bodies came from the read-only Claude campaign branch
`claude/breakable-surfaces-20260830`, commit
`e02971dfdba111d5481c5b7d7b6fc38a9e90d155`.  Its Observer source is Git
blob `46b386ba24556d7af298c6dfd1c5d3e1a73dc031`.  The donor was not copied
wholesale: its local duplicate command type, raw private names, and unrelated
collision helpers were excluded.

The semantic target and source topology were independently supplied by the
read-only Fable branch at commit `21f4fc550`; its Observer source and header
are Git blobs `16e47da343213f6f8112111ac905f3ef058fb41f` and
`0085325058e03b5a0e27c30283598734421bf0cf`.  Fable's rename ledger records
`observer_clear` as recovered from the January linker map and corroborated by
HCEA.  `observer_get` is the conventional private name selected for the
exact, inlined HCEA lookup idiom because no surviving name oracle was found.

The authenticated HCEA tree is commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`.  The relevant primary-source
blobs are:

- `src/blam/camera/observer_set_camera.c`:
  `c2e805f2adf06b9139cc316a717dba428c35ad98`;
- `src/headers/observer.h`:
  `7abb89816a6ffadfe68eddc65eab6f26fa0fbb95`;
- `src/headers/observer_command.h`:
  `a8c97d3b1aeeaf6aa0d2f99770943ae717c24a9a`;
- `src/headers/observer_command_flags.h`:
  `40c5ac42147cffbd7843c28e69af935f7765140a`;
- `src/headers/observer_globals.h`:
  `ecb4fe89c9014c37131456b34b486abba5593cf1`.

HCEA authenticates `observer_globals`, `dtime`, `local_players`,
`pending_command`, `last_command`, `updated_for_frame`, `first_command`, the
`focus_*` command members, the six command-flag names and values, the
11-element derivative record, and the private Observer member order.  The
source therefore contains no code-address or BSS-address placeholder names.
Public declarations and command enums live in the owning
`source/camera/observer.h`; private state and layouts remain in
`source/camera/observer.c`.

## Frozen private layout

The reconstruction uses the canonical `real`, `real_point3d`, and
`real_vector3d` types.  Compile-time proofs fix `last_command` at `+0x08`,
`result` at `+0x74`, `velocities` at `+0xE8`, `accelerations` at `+0x120`,
`displacements` at `+0x260`, one Observer at `0x29C`, and the four-player
global owner at `0xA74`.  `observer_globals` is a TU-private typed BSS owner,
not an external raw-address alias.  Command flags are expressed through the
named enum with `TEST_FLAG` and `SET_FLAG` rather than open-coded bitwise
operations.

This topology is material to exactness.  The exact helper relocation
identities convert the formerly isolated `observer_initialize_for_new_map`
and `observer_obsolete_position` packets into exact callers without source
tricks or private register-ABI declarations.

## Strict function packets

The semantic-alias comparator checks padded bytes plus each relocation's
offset, type, identity, addend, and order.  All eight retained packets match:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_observer_get` | 64 | 5 | `e58212a05c988479a13bbae14e1fb9f8d7f68e98fdcb9901b74776288a7141ca` |
| `_observer_clear` | 304 | 7 | `4059d49379400f22f56332f7c3b027b76e10198f50c3cbbe40f86bd91ae9a958` |
| `_observer_initialize_for_new_map` | 80 | 6 | `0600e84f80dec36718469b77aed1167213fc2e63587a8af567de85a124118c8d` |
| `_observer_get_camera` | 144 | 10 | `720068a2ab63a4056ea484eb2184cbab76f29d905039ab2abfaf231860242806` |
| `_observer_command_has_finished` | 128 | 7 | `c8c7f1a9bdcdac36741a757fb4ba7ff13784b3e62a5504711f1396d0d8b95d37` |
| `_observer_obsolete_position` | 80 | 6 | `5a21f2d03d67c9c3b06b1545e5af0422938d458e6807324e2a3021b2d3fa5e2c` |
| `_observer_up_from_forward` | 144 | 2 | `8ebe409a2d4adc84128be82fe7118900e03f218b2fa8f78548092681e86fa136` |
| `_observer_set_camera` | 944 | 32 | `40326c4d7b766d07554a821ec05ae32441d7acbd119276a1271fd308f3c95c92` |

The whole-TU semantic comparison reports 11 exact, zero residual, and 15
unwritten functions.  The inherited `_observer_initialize`,
`_observer_dispose_from_old_map`, and
`_observer_reconnect_to_structure_bsp` packets remain exact.

The ordinary raw-name gate reports three exact and six relocation-identity
residuals until the edited symbol aliases are regenerated.  That result is
expected: this lane was explicitly forbidden from running configure.  A
fresh canonical compile compared with the Fable semantic-name target proves
all eight packets above, including the exact `observer_globals`,
`observer_get`, and `observer_clear` identities.

## Owner-header blast radius

All 14 direct `observer.h` includers were enumerated and compiled to
PID-named scratch objects.  Exact-function sets before and after the owner
header correction were:

| Translation unit | Before | After | Lost exact functions |
| --- | ---: | ---: | ---: |
| `source/game/players.c` | 52 | 52 | 0 |
| `source/camera/bored_camera.c` | 9 | 9 | 0 |
| `source/game/cheats.c` | 14 | 14 | 0 |
| `source/camera/first_person_camera.c` | 5 | 5 | 0 |
| `source/camera/director.c` | 23 | 23 | 0 |
| `source/camera/dead_camera.c` | 2 | 2 | 0 |
| `source/effects/material_effects.c` | 3 | 3 | 0 |
| `source/camera/following_camera.c` | 8 | 8 | 0 |
| `source/camera/flying_camera.c` | 3 | 3 | 0 |
| `source/camera/orbiting_camera.c` | 2 | 2 | 0 |
| `source/camera/observer.c` | 3 | 11 | 0 |
| `source/saved games/game_state.c` | 24 | 24 | 0 |
| `source/scenario/scenario.c` | 46 | 46 | 0 |
| `source/ai/ai_debug.c` | 56 | 56 | 0 |

Six camera consumers now include the owner header instead of locally
redeclaring `observer_up_from_forward`; `scenario.c` likewise uses the
owner declaration for `observer_reconnect_to_structure_bsp`.  This satisfies
prototype ownership without changing any consumer function packet.

The section-fingerprint blast test must be disclosed explicitly: every one
of the 13 non-Observer consumers changes anonymous `.debug$S`, and no other
section changes.  No code packet or runtime-data section changes, appears, or
vanishes.  `players.c`, `director.c`, and `ai_debug.c` initially also reported
a symbol-inventory delta; detailed comparison proves this is only VC7
compiler-local `$Lnnnnn` label renumbering.  Each label retains the same
function owner, offset, type, and storage class, with its numeric suffix
increased by exactly 25 because the owner header gained 25 preprocessed source
lines.  No
semantic, external, or static program symbol appears or vanishes.

The `.debug$S` consequence is an intentional owner-header boundary and must
be adjudicated by the orchestrator's consolidated regression/snapshot gate;
it is not hidden or treated as runtime equivalence evidence.

## Source-integrity policy and deferred gates

The retained production source contains no inline assembly, `volatile`,
`register`, pragma, intrinsic, attribute, optimizer barrier, raw address or
offset access, pointer/integer reconstruction, representation pun, noinline
directive, undefined behavior, or byte-forcing construct.  It uses natural
C89 control flow, owner types, cseries scalar types/macros, semantic members,
and the public owner header.  Hard residual Observer functions remain
unwritten; no fuzzy residual was tuned in this lane.

The retained `source/camera/observer.c` is Git blob
`7ace5422b8cbc48b0862ac73cb8b3c168043863d`, working-tree SHA-256
`896966189e5a4874dd9c2a6d2f90aca7f5f00a53386369bc80095c237ca4ff69`.
The retained owner header is Git blob
`34876eb2dc424d35545ceacf147d27203e8a0276`, working-tree SHA-256
`18bdfdf2de08baa9b2614c220c05f570643e6dc7fd35448984fd8ee2138602db`.

Focused compilation, the semantic packet comparator, and the 14-includer
blast test passed.  Per orchestrator instruction, this lane did not run
`configure.py`, Ninja, pytest, commit, or push.  Those steps remain for the
single consolidated validator after all active agent edits are quiescent.
