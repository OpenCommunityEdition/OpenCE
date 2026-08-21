# `hud_messaging.obj` Jonas six-leaf wave

## Result

This bounded source-only wave starts from authoritative integration commit
`bb360484e9f30c00d241b8f26d3fecac8e1dcd43`. All six requested HUD
messaging bodies were strict on the first and only naturally emitted candidate
object, so all six are retained without a source-shape or compiler-control
retry. Together with the two untouched exact dispose stubs,
`hud_messaging.obj` advances from 2/36 to 8/36 exact functions and remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_hud_messaging_globals_update` | 13 | 16 | 1 | `803edb48122438ebe006bc0648fe6e9960e2c7243c3830df3798b93c128141d1` |
| `_scripted_hud_show_timer` | 20 | 32 | 1 | `e2d70a5beee0fbb9e1952b9b08f39396410f2f799dcab7861706bc5d674c740f` |
| `_scripted_hud_time_code_reset` | 25 | 32 | 4 | `910dab26bf2faa315136095741adc4d2d3071b969c24e49128b3539bad0cab7c` |
| `_hud_messaging_initialize` | 26 | 32 | 3 | `0fdc7d86986c6f5cc03d2c558b059abf6279e2cf8eda38b94637b621fe419b2d` |
| `_scripted_hud_set_timer_warning_cutoff` | 32 | 32 | 1 | `6c56d8335ee3a38b52a1a8fb998c70f886c1399718f19d8e2577ea7f5bdf6012` |
| `_hud_messaging_initialize_for_new_map` | 33 | 48 | 4 | `73ebfc221fb6e0ceca1d0780a1f8f76f8f8bf72d7ee15ed7ff6c5ca890d824dd` |
| **Wave gain** | **149** | **192** | **14** | |

The hardened comparator proves exact padded bytes plus relocation addresses,
types, names, destinations, and addends. The final accepted eight-function
subset is 151 meaningful bytes, 224 padded bytes, and 14 relocations. The
remaining twenty-eight target functions and all 632 target non-code bytes stay
outside this wave's credit.

## Provenance

- Baseline `source/interface/hud_messaging.c` blob:
  `8a9b77e4fab98c11097d74101fd2424ed64a2bad`.
- Retained source blob/SHA-256:
  `170ff77eb9c53e6ec13f871c53fc74a4d8fc07f7` /
  `7e3867674429eb311dea9dce21da6c34fc6f2b3fa0aaf4ab5edbb07cd82ea966`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `3c5c3b8162268f4803a4c98742a28d6a76e61c3d6632c6995075c2ecaf0d152c`.
- Extracted January PDB-symbol JSON SHA-256:
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
  It independently fixes all six exact labels and their January RVAs.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name
  `hud_messaging_globals_update.c`, `scripted_hud_show_timer.c`,
  `hud_messaging_initialize.c`,
  `scripted_hud_set_timer_warning_cutoff.c`, and
  `hud_messaging_initialize_for_new_map.c` files supply the natural named
  behaviors and typed state relationships. HCEA's time-code evaluator also
  independently fixes the reset routine's void/no-argument call contract.
- Preserved January-PC reconstruction commit:
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its
  `src/halo/interface/hud_messaging.c` blob is
  `6b59533b11d13386d279ce39e7e0daa134282cac`. It independently corroborates
  the PC runtime offsets, timer arithmetic, time-code reset, allocator call,
  and map-reset behavior.

HCEA is the semantic/type oracle. January PC COFF and its independent PC
reconstruction are authoritative where platform layouts differ, and January
COFF alone decides exact bytes, padding, and relocation ownership.

## Signatures and caller contract

```c
void hud_messaging_globals_update(void);
void scripted_hud_show_timer(boolean show);
void scripted_hud_time_code_reset(void);
void hud_messaging_initialize(void);
void scripted_hud_set_timer_warning_cutoff(
    short minutes,
    word seconds);
void hud_messaging_initialize_for_new_map(void);
```

The existing HaloScript caller declaration already uses `boolean` for timer
visibility, no arguments for time-code reset, and the exact `short, word`
argument record for warning cutoff. The evaluator's second field is a
16-bit `word`; January loads the promoted 32-bit arguments and truncates the
computed tick count into the signed 16-bit cutoff field. No caller or shared
header repair is needed.

## Authenticated typed state

The source-local runtime view is ordinary typed state, not an address overlay.
Compile-time checks prove the January-PC layout used by these bodies:

- the complete allocated/reset block is `0x11A8` bytes;
- the per-frame message marker is the byte at `+0x1185`;
- the 16-byte timer begins at `+0x1198`;
- its signed-short warning cutoff is at `+0x119E`;
- its Boolean visibility field is at `+0x11A7`;
- the HUD messaging parameter block is `0x120` bytes and is the first member
  of `hud_globals`, so `&hud_globals->messaging` has no synthetic offset.

HCEA's typed model independently establishes the same field roles and
relationships. Its console runtime block is smaller because the number and
layout of per-player records differ; the January PC allocation immediate,
field displacements, full clear, and PC reconstruction establish the `0x11A8`
variant retained here.

The January anchor `bss_00453ab8` is declared as a pointer to the typed runtime
record. `hud_globals`, `hud_msg_def`, `time_code_time`, and
`time_code_stop_time` are also typed externs. All five remain undefined in the
candidate COFF; no COMMON, `.bss`, or `.data` storage is created. The allocator
body necessarily emits the target-authentic `"hud messaging"` literal, but
this wave makes no non-code ownership or data-matching claim.

## Behavior and one-shot boundary

`hud_messaging_initialize` allocates the complete runtime record through the
existing typed `game_state_malloc` declaration. The new-map initializer points
the active definition at the named first member and clears the complete record
through `csmemset`. The two timer setters write their named fields, and the
cutoff expression performs the HCEA/January `30 * (60 * minutes + seconds)`
conversion. Time-code reset samples `game_time_get` once, always resets the
active time, and updates the stop time only while it is not `NONE`. The globals
update clears the per-frame marker byte.

All six bodies were compiled together once. Every body was independently
strict on that object; no rejection, declaration permutation, spelling retry,
statement reorder, or code-generation experiment followed. Every void body
has an explicit terminal `return;`.

The untouched exact siblings remain directly strict:

| Preserved function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_hud_messaging_dispose_from_old_map` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_hud_messaging_dispose` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |

## Policy, scope, and validation

Only `source/interface/hud_messaging.c` and this new Jonas-owned ledger are
changed. No shared header, caller, configuration, frozen-five source,
semantic exception, parked record, pre-existing Markdown, or Claude-owned
file is edited.

The retained code is readable typed C. It contains no assembly, raw address or
byte-offset dereference, pointer/integer reconstruction, pointer or union pun,
inactive-union access, undefined overflow, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, synthetic anchor, or
object-byte forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass (568 actions).
- Direct hardened comparison: all six new bodies and both preserved stubs pass.
- Semantic audit: 470 units, 4,035 functions evaluated, 3,895 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,956 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,943/11,060 exact functions,
  474,797/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. The HUD wave itself adds six
  functions and 149 meaningful bytes with no data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

The fail-closed pre-wave snapshot was captured after canonical advanced to
`90475f0a`, which retained the authoritative HUD source blob unchanged. Its
check identifies exactly these six functions as `NEWLY_EXACT` and reports
`changed_nonexact: []`. Adding earlier COMDATs renumbers the two dispose
sections and their associative debug sections, so the generic whole-TU
fingerprint reports those accepted entries and debug/symbol surfaces as
changed; direct hardened comparison independently proves both dispose bodies
retain identical normalized bytes, padding, flags, and zero relocations. No
adjudication or configuration exception hides that structural report.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `hud_messaging.obj` and its reports closes the current section
ownership surface before handoff. No push is performed.
