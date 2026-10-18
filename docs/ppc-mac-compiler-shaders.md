# Compiler, ABI and GLSL 1.20 results

Follow-up: the native scripting defect below has now been patched and verified
with 318 checks on both G4s and Intel. See [ppc-mac-scripting.md](ppc-mac-scripting.md).
The failure results here document the pre-fix baseline, not the current status.

2026-10-15. Built on `intel` using Apple GCC 4.0, `-arch ppc`, the 10.4u SDK
and deployment target 10.4. Executed on `powerbook` and `ibook`, both reporting
modified Leopard 10.5.9. These tests do not yet constitute a game build.

## Passing checks

| Check | PowerBook G4 | iBook G4 |
| --- | --- | --- |
| Engine-header layout and anonymous union access | pass | pass |
| Correctly typed aggregate return across two translation units | pass | pass |
| GLSL 1.20 vertex/fragment compile and link | pass | pass |
| RGBA texture upload, shader sampling and framebuffer readback | 17,83,201,255 | 17,83,201,255 |
| Dynamic indexing of 192 vec4 vertex constants | pass | pass |
| Nonzero final constant moves triangle offscreen | pass | pass |
| GL error after framebuffer/depth/blend/shader tests | zero | zero |

Actual engine types from cseries, real_math and tag_groups headers measured:
pointer=4, long=4, short-wchar=2, real_vector3d=12, real_matrix4x3=52,
tag_block=12, tag_data=20, tag_reference=16 bytes. Checked pointer offsets:
tag_block.address=4, tag_data.address=12. A char followed by double has the
double at offset four under the tested PPC ABI. This is a small selection,
not a complete cache or engine ABI certification.

Both drivers report 4096 vertex uniform components, 4096 fragment uniform
components and 16 fragment texture units. The test keeps all 192 vertex
constants live through dynamic indexing. It first reads back the exact texture
color, then changes bank[191] to move the triangle out of view and checks black.
This validates actual uniform use, not merely successful shader compilation.
The shader is deliberately simple; it does not validate full NV2A instructions,
complexity limits or generated OpenCE shaders. CGL requests an accelerated
context, but shader-specific software fallback and throughput were not measured.

`source/math/real_math.c` compiles into a PPC Mach-O object at -O2 using the
diagnostic prefix. The emitted assembly has zero fmadd/fmsub/fnmadd/fnmsub
instructions with `-mno-fused-madd`. This checks that unit only; it is not a
floating-point determinism test. It has not been linked into a running engine.

## Compiler work resolved and still outstanding

Apple's stdlib declares `random`, which conflicts with the game's declaration.
`__STRICT_ANSI__` alone did not hide it in the tested SDK. The diagnostic prefix
loads libc headers before mapping the engine name to `halo_probe_game_random`.
The prefix also includes OpenCE's existing float compatibility declarations,
maps __int64, and suppresses an unused extern TLS qualifier for these probes.
This is not a production platform prefix: real thread-local state and PPC
floating-point control implementations remain necessary.

GCC 4.0 accepts the tested anonymous structs/unions with -fms-extensions.
Do not globally redefine __inline before Darwin headers: this previously
produced duplicate-static errors. The ABI fixture changes it only after loading
system headers; the real math compile leaves it alone. Multiple engine units
still need a real inline-definition/linkage strategy. Existing header warnings
and the SDK crt1.o obsolete long-branch warning remain in the raw build log.

## Correction to the initial ABI audit

The current hs_runtime.c converter declarations and definitions return `long`.
The claim that its conversion table calls union-returning functions was wrong,
based on an outdated build-script explanation rather than those definitions.
Do not add wrappers for a nonexistent current mismatch.

PPC aggregate-return behavior itself was verified separately: the tiny external
callee stores r4 through the hidden output address in r3 (`stw r4,0(r3)`).
The correctly typed caller succeeds. This generic ABI result is not evidence
of a defect in the current scripting conversion table.

## Reproduced scripting endian blocker

`tools/mac_script_probe.py` extracts the real conversion definitions from
hs_runtime.c using the repository's existing harness helper. It does not copy
or reimplement their algorithms. The generated executable composes
hs_long_to_short, hs_short_to_real and hs_real_to_short.

| Input | Intel 32-bit short slot / real result | Both PPC G4s short slot / real result |
| --- | --- | --- |
| -7 | 0000fff9 / -7 (pass) | fff90000 / 0 (FAIL) |
| 0 | 00000000 / 0 (pass) | 00000000 / 0 (pass) |
| 7 | 00000007 / 7 (pass) | 00070000 / 0 (FAIL) |

The PPC diagnostics intentionally return 1 to reproduce this unresolved defect.
They are not passing regression gates. No production scripting code was changed.

The code mixes first-address byte/short storage with numeric low-bit casts:

- hs_runtime.c:1652–1741: scalar converters store through boolean/short pointers.
- hs_runtime.c:1972: equality compares only the first type-sized bytes.
- hs_runtime.c:2019,2109,2548: inequality/if/sleep use numeric casts.
- hs_runtime.c:2058: logical operators read a boolean at the slot start.
- hs_runtime.c:2937,2947 versus 3166,3175: external-global reads and writes
  also mix these representations.
- hs.c result/argument unions and narrow-field argument structs, plus
  hs_compile.c syntax-node narrow writes, require coordinated treatment.

Recommended contract for the eventual fix: canonical numeric low-bit scalar
slots, explicit accessors at all narrow-value boundaries, preserving existing
little-endian zero extension and the source's existing boolean conversion
behavior. Changing only converters is not sufficient. First extend fixtures
to mixed logical/equality/argument/global consumers, then land a coordinated
patch with unchanged Intel behavior and corrected PPC behavior.

## Reproduction and next steps

From the repository root on the MacBook (or a staged tree with those inputs):

```sh
sh tools/mac_compiler_probes.sh
```

This produces the optimized math object/assembly, ABI executable and graphics
executable under build/mac-audit. Run the latter two on each G4. Source staging
used `$PROBES/20261015`; no system installations or desktop windows.

For the unresolved scripting diagnostic, generate on the modern host:

```sh
python3 tools/mac_script_probe.py build/mac-audit/script_probe.c
```

Build that generated file on the Intel build Mac with gcc-4.0, -arch ppc, -O2,
-fno-strict-aliasing, the 10.4u SDK and deployment target 10.4. Build an i386
control with -arch i386. Run PPC on the G4s; expect the documented failure until
the scripting fix is implemented. Do not mistake the nonzero status for an
SSH failure or keep retrying it.

Next priorities: a coordinated scripting-slot test/fix, multi-unit inline/TLS
support, additional map layouts and complete little-endian loading, and a
GLSL 1.20 emission path for actual NV2A shaders. Blood Gulch still cannot load;
no game data or complete platform backend is available in this checkout yet.
