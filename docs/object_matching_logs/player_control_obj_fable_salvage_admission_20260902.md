# `player_control.obj` Fable salvage audit (2026-09-02)

## Verdict

The preserved Fable wave contains one new strict closure and two useful,
complete semantic reconstructions.  After house-rule reconciliation,
`source/game/player_control.obj` advances from 41/49 to 42/49 strict-exact
functions, with no loss from the canonical exact set.  The admitted exact gain
is `_player_control_permanent_impulse`, worth 64 target padded bytes.  The two
large reconstructed functions remain honest fuzzy parks and receive no exact
credit.  Five functions are still unwritten, so this is not an object-complete
claim.

The audit was performed in the isolated worktree
`work/fable-player-control-integration-20260902` from canonical commit
`293d9d0c4`.  The preserved input was commit `4eb51d68e` in
`work/fable-small-families-salvage-20260902`.  Nothing was pushed and the
canonical checkout was not modified.

## Authorities and provenance

- January's split `source/game/player_control.obj` is the byte authority.
  Measurements use XDK 3911 and `tools/campaign/gate.py`; strict exactness
  includes padded bytes, normalized bytes, and relocation identity/count.
- January disassembly and relocations establish the bodies, constants, and
  ownership.  The HaloCEA sources under `research-cache/halocea-cseries-20260820`
  independently corroborate the gameplay behavior and the public
  `player_control_camera_control_is_active` name.
- PDB evidence authenticates the private names
  `player_control_action_test_check_reset_input_blob`,
  `get_local_player_input_blob`, `player_control_modify_desired_angles`, and
  `handle_one_player_input`.  `player_action_clear` is the best semantic name
  inferred from its target behavior; the address-derived placeholder is not
  retained.  The player-control global owner is named consistently with the
  existing subsystem type and later-source convention.
- Existing local cseries types, flags, accessors, and subsystem headers take
  precedence over later-port spelling where they differ.

## Strict baseline and gain

The detached canonical baseline at `293d9d0c4` gates as:

```text
== exact 41  residual 0  unwritten 8  (of 49 listed)
```

The reconciled candidate gates as:

```text
== exact 42  residual 2  unwritten 5  (of 49 listed)
```

The exact padded-byte total increases from 3,200 to 3,264.  The only new exact
function is:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_player_control_permanent_impulse` | 64 | 5 | `85f3ed804f320738d098cbfef1b83c09ff52df74eb62555801272f1eac7c8925` |

An independent raw exact-set comparison across all 572 rebuilt translation
units found zero lost exact functions and exactly that one new function.  The
whole-tree raw totals moved from 5,045 functions / 621,420 meaningful code
bytes to 5,046 / 621,484.  The accepted progress totals moved from 5,135 /
679,974 to 5,136 / 680,038.  Object completion remains 384/833.

The candidate also reproduces the target's 1,536-byte, one-relocation initialized
data owner with normalized SHA-256
`dbae63e5f80748fce9805b9e115a8e1d91ed511038531031db80c81b0f07d226`.
That adds 1,536 exact data bytes without being misreported as code progress.

## House-rule reconciliation

- Address-derived function and BSS names were replaced with the strongest
  available PDB, independent-source, or convention-backed semantic names.
  Private symbol ownership is represented in `config/symbols.json`; the one
  public camera-control query has its prototype in the owner header.
- The temporary attempt to mark
  `player_control_camera_control_is_active` private was rejected: because its
  two target callers are still unwritten, normal dead-code elimination removed
  the function from this partial translation unit.  It remains an ordinary
  externally emitted function with an owner-header prototype until those
  callers are reconstructed.  No keepalive, fake call, or storage trick was
  added merely to retain its bytes.
- `float` was replaced with the canonical `real` type.  Tag and object access
  use `TAG_BLOCK_GET_ELEMENT`, `unit_get`, and `unit_definition_get`; bounds and
  flags use existing cseries helpers and named constants.
- The natural January-proven `2/pi` scalar is written as `0.63661975f`.
  Replacing the later donor's full-precision expression corrected the
  relocation value without introducing a fabricated dependency.
- Function declarations live in `source/game/player_control.h` or as private
  declarations in the owning `.c`.  Parameters follow the established
  multiline format, no-argument lists use `void`, and new void functions end
  in an explicit `return;`.
- No forced inline/noinline annotation, volatile/register forcing, barrier,
  pragma, raw offset access, invented address identifier, fake dependency,
  nonsensical branch, or inline assembly is retained.  The scratch object emits
  no `point_from_line3d` symbol or COMDAT and therefore preserves the January
  inline schedule constraint.

## Honest fuzzy parks

The complete semantic bodies below are retained because they replace unwritten
slots with useful and well-supported source, but they are not called exact.
Detailed evidence and reopen criteria are recorded in `config/parked.json`.

| Function | Target/base padded bytes | Target/base relocs | Objdiff | Class |
|---|---:|---:|---:|---|
| `_player_control_initialize_for_new_map` | 144/144 | 10/10 | 92.888885% | unclassified |
| `_player_control_modify_desired_angles` | 1,584/1,584 | 83/83 | 99.59677% | instruction scheduling |

The map initializer is exact from offset `+0x0f` through its epilogue.  Its only
remaining difference is the prologue order of two independent flag stores and
the EDI save; bounded ordinary-C shapes did not recover January's schedule, and
the fail-closed classifier returns unknown.

The desired-angle function has all 83 relocation identities correct.  Its
remaining difference is one dependency-free else-arm schedule: January
materializes constants and the frame pointer between a velocity load and x87
multiply, while VC7 emits the multiply first.  The fail-closed classifier proves
independent instruction scheduling.  Both functions are parked rather than
forcing an optimizer-only source shape.

## Remaining unwritten functions

```text
3456  _get_local_player_input_blob
1568  _handle_one_player_input
  16  _player_action_clear
 512  _player_control_action_test_check_reset_input_blob
 128  _player_control_update
```

These five functions account for the remaining unwritten portion of this unit.

## Verification

- Final isolated gate: 42 exact, two residual, five unwritten; 3,264 exact
  padded bytes.
- Complete canonical and candidate `ninja all_source` builds passed before the
  final name-only camera-control relabel.  The independent 572-unit exact-set
  comparison reported zero regressions.  A final one-TU scratch compilation
  and gate after that relabel revalidated all 42 exact functions.
- `python -m pytest -q -p no:cacheprovider`: 258 passed.
- `python tools/fake_match_scan.py --fail-on-findings` on both changed source
  files: zero review leads.
- `python -m tools.parked_functions`: 110 active, zero stale, zero invalid.
- Object-admission audit: zero candidates or contradictions; the only listed
  rejection is the pre-existing unrelated `player_rumble` COMDAT record.
- Scratch COFF symbol scan: no `point_from_line3d` owner.
- `git diff --check`: pass.

The manifest-oriented regression helper is not used as the exact-set authority
for this wave because the deliberate semantic relabels in `config/symbols.json`
change target symbol labels.  Raw normalized function identity was compared
directly instead, avoiding both false regressions and false gains from names.

## Do not repeat

- Do not make the camera-control query private until its natural callers are
  present; do not retain it with a fake reference.
- Do not replace `0.63661975f` with a higher-precision expression; the January
  target relocation records the single-precision value used here.
- Do not retry volatile/register qualifiers, barriers, pragmas, forced
  inlining, raw overlays, fake dependencies, or assembly to close either parked
  scheduling residual.
- Do not count the two complete fuzzy bodies as exact bytes or declare
  `player_control.obj` complete while the five functions above remain unwritten.
