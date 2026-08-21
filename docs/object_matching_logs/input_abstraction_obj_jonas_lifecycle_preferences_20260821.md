# `input_abstraction.obj` Jonas lifecycle/preferences wave

This Jonas-owned ledger records one bounded natural-C experiment for six
`input_abstraction` lifecycle/preferences functions. Five functions were
strict in the first code-producing candidate and are retained. The nonexact
initializer was removed without any declaration, expression-order,
control-flow, or compiler-shape retry. The pre-existing exact controller
detection timer is preserved. The object advances from 1/10 to 6/10 exact
functions and remains `NonMatching`; no data credit is claimed.

## Scope and provenance

- Authoritative integration base:
  `7d2ff479dde012f94cc08857e4cc74650b85b9af`.
- Baseline `input_abstraction.c` blob:
  `2b5561422386fddfc7b595bd3425fe0af1a2f47c`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `input_abstraction.obj` SHA-256:
  `bcc7e63bb068e060ce5cc93de3481d797d6890e1df3df1c9deddacaa909e8ca4`.
- Local HCEA source oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`.
- Relevant HCEA body blobs are `a97b9bedff70ff1f98359e216f89eb398ba0ce50`
  (dispose), `e13ea831c4b0f71e53542ef58b19a0e71f135d43` (get
  preferences), `f11f81f260386a91c7443f5a2efeb37fdaa248b6` (update
  preferences), `a766c3944f336adf565f98f03837fc224b138a7d` (get state),
  `48df63a54e95530fc9c034625db0416a8cac54e7` (device changes),
  and `e546384e54d4677db88bc560c9f2198d4dd5be85` (initialize).
- HCEA typed-header blobs are
  `156bbe73eee46dcf73e15b8d400ddc648768b34d`,
  `84867cba94bb6da7c669a8ed7922f763fd8d8d10`, and
  `f52cce54908502a6d1cf475d66ce098d690691fc` for preferences,
  input state, and globals respectively. HCEA is a semantic oracle here; its
  CE/A structure sizes and global tail differ from this January PC target.
- The local January-era reconstruction oracle is commit
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`, with committed
  `src/halo/input/input_abstraction.c` blob
  `ee7110b6013c7c6d08c42011eba9653278e449b4`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler
  `13.00.9254.1` for 80x86, `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Generated flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 and source include paths.

The local reconstruction confirms the January PC 24-byte preference
defaults and the `0xDC` clear boundary, but uses raw executable addresses and
is not copied. HCEA supplies readable field roles and function intent.
January COFF remains authoritative for PC byte extents, assertions, calls,
global addends, and exact ownership.

## Exact candidate signatures

The bounded six were:

```c
void input_abstraction_dispose(void);
void input_abstraction_get_local_player_preferences(
    short local_player_index,
    struct game_input_preferences *preferences);
void input_abstraction_update_local_player_preferences(
    short controller_index,
    struct game_input_preferences const *preferences);
struct game_input_state *input_abstraction_get_input_state(
    short local_player_index);
void input_abstraction_update_device_changes(
    unsigned long device_change_flags);
void input_abstraction_initialize(void);
```

## Typed PC boundary

The retained source uses ordinary natural structures and compile-time proofs:

- `game_input_preferences`: size `0x18`, with two rates, twelve button
  mappings, a signed joystick preset, and two boolean inversion fields;
- `game_input_state`: size `0x1C`, with twelve button values and four real
  movement/look values;
- the preference array begins at `+0x00` and the state array at `+0x60`;
- startup timer `+0xD0`, controller availability `+0xD4`, initialized flag
  `+0xD8`, and first-insertion timestamp `+0xDC`;
- the natural aggregate size is `0xE0`, while dispose clears only the
  authenticated prefix ending at `+0xDC`.

The global is declared only as an undefined typed `extern`. This lane does
not define the historical 224-byte BSS owner and does not claim its data.
The named offsets are independently exercised by exact January relocations;
the size/offset assertions prevent silent layout drift.

## One bounded candidate

The initial invocation stopped in parsing because this branch's
`cseries.h` does not own the `real` typedef. With explicit campaign-lead
approval, only the existing authenticated owner header
`math/real_math.h` was added; all six bodies and their source shape remained
unchanged. That parse-only stop emitted no candidate object. The subsequent
invocation was the sole code-producing candidate compile.

| Function | Meaningful / padded | Relocs | Target normalized SHA-256 | Decision |
| --- | ---: | ---: | --- | --- |
| `_input_abstraction_dispose` | `21 / 32` | 2 | `8bfdf7317781b03600f18c12d62d6d758db156536b5e96ddfc91290de2b7603a` | retained |
| `_input_abstraction_get_local_player_preferences` | `120 / 128` | 10 | `f7c8c3b171242c08ac8b8680433aedc0ef9ce6497dbd59ea355d3325f48c8625` | retained |
| `_input_abstraction_update_local_player_preferences` | `164 / 176` | 14 | `e3a4199b6c653f3032665ed183544e27fcf9ecd476e9da26abe89f7774ddd211` | retained |
| `_input_abstraction_get_input_state` | `65 / 80` | 5 | `112ade4a567a9952d4811b8e05bdf86ea0fef61e311f6796d0d1b8101b27b248` | retained |
| `_input_abstraction_update_device_changes` | `115 / 128` | 12 | `1ae2914d18ec4be87d3d15aafc58e63e9b1b5ccc3c04e813eba222c1a84a6c9f` | retained |
| `_input_abstraction_initialize` | `150 / 160` | target 9 / candidate 8 | target `b6da87e4cb2be8447d8bbea226c2ca47deada5ffd3e1766bf1c8c3066f76ac06` | removed |

The five retained functions contribute 485 meaningful and 544 padded code
bytes with 43 relocations. The nonexact initializer had the correct padded
extent, but its candidate normalized SHA-256 was
`9b931b43db8ce66ffba167ea3967e952282ece327ee821271a0f1fb5686edb5b`.
It lacked the target's independent `_input_abstraction_globals+0x68`
relocation and shifted the later global relocations. It was removed
immediately without a second code-producing candidate.

The pre-existing
`_input_abstraction_reset_controller_detection_timer` remains strict at
11 meaningful / 16 padded bytes, two relocations, and normalized SHA-256
`6401d253ae381cdce5605596c6384a543baf63876176a2fb07920f18734d9de8`.
The final direct comparator accepts all six emitted function sections.

## Semantic and policy boundary

The retained bodies use typed aggregate and array access, `csmemset`,
`csmemcpy`, ordinary assertion macros, system time, error reporting, and the
existing Bink stop API. They contain no raw address, byte-backed aggregate
view, row-pointer cast, pointer pun, union, assembly, `volatile`, force-inline
annotation, optimizer pragma/barrier, undefined aliasing, private-EAX ABI
invention, or byte-forcing expression.

Normal string-literal COMDATs required by the exact assertions and device
message are emitted. Because the object remains incomplete and no full
natural data-owner boundary is established, they receive no separate data
credit. No source-defined global, COMMON owner, `.bss`, or `.data` is added.

Do not retry the initializer through declaration order, pointer induction,
expression reordering, synthetic anchors, raw-address reconstruction, or
compiler controls. Reopen it only with independent January-era source, PDB
local topology, or another compiler-authentic artifact that explains its
extra `+0x68` global access naturally.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Strict semantic audit: 470 units, 3,999 functions evaluated, 3,859
  semantic exact, 101 hidden exact / 61,876 hidden code bytes, 3,920 accepted
  exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,907/11,060 functions,
  472,728/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 data bytes.
- Halo progress: 273/468 complete objects, 3,740/7,574 functions,
  459,814/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 data bytes.
- `input_abstraction.obj`: 6/10 functions and 496/3,314 meaningful code
  bytes; no data credit. It remains `NonMatching`.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- The fail-closed pre-wave manifest is pinned to the authoritative base and
  reports exactly the five intended functions as `NEWLY_EXACT`, with
  `changed_nonexact: []`. The generic old/new structural comparison reports
  the expected new function/debug/string COMDAT and symbol-set review
  findings. It also conservatively reports changed accepted evidence for the
  timer sibling; direct strict comparison proves that sibling's normalized
  bytes and both relocation destinations remain identical. No adjudication,
  configuration exception, or waiver is added.
- `git diff --check` and the explicit forbidden-source scan pass. Frozen
  `vehicles`, `ai_debug`, `ai_script`, `actions`, and `units` surfaces are
  unchanged.

Final source identities before the local checkpoint commit:

- Git blob: `b29bf5358fdb6e47d9e5df1e8acae96bcaf3deb8`;
- SHA-256:
  `ba226d9d547a65bcfd3b3da5eb30df91ecfa07a333604e1de73076586ad6e051`.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `input_abstraction.obj` is the final same-path reproducibility
proof and is recorded in the handoff. No configuration, exception ledger,
parked record, frozen object, or pre-existing Markdown file is changed, and
no push is performed.
