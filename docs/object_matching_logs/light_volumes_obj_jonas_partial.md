# light_volumes.obj Jonas partial donor reproof

## Scope

- Base: `dd371124269b80d25227a42501ae5ef883f618e0`.
- One donor compile only, using the source foundation from `dca60279`.
- The frozen five objects and all existing Markdown logs remained untouched.
- No assembly, volatile scheduling, force-inlining, pragmas, barriers, undefined
  behavior, raw-address access, inactive-union punning, byte forcing, or
  synthetic anchors were used.

## Independent evidence

- The January target disassembly and relocations remain authoritative.
- HCEA independently identifies the private helper as `pow1(float, float)`,
  confirms the submit control flow, and confirms the original same-frame
  interpolation quirk rather than a second-frame read.
- Stian's tag reference confirms the attachment marker, fade distances,
  brightness source, bitmap/sequence/count, animation source, and two-frame
  maximum used by the recovered typed layouts.
- The local Demon checkout contains no light-volume implementation or stronger
  source donor.

## Single compile result

- `_light_volume_submit`: strict exact, 272 padded bytes, 14 relocations,
  normalized SHA-256
  `ced120cb1f793f42de38009d866f577f537df3d994778c8426b117cf1c3af2b7`.
  This contributes 269 meaningful code bytes.
- All seven baseline exact functions remained strict exact in that compile.
- `_bss_00456d90`: exact external `.bss` owner, 180 bytes, zero relocations,
  normalized SHA-256
  `10b2a66888c58a54b277fe2e68fb6e87150c3cd2c537b7f6a2d84559017438c7`.
- The three target-owned strings also remained exact: 39, 47, and 14
  meaningful bytes. Together with section padding and the BSS, all 284 target
  non-code bytes are recovered.
- `_code_00124490` was 528/528 with all 27 relocation identities, but making it
  externally reachable so it survived without the renderer changed its byte
  topology. It was nonexact and was removed after the one permitted compile.
- The historical renderer is known nonexact (928/912 padded bytes in the best
  faithful source shape) and was never retained in this lane.

## Retained result

The final source retains only the independently exact submit body, the typed
tag/data accessors and layouts required by it, and the exact BSS owner. The
object advances from 7/10 to 8/10 strict functions and from 104/284 to 284/284
target non-code bytes after the integration rebuild. The two absent bodies
remain `_code_00124490` and `_light_volume_render`.
