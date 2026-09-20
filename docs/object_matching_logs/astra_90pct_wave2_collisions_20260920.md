# Wave 2: collision_move_point evidence and bounded probes

**No landing: zero new exact bytes.** At frozen HEAD
`905e5e3b41591af98c82accf3f57fc38118c2e7f`, the fresh scratch baseline gates
19/20 strict exact. The sole residual is `_collision_move_point`, 4,744
meaningful / 4,752 padded bytes and 226 relocations. Four independently
motivated source shapes were tested; three are byte-inert and one grows the
function. No production source, header, configuration, guard or shared build
output was edited. No Ninja, commit or admission was run.

## Fresh evidence and limits

Fresh DIA reads of the supplied HCEA `HCEX_Release.pdb` confirm seven parameters,
including `maximum_collision_count: short`, and ten named aggregate locals:
`velocity`, `clip_line_vector`, `clip_collisions: short[3]`, `clipped_velocity`,
`clipped_position`, `clip_plane`, `clip_line_point`, `position`, `clip_point`,
`new_clip_collisions: short[3]`. There are 62 source-line records with file MD5
`FE4069DC1475EC65FC8CF6A1CD398E42`. These are 2011 PPC types/lines, not January
x86 locations or complete scalar lifetime information. In particular, DIA's
x86-looking register labels must not be reused as x86 allocation evidence.
The current source already has the authenticated aggregate widths.

The primary 2020/v140 unoptimized body at `0x007A52B0`, independently located by
physics/collisions assertions and complete call/topology correspondence,
contains ten matching RTC aggregate records. Its image SHA-256 is
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
Same-source-revision identity with January remains unverified. It additionally
contains feature-capacity logic absent from January; no whole-body transplant
is justified.

New direct distinctions were checked against the retained Sept14 and Sept19
exhausted-family records before probing:

| Shape | Primary witness | Fresh result |
|---|---|---|
| Entry `position = *old_position`, then `clipped_position = position`; remove the inner position declaration | Copies at `0x7A52DC..0x7A52ED`, followed by `0x7A5304..0x7A5313`; RTC position is 12 bytes | 4,752/226; exactly baseline bytes |
| Maximum-count break inside the successful collision arm | After memcpy at `0x7A6011`, comparison `0x7A6019..0x7A6023`; miss arm starts separately at `0x7A602F` | 4,752/226; exactly baseline bytes |
| Epsilon-first comparison with `fabs(component - 0.0f)` | Subtractions and epsilon-first compares at `0x7A5518..0x7A55AC`; matching ordinary idiom exists in real_math.h | 4,752/226; exactly baseline bytes |
| Synthetic collision copies directly from indexed last collision, then initializes minimum/steepest after output stores | Repeated indexed reads `0x7A64CA..0x7A650D`; output stores through `0x7A6547`; scalar initialization `0x7A654B..0x7A6559` | 4,768/226; worse, 44 opcode-only aligned edit regions |

The baseline and first three shapes share normalized SHA-256
`9b2a27828ed3963f40ba8ea3cb0dc6f465a661fccc29838a39af81976451374e`.
January is `8b2f29007193d3aacd830e10fa99886acd6a3bc1e9cd604483343340f6117e2c`.
All four probes preserve all 19 exact siblings, all 81 named owner tuples,
all runtime noncode, and both inherited one-byte COMMON symbols
`_debug_collision_skip_objects` / `_debug_collision_skip_vectors`. No point
helper is emitted. Four complete candidate sources have zero fake-scan leads.
Only the fourth probe changes any code owner, and only move_point changes.

## Narrowed blocker

The exact first divergence remains January's preheader loads at function+0x17B
and +0x17F (`cx = maximum_collision_count`, `edi = collision_count`) versus the
candidate's jump/alignment and delayed loads. Its matching back-edge refreshes
are at January+0xBA9/+0xBAC. Later differences include the integer/x87 interleave
around +0x598/+0x7AA, allocation for plane indexing around +0x7F6..+0x849,
and NONE-constant register lifetime in the synthetic plane at +0xF3A onward.
Baseline diagnostic alignment has 42 opcode-only edit regions, including
padding; this diagnostic strips relocation annotations and branch destinations
only to inspect scheduling. It never substitutes for the unchanged strict gate.

The new position lifetime, success-arm boundary and explicit zero-comparison
spelling are now measured negative hypotheses. The primary synthetic-store
order is independently supported but does not improve the January result.
No fifth shape is justified by these measurements. Do not resume broad scalar,
array, helper, scope or declaration permutations. A useful reopening needs
independent January-era IL/location/header/source evidence explaining the
preheader/back-edge allocation or synthetic-plane constant lifetime. Typed
aggregate widths and later RTC slot order do not supply that missing fact.

Reproduction/evidence: `scratch/astra-alias-wave2-collisions.py`,
`scratch/astra-alias-wave2-collisions-review.json`, matching candidate `.c`,
`.obj`, `.gate.txt`, `.source.diff`, and `.shape.txt` files; fresh raw
`scratch/astra-alias-wave2-collision-move-hcea-{sym,lines}.txt`;
`scratch/astra-alias-wave2-collisions-fake.json`; and
`scratch/astra-context-collision_move_point-2020.disasm.txt`.
