# `recorded_animations.obj` closure census and blocker handoff (2026-08-30)

## Outcome

The object remains at 12/16 strict-exact functions and 1,648/2,800 padded
code bytes. No production source was changed. The four existing readable
typed-C residuals remain the only blockers:

| Function | Target/base padded | Target/base relocations | Target normalized SHA-256 | Base normalized SHA-256 |
| --- | ---: | ---: | --- | --- |
| `_code_000839a0` | 640/640 | 38/38 | `df113ef0763a2a945aaf24c999e7976d55b0f998719fff88ba52585d73eb1a7c` | `dfbbcf70766a238d3a819caed656a9c2c93629ad673872d8a636608c5b9dafa2` |
| `_recorded_animation_get_time_left` | 144/144 | 8/8 | `0a2dcb5f8a2e729bd75232fdb280467df53c010183b84c877c2c4cdcdb051ba8` | `50c891f4a4d97d4f0e81027545e3677068a4c0c0f3815a5b319d170f0d009d59` |
| `_recorded_animation_kill` | 96/96 | 4/4 | `e642763b90b04578f40f84ab835fdbf36868dfbb37bacf6907362acc16743c5e` | `3b438ac19d3be738127533fdc00edb7d364a88d9e22644771960307376abc0f8` |
| `_recorded_animation_verify` | 272/272 | 14/14 | `4d24ab3d757d1d9191ab3f6c09120051a67c02136fa6210039f858e5de73cfff` | `4df32a08b3e9b1f7ad1d05c7bef15757d9296a17e3958713d7faf9740f9be9a2` |

The ordinary report measures 1,556/2,684 meaningful code bytes and 848/894
data bytes. All twelve inherited exact owners, their sections and
relocations, and the existing near-exact data remain untouched.

Stable identities at the audited base are:

- source Git blob: `a5572cac7f35d1ccc111fe208e885cb54cc02038`;
- rebuilt object SHA-256: `7D35849AF841CE65C9568A9CF31FB137F972337CA8707F7630F69FE5C1F643AB`;
- January split-object SHA-256: `E21A14FCBBA6F5CCBCAD6DC97E8D195C29DBA6113EF543E18AEBBDDCF3F8A5A9`.

## Exhaustive local census

The census read every registered source copy and every target-named object
under the registered `build`, `scratch`, `research`, and `artifacts` trees
before any compile decision:

- 1,576 registered worktrees, with no missing source copy;
- eight physical source variants, all reducing to the current implementation,
  the earlier partial implementations, or the original skeleton;
- 2,322 `recorded_animations.obj` artifacts, with zero COFF parse failures;
- for each residual, exactly two candidate-origin fingerprints: the January
  target and the current baseline; no third compiled family and no harvestable
  strict candidate.

The one unique noncanonical source copy in `units-wave-20260828` was an older
partial that removes later exact owners. Reachable Git history likewise
contains only the known skeleton, dispose-only, two-owner, and current
implementation stages.

Authenticated HCEA, its full history, Stian's full history, Pastudan's full
history and batch fragments, the Punpck copy, and the registered Claude/Fable
trees were checked read-only. HCEA is a later-build reconstruction. Pastudan's
readable kill and time-left shapes are already represented by the measured
loop/early-return families, while its verify and playback bodies are forbidden
naked-assembly drafts. Stian adds one useful January compiler fact but no new
source topology: the verifier reuses the dead parameter home for
`relative_ticks`.

## Exact residual diagnosis

### Playback worker

The 640-byte `_code_000839a0` implementation already has target size and all
38 relocation identities. A full lockstep walk reports 70 remaining events:
48 structural, 10 candidate-only, eight target-only, and four register events.
The first divergence assigns the long-lived unit handle to ESI instead of the
target's EDI. The target then keeps the animation index in ESI and orders the
initialization/error block before the playback-state stores; the candidate
keeps the opposite register roles and schedules those blocks differently.
This is the large closure blocker. No lawful January source/scope evidence was
found to select a different lifetime topology.

### Verifier

All instructions, frame size, and relocations agree except for two
interchangeable stack homes. January places `playback_stream` at `[ebp-4]`
and reuses the dead argument home `[ebp+8]` for `relative_ticks`; VC7 assigns
those homes in the opposite direction. Stian independently confirms the
January argument-home reuse. The existing declaration/scope experiments have
not supplied an admissible source-level tie-breaker.

### Kill and time-left leaves

The kill target retains a redundant null test after the matching iterator
entry has been found; the current compiler eliminates it. Time-left requires
a shared zero epilogue, a separately aligned match path, and a redundant
post-loop null test. Natural `while`, `for`, break, nested-if, shared-result,
and early-return shapes from the local and authenticated donors were already
measured and do not reproduce those schedules.

## Claude-ready reopen condition

Do not repeat declaration-order, loop spelling, early-return, nested-if, or
block-order tuning. The highest-leverage missing evidence is January-era
local-variable/scope information for `_code_000839a0` (unit handle versus
animation-index lifetime and the allocation/error block) or original source
for that function. For `_recorded_animation_verify`, a CodeView local record
that fixes `playback_stream` versus `relative_ticks` scope would be sufficient.
For the two leaves, reopen only with original source or a distinct
provenance-backed CFG. Any future candidate must preserve the twelve exact
owners and 848 matched data bytes and pass the hardened relocation comparator.

No assembly, `volatile`, `register`, pragma, intrinsic, attribute, raw
address/offset access, punning, undefined behavior, byte forcing, object
patching, compiler flag, comparator exception, or storage rehoming was used.
