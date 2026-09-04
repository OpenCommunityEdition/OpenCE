# `input_xbox.obj` device and keyboard poller wave, 2026-09-04

## Scope and result

This bounded wave reopens the January Xbox device and debug-keyboard polling
cluster after new SDK-layout and owner evidence resolved the earlier broad
topology rejection.  January split COFF remains the sole authority for bytes,
ABI, relocation ownership, and boundaries.  The stock XDK 3911 headers fix the
platform structures and constants; preserved Stian/Pastudan reconstructions
corroborate high-level behavior only.

The frozen natural source advances `input_xbox.obj` from **16 exact, zero
residual, 11 unwritten** to **18 exact, three residual, six unwritten** against
the pre-rename target.  The two strict closures are `_input_update` and
`_input_frame_begin`, for **96 meaningful / 112 padded bytes**.  All 16
inherited exact functions remain exact.  The two large private pollers and the
analog threshold helper are honest fuzzy reconstructions, not exact claims.
No parked or admission configuration is changed in this lane.

The source is frozen at SHA-256
`678dbce1c5035b52b95c8b13398b904497f29c6fc147d74560154ca92d51269a`.
The semantic-source scratch object is
`scratch/input-xbox-device-keyboard-final-source-names-20260904.obj`, SHA-256
`7b671456356384f2cdea85f8bd3c11b413654e9e01855709b67675a41d11e466`.

## Reconstructed source and ABI

The source now uses the genuine XDK `XINPUT_FEEDBACK`, `XINPUT_STATE`, and
`XINPUT_DEBUG_KEYSTROKE` types.  Its private `input_globals` layout is exactly
`0x40C` bytes, with compile-time checks for every field used by this wave:

| Field | Offset / extent |
| --- | ---: |
| `gamepad_feedbacks[4]` | `0x000 / 0x118` |
| `raw_gamepad_states[4]` | `0x118 / 0x020` |
| `suppressed`, `update_thread_started` | `0x138`, `0x139` |
| `gamepad_handles[4]` | `0x13C / 0x010` |
| `gamepad_states[4]` | `0x14C / 0x0A0` |
| `suppressed_gamepad_state` | `0x1EC / 0x028` |
| `gamepad_rumbler_states[4]` | `0x214 / 0x010` |
| update thread/event state | `0x224` through `0x22D` |
| keyboard handle/ticks/latches/ring | `0x234` through `0x40C` |

The global is ordinary semantic source:
`static struct input_globals input_globals = {0};`.  Its name is authenticated
by January assertion literals.  File-static linkage is strongly corroborated
by the HCEX PDB File Static record, its absence from the January public-symbol
inventory while adjacent public globals are present, and a source census that
finds no consumer outside this translation unit.  January itself is stripped,
so the linkage inference is recorded rather than overstated as direct January
debug-symbol proof.

`input_get_device_states` performs genuine XDK gamepad and memory-unit hotplug
handling, translates SDK masks into named internal change bits, notifies the
input abstraction owner through its real header declaration, polls four
gamepads, updates button duration and threshold state, and stores raw and
dead-zone-adjusted sticks.  `input_update_keyboard_devices` performs debug
keyboard hotplug handling, ages key ticks, drains the genuine SDK keystroke
queue, maps modifier/ASCII/virtual-key state, and updates the existing key
ring.  The source contains no raw-address call, representation cast, fake
stub, helper anchor, forced inlining, assembly, volatile/register control, or
optimizer pragma.

The ordinary static analog threshold helper is not an invented candidate-only
owner.  January owns `_code_000be800` as a 48-byte, zero-relocation function
with exactly that press/release threshold behavior.  January's broad poller
has no call relocation because VC7 auto-inlined the helper while retaining its
out-of-line copy.  The candidate obtains the same topology from an ordinary
`static` definition; it uses no `inline` keyword or forcing mechanism.  The
semantic name is inferred from the independently preserved donor name and the
body's behavior rather than from its address.

## Strict closures

| Source name | January name | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_input_update` | same | `83 / 96` | 14 | `de67c361cd5d6ad0b7a5f1c2c7e6d9892c030b7269fa9dd6f5ebab4e324f4d85` |
| `_input_frame_begin` | same | `13 / 16` | 2 | `7f8a484a3e821ca5cd2e5ae3f709d8a1adcad776dc63b32059e7b44d3a6b0c3b` |

The update body clears suppression, resumes the already-created update thread
once, calls the private keyboard poller, and invokes `HATRun` on all four
40-byte gamepad states.  The frame-begin body calls the private device poller
and sets `frame_active`.  Padded bytes, normalized bytes, ordered relocation
types, symbolic destinations, and addends all equal January in the diagnostic
object.  These closures are now link-closed because their private callees have
real definitions in the same translation unit; their callees need not be
byte-exact for the caller sections themselves to be strict.

## Honest fuzzy boundary

| Source name | January owner | Target size / relocs / SHA-256 | Candidate size / relocs / SHA-256 | Distance and disposition |
| --- | --- | --- | --- | --- |
| `input_update_analog_button_state` | `_code_000be800` | `48 / 0 / 5850a17e99380c5499a719ba1667ef91f992a37284afe5049ba6e7f04e79c826` | `48 / 0 / 78f85bb297cc663f10c1fcea301827288980a1f2dc6cf99b0012b4fe641a1bb9` | `97.00%`; sole `sub al,32` versus compiler-canonical `add al,-32`; fuzzy, no exact claim |
| `input_get_device_states` | `_code_000bebd0` | `1232 / 28 / 56316233dc2ab683b304fa910b38ac5a82e29e8a13850a48cf31af300c81d580` | `1280 / 32 / b7cded6529564446f880d89597e525942284f3f273aea62da3e10b259c0acdde` | `89.21%`; real frame/register/table-hoisting and schedule differences; fuzzy |
| `input_update_keyboard_devices` | `_code_000bf150` | `640 / 49 / 80bcd2f76c52c728ac2196960ec81c6f2364ab006debe2291d6405a3c7372d89` | `640 / 51 / 3e938f14594007d55c8c4582e070f1912ce6788c9ebbb76d7eae75d5bd2923dc` | `93.65%`; equal extent but genuine scheduling/relocation differences; fuzzy |

The prior poller candidate was 1,680 padded bytes.  Replacing indexed/global
shapes with natural pointer-and-count loops reduced it to 1,280 and restored
the target's broad control-flow topology.  The remaining differences are not
classified as equivalent or exact.  No declaration, expression, branch, or
source-order lottery is attempted after this bounded natural pass; the
campaign's fuzzy-first policy applies.

## Read-only table ownership

The source defines four independently meaningful private tables, not a fake
aggregate.  They occupy one natural 784-byte `.rdata` contribution whose raw
bytes exactly equal January (both SHA-256
`764bf177da47695eed5fa6a1b745d42a18cf1a326649fdf222051729066cc56d`).

| Candidate owner | Contribution offset | Absolute January boundary | Size | January relocation proof |
| --- | ---: | ---: | ---: | --- |
| `gamepad_analog_button_indices` | `0x000` | `0x26F570` | 8 | device-poller relocation at file offset `781865` targets table `+0` |
| `gamepad_binary_button_masks` | `0x008` | `0x26F578` | 8 | device-poller relocation at file offset `781975` targets table `+8` |
| `virtual_key_to_key_code` | `0x010` | `0x26F580` | 512 | keyboard-poller relocation at file offset `783127` targets table `+16` |
| `ascii_to_key_code` | `0x210` | `0x26F780` | 256 | keyboard-poller relocation at file offset `783057` targets table `+528` |

The exact source offsets above should be treated as relocation-address
descriptions; the authoritative symbolic facts are the target references to
contribution addends `0`, `8`, `16`, and `528`.  Configuration should preserve
the existing `0x310` contribution extent, rename its first owner, and add the
three interior semantic boundaries.  It must not describe the whole span as a
single source aggregate.

The candidate also emits exactly one 1,036-byte uninitialized `.bss` section,
flags `0xC0400080`, owned by `_input_globals`.  It emits no other writable data
owner.

## Symbol/accounting handoff

The stripped January owners need these evidence-backed semantic metadata
renames before a no-alias final gate:

| January file offset | Existing owner | Semantic owner | Linkage |
| ---: | --- | --- | --- |
| `780288` | `_code_000be800` | `_input_update_analog_button_state` | static |
| `781264` | `_code_000bebd0` | `_input_get_device_states` | static |
| `782672` | `_code_000bf150` | `_input_update_keyboard_devices` | static |
| BSS `4535968` | `_bss_004536a0` | `_input_globals` | static (corroborated inference) |
| RDATA `2553200` | `_rdata_0026f570` | `_gamepad_analog_button_indices` | static |
| RDATA `2553208` | interior | `_gamepad_binary_button_masks` | static |
| RDATA `2553216` | interior | `_virtual_key_to_key_code` | static |
| RDATA `2553728` | interior | `_ascii_to_key_code` | static |

The final candidate code inventory is exactly the 16 inherited owners, these
three private fuzzy owners, and the two new strict public callers.  It emits
no unexpected code section or compiler helper, and the
`_point_from_line3d` guard passes.  The exact `.rdata` and `.bss` shapes do not
make the incomplete object `Matching`; six January functions remain
unwritten and three remain fuzzy.

## Validation boundary

- PID-isolated baseline gate with emitted-symbol guard: **16 exact / 0
  residual / 11 unwritten**.
- Frozen pre-rename diagnostic gate using only identifier aliases for the four
  renamed target owners: **18 exact / 3 residual / 6 unwritten**.
- Pre/post exact-set comparison: no inherited exact loss; only
  `_input_update` and `_input_frame_begin` become exact.
- Complete candidate function/data owner census: no orphan or unexpected
  owner; 784-byte exact read-only contribution; 1,036-byte uninitialized
  global.
- No `ninja`, configure, build-tree mutation, configuration edit, commit, or
  push is performed in this lane.  Canonical target regeneration and the full
  stable regression sweep remain root-owned requirements before integration.

## Root integration and inherited naming debt

The root applied the semantic owner metadata above and independently rebuilt
the production split/candidate objects without aliases. The result remains
18 exact / three residual / six unwritten. Full-tree snapshot comparison
preserves all 6,176 baseline exact owners and gains five functions across this
batch, including the two input callers. The production fuzzy percentages are
97.0 / 89.23637 / 93.70744 for threshold/device/keyboard respectively; the
earlier approximate diagnostic percentages above are not canonical records.

Two inherited empty function identifiers were then changed, without changing
their bodies, to `input_unidentified_noop_1` and `input_unidentified_noop_2`.
These are explicitly inferred descriptive labels, NOT recovered original
names. January owners at 781232 and 781248 contain only RET plus 15 padding
NOPs; the relocation census finds no incoming references, and the stripped
January PDB and pinned HCEA/Stian counterparts provide no names or caller
semantics. Their inherited arity and external linkage remain provisional:
no new public API/prototype, invented call, static-emission workaround or
lifecycle meaning is asserted. This uncertainty remains whole-object
admission debt and grants no new exact credit. The final source hash and
rename-stable verification are recorded in the batch integration ledger.

The four genuine table owners exactly reproduce the natural 784-byte
read-only contribution, but are not added to canonical data credit: the
current fail-closed data policy requires covering the entire 1,712-byte
unmatched read-only span, which also contains strings for unwritten functions.
No policy change or fake aggregate is used to bypass that boundary. The real
1,036-byte BSS restoration receives ordinary exact data credit.
