# `input_xbox.obj` pollers: independent review, 2026-09-04

## Scope and verdict

This is an independent read-only source, ABI, COFF-owner, and accounting review
of the device/keyboard poller packet documented in
`input_xbox_obj_device_keyboard_pollers_wave_20260904.md`. January split COFF
is the byte and ABI authority; XDK 3911 headers authenticate platform types and
calls, while preserved donor source is corroboration only.

**Approve this packet for source admission.** It legitimately adds two strict
functions, `_input_update` and `_input_frame_begin`, totaling **96 meaningful /
112 padded code bytes**, while preserving all 16 inherited exact functions.
The helper and two pollers remain three explicitly measured fuzzy parks, and
six January functions remain unwritten. This approval therefore does **not**
assert that `input_xbox.obj` is wholly `Matching`.

Final reviewed source:

- `source/input/input_xbox.c`
- SHA-256:
  `7e2c3727dfd71c1c2a5ed0a9495426c9b6a70996bf2342c67471bb92f33a7cef`

## Source and ABI findings

- The translation unit includes the genuine XDK declarations through
  `<excpt.h>`, `<windef.h>`, `<winbase.h>`, `<xbox.h>`, and `<xkbd.h>` under the
  required Xbox `_X86_`/debug-keyboard feature guards. It does not reproduce
  SDK prototypes locally.
- `XINPUT_FEEDBACK`, `XINPUT_STATE`, `XINPUT_DEBUG_KEYSTROKE`, `HANDLE`, and
  the SDK constants/calling conventions agree with the target call shapes.
- January performs a signed `test`/`jl` failure check after `XInputGetState`.
  The source's explicit `long` result is unusual relative to the API's DWORD
  spelling, but is target-backed rather than a matching trick.
- Gamepad and memory-unit hotplug masks map to the named internal change bits
  in target order. The abstraction callback is declared by its owning input
  abstraction header rather than locally.
- The four-gamepad polling, analog hysteresis, button tick aging, raw-stick
  storage, and dead-zone-adjusted stick storage follow the target's control
  and data flow.
- The keyboard path authenticates the 32-bit device-mask scan, 104 key-tick
  updates, bounded key ring, modifier flags, signed ASCII check, virtual-key
  table lookup, and the one-argument XDK debug-keystroke queue call.
- Source uses semantic types/constants/macros, ordinary declarations, and
  explicit returns. It introduces no raw-address calls, representation
  punning, fake lifetime or padding control, optimizer pragma, `register`,
  `volatile`, forced-inline directive, inline assembly, or forbidden
  `point_from_line3d` emission.

The local `DEBUG_KEYBOARD` feature definition only exposes the real XDK debug
keyboard declarations and produces no incidental code or data owner. It is
not a caller-local API substitute.

## Helper and private-name review

`input_update_analog_button_state` is not an invented candidate-only helper.
January has a standalone 48-byte, zero-relocation owner at `0x000BE800` with
the same press/release threshold behavior. VC7 also naturally inlines that
ordinary static helper into the large device poller while retaining the
out-of-line owner. Candidate source obtains the same topology without an
`inline` keyword or force control. Its semantic name is inferred from its
behavior and independently preserved donor naming, not generated from the
address.

The two inherited RET-only functions are now named
`input_unidentified_noop_1` and `input_unidentified_noop_2`. A January
relocation census and available symbol/donor records reveal no callers or
original names. These labels are explicitly provisional descriptions, grant
no new exact credit, and do not justify new public prototypes or lifecycle
semantics.

## Exact and fuzzy accounting

| Owner | Target / candidate padded bytes | Relocations | Disposition |
| --- | ---: | ---: | --- |
| `_input_update` | `96 / 96` | `14 / 14` | strict exact; 83 meaningful bytes |
| `_input_frame_begin` | `16 / 16` | `2 / 2` | strict exact; 13 meaningful bytes |
| `_input_update_analog_button_state` | `48 / 48` | `0 / 0` | fuzzy park, `97.0%`; one compiler spelling difference |
| `_input_get_device_states` | `1232 / 1280` | `28 / 32` | honest natural fuzzy park, `89.23637%` |
| `_input_update_keyboard_devices` | `640 / 640` | `49 / 51` | honest natural fuzzy park, `93.70744%` |

The candidate code inventory contains the 16 inherited exact owners, the two
new exact callers, and the three reconstructed residual private owners. No
extra helper or other orphan code owner is emitted.

## Non-code ownership

The four private lookup tables form a natural 784-byte `.rdata` contribution
whose raw bytes equal January (SHA-256
`764bf177da47695eed5fa6a1b745d42a18cf1a326649fdf222051729066cc56d`).
Their independent boundaries are authenticated by target relocations at
contribution offsets `0`, `8`, `16`, and `528`; they are not modeled as a fake
aggregate. Canonical data accounting correctly withholds credit because the
larger unmatched read-only span also contains strings owned by unwritten
functions.

The candidate emits one 1,036-byte uninitialized `.bss` section, flags
`0xC0400080`, owned by file-static `input_globals`, and no other writable-data
owner. The name is present in January assertion literals and static ownership
is corroborated by later PDB/source evidence plus absence of cross-TU users.

One inherited DWORD at `input_globals + 0x230` is written by January
initialization but still lacks an authenticated semantic field name. It must
remain disclosed layout debt: do not invent an aggregate, fake member purpose,
or separate owner merely to cover it. This uncertainty does not affect the
fields exercised by this packet and is not a blocker to the two exact caller
closures or the honest fuzzy poller admission.

## Integration checks

Root's final no-alias regeneration and full stable sweep reported:

- final source hash above accepted by the gate;
- 18 exact, three residual, six unwritten functions for `input_xbox.obj`;
- all 16 inherited exact functions retained;
- full campaign strict-owner count `6181`, with zero exact regressions;
- all 249 fuzzy parks valid; and
- all 284 tooling tests passing.

On that evidence, this packet is ready to publish as a two-function exact gain
plus three source-authentic fuzzy reconstructions. Whole-object completion and
the unresolved `+0x230` field remain future work.
