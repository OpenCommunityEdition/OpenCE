# `player_profile.obj` highest-completed-solo-level first shot — 2026-08-30

## Result

The sole natural typed candidate for
`_player_profile_get_highest_completed_solo_level` is a strict miss and has
been inverse-reverted. January requires 773 meaningful / 784 padded bytes and
four relocations. The frozen VC7 candidate emitted 157 meaningful / 160
padded bytes and the same four ordered relocation identities at the same
addresses. Its loop remained a compact ten-iteration loop; January is a
complete ten-level unroll. No production source, header, configuration,
compiler flag, semantic exception, parked entry, or object patch is retained.

`source/saved games/player_profile.obj` therefore remains 9/22 strict owners
and 448/4,272 padded code bytes. The target remains unwritten. This additive
ledger is the lane's only intended tracked change.

| Measurement | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful / padded bytes | 773 / 784 | 157 / 160 |
| Relocations | 4 | 4 |
| Normalized SHA-256 | `f5432152d8507a8d6ed09c1a693938385021c273b34513a0aaec6f64691a9fd8` | `add55bf06c41a5ca59bdb7c392af6493320c6adcaadae85d4cfcb16ba9674a3d` |
| Normalized differing bytes | — | 720 |
| Relocation destination sequence | — | exact |
| Relocation address sequence | — | exact |

The four relocations in both objects are:

| Function offset | Type | Destination |
| ---: | --- | --- |
| `+0x23` | `DIR32` | exact `c:\\halo\\SOURCE\\saved games\\player_profile.c` COMDAT |
| `+0x28` | `DIR32` | exact `profile && level && difficulty` COMDAT |
| `+0x2D` | `REL32` | `_display_assert` |
| `+0x34` | `REL32` | `_system_exit` |

## Provenance and prior-attempt census

Before editing, the applicable `AGENTS.md`, `CLAUDE.md`, verification policy,
prototype-inference and knowledge-base rules, campaign tooling README, complete
Player Profile ledgers, owner source/header, callers, January COFF
disassembly/relocations, HCEA donor, and tree checkpoint were read. The
registered-worktree census covered 1,524 worktrees. History, tracked source
blobs, ledgers, and registered worktrees contained no prior definition or
bounded compile attempt for this function. Untracked and scratch evidence was
excluded.

Immutable authority and environment:

- isolated starting tip: `b22d827d4a06c54f5ff1000b1acc359ac1aab2d6`;
  its local ancestors after `e4713b11ef5e3ac8e457baeca069ce6daf19e35e`
  are ledger-only and do not change Player Profile production source;
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`;
- January `player_profile.obj` SHA-256:
  `22c5fc38c3b1090f8e0ef271ce2119fa57d8c5c8aafe65c4947d638fc96d6c57`;
- XDK 3911 VC7 `13.00.9254.1`, `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- natural flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`;
- baseline/final owner-source Git blob:
  `fe35ea5051002e2bc2bf91ae50070247cc396503`;
- unchanged owner-header Git blob:
  `210ea1b1597fc03e339a27fdc31714c99a7c3921`;
- pre-edit frozen regression manifest SHA-256:
  `58a31307626c9ec11394790acd1a1476686f81cad4bdc7e12ef9800f20e9a5f1`.

The authenticated semantic donor is HCEA commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c`, file
`src/blam/interface/player_profile_get_highest_completed_solo_level.c`, Git
blob `a74061698dd2273dbd28825ab00c3223dbf3fe88`, and file SHA-256
`0158a625720523d813ad37dbc0eaaf204bfdd44627999ce3e652d7bd59b32747`.
It explicitly restores a natural ten-iteration loop from a decompiler's
five-wide PowerPC unroll. January PC independently proves the same ascending
level scan and descending difficulty-bit priority, but emits ten complete
copies. HCEA is semantic/layout evidence only; January remains the byte and
ABI authority.

## Frozen candidate and measured boundary

The candidate introduced a translation-unit-local typed 0x30-byte
`player_profile` layout with the flags array at +0x1C, the natural assertion at
source line 440, and the donor's single fixed-count loop. It initialized level
to `NONE` and difficulty to normal, scanned ten flags in ascending order, and
tested difficulty bits impossible, hard, normal, easy. It used ordinary cdecl
parameters and named typed field access. The candidate source Git blob was
`41b48b1871d0b79c5a13391406fa6873aac90746`; its working-file SHA-256 was
`8115bc72f0021c0b9a2bf8feb78c0325ddbd8b18ae09f8cc1f11eedd550c8828`.

The one and only production Ninja edge compiled successfully. The candidate
prologue, complete assert topology, defaults, relocation schedule, bit-test
priority, and semantics agree with January. From `+0x50`, however, VC7 emits
one compact loop body and closes it with `inc eax`, `inc ebx`, `cmp ax, 10`,
`jl`. It returns at `+0x9C`. January instead repeats the per-level body ten
times and returns at `+0x304`. The preserved candidate object is ignored
evidence at
`build/audit/player_profile_highest_level_first_shot_20260830/player_profile.obj`;
its raw SHA-256 is
`5d83905fc371b0ce80f51b002af574b40fe0db1e5a3ec4df704b3ef54c72bb74`.

This is a compiler-unroll fixed point, not evidence authorizing a hand-unrolled
reconstruction. No follow-up declaration, spelling, predicate, expression,
include-order, manual-unroll, or control-flow variant was compiled.

## Do not repeat and reopening condition

- Do not retry this natural typed ten-iteration HCEA topology.
- Do not hand-unroll from the January disassembly or sweep declaration,
  predicate, branch, or epilogue spellings to force January's bytes.
- Do not use pragmas, optimizer controls, volatile/register devices, raw
  offsets, object-representation casts, aliasing, undefined behavior,
  synthetic anchors, or object patches.
- Reopen only with newly authenticated January-PC source or compiler/TU
  provenance that naturally explains why VC7 selected the full ten-level
  unroll. The target disassembly alone is not source provenance.

## Restoration and validation

The include, typed layout, layout assertions, and candidate body were removed
with an inverse `apply_patch`. The final source Git blob exactly equals the
baseline value above. A normal production-edge rebuild restored the base
object; its phase-specific raw SHA-256 is
`a7fc4d7b4927e76b7df2312931a6508c71f80f04e2aea2cdf74e83c7ab92b5bb`.
The frozen whole-TU regression reports all nine inherited exact functions
`still_exact`, with no changed nonexact, newly exact, failure, or warning.

- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress`: pass.
- Semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 hidden bytes, 4,810 accepted exact, zero ordinary
  rejection, and zero unit errors.
- Progress: 384/833 objects, 4,771/11,060 functions,
  606,187/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes.
- Strict board: 277/619 objects, 4,782/8,246 functions, and
  663,079/1,922,669 padded code bytes.
- Protected Units: 189/189 and 54,864/54,864 padded bytes. Sentinel
  `_unit_preprocess_node_orientations` remains exact at 1,920 padded bytes,
  87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Complete Python tooling suite: 212/212 pass; the only warning is the
  sandbox-denied optional pytest cache.
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`.
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`.
- Final `git diff --check`, deleted-path review, protected-source review, and
  prohibited-construct audit: pass.

The frozen production candidate contained only readable typed C89 with an
explicit return and one parameter per line. It contained no assembly,
volatile/register/pragma/intrinsic/attribute/barrier, raw address or offset
access, representation pun, undefined behavior, byte forcing, or tool/compiler
exception. No candidate source or production object survives, and nothing is
pushed from this lane.
