# `input_xbox.obj` SDK leaf wave (2026-09-04)

## Scope and authority

- Canonical base: `96086c9c4c187dd539af13bdaa9252d6e46ad8f6`.
- Sole code owner: `source/input/input_xbox.c` / `source/input/input_xbox.obj`.
- January target: `build/split/source/input/input_xbox.obj`.
- This bounded wave owned only the three previously unwritten public SDK leaves
  `_input_dispose`, `_input_set_gamepad_rumbler_state`, and
  `_input_vertical_blank_interrupt`.

January COFF is the sole byte, ABI, relocation, and ownership authority. The
campaign methodology and throughput notes were read before editing, together
with `input_xbox_obj_jonas_public_leaf_pair_20260828.md`,
`input_xbox_obj_jonas_inline_helper_pair_20260827.md`, and
`input_xbox_obj_jonas_gamepad_state_leaf_20260821.md`. Their fixed rejections
for the broad pollers, keyboard bodies, hysteresis/helper work, and prior
residuals were not reopened.

The frozen Stian/Pastudan retail reconstruction independently corroborates the
cleanup loop, requested-rumble storage, and alternating event signal. It is a
semantic donor only: its raw addresses, local SDK function-pointer typedefs,
private names, and reconstructed control text were not copied. The retained
source instead uses the repository's owning `HaloAutoTest.h` and `player_ui.h`
plus the established minimal XDK include sequence: the guarded platform
`_X86_` definition, `excpt.h`, `windef.h`, `winbase.h`, and `xbox.h`. This
supplies the real declarations for `XInputClose`, `SetEvent`, and `HANDLE`
without caller-local prototypes or unrelated graphics headers.

## Baseline

The required pre-edit PID-isolated gate object was
`scratch/input_xbox_sdk_leaves_baseline_20260904.obj` (4,167 bytes, raw
SHA-256 `e8b16f95205e17fe9d7e18ff007b27e36e2243335bbbd63a97d4a8c38737ddde`).
It reported **13 exact, 0 residual, 14 unwritten**. The three selected owners
were unwritten:

| Owner | Padded bytes / relocs | Target normalized SHA-256 |
|---|---:|---|
| `_input_dispose` | 80 / 6 | `337fdbf044e9061222d68c614d21aaa4d4a73730acf0fe818aec3c60fd5a317e` |
| `_input_set_gamepad_rumbler_state` | 96 / 7 | `a45df86e5e59c9a5ad6fede77c449adff25cc1ac4f04a0bb1aca04cb606ae514` |
| `_input_vertical_blank_interrupt` | 48 / 5 | `b6ac182841427062516e4b85d708b17ec7b0018faa82af148acfc03fa1a7118a` |

## Authenticated state layout

The existing global remains one naturally typed, 1,036-byte tentative COMMON
symbol. No second backing store, offset pointer, overlay, or aggregate facade
was introduced. January's relocation addends and the neighboring target
owners establish the newly exposed fields:

| `input_globals` field | Offset | Evidence |
|---|---:|---|
| `gamepad_rumbler_states[4]` (`struct vibrate_data`) | `+0x214` | the two word stores in the rumbler setter; the array starts immediately after the 40-byte suppressed gamepad state |
| `update_event_handle` (`HANDLE`) | `+0x228` | `SetEvent` in the vertical-blank interrupt, `CreateEventA` in January initialization, and `WaitForSingleObject` in the thread owner |
| `update_event_pending` (`boolean`) | `+0x22C` | all three loads/stores in the interrupt and January initialization's initial `TRUE` store |
| `keyboard_handle` (`HANDLE`) | `+0x234` | cleanup load/clear and January keyboard-device assertion strings |

The already exposed gamepad-handle array is now typed as `HANDLE[4]`. Explicit
`offsetof` checks cover each new boundary, and the existing
`sizeof(struct input_globals) == 0x40C` check still compiles. Bytes not required
by this wave remain honest reserved spans; they were not assigned speculative
members merely to complete the layout.

## Retained source and exact proof

The first ordinary implementation passed strictly. There was no source-shape,
declaration-order, register, or optimizer search.

- `input_dispose` calls `HATCleanup`, closes and clears the keyboard handle,
  then closes and clears all four non-null gamepad handles.
- `input_set_gamepad_rumbler_state` preserves the January line-`0x198` range
  assertion and records both requested motor frequencies only when the owning
  player UI does not disable rumble.
- `input_vertical_blank_interrupt` signals the event when the pending byte is
  set, then logically toggles that byte. The post-call reload visible in the
  target follows naturally from the ordinary C expression.

All ordered relocations below match January in offset, COFF type, symbolic
destination, and addend:

| Owner | Relocation offsets and destinations |
|---|---|
| `_input_dispose` | `+0x01` REL32 `_HATCleanup`; `+0x06` DIR32 global `+0x234`; `+0x10` REL32 `_XInputClose@4`; `+0x16` DIR32 global `+0x234`; `+0x21` DIR32 global `+0x13C`; `+0x38` REL32 `_XInputClose@4` |
| `_input_set_gamepad_rumbler_state` | `+0x1A/+0x1F` exact assertion literals; `+0x24` REL32 `_display_assert`; `+0x2B` REL32 `_system_exit`; `+0x34` REL32 `_player_ui_rumble_disabled`; `+0x50/+0x57` DIR32 global `+0x214/+0x216` |
| `_input_vertical_blank_interrupt` | `+0x01` DIR32 global `+0x22C`; `+0x0A` DIR32 global `+0x228`; `+0x10` REL32 `_SetEvent@4`; `+0x15/+0x20` DIR32 global `+0x22C` |

The final PID-isolated candidate is
`scratch/input_xbox_sdk_leaves_minimal_headers_20260904.obj` (5,275 bytes,
raw SHA-256
`19532f07b5fcb876f608cc580734c528e7b1b7fde79244aa92cf5215778af229`).
Its final gate reports **16 exact, 0 residual, 11 unwritten**. Every inherited
exact owner remains exact, so `lost_exact: []`. The exact padded-code gain is
224 bytes, taking this unit from 544 to **768 / 3,808 exact padded bytes**.
Meaningful instruction coverage advances by 206 bytes, from 404 to
**610 / 3,557 bytes**. The narrowed SDK includes emit no candidate-owned XDK
graphics constants; the only non-debug, non-directive candidate data are the
two exact assertion literals. This is a function-level gain only; the object
is not claimed complete and no non-code credit is claimed.

The retained source Git blob is
`2c493beb7e1b5b7864de0900764df45036a7752d`; its working-tree SHA-256 is
`f6cff80fedd1114c84b42d0c5915bfcd10165ed66de79febc99d155103ec2390`.
The emitted-symbol guard confirms no `_point_from_line3d` owner. The source is
ordinary typed C with semantic names, one parameter per line, explicit `void`
and terminal returns, real SDK declarations, and no inline assembly, forced
inlining, raw address access, representation punning, fake lifetime, fake
padding, volatile/register steering, pragma, metadata alias, or comparator
exception.

No residual was retained and no park is proposed. No header, configuration,
build-tree, commit, or remote operation was performed. Full repository build,
stable exact-set sweep, and publication remain the canonical integrator's
responsibility.
