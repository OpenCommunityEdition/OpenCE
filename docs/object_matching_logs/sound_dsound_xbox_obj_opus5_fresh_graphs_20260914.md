# `sound_dsound_xbox.obj` Opus 5 fresh-graphs wave 2 (2026-09-14)

## Scope and provenance

- Translation unit `source/sound/sound_dsound_xbox.c`, target
  `build/split/source/sound/sound_dsound_xbox.obj` (January 2002 Xbox debug
  build).
- Compiler: XDK 3911 `CL.Exe` 13.00.9254, unit flags `/O2 /Oy- /DDEBUG /Dxbox`
  with the repository include graph (unchanged).
- Lane `opus/30k-fresh-graphs-20260914`, worker scope: this one `.c` file only.
  No header, configuration, park or tooling edit.
- Evidence read before any experiment: all seven previous
  `sound_dsound_xbox_obj_*` ledgers, `git log --all` for the file (e481d5142,
  0c18c70e0, 608c66380, b654ee4f9, b8c442f9d, f54ec267f, ecd0fff2d,
  e830efbd7), the Ghidra decompilation, the January map atlas, Stian
  `sound_dsound_xbox.c:790-890` and the Pastudan lift (naked assembly only).

## Validated baseline

Gate at lane HEAD (point guard passed): **35 exact / 4 residual / 0 unwritten**
of 39. No parked entries for this unit. The final real-file gate is identical;
the source file is unmodified (`git diff --stat` empty).

| Function | Target size / relocs / nsha16 | Base size / relocs / nsha16 | First divergence |
| --- | --- | --- | --- |
| `_dsound_set_listener_properties` | 816 / 63 / `2c07c059201ef12a` | 784 / 63 / `39384b94a33e124c` | +0x5b: swapped coordinate args pushed through the FPU |
| `_channel_set_location` | 752 / 43 / `b509812c41541745` | 720 / 43 / `88e79a8ebcc18a51` | +0xee: same FPU-argument family |
| `_dsound_initialize` | 720 / 52 / `d73c7ae0918c2fc4` | 720 / 52 / `11e94e43be496945` | +0x29f: cross-jump survivor among four identical failure arms |
| `_channel_queue_packet` | 640 / 37 / `eedc03a5428064c8` | 640 / 37 / `b864c6f9b8f3ea40` | +0x2c: ebx/edi colour swap; +0xe9: zero-register pin |

## Accepted controls

None added. No experiment produced a strict-exact function with an authentic
spelling, so nothing was retained.

## Experiment matrix (all in scratch copies; all reverted)

### FPU swapped-coordinate arguments (`_dsound_set_listener_properties`, `_channel_set_location`)

January passes each DirectSound vector as `(x, z, y)`. The first coordinate is
an integer copy (`mov reg,[src]; push reg`); the two swapped coordinates are
evaluated on the FPU at push time
(`fld [src+4]; ...; sub esp,8; fstp [esp+4]; fld [src+8]; fstp [esp]`). The
current source integer-copies all three.

| Shape | Result |
| --- | --- |
| Mechanism probe `p1`: only the swapped args wrapped in a comma expression | listener **816 / 63 / `2c07c059201ef12a` = target** (gate 36/3). Not source: a fake-match generator |
| Same probe on all three coordinates in the full function (`p2`) | 800; the first coordinate goes FPU as well |
| `(real)` / `(double)` / `(real)(double)` casts, `n[]` index, `+P`, `P+0.f`, `P*1.0`, pointer to the point, `D3DVAL` | integer copy (folded) |
| `real` block locals (`e3`, `e4`, `e5`) | 848 / 800 / 800: the locals go into integer registers with frame homes as soon as the HRESULT is used |
| `double` locals (`e6`, `e7`) | 800: FPU, but pre-evaluated order (`fld,fld` or `fxch`) |
| `set_real_point3d` into a local point (`p3`) | 800, frame 0x3c |
| Uniform FP arithmetic on all three coordinates of the position call only (`cm4` `(real)(P*1.0f)`, `cm5` `(P)*1.f`, `cm6` `(real)(double)(P)`) | position block byte-identical to January; only the untouched orientation and velocity calls remain (800) |

In `_channel_set_location`, the comma probe (`p4`) brings the size to 752. Two
tail differences remain. Store order occlusion, obstruction, attenuate (`p5`,
752 / 43 / `7cdcc4bcb27bfd98`) fixes the final stores. After that, only the
obstruction compare is left: January emits `fld field; fsubr param`. Neither
`fabs(channel->obstruction-obstruction)` (`p6`) nor
`!(fabs(...)<0.001f)` (`p7`) reproduces it.

### `_dsound_initialize` failure tail

| Shape | Result |
| --- | --- |
| `i1` failure-first nesting | 784 / 58 relocs, arms inlined (worse) |
| `i2` one shared `if (!success) dsound_dispose();` | byte-identical to baseline (normalised) |
| `i3` uninitialised `success`, assigned FALSE per arm | loses January's `xor bl,bl` zero reuse (worse) |

### `_channel_queue_packet`

| Shape | Result |
| --- | --- |
| `q1` then-arm order `playing= queued; packet.dwMaxSize= remaining_size; queued= NULL; sample_offset= 0;` | zero pin gone, 221/221 instructions, nsha `0567b7598b4b3bc9`; only the ebx/edi colour swap remains (8 instructions) |
| `r1` q1 + `remaining_size` declared before `packet` | inert |
| `r2` q1 + `result`/`packet` declared first | inert |
| `r3` q1 + `sound` hoisted to function scope | inert |

The sibling regression check was clean for every shape: the 35 exact rows
stayed exact in every variant gate.

## Do-not-repeat list

- Any cast-only or constant-folding spelling of the swapped coordinates, and
  `real`, `double` or point locals for them. All are measured above.
- Comma or identity arithmetic (`*1.f`, `(real)(double)`) as landed source.
  These are proven mechanism probes, but they are nonsense without an original
  macro or constant.
- Failure-first nesting, a shared dispose, or per-arm `success` stores in
  `_dsound_initialize`.
- Declaration order and `sound` scope for the `_channel_queue_packet` colour
  swap.
- Prohibited throughout: volatile, register, pragmas, inline assembly, and
  header or flag changes.

## Residual classification

- **Listener and channel location: source/topology, unresolved spelling.**
  - Measured: an FPU-typed expression on every coordinate, folded after VC7
    has chosen how to pass the argument, reproduces January exactly (probe sha
    equals target).
  - Inference: January applied a per-coordinate conversion, such as a units or
    handedness macro or `real` arithmetic with a unit constant. It is not in
    the lane tree, HCEA, Stian, Pastudan or the XDK headers.
- **`_dsound_initialize`: scheduling/layout.** The cross-jump survivor among
  identical tails; three structural shapes are exhausted.
- **`_channel_queue_packet`: register colour tie.** Only this remains after the
  structural `q1` fix; the colour budget is spent.

## Reopen criteria

1. Find the original coordinate-conversion macro or constant, or other
   January-era evidence of per-coordinate FP arithmetic. Then apply it
   uniformly to all DirectSound vector arguments in both functions.
   - Expected: `_dsound_set_listener_properties` becomes exact (probe `p1`
     already equals the target).
   - For `_channel_set_location`, also apply the `p5` store order and find an
     obstruction compare that evaluates the channel field first.
2. `_channel_queue_packet`: take `q1` as the starting point. Reopen only with a
   use-weight lever that moves `sound` to ebx and the cache-base/sample-offset
   value to edi.
3. `_dsound_initialize`: reopen only with evidence that the four failure arms
   differed structurally in source.

Probe and variant scripts: `scratch/workers/sound_dsound_xbox/` (`variant.py`,
`p1.py`, `p5.py`, `q1.py`, `exp*.c`).

## Disposition

Still active, `NonMatching`: 35/39 exact, unchanged. No new exact credit, no
park, no fuzzy landing. Nothing was unwritten.
