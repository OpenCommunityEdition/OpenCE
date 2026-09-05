# `input_xbox.obj` remaining public leaves, 2026-09-04

## Scope and result

This bounded follow-up begins at canonical commit
`d556ba5304eeb4ee663b7c81998925b36f2d5cd7`, after the device/keyboard
poller wave and its semantic data ownership were published.  January split
COFF remains the sole authority for code bytes, ABI, relocations, and owner
boundaries.  The previously audited retail-Xbox reconstructions corroborate
behavior only; no donor raw address, private ABI guess, or source spelling is
accepted as a byte oracle.

The frozen natural source advances `input_xbox.obj` from **18 exact, three
residual, six unwritten** functions to **20 exact, four residual, three
unwritten** functions.  `_input_get_key` and `_input_key_is_down` are newly
strict, for **289 meaningful / 304 padded bytes** and 28 exact relocations.
`_input_get_raw_data_string` is retained as an honest, useful fuzzy
reconstruction.  All 18 inherited exact functions and all three inherited
park measurements remain unchanged.  No configuration, parked manifest,
header, build tree, or completion label is changed in this lane.

The frozen source SHA-256 is
`b8195cdc8bb4e07791182c036e35ef96e6fd753d57f72f1f853e2e6c7f34cbd3`.
The PID-isolated scratch object is
`scratch/input-xbox-root-terminal-return-20260904.obj`, SHA-256
`a85da70887081eca1c01d2db7fb7cbb082ad87ff2c7b96efe1aa561905751510`.
The raw object hash includes compiler debug-path material; the per-owner
normalized hashes below are the stable comparison evidence.

## Source and public ABI

The existing owning header `source/input/input.h` already declares all three
public interfaces with the correct ordinary i386 cdecl ABI:

```c
boolean input_key_is_down(short key_code);
boolean input_get_key(struct key_stroke *key);
void input_get_raw_data_string(char *buffer, short size);
```

No caller-local prototype, duplicate declaration, new shared type, or header
edit is introduced.  The definitions use one parameter per line, the real
`boolean`, `short`, `byte`, `HANDLE`, `point2d`, `gamepad_state`, and
`key_stroke` types, named key/stick enum constants, semantic `input_globals`
fields, and explicit terminal returns.

`input_key_is_down` implements the January suppression check and combines the
left/right modifier keys with the existing `MAX` macro for the generic shift,
control, Windows, and alt cases.  Its default path uses the exact January
assertion source, line, and range expression before reading `key_ticks`.
`input_get_key` naturally dequeues the next four-byte `key_stroke` from the
64-entry ring and advances the read index after the exact January assertion.
`input_get_raw_data_string` uses the signed 16-bit size contract, the real raw
stick and handle arrays, January's literal `|n`/`|t` formatting strings, and a
pointer-and-count loop over four gamepads.

The source uses no inline or assembly control, `volatile`, `register`, raw
address, offset cast, representation pun, metadata alias, fake helper, anchor,
optimizer pragma, or nonsensical control flow.

## Strict closure

| Source / January owner | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_input_get_key` | `97 / 112` | 9 | `a3b48c1ac1818bbd2c937a5f56fa988454af93d4a023201c03682be5fd73f436` |
| `_input_key_is_down` | `192 / 192` | 19 | `c1ab3b0043e411c7e6695fb129837501c3bca12a2477d9497b5123f3eaf17805` |

Both candidate owners have identical padded bytes, normalized bytes, and
ordered relocation types, offsets, destinations, and addends to January.  The
dequeue's nine relocations resolve to `_input_globals+0x308`,
`_input_globals+0x30A`, the January source/assertion literals,
`_display_assert`, `_system_exit`, and the ring at
`_input_globals+0x30C`.  This is an ordinary context closure: the same
natural public body was previously withheld as a small scheduling residual,
then became strict after the authentic surrounding translation-unit
reconstruction.

The key predicate's final source uses one genuine `boolean result`, assigns it
in the suppression/switch state machine, and returns it once at the end.  This
is the natural structure required by the explicit-terminal-return house rule;
it also reproduces January's 192-byte jump-table owner exactly.  The review
did not add a dummy fallback value or tune declaration order, register use,
expression spelling, or compiler controls.

## Honest fuzzy boundary

| Source / January owner | Target size / relocs / SHA-256 | Candidate size / relocs / SHA-256 | Disposition |
| --- | --- | --- | --- |
| `_input_get_raw_data_string` | `240 / 14 / 3a95b06caa2d47b995d9c24ba0a7450d1ccbc091e6a4dc288cc6a986d8f76c5a` | `240 / 14 / 1032cde83ef635472499ea3ef854f7bbd2877bc5fb0ef29aa4dc4f41c631557e` | Equal extent and relocation count.  Candidate materializes handles then raw states while January's two global loads occur in the reverse order; no declaration-order tuning is attempted. |

The residual is not described as byte-equivalent or exact.  Root may add a
fresh fail-closed park record only after the production object and objdiff
report reproduce this measurement.  This lane does not edit
`config/parked.json`.

The inherited fuzzy owners remain stable at their published measurements:

| Owner | Candidate size / relocs / SHA-256 |
| --- | --- |
| `_input_update_analog_button_state` | `48 / 0 / 78f85bb297cc663f10c1fcea301827288980a1f2dc6cf99b0012b4fe641a1bb9` |
| `_input_get_device_states` | `1280 / 32 / b7cded6529564446f880d89597e525942284f3f273aea62da3e10b259c0acdde` |
| `_input_update_keyboard_devices` | `640 / 51 / 3e938f14594007d55c8c4582e070f1912ce6788c9ebbb76d7eae75d5bd2923dc` |

Review history is preserved without making it the final claim.  The initial
multi-return key predicate emitted a credible `208 / 19` residual with hash
`4acb7c6f4e39a8a6fed34a3c496b0d42b991b4575da0a4f48429bc2051bc0e08`.
Adding an unreachable `return FALSE;` only to provide a textual footer did not
change that code and was rejected as fake source.  The final single-result,
single-return form is both source-natural and strict; the obsolete residual is
not parked or counted.

## Initialization, rumble, and thread boundary

January's remaining private 176-byte rumble flusher and 32-byte input thread
form a coherent lifecycle cluster with `_input_initialize`.  Natural private
reconstructions were checked once against the real XDK
`XINPUT_FEEDBACK`/thread ABI.  They emitted no code because the only genuine
caller topology is the still-unwritten initializer: ordinary static VC7
dead-code elimination removed both definitions.  Making either function
external, adding a synthetic anchor, or inventing another call solely to
retain a section would violate the source and ownership rules, so those
unemitted definitions were pruned from the frozen source.

`_input_initialize` remains deliberately unwritten.  January writes a DWORD
at `input_globals+0x230`, between the authenticated event state and keyboard
handle.  The published private layout still labels this six-byte interval as
reserved storage, and the available donor merely repeats the raw address; no
PDB, January symbol, use census, or owning API currently authenticates a
semantic field name or purpose.  The wave does not manufacture a field,
offset cast, or padding-purpose claim to gain the initializer and its private
callees.  The inferred private names `input_update_gamepad_rumbler_states`
and `input_update_thread`, and the latter's real `DWORD WINAPI(LPVOID)` SDK
callback ABI, remain research notes rather than emitted source owners.

The exact remaining target boundary is therefore:

| Unwritten January owner | Padded / relocs / SHA-256 | Blocker |
| --- | --- | --- | --- |
| `_code_000bf0a0` | `176 / 8 / f473cb2d504d90b7ac4014fa2d05f189e9ef27ace087fd7fb23d69a6c9c19949` | Private rumble owner has no authentic emitted caller until initialization is reconstructed. |
| `_code_000bf540` | `32 / 3 / bb9c454c6ca60f04e12b08b0ebe14f2a43c22eb08e8488d72cf2e13a9ab563d0` | Private wait/callback owner is reached only through the blocked initializer. |
| `_input_initialize` | `256 / 22 / cf5e2e92a28cbbf77bea5849fe3771d78c9a3aaebe152b3abac243e465870a4c` | `input_globals+0x230` ownership and semantics are unauthenticated. |

## Complete owner and data census

The frozen gate lists exactly 27 January function owners: 20 strict, four
fuzzy, and the three unwritten owners above.  The candidate emits no
candidate-only code owner or compiler helper.  In particular, neither private
rumble/thread function is present and the `_point_from_line3d` emitted-symbol
guard passes.

Non-code topology remains the published InputXbox topology: one natural
784-byte read-only contribution with four semantic lookup-table owners and one
1,036-byte uninitialized `_input_globals` owner.  The public leaves add only
their January-owned assertion and formatting literals.  This wave makes no
new data-credit or whole-object admission claim.

## Validation and root handoff

- PID-isolated baseline: **18 exact / 3 residual / 6 unwritten**.
- Frozen PID-isolated gate: **20 exact / 4 residual / 3 unwritten**.
- Emitted-symbol guard: pass for `_point_from_line3d`.
- Inherited exact set: all 18 preserved; newly exact only `_input_get_key` and
  `_input_key_is_down`.
- Inherited park hashes: all three preserved.
- Complete emitted function inventory: 24 target-owned sections, no extras.
- `git diff --check` for the owned source: pass.

No `ninja`, configure step, build-tree mutation, header/config edit, commit, or
push is performed here.  Root must regenerate the production object, measure
a fresh objdiff percentage for the proposed raw-string park, run the full stable
regression and policy suite, and make the final integration decision.
