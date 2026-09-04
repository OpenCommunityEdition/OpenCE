# `devices.obj` touch/group-delete Opus reconciliation (2026-09-04)

## Scope

This packet selectively reconciles two small exact functions from the frozen
Opus donor `opus/small-families-30k-20260902`, commit `8aed00b49`:

- `_device_touched`: 80 padded bytes
- `_device_group_delete`: 64 padded bytes

Together they contribute 144 strict padded bytes and 116 meaningful code
bytes. The donor commit was not cherry-picked. The donor's three HS-facing
device accessors remain deferred so their declarations can be reconciled with
the active HS owner-header work rather than preserving caller-local prototypes.

## Name, linkage, and contract review

The later HCEA reconstruction independently supplies the semantic
`device_group_delete` identity and behavior. The frozen Opus name-gap report
maps that function uniquely onto January's 64-byte, four-relocation
`_code_00084f30` target section. The canonical symbol map now records the
semantic name and target-private linkage, and the source definition and
prototype are both genuinely file-static in the owning translation unit.

`device_touched` uses the existing typed `device_get` accessor and the correct
object-type enum constants to dispatch machine and control interactions. Its
public declaration already belongs to `devices/devices.h`. Auditing the called
control function exposed an older one-parameter declaration error: January
passes both the control and unit indices, as independently corroborated by the
HCEA reconstruction, although the callee does not consume the latter. The
definition and its declaration in `device_controls.h` now both carry the real
two-parameter contract. The only two direct includers of that header were
rebuilt, and the whole-tree sweep found no definition-position regression.

`device_delete` now calls the natural private helper for its power and
position groups. VC7 preserves the already-exact January body while emitting
the separately exact private owner. The retained source has no address-derived
identifier, raw offset, forced inline, inline assembly, volatile/register
steering, representation pun, fake dependency, or nonsensical matching-only
logic.

## Verification

- `devices.obj` gate: 16 exact, 0 residual, 14 unwritten (previously
  14/0/16); both new functions are exact at 80/80 and 64/64 padded bytes.
- `device_controls.obj` gate: all 11 functions remain exact after correcting
  the public contract.
- Rename-stable whole-tree sweep: +2 exact functions / +144 padded bytes,
  zero regressions across 8,245 target functions.
- Full Ninja build and semantic report: success; 5,904 semantic exact, 5,921
  accepted exact, zero unit errors.
- Progress report: 837,283 / 2,198,102 matched code bytes and 5,853 / 11,060
  matched functions; data remains 2,001,713 matched bytes and 391/833 objects
  are linked.
- Fake-match scan: zero review leads in all three changed source/header files.
- Park audit: 185 active, 0 stale, 0 invalid.
- Object-admission audit: 0 candidates, 0 contradictions; the two inherited
  explicit rejections are unchanged.
- Tool tests: 261 passed.
- Candidate and target objects emit no `point_from_line3d` symbol or COMDAT.
- The protected bitmap files retain their pre-packet SHA-256 identities.

Stable snapshots:

- before: `scratch/after-network-server-pregame-pair-20260904.json` (5,893 exact)
- after: `scratch/after-devices-touch-group-delete-20260904.json` (5,895 exact)
