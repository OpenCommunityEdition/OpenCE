# HUD, devices and input: next integration batch — 2026-09-04

Starting publication baseline:
`4795da4089557f919fd9877b3c0a3b5f1e648931` on both bnunu/halo branches
`jonas/exact-pilots` and `jonas/ai-debug-integration-20260829`.

The baseline credits 895,918 meaningful code bytes, 6,129 functions,
2,021,802 data bytes, and 392/833 configured Matching objects. Its stable
snapshot contains 8,245 owners, 6,176 strict exact. These are distinct
accounting scopes; ordinary fuzzy percentages and padded sizes are not
interchangeable with meaningful canonical credit.

## Integration work

The two frozen device functions have their real owner declarations in
`source/devices/devices.h`; Object Types includes that header and no longer
locally redeclares the preprocess callback. The complete source/ABI audit is
`devices_obj_desired_value_animation_wave_20260904.md`. Both remain exact
after the API change: 640 padded / 623 meaningful new bytes, with all 20
baseline exact functions preserved.

The previously unwritten `device_update` is reconstructed in natural typed C.
The candidate has the January 736-byte padded extent, 242 instructions,
0x18-byte frame and all 19 ordered relocations. Two initial floating
predicates were corrected to preserve January's unordered-value behavior;
the remaining differences are stop-effect argument register choices. This
body is non-exact, not credited code. Its proper public declaration is also
in `devices.h`, replacing Object Types' caller-local prototype.

The missing `hud_globals` and `hud_scripted_globals` storage is restored in
its genuine owner, `source/interface/hud.c`: two public NULL-initialized
pointers at offsets 0 and 4 of the January eight-byte BSS section. All 19
exact functions and three existing parks are unchanged. Existing candidate
math helper owners are inherited, not introduced or newly approved here.
HUD remains incomplete; the storage fix does not resolve its legacy partial
cross-TU type views or its function/extra-owner admission debts.

Two existing APIs required by the new callers now have correct owner-header
declarations: `hud_globals_get_scale(boolean)` in `hud_draw.h`, and
`input_abstraction_update_device_changes(unsigned long)` in
`input_abstraction.h`. Neither uses a caller-local guessed prototype.

HUD damage indicators add a third strict function: 688 padded / 685 meaningful
bytes and 39 ordered relocations. The function remains exact on the root's
independent gate, and a second reviewer independently verifies all nine
PDB-backed prefix types, the edge enum, coordinate conversions and actual
callee APIs. No new helper or writable storage is emitted. All 18 inherited
HUD Unit exacts survive. The new assertion literal is compiler-pooled output
required by January's own relocation, not separately claimed data progress.
See `hud_unit_obj_damage_indicators_exact_20260904.md`.

The input device/keyboard poller cluster adds two strict public callers,
`input_update` (83 meaningful bytes) and `input_frame_begin` (13), with real
same-TU private callees. The three private implementations are retained as
measured, unclassified fuzzy parks, not exact credit. The genuine private
`input_globals` restores 1,036 bytes of BSS; four semantic lookup arrays own
the exact natural 784-byte read-only contribution. The latter is uncredited
until the full unmatched read-only span is restored under existing policy.
See `input_xbox_obj_device_keyboard_pollers_wave_20260904.md` and the separate
global-ownership audit. The two inherited anonymous empty functions now have
honest descriptive names, with unresolved original linkage/arity explicitly
documented rather than invented public APIs.

The five verified closures total 1,440 padded / 1,404 meaningful bytes.

## Focused checks

`scratch/hud_device_api_blast_20260904.py` independently compiles 31 relevant
units to scratch objects. It preserves all 1,347 inherited exact owners and
every inherited park in those units, with zero compile errors. This includes
Units' 189 exact functions and Object Types' 31 exact functions. The unchanged
Units surplus-helper whole-object veto is not waived.

The diagnostic outputs are in `scratch/hud-device-api-blast-20260904/`, with
the complete unit list and per-unit results in `report.json`. This focused
check does not replace the final frozen whole-tree build and stable sweep.

The tool suite passes 284 tests; the only warning is the existing sandbox
denial for pytest's optional cache. No tool, compiler flag or comparator has
been changed.

## Frozen whole-tree validation

- Full Ninja build and production semantic progress pass.
- Meaningful code: **895,918 -> 897,322** (+1,404), functions **6,129 -> 6,134**.
- Data: **2,021,802 -> 2,022,850** (+1,048): 1,036 input BSS, eight HUD BSS,
  and the independently verified four-byte HUD default-string pointer.
- Stable sweep: **6,176 -> 6,181 exact**, all 8,245 target owners inspected,
  **zero inherited exact regressions**, including Units' 189 exact functions.
- **392/833 Matching objects**, unchanged. This batch restores implementation
  and data within incomplete objects; it does not claim a whole-object closure.
- Tool tests: **284 passed**, existing optional pytest-cache permission warning.
- Parks: **249 active**, zero stale/invalid; four new honest residual records.
- Admission: zero new candidates, contradictions or revocations; three
  inherited ownership rejections remain unchanged.
- All eight changed C/header files pass the fake-match review-lead scan.
- Complete object reviews and emitted-symbol guards find no new orphan code
  or point_from_line3d. Existing HUD/Units ownership debts are not waived.
- Protected bitmap source/header hashes remain unchanged; no matrix math,
  vehicles or AI Debug implementation changes are included.
- `git diff --check` passes. Pre-existing timestamp/line-ending-only dirt in
  Decals header, Game header and Files source is not part of the batch.

The frozen sweep files are
`scratch/hud-device-input-next-before-20260904.json` and
`scratch/hud-device-input-next-after-20260904.json`; final snapshot SHA-256:
`006d1ddfd4633bd59f152da22f37f6e7cd00624c989cdeedc28701f64fcf2aba`.
Park/admission reports use the same batch prefix in `scratch/`.

Final source SHA-256 values:

| Source | SHA-256 |
| --- | --- |
| `source/devices/devices.c` | `f9e350bc2ee09c291be7ccb62c80f5b38f3f6eed0344e7865cc289d5b6f96b87` |
| `source/input/input_xbox.c` | `7e2c3727dfd71c1c2a5ed0a9495426c9b6a70996bf2342c67471bb92f33a7cef` |
| `source/interface/hud_unit.c` | `8566b10e06a57443e7887e30ab6a4db00f40f7f716cb025d87dc383191d92496` |
| `source/interface/hud.c` | `b68ad8c2ecebd98c7b993e6413aaf3dc0ad2c8803e1f06d447295217b4959eb7` |

## Publication boundary

The final independent input review approves the reconstructed packet, not
whole-object Matching admission. It independently checks the SDK ABIs,
signed status test, hotplug masks, controller loops, keyboard queue logic,
natural helper emission and exact table/BSS owners. The unassigned dword at
`input_globals +0x230` and the empty-function provenance remain explicit
follow-up debts; neither is given invented semantics or extra credit.

Publication follows explicit-file staging of this reviewed frozen packet.
Destinations are only the two bnunu/halo publication refs named above, using
the `jonas` remote without force. The committed ledger records verified work;
remote publication is separately confirmed with `git ls-remote`.
