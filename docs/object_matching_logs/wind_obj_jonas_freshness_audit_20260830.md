# `wind.obj` freshness audit (2026-08-30)

## Outcome

`source/scenario/wind.obj` remains `7/9` strict exact.  A machine-wide source
and COFF census found no admissible artifact that closes both residuals.  No
production source, header, configuration, ownership, park, or comparison
policy changed.

| Function | Target / current padded bytes | Relocations | Boundary |
| --- | ---: | ---: | --- |
| `_code_0017f750` | `224 / 208` | `8 / 8` | lawful VC7 `fabs` versus January full-width sign-bit clearing |
| `_code_0017fbd0` | `384 / unwritten` | `9 / 0` | best rejected candidate is `384 / 384`, `9 / 9`, but has one surplus spline-phase stack lifetime |

All seven inherited exact functions and all 100 target-owned data bytes remain
exact.

## Machine-wide census

The census covered all 1,575 registered worktrees, reachable source history,
normal and ignored build artifacts, workspace `scratch`/`outputs`, and the
authenticated Punpckhdq, Stian, Pastudan, HCEA, Claude, and Fable material.
It inspected 17,956 candidate paths, 17,545 distinct raw COFF objects, and 255
objects carrying wind symbols.

Aside from the immutable January target, there is exactly one residual exact
artifact: `_code_0017f750` in
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\wind-atomic-20260812\build\base\source\scenario\wind.obj`.
That historical experiment obtains exact code by reading an inactive union
member to reinterpret a float, which is prohibited representation punning.
Its source was correctly reverted and cannot be admitted.

No artifact closes `_code_0017fbd0`.  The strongest preserved lawful candidate
is
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\units-integration-20260824\build\audit\wind_seed_pointer_candidate_20260826.obj`,
raw SHA-256
`0162b29c40f77c6366058258355e3f1444f5508de1cb9e70406672e5637c4477`.
It reaches `384/384`, all nine relocation identities, and the exact January
seed phase.  Its frame is still `0x3C` with the seed counter at `-0x14`; the
target uses `0x38` and `-0x10` because its spline phase colours one fewer
dword lifetime.

## Fixed boundaries and handoff

For `_code_0017f750`, the retained unsigned-byte sample-key representation and
axis-countdown topology are the lawful fixed point.  The target clears the
float sign bit through full-width integer code; XDK VC7 canonicalizes every
tested lawful `fabs` spelling to x87 `fabs`.  Reopen only with original-source
provenance for a defined typed-C full-width sign-clear representation.

For `_code_0017fbd0`, retain the Wave-2 seed topology: one loop-carried
`real_vector3d *seed_result`, one phase-scoped axis pointer, eight/three
down-counters, `+0x60` per keyframe, and `+0x300` per axis.  The next admissible
lead must explain the target's smaller spline value graph and phase-local stack
overlap.  Do not repeat the explicit phase union or the Wave-3
one-local-per-target-slot graph.

Primary evidence:

- `docs/object_matching_logs/wind_obj_jonas_partial.md`
- `docs/object_matching_logs/wind_obj_jonas_phase_union_rejection_20260826.md`
- `docs/object_matching_logs/wind_obj_jonas_axis_countdown_20260827.md`
- `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\outputs\wind_closeout_evidence_20260826\january-target-wind.obj`

