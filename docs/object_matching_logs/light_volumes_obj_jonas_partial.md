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

## Rasterizer widget ABI correction (2026-08-20)

The submit donor originally carried a translation-unit-local four-argument
declaration while the public rasterizer header and forwarding wrapper retained
an obsolete one-argument declaration. The actual HCEA PDB type records in both
`HCEX.pdb` and `HCEX_Release.pdb` identify the public and private functions as
`void (long, long, real_point3d const *, function *)`, with the named parameters
`object_index`, `widget_index`, `centroid`, and `render_proc`. The January
private target independently reads those four stack arguments at `ebp+8`,
`ebp+C`, `ebp+10`, and `ebp+14`; the public target is a 16-byte tail-jump wrapper
to that private function. Any value incidentally left in EAX is therefore not
a declared return value.

The correction adds one shared two-index callback typedef to the rasterizer
API, gives the public and private submit declarations the same four-argument
`void` type, and makes `light_volumes.c` consume that API rather than redeclare
it. The wrapper naturally forwards all four arguments and retains an explicit
void return. No cast, varargs escape, incompatible function pointer, assembly,
volatile shaping, compiler directive, or undefined behavior is used.

Zero-emission `offsetof` assertions now pin every layout field read by the
retained submit source: datum `definition_index` at `0x4`; globals
`light_volumes` at `0xB0`; definition `attachment_marker` at `0x0`,
`far_fade_distance` at `0x38`, `brightness_scale_source` at `0x44`, `count` at
`0x6E`, and `frames` at `0x120`. A composed assertion also proves that
`frames.count` is observed at definition offset `0x120`.

Final strict evidence remains unchanged:

- all eight retained light-volume functions are exact, including
  `_light_volume_submit` at 272 padded bytes, 14 relocations, and normalized
  SHA-256
  `ced120cb1f793f42de38009d866f577f537df3d994778c8426b117cf1c3af2b7`;
- `_rasterizer_widget_submit` is the exact 16-byte, one-relocation tail-jump,
  normalized SHA-256
  `6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51`;
- `light_volumes.obj` remains 8/10 strict with all 284/284 target non-code
  bytes exact;
- the two `light_volumes.h` consumers compile with the offset proofs, and a
  bounded consumer/frozen A/B finds no runtime code, runtime data, relocation,
  or meaningful ownership change. In particular, `vehicles.obj`,
  `ai_debug.obj`, `ai_script.obj`, `actions.obj`, and `units.obj` retain their
  accepted evidence without source or configuration edits. Raw `.debug$S`
  records differ only because the baseline and candidate used distinct
  absolute worktree paths.

The final clean build completed all 569 `halobetacache_build` and `libcmt_build`
edges. Ordinary and semantic progress completed with zero unit errors; the
semantic report accepted 3,844 exact functions. Admission reports zero
candidates and zero revocations, apart from the established unrelated
`shell_xbox` contradiction. Park validation reports three active, zero stale,
and zero invalid entries, and all 179 tooling tests pass. No frozen source,
configuration status, existing Claude Markdown, or Claude log was edited or
deleted.
