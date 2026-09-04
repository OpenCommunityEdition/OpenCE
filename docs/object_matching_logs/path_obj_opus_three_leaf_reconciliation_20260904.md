# `path.obj` three-leaf Opus reconciliation (2026-09-04)

## Scope

This packet selectively reconciles three small public path API leaves from the
frozen Opus donor `opus/small-families-30k-20260902`, commit `8aed00b49`:

- `_path_input_set_attractor`: 64 padded bytes
- `_path_input_set_search_bounds`: 32 padded bytes
- `_path_get_node`: 128 padded bytes

Together they contribute 224 strict padded bytes and 187 meaningful code
bytes.  The donor commit was not cherry-picked.

## Source-quality review

The setters use the existing typed `path_input` fields and engine `real` and
point types.  The attractor point uses a natural aggregate assignment, which
accounts for January's copy schedule without manual word copies or raw
offsets.  `path_get_node` preserves January's two source-line assertions and
returns the typed array element.

All three declarations were already correctly owned by `ai/path.h`, so no
header edit or caller-local prototype was needed.  The donor's additional
unused static `path_state_reset` body was deliberately excluded: it did not
contribute to the proved three-function gain and would have emitted an
unneeded, unauthenticated candidate section.

The retained source contains no address-derived identifier, inline assembly,
forced inline, volatile/register steering, representation pun, fake
dependency, or nonsensical matching-only logic.

## Verification

- Focused gate: all three functions exact (64/64, 32/32, and 128/128).
- Object gate: 13 exact, 0 residual, 19 unwritten (previously 10/0/22).
- Rename-stable whole-tree sweep: +3 exact functions / +224 padded bytes,
  zero regressions across 8,245 target functions.
- Full Ninja build and semantic report: success; 5,898 semantic exact, 5,915
  accepted exact, zero unit errors.
- Progress report: 836,896 / 2,198,102 matched code bytes and 5,847 / 11,060
  matched functions.
- Fake-match scan: zero review leads in the changed source.
- Park audit: 185 active, 0 stale, 0 invalid.
- Object-admission audit: 0 candidates, 0 contradictions; inherited explicit
  rejections unchanged.
- Tool tests: 261 passed.
- The January inline schedule remains intact and the packet emits no
  `point_from_line3d` COMDAT.

Stable snapshots:

- before: `scratch/after-sound-dsound-xbox-20260903.json` (5,886 exact)
- after: `scratch/after-path-input-node-leaves-20260903.json` (5,889 exact)
