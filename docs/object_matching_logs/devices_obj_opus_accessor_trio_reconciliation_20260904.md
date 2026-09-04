# `devices.obj` accessor-trio Opus reconciliation (2026-09-04)

## Scope

After the HS runtime owner-header reconciliation landed, this packet completes
the three device leaves deliberately deferred from the first devices packet:

- `_device_get_position`: 48 padded bytes
- `_device_get_power`: 48 padded bytes
- `_device_operates_automatically_set`: 64 padded bytes

The bodies originate in the frozen Opus donor
`opus/small-families-30k-20260902`, commit `8aed00b49`, and were selectively
reconstructed rather than cherry-picked. Together they contribute 160 strict
padded bytes and 141 meaningful code bytes.

## Source and declaration review

Both accessors use `device_get`, engine `real`, the typed device datum fields,
and the natural `NONE` fallback. The machine setter uses
`machine_try_and_get` and the shared `SET_FLAG` macro with the named
`_machine_does_not_operate_automatically_bit`; no manual mask arithmetic is
needed.

All seven device APIs consumed by `hs.c` now have declarations in their owning
`devices/devices.h`. The duplicate caller-local prototypes were removed and
`hs.c` includes the owner header. A full transitive rebuild covered devices,
device controls, machines, light fixtures, objects, players, HUD, and HS; the
rename-stable sweep found no definition-position or code-allocation loss.

The retained source contains no address-derived identifier, raw offset,
inline assembly, forced inline, volatile/register steering, representation
pun, fake dependency, or nonsensical matching-only logic.

## Verification

- `devices.obj` gate: 19 exact, 0 residual, 11 unwritten (previously
  16/0/14; 14/0/16 before the two devices packets).
- `hs.obj` gate: 445 exact, 3 residual, 0 unwritten; its accepted set is
  unchanged by replacing local declarations with the owner header.
- Rename-stable whole-tree sweep: +3 exact functions / +160 padded bytes,
  zero regressions across 8,245 target functions.
- Full Ninja build and semantic report: success; 5,932 semantic exact, 5,949
  accepted exact, zero unit errors.
- Progress report: 843,583 / 2,198,102 matched code bytes and 5,880 / 11,060
  matched functions; data remains 2,001,721 matched bytes and 391/833 objects
  are linked.
- Fake-match scan: zero review leads in the changed source/header surface.
- Park audit: 188 active, 0 stale, 0 invalid.
- Object-admission audit: 0 candidates, 0 contradictions, 0 revocations; the
  two inherited explicit rejections are unchanged.
- Tool tests: 261 passed.
- Rebuilt and target device/HS objects emit no `point_from_line3d` symbol.
- Protected bitmap source hashes remain unchanged.

Stable snapshots:

- before: `scratch/after-hs-runtime-opus-integration-20260904.json` (5,920 exact)
- after: `scratch/after-devices-accessor-trio-20260904.json` (5,923 exact)
