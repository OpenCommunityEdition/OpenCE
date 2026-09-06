# Units assertion and common-constant reference cleanup

## Result

The inventory-weapon diagnostic now uses the shared `match_vassert` macro.
The condition is `!TEST_FLAG(weapon->object.flags,
_object_connected_to_map_bit)`: a connected weapon fails, as in January.
The format string, argument order, source path, original line 8505, fatal
report, and `system_exit(-1)` are preserved. Diagnostic formatting remains
lazy on the failure path.

The source-private helper formerly named `code_0019b4c0` is now
`unit_drop_inventory_weapons`. The declaration, definition, actual caller,
source inventory comment, and one split-symbol metadata record agree. The
target metadata now marks the owner private; the source was already static.
No public prototype, ABI change, inline directive, or helper body was added.

The shared Cseries header adds `dassert`, `dwarn`, `dhalt`, and their
`match_d*` counterparts as diagnostic aliases. Existing assertion, warning,
and halt semantics and message-expression APIs are unchanged.
The new documentation is linked from the root README, matching methodology,
and object-ledger README:

- [Common constants](../common_constants.md)
- [Assertion API and migration examples](../assertions.md)
- [User-supplied source reconstruction map](../user_source_reconstruction_map_20260906.md)

This is source-quality/tooling progress, **zero new exact bytes or objects**.

## Authority and limited changes

Baseline: canonical `98f3bb719d7af4ccf572c51d126e5534356d7709`.
The old complete inventory closure ledger is
[units_obj_jonas_inventory_weapon_closure_20260824.md](units_obj_jonas_inventory_weapon_closure_20260824.md).

January target file offset 1684672 (`0x0019B4C0`) is the 256-byte padded
inventory helper (250 meaningful bytes, 13 relocations). Its genuine caller
is `unit_died`; it drops non-current inventory weapons, repairs the desired
slot and deletes unusable weapons. The later HCEA semantic counterpart is
`unit_drop_inventory_weapons`; Stian's PC reconstruction calls the analogous
owner `unit_drop_weapons_on_death`. This is a corroborated semantic name, not
proof of a recovered January source identifier. No later-build behavior or
four-slot inventory limit was imported.

Local donor files inspected:

```text
../../_research_cache/halocea/src/blam/units/unit_drop_inventory_weapons.c
../../_research-stian-halo/src/halo/units/units.c
```

The pinned compiler remains XDK 3911 VC7 13.00.9254.1, unchanged repository
flags. A scratch preprocessing probe rejected variadic macro syntax with
C2010 and `__VA_ARGS__` undeclared. The supported source form passes the
ordinary variadic `csprintf` call as the message expression to `vassert`.
No replacement compiler, new runtime formatter, or forced inline was used.

## Verification

- Full `ninja -j4`: pass, including all 355 affected compile edges and split,
  report, semantic-report, and progress stages.
- Whole-tree stable verdict sweep: **6,271 / 8,245 exact; zero gains and zero
  regressions**. Units remains **189 / 189 strict exact**.
- Complete frozen-object comparison: all **571 other built objects** are
  byte-identical after zeroing only COFF timestamp bytes 4..7.
- Units: **447 candidate sections and 318 target sections**, including all
  section headers, code/data payloads and raw relocation tables, are unchanged.
  The only parsed symbol change is the helper rename; target storage changes
  from external to static as documented above. Source-private storage remains
  static. All 832 other split objects are timestamp-normalized identical.
- Units already emitted a 48-byte candidate-only `point_from_line3d` copy.
  A blanket emitted-symbol prohibition therefore fails even on the inherited
  baseline. The complete comparison verifies this copy is unchanged, not
  added; it remains absent from the January Units target before and after.
  This cleanup does not claim to resolve that inherited ownership discrepancy.
- **184 executable diagnostic cases pass**, compiling the current header with
  real VC7 in matching/`NON_MATCHING` and debug/`NDEBUG` modes and executing its COFF code
  under bounded diagnostic API doubles. Checks include false/true/noncanonical
  truthy conditions, single condition evaluation, failure-only diagnostic
  evaluation/formatting, correct messages/location/fatal flag/exit code,
  success-return ABI, four connected/unconnected weapon-flag combinations,
  nonfatal warning continuation, and unconditional halt dispatch. Fatal API
  dispatch is tested, not the termination implementation itself.
- Full tools suite: **838 tests and 26 subtests pass**.
- Canonical meaningful totals remain **919,549 / 2,198,102 code bytes**,
  **6,225 / 11,060 credited functions**, and **391 / 833 Matching objects**.

The test harness initially attempted to relocate debug-only records; it was
corrected to load only runtime sections and fail closed on unknown runtime
relocations. Unicorn's internally handled Windows memory-probe diagnostics
use the existing test-suite scoped faulthandler suppression pattern; Python
assertions, emulator errors, and instruction/ABI bounds remain enforced.

Immutable local evidence:

| Artifact | SHA-256 / content |
| --- | --- |
| `scratch/damage-five-owner-before-20260906.json` | `ffebde75999c17b99fbe0874eb564f6d1a343376abbbb3526f0023c0fb1bbd6d`; all 572 base and 833 split objects frozen before any production edit |
| `scratch/assertion-cleanup-before-20260906.json` | focused source/config/Units/Cseries originals |
| `scratch/assertion-cleanup-final-20260906.json` | first assertion-only revision; retained as intermediate evidence |
| `scratch/assertion-cleanup-family-final-20260906.json` | final complete diagnostic-family source/config/focused objects |
| `scratch/diagnostic-family-whole-object-review-20260906.json` | `9f4994e15589a0eca9e37400525da7a8bf66a18bcefb0926770d309144b902c3`; full final rebuild compared with frozen baseline |
| `scratch/damage-five-owner-before-stable-20260906.json` | 6,271 exact baseline |
| `scratch/diagnostic-family-after-stable-20260906.json` | 6,271 exact, zero regression after the complete family |

Original objects and scratch captures are local evidence, not publication
payload. The tests, fixtures, reference documents, owner changes, and this
ledger are suitable for the reviewed source batch. No active Opus worktree
was modified. The Damage reconstruction is deferred with its read-only
private-helper audit preserved; no Damage production candidate was applied.
