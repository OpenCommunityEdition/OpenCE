# `input_xbox.obj` lifecycle-caller boundary, 2026-09-04

## Scope and result

Subsequent same-day update: genuine device/keyboard poller implementations
resolved this caller-only C2129 boundary. The two public callers are now
strict exact with real same-TU callees. See
`input_xbox_obj_device_keyboard_pollers_wave_20260904.md` and the root batch
ledger; the earlier rejected partial packet below remains useful history.

This bounded review starts from canonical commit
`4795da408` and considers only the small lifecycle cluster
`_input_frame_begin`, `_input_update`, and `_input_initialize`. It preserves
the fixed prior rejections for the broad device and keyboard pollers, the
keyboard/raw-data bodies, and the analog hysteresis helper. January PC COFF is
the sole byte, ABI, relocation, and symbol-ownership authority; the Stian and
Pastudan retail reconstructions are semantic corroboration only.

No source body is retained and the exact gain is **zero**. This is an
intentional source-fidelity boundary, not an unsuccessful byte-tuning park.
The final replay object is
`scratch/input-xbox-small-lifecycle-final-20260904.obj` and remains **16 exact,
zero residual, 11 unwritten**. All 16 inherited exact functions are preserved,
and the `_point_from_line3d` emitted-symbol guard passes.

## January topology

The three public owners themselves are small and their high-level intent is
well corroborated:

| Function | Padded bytes / relocations | Normalized SHA-256 | January dependencies |
| --- | ---: | --- | --- |
| `_input_frame_begin` | 16 / 2 | `7f8a484a3e821ca5cd2e5ae3f709d8a1adcad776dc63b32059e7b44d3a6b0c3b` | private `_code_000bebd0`, then `frame_active = TRUE` at global `+0x22D` |
| `_input_update` | 96 / 14 | `de67c361cd5d6ad0b7a5f1c2c7e6d9892c030b7269fa9dd6f5ebab4e324f4d85` | `ResumeThread`, private `_code_000bf150`, and four `HATRun` calls |
| `_input_initialize` | 256 / 22 | `cf5e2e92a28cbbf77bea5849fe3771d78c9a3aaebe152b3abac243e465870a4c` | private `_code_000bf540`, private `_code_000bebd0`, and `_input_update` |

`input_update` clears suppression at `+0x138`, conditionally resumes the thread
handle at `+0x224` according to the byte at `+0x139`, invokes the private
keyboard/device updater, and runs HAT over all four typed 40-byte gamepad
states beginning at `+0x14C`. `input_initialize` uses the genuine XDK
`XDEVICE_PREALLOC_TYPE` and `XINPUT_DEBUG_KEYQUEUE_PARAMETERS` layouts,
preallocates gamepad/debug-keyboard/memory-unit devices, creates the event and
thread, initializes the keyboard queue and HAT, performs an initial device
poll/update, and returns true. These facts authenticate semantics and field
boundaries, but do not remove the private-definition dependency.

The retail reconstructions provide reasonable semantic names for the target
private owners—`input_get_device_states` for `_code_000bebd0`,
`input_update_keyboard_devices` for `_code_000bf150`, and
`input_keyboard_thread` for `_code_000bf540`. They do not prove original debug
symbol spellings, and address-derived `code_...` identifiers are not acceptable
source names under the campaign rules.

## Why the callers are withheld

A natural diagnostic draft used semantic, TU-private declarations and ordinary
typed caller bodies. VC7 correctly rejected it with C2129 because a `static`
function was declared and called but not defined in the translation unit. The
alternative ways to make that draft compile are all inauthentic:

- declaring the target-private owners `extern` would give them false external
  linkage;
- retaining stub definitions would emit wrong helper COMDATs and fake runtime
  behavior;
- using function-pointer casts or address calls would discard the real ABI and
  violate the raw-address and prototype rules;
- expanding or forcing the helper work into the callers would contradict
  January's explicit call relocations and the no-forced-inlining rule.

Accordingly, an exact-looking caller cannot be admitted until its private
callee has a credible definition in the same C file. `_input_frame_begin` and
`_input_update` therefore remain unwritten, and `_input_initialize` is also
withheld because it transitively requires both of them plus the thread owner.
No production declaration, field, include, or function body from the
diagnostic draft remains.

## Reopen condition

Reopen this cluster only after new compiler-authentic ABI/layout evidence
supports natural reconstructions of the fixed-rejection pollers, especially
the 1,232-byte device-state owner and 640-byte keyboard owner. Once those
private definitions exist under semantic names, the 16-byte frame caller and
96-byte update dispatcher should be retested as ordinary source; initialization
can then follow with the genuine XDK types and the recovered thread owner. Do
not admit these callers first through external aliases or fake helper stubs.

## Verification boundary

- Pre-review and post-review PID-isolated gate: 16 exact / 0 residual / 11
  unwritten.
- All prior input ledgers were read, including their fixed rejection evidence.
- No source/header/configuration edit, residual park, admission exception,
  build-tree mutation, commit, or push is retained or claimed by this review.
