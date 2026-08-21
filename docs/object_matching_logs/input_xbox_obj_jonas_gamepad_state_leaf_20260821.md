# `input_xbox.obj` Jonas public gamepad-state leaf

## Result

This Jonas-owned ledger records one fixed two-body, ordinary-C wave from exact
canonical commit `57cc2d4a5f38b635fff54ec1337b048f73243024`.
`_input_get_gamepad_state` was independently strict in the first and only
code-producing two-body candidate and is retained. The equally sized
`_input_get_raw_data_string` candidate was nonexact and was removed immediately
without a spelling, declaration, expression-order, control-flow, or compiler
retry. `_input_frame_begin` and every private/raw helper were outside the wave
before source work began and remain absent.

`input_xbox.obj` advances from 8/27 to 9/27 exact functions and from 26/3,557
to 133/3,557 meaningful code bytes. It remains `NonMatching`; no data credit
is claimed.

| Candidate | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 | Decision |
| --- | ---: | ---: | ---: | --- | --- |
| `_input_get_gamepad_state` | `0x000BEAB0` | `107 / 112` | 8 | `eac1914f21723d418481596ddcec3df48ff3614a52836c12c85c86799705ab10` | retained |
| `_input_get_raw_data_string` | `0x000BF3D0` | `237 / 240` | 14 | target `3a95b06caa2d47b995d9c24ba0a7450d1ccbc091e6a4dc288cc6a986d8f76c5a`; candidate `42e834fea3fadb8e97330480e4ab0394f439e726ab8c1482a635901879789bab` | removed |

The strict net gain is 107 meaningful bytes, 112 padded bytes, and eight exact
relocations. The other 18 target functions and all target non-code data remain
outside this wave's credit.

## Authenticated artifacts and source identities

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/input/input_xbox.obj` SHA-256:
  `a956f90624ce49ef7ecbdaf6005c5f008695170ebbee05687ed0dd08e4316457`.
- Baseline `source/input/input_xbox.c` Git blob is
  `4ee5e62702096b8ca2119eea1778adae5da08e13`, 3,734 bytes, with Git-blob
  payload SHA-256
  `2a79ea5557d5bc2f377f13c8c90a08ade73c0203eea4ecb4577aff5c933a1014`.
- The retained source's staged, and therefore commit-intended, Git blob is
  `5cedda25dfa003d483f315b8c96c175b901b1971`, 5,045 bytes, with SHA-256 of
  the exact `git cat-file blob` payload
  `d634753900c75e18c61c32a61563cd4aec1e54f4c95a7cd141e1fce79e568f76`.
  These are Git-blob bytes, not CRLF working-tree bytes.
- The unchanged shared `source/input/input.h` blob is
  `bf7c2cdb342e555087d5550eec75088b4101f581`, 4,258 bytes, payload SHA-256
  `e284e37b16b4091b30c7a30cf12af462c373ae914eb4eeae4273eddec8a53415`.
- Clean HCEA donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its tree-reachable blobs are:
  `src/input_get_gamepad_state.c` at
  `5022b3d2ab05f3f97fee1976c1228c07f5d29ebc` (403 bytes, payload SHA-256
  `5a67d6a2babbf4ef105d3b1cb288fd2ab8bcf24ff0203562c27c68fe4f42d22f`),
  `src/input_get_raw_data_string.c` at
  `0c9712fb6532022fb7a358836e3b94efa6d365e1` (100 bytes, payload SHA-256
  `7e246e33ff3bcc274a92339a106602ecca36f63209f50d4138892420c7bfc02d`),
  `src/headers/gamepad_state.h` at
  `c4e987775fea94cf868e2f7bb2d0b26b45c1eca3` (687 bytes, payload SHA-256
  `f26e64ccd2513e96fafa281e250125effd862568a6d4050c24351a60462e2281`),
  and `src/headers/input_globals_xbox.h` at
  `219367126eb168568960b034ff886f295e6cd680` (622 bytes, payload SHA-256
  `52b6031daf2bd9d5c18e8e535cd0643d687b92f9013e127a39141e005f1ffc5f`).
- Clean Stian reconstruction commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`. Its relevant
  `src/halo/input/input_xbox.c` blob is
  `acfee91820ef5d1fb6414f2de862fd964a3783b4`, 34,361 bytes, payload SHA-256
  `458ae8b15995cb959ef2b4aceeafb4e1ce3327456948d650653252709227cde3`.
  Its methodology/build identification comes from `README.md` blob
  `76b9608518552ad98077bba98e59a654430d1e55`, 7,743 bytes, payload SHA-256
  `d62afb152e0295fdc39b65961ea90290a59bcf80498a6065299576445cc47049`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
  flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the repository's
  unchanged XDK 3911 include graph.

HCEA authenticates the public names, signed 16-bit index/size domains, the
40-byte gamepad-state shape, and basic intent, but its CE/A gamepad accessor
does not contain January's connected-handle/suppression behavior and its raw
string body is only a stub. Stian is an explicitly binary-backed,
LLM-assisted reconstruction of retail Xbox `01.10.12.2276`; it is not
original source and is not a January-PC byte oracle. It supplies independent
semantic topology for the handle/suppression choice and raw formatting loop.
No Stian raw address, pointer cast, private ABI, or expression spelling was
copied. January COFF remains authoritative for the PC signatures, code
extents, assertions, call identities, global addends, relocation order, and
acceptance.

Repository history contains no prior committed implementation or matching log
for either exact body. The January target defines each symbol exactly once.

## Public signatures and cross-TU compatibility

The unchanged shared header already declares the fixed ordinary i386 cdecl
interfaces:

```c
const struct gamepad_state *input_get_gamepad_state(short gamepad_index);
void input_get_raw_data_string(char *buffer, short size);
```

The retained definition is exactly compatible with the first declaration.
All current source callers include the shared `input.h`; repository-wide
declaration search finds no competing prototype or duplicate definition.
Current typed callers in `game/cheats.c`, `game/game_engine.c`, and
`interface/event_manager.c` consume the returned
`const struct gamepad_state *` without a cast. January target relocation
evidence additionally identifies callers in `bink_playback`,
`camera/director`, `game/player_control`, `input/input_abstraction`,
`interface/ui_widget`, and `main/main`; none establishes a conflicting ABI.
January's only raw-string caller is `render/render_debug`, and the unchanged
shared declaration remains compatible even though this wave rejected that
body.

The shared `struct gamepad_state` has ten analog-button values, ten threshold
values, four button bytes, and two `point2d` sticks. Its natural 40-byte size
and array stride are cross-TU stable under the unchanged public header.

## Typed storage boundary and exact relocation proof

The pre-existing 0x40C-byte `input_globals` tentative object remains a COFF
COMMON symbol: `_bss_004536a0` is still section zero, external storage class,
value/size 1,036. This wave does not create a `.bss` section, writable
`.data`, or a new storage owner. It merely replaces padding inside the
already-sized local aggregate with typed fields and compile-time checks:

- suppression flag at `+0x138`;
- four pointer-sized gamepad handles at `+0x13C`;
- four 40-byte gamepad states at `+0x14C`;
- suppressed gamepad state at `+0x1EC`;
- inherited frame-active flag at `+0x22D`;
- total aggregate boundary `0x40C`.

The final candidate's eight `_input_get_gamepad_state` relocations match
January exactly in offset, type, destination, order, and addend:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x1E` | `DIR32` | exact source-path literal |
| `+0x23` | `DIR32` | exact gamepad-index assertion literal |
| `+0x28` | `REL32` | `_display_assert` |
| `+0x2F` | `REL32` | `_system_exit` |
| `+0x3C` | `DIR32` | `_bss_004536a0+0x13C`, indexed handle |
| `+0x46` | `DIR32` | `_bss_004536a0+0x138`, suppression flag |
| `+0x50` | `DIR32` | `_bss_004536a0+0x1EC`, suppressed state |
| `+0x5E` | `DIR32` | `_bss_004536a0+0x14C`, indexed live state |

The retained body therefore returns null for a disconnected index, otherwise
returns the singleton suppressed state or that index's live state. The exact
assertion emits two compiler-owned runtime `.rdata` COMDATs: the 34-byte path
chunk has normalized SHA-256
`38f9edd23201fe0784f235a110eb405bf630a18fc943ae05a92264168fa82ec0`,
and the 51-byte predicate chunk has normalized SHA-256
`0a8f96cfdb2df786001702cc08e49fd407225f43097151aef9e0023152f92663`.
Both chunks and their ownership identities equal January, but this incomplete
object receives no non-code credit. Relative to the base object, all other
runtime non-code shape is unchanged; compiler debug metadata changes only as
expected for the new COMDAT.

## One-shot classification and fail-closed removal

Both natural bodies and the typed aggregate were fixed before the first
code-producing invocation:

```text
ninja -v build/base/source/input/input_xbox.obj
```

That sole two-body candidate object has whole-file SHA-256
`4b8af09d9f8d631f3f6f42d1b1dd3b5b1f3131c47bbd3f95401d79782bca0671`
and length 4,376. The gamepad accessor was strict immediately. The raw-string
body had the same 237 meaningful bytes, 240 padded bytes, and 14 relocations
as January, but not the same normalized bytes or relocation schedule. Its
header-format/call pair was at `+0x6A/+0x74` instead of January's
`+0x6E/+0x78`; its two COMMON references were handles at `+0x82` then raw
sticks at `+0x87`, instead of January's raw sticks at `+0x84` then handles at
`+0x8B`; and its per-gamepad format/call pair was at `+0xBA/+0xC3` instead of
`+0xBE/+0xC7`.

The raw body and its support-only typed fields were removed immediately. No
second declaration form, loop form, induction variable, cast, qualifier,
expression order, control-flow shape, or compiler switch was attempted. Reopen
it only with independently preserved January source or another compiler-
authentic artifact that explains the schedule naturally. `_input_frame_begin`
(January 13 meaningful / 16 padded, two relocations, normalized SHA-256
`7f8a484a3e821ca5cd2e5ae3f709d8a1adcad776dc63b32059e7b44d3a6b0c3b`)
was explicitly precluded, as were `_fix_dead_zone`, `_update_ticks`, every
unnamed/private `_code_*` body, and all raw helpers.

The retained-source object before the committed replay has SHA-256
`b5402e43606d5ca884ddca7a91cdc8a073b496772918600479a6db0f2608d777`
and length 3,152. Its complete function-definition inventory is exactly the
eight inherited accepted functions plus `_input_get_gamepad_state`:
`_code_000bebb0`, `_code_000bebc0`, `_input_suppress`, `_input_activate`,
`_input_deactivate`, `_input_get_mouse_state`,
`_input_mouse_button_is_down`, `_input_frame_end`, and the new accessor.
Direct hardened comparison proves all nine independently strict.
`_input_get_raw_data_string`, `_input_frame_begin`, and every other target
function remain absent.

## Defined-C, ownership, and scope policy

The retained source is readable typed C using the shared public structure,
ordinary pointer/array selection, an existing assertion macro, and an explicit
terminal return. It contains no assembly, raw address or byte-offset
dereference, pointer/integer reconstruction, object-representation cast,
pointer or union pun, inactive-union access, undefined signed overflow,
private/register ABI invention, volatile scheduling device, force-inline
annotation, optimizer pragma/barrier, synthetic anchor, or object-byte forcing.

Only `source/input/input_xbox.c` and this new Jonas-owned ledger are tracked
changes. No shared header, configuration, semantic exception, parked record,
completion label, storage size/definition, protected-five source, pre-existing
Markdown, Claude-owned file, or tracked deletion is changed. The protected
five are `source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/hs/ai_script.c`, `source/ai/actions.c`, and `source/units/units.c`.

## Validation

- Complete 568-edge `halobetacache_build` plus `libcmt_build`: pass.
- Direct hardened COFF comparison: all nine emitted functions have exact
  padded bytes, normalized hashes, relocation offsets/types/destinations, and
  addends; both rejected/excluded public symbols are absent. All eight
  inherited accepted COMDATs remain strict.
- Semantic audit: 470 units, 4,119 functions evaluated, 3,979 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,040 accepted exact, 36
  ordinary-only, 35 structurally accepted, one rejected, and zero unit errors.
- Campaign progress: 375/833 matched objects, 4,027/11,060 functions,
  480,137/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 data bytes.
  Halo contributes 273/468 objects, 3,860/7,574 functions, and
  467,223/1,770,166 code bytes; libcmt contributes 102/212 objects,
  167/476 functions, and 12,914/55,015 code bytes.
- `input_xbox.obj`: 9/27 functions and 133/3,557 meaningful code bytes; it
  remains nonmatching and claims no data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check`, source scope,
  protected-five, and banned-construct checks are clean.
- The clean committed-state one-unit snapshot/check, verified same-path
  deletion and forced rebuild of `input_xbox.obj`, second regression check,
  direct nine-body COFF comparison, rejected-symbol absence check, COMMON
  boundary check, and committed Git-blob payload recheck are performed as the
  final handoff replay. No push is performed.
