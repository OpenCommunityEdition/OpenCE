# `ai.obj` Jonas vehicle-event pair recovery

## Result

This bounded source-only wave starts from canonical commit
`09186240fea218e8f19d9dc952125e94179a0775` and recovers both public AI
vehicle-event handlers on the first natural candidate compile. The seven
pre-existing exact functions in `ai.obj` remain strict, so the translation
unit advances from 7/46 to 9/46 exact functions and remains `NonMatching`.

| Exact function | RVA | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | ---: | --- |
| `_ai_handle_enter_vehicle` | `0x2EFB0` | 53 | 64 | 2 | `08b892066d796a97005f63cce5fe171dfae62e30cb7e7d77b9dcbcbb7de47a09` |
| `_ai_handle_exit_vehicle` | `0x2EFF0` | 89 | 96 | 4 | `740b319cf7f321a5541e83058e694275237821778553fda30fd1cbf0bd4e1d50` |
| **Wave gain** | | **142** | **160** | **6** | |

The retained nine-function subset is 421 meaningful bytes, 496 padded bytes,
and 36 relocations. Hardened comparison reports exact normalized bytes,
padding, relocation types and order, destinations, and addends for both new
functions and all seven inherited exact functions.

## Provenance and signatures

- The canonical baseline `source/ai/ai.c` blob is
  `e87419ce28529e2a6672362cee8e68575b738f5f`; the retained source blob is
  `6b5ec508389c499d940cf24e8fe9ba43edfe3f19`.
- The January executable SHA-256 is
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- The January split `source/ai/ai.obj` SHA-256 is
  `5b207c27308c3e78b7adf5bfc14f65c7cf8cc99aeba15b845270eb572265ced2`.
- The extracted January PDB-symbol JSON SHA-256 is
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
- The clean HCEA oracle is pinned at
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact function blobs are
  `d66ca8f834043f878ddec2f269748bf947acc421` (enter) and
  `b35a2d363ef8993196c3777f4c952a8164c04570` (exit).
- The clean January-PC oracle is pinned at
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; its `src/halo/ai/ai.c` blob is
  `e273c750eea58d0e43daf1eb0a9037685cf01094`. Its VC7.1 verification gives
  100% instruction and operand scores to the same bodies at
  `0x409A0..0x409D5` and `0x409E0..0x40A39`.
- The compiler is Microsoft 32-bit C/C++ Optimizing Compiler
  `13.00.9254.1` for 80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the repository's
  unchanged XDK 3911 and include paths.

January callers and the two-parameter HCEA declarations establish the public
signatures:

```c
void ai_handle_enter_vehicle(long unit_index, long vehicle_index);
void ai_handle_exit_vehicle(long unit_index, long vehicle_index);
```

The January `units.obj` relocation in `_unit_enter_seat` is at section offset
`+559`: the call at `+0x22E` pushes the vehicle handle from `[ebp+0xC]` and
then the unit handle in `ESI`. `_unit_try_and_exit_seat` relocates the exit
call at `+251`: the call at `+0xFA` pushes the parent vehicle from unit
`+0xCC`, then the unit handle in `EDI`. Both callees end in an ordinary `ret`.
This proves two 32-bit cdecl arguments even though the second is unused.

The C identifiers map to the January external COFF symbols
`_ai_handle_enter_vehicle` and `_ai_handle_exit_vehicle`. Stian's semantic
enter donor is named `unit_vehicle_board_notify`, and its exit declaration
shows only the one parameter read by that body; neither target-specific
spelling is transplanted. HCEA and the January caller ABI fix the production
names and two-argument signatures above.

## Authenticated control and layout

Both functions obtain the unit through the canonical `unit_get` macro. The
January target calls `_object_get_and_verify_type` with mask `3`, then reads
the actor handle at unit `+0x1A4`. Enter returns when that handle is `NONE`;
otherwise it calls `_ai_communication_event` with type `0x24`, the unit, four
`NONE` indices, and a null context.

Exit performs the same unit lookup and actor-handle test, then uses the
canonical `actor_get` macro. Its four relocations are exactly
`_object_get_and_verify_type`, `_actor_data`, `_datum_get`, and
`_ai_communication_event`. It tests actor byte `+0x38C`; when clear it emits
event `0x25` with the same remaining arguments, then clears the byte on both
paths.

The target-specific typed layout is authenticated by committed January
sources: `units.h` blob `54492a4ee4cb8b37739f9ebbfa89c72fbd0e08fc`,
`objects.h` blob `738c831f77a73e2dc3a1e7e9443144857c480ce8`, and
`actors.h` blob `e061b529b05c782b1220cff8a05dd89e6e2f3ed2`.
`scenery.h` blob `33623b67e0c3b8cb924161f396b4751b092d1134` proves the
base object size `0x1A4`; `actor_perception.c` blob
`07f56945a860f0b9b1d0146b00298eb834374281` independently asserts actor
emotions at `+0x350`. HCEA's `actor_emotion_data.h` blob
`3dd93ea5e57bbd183a6fadfec6a23a66e8f0c70c` places
`last_vehicle_exit_forced` at relative `+0x3C`, yielding `+0x38C`.

HCEA's PC `unit_datum.h` blob
`1a8bcca480b4e2c25ecb3f87716be80911d1366f` is retained as semantic
provenance but is not used to override the January layout: its later PC
object prefix places the unit payload at a different offset. The source uses
the January `units.h` and `actors.h` owners and proves the two accessed offsets
at compile time.

## Bounded compile and storage boundary

Both definitions were emitted together in exactly one natural candidate
compile. Each body was independently strict, so no declaration, expression,
or control shape was changed and no retry was performed. A later clean forced
rebuild is only a reproducibility replay of this accepted source.

The target owns one external type-`0x20`, storage-class-2 definition of each
symbol, in `ai.obj` sections 28 and 29. The only unresolved consumers are the
two calls in `units.obj`. Neither body references or owns a string, literal
pool, global definition, `.data`, `.bss`, or other non-code section; all event
and sentinel values are immediate constants. `ai.obj` therefore advances to
421/12,932 meaningful code bytes while claiming 0/3,324 target non-code bytes.

## Policy and scope

Only `source/ai/ai.c` and this new Jonas-owned ledger are changed. No shared
header, configuration, protected AI source, semantic exception, parked
record, completion label, pre-existing Markdown, or storage definition is
modified. `ai_dispose`, the already-attempted race mapper, private callbacks,
and every other unreviewed body remain omitted.

The implementation uses typed public helpers, named fields, ordinary cdecl
functions, and readable control flow. It contains no assembly, volatile
scheduling, forced inline, optimizer pragma or barrier, raw address, pointer
arithmetic, type pun, unaligned load, inactive-union read, undefined overflow,
synthetic anchor, or object-byte forcing.

## Regression and validation

- Complete 568-action `halobetacache_build` and `libcmt_build`: pass.
- Semantic audit: 470 units, 4,130 functions evaluated, 3,990 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,051 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,038/11,060 exact functions,
  480,818/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. Halo contributes 3,871/7,574
  functions, 467,904/1,770,166 code bytes, and
  1,829,896/3,923,451 data bytes.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.
- The pre-wave TU gate identifies exactly the pair as `NEWLY_EXACT` and
  reports `changed_nonexact: []`. Adding the two COMDATs shifts metadata for
  the two later exact wrappers and changes compiler debug and symbol sections;
  the gate exposes those expected structural findings. Direct hardened
  comparison independently proves all seven inherited functions strict. No
  adjudication or configuration exception is added.
- `git diff --check`, deleted-path, source-policy, and protected-scope checks
  pass. Only the translation unit and this ledger are tracked changes.
- A clean committed-state snapshot/check followed by deletion and forced
  rebuilding of `ai.obj` reproduces the accepted object and leaves all nine
  accepted functions strict with no changed nonexact sibling.

No push is performed.
