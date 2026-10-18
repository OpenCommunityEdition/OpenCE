# Legacy ARB vertex-program feasibility

An isolated probe translated retail vertex shader table entries 31 and 50
into `ARBvp1.0`, ran them beside a GLSL fragment shader in a hidden CGL
context, and compared each component of all eight `vec4` varyings separately.
It used the normal fixture plus one bounded case with nonidentity matrix rows
and varied per-vertex inputs. This establishes a bounded arithmetic and
varying-transport path on the tested inputs; it is not an engine integration
or a general NV2A shader translator.

The probe passed 773 checks on the MacBook Intel host, PowerBook G4, and iBook
G4. Every compared component matched the GLSL reference exactly. The probe
used six gathered constants in attributes 10–15, eight varying slots in
`result.texcoord[0..7]`, and transported fog x through primary-color x. The
G4 reported native ARB vertex processing for the candidate programs. Resource
counts were:

| Host | Shader 31 | Shader 50 |
| --- | --- | --- |
| Intel MacBook | 238 instructions, 26 parameters, 30 temporaries | 210 instructions, 23 parameters, 30 temporaries |
| PowerBook G4 | 122 instructions, 43 parameters, 14 temporaries | 104 instructions, 42 parameters, 14 temporaries |
| iBook G4 | 122 instructions, 43 parameters, 14 temporaries | 104 instructions, 42 parameters, 14 temporaries |

Both programs were under the host-reported native limits. The G4s reported
GPU vertex processing for every ARB draw. The standalone mixed ARB vertex / GLSL
fragment program linked and drew successfully. This does not establish that
the engine’s shader cache, state owner, or fragment-source inputs can use that
combination.

The RCC helper is exercised through the same emitter used by the probe
translator. Positive, negative, positive-zero, and negative-near-zero inputs
matched expected clamped values. For signed negative zero, Intel’s GLSL and
ARB paths both produced zero in the mapped output; both G4s’ GLSL and ARB paths
both produced positive saturation. The candidate therefore matches the
same-host GLSL behavior for that case, while the host results differ.

Evidence is preserved in ignored `build/mac-audit/arb-retail-probe/`:

| Artifact | SHA-256 |
| --- | --- |
| `tools/mac_arb_retail_probe.c` | `64f77b62f1e3dbc8fbbe85ebcee6f78f5ee33dc3f3baef3f700ac75fd0170497` |
| `tools/mac_arb_retail_checks.sh` | `955f7395e825e0d1a6a1730076b91b29e7864fa77ff97bb73da303e601360874` |
| PPC probe binary | `56516bbc423a9f62d57c8c3310fff4d6dafd5c64d00c4f6e4cc972ecc41c59ac` |
| i386 log | `a9c171a63e0ff5137e491381891ab5bad52d0504a29bba05da2cf0452e19bfae` |
| PowerBook log | `e8a6ed56bc58427300817b61805b5307c5b32a7596fa2a70dbabfb91147b9c9e` |
| iBook log | `e8a6ed56bc58427300817b61805b5307c5b32a7596fa2a70dbabfb91147b9c9e` |

The test source deliberately accepts only the two selected table entries in
its caller, but its internal decoder is not yet a production-safe recognizer.
It substitutes relative reads in encounter order and checks their offsets;
it does not independently prove both address-register dataflows, their exact
three-read groups, or absence of additional address-register uses. Production
use must gate on exact shader identity and code fingerprint, or add a strict
raw-bytecode recognizer correlated with the existing bounded bone-gather plan.

The clip-position path currently has fixture constants baked into its ARB
program. It must instead read the live viewport scale, viewport offset, and
screen offset, and implement the shared zero-scale fallback. The shared
reference is in `port/linux/src/nv2a_vsh.c`: it reconstructs screen-to-clip,
uses the captured clip position when available, remaps depth for Mac GL, and
replaces positions whose `w` is zero or NaN with `(0, 0, 0, -1)`. The probe
does none of that generally.

For a finite reconstructed position, an ARB `SGE` validity flag followed by
`MAD` can select `(0, 0, 0, -1)` when `w == 0`; the existing ARB subset already
uses the necessary arithmetic and comparison operations. This is only a
finite-input construction. If any selected position component is NaN,
arithmetic selection still evaluates products such as `0 * NaN`, which stay
NaN. The reference guard also catches NaN `w`, so an arithmetic mask is not a
general equivalent. This investigation has not established an ARBvp1.0
conditional select with equivalent behavior. Zero/NaN clip handling therefore
remains a production blocker; do not infer safety from the finite fixture.

The current clip translation also needs live viewport uniforms, the shared
zero-scale fallback, the Mac depth remap, and the reference no-captured-clip
path before it can serve all relevant shaders.

The probe transports D0, D1, B0, B1, and T0–T3 as eight texture-coordinate
varyings, then fog x through primary-color x. The G4’s 40-float varying limit
accommodates those 33 floats. Per-pixel-lighting outputs add world-position
and world-normal varyings and were not tested. The engine’s generated fragment
source currently expects named varyings; adapting that source to the ARB
built-in transport, preserving output clamps and fog semantics, remains
unimplemented. Unknown shaders and unsupported instructions must continue to
fall back to the existing path.
