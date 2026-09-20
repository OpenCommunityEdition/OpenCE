> Research only: zero admission credit; no retained source/header/configuration change.

# Independent damage review: hold, zero credit

The one proposed helper form remains non-exact and adds a behavioral helper
owner absent from both January Damage and the frozen production object.
No compiler or source/header edits were performed by this reviewer.

Independent raw PE decoding confirms both calls (0x76c50b, 0x76c558) reach
thunk 0x40556f and body 0x432880. That body creates a local three-component
vector, subtracts point0 from point1 through the vector helper, and passes it
to collision_test_vector. Its operation matches the existing typed
`collision_test_line` inline in `physics/collisions.h`.

Fresh independent HCEA DIA records additionally authenticate the name,
five parameter types, local `real_vector3d vector`, and inline/was-inlined
attributes. The line records identify `physics/collisions.h:178..184`, MD5
`8E973BF002E27A508E76340F72C1B8B1`. The exact symbol name of the 2020
callee is still inferred from body/ABI correspondence; its source revision
relative to January is unverified. January remains the match authority.

The helper is a genuine existing owner API, so one bounded call-boundary
test was supported. The added A1 branch-local aggregate scopes and common
post-merge PIN were already researched controls, not new discoveries. The
September 4 area-damage ledger describes the old explicit vectors as natural
reconstruction; I found no explicit historical proof that they were introduced
as a deliberate ownership workaround. Do not invent that motivation. The
producer's 57 later scratch-file scan found no prior helper call, but is not
proof that every historical artifact was searched.

Independent full-section comparison of the existing objects confirms:

| Measurement | Baseline | Candidate | January |
| --- | ---: | ---: | ---: |
| AOE padded bytes | 1312 | 1312 | 1328 |
| Relocations | 52 | 51 | 51 |
| Normalized SHA prefix | 46952a11 | 93606db1 | d3f929d4 |

All 32 inherited exact target functions remain exact. Only the AOE body
changes among inherited emitted functions. The candidate is fingerprint-
identical to the old September 14 `park_rebase_candidate.obj` AOE body, verified
independently from that archived object. It does not improve the prior best.

The object adds `_collision_test_line`, 80 padded bytes / 1 relocation to
`_collision_test_vector`, normalized SHA256
`ca7acee0998d390837fc21d0a7a58241afcb65a4195116dfc3fb511af05e9c12`.
No candidate relocation calls that newly emitted copy: the two use sites
inline successfully. Its natural, unused emission does not establish January
Damage ownership or permit ignoring it. No helper suppression, selection
change, invented visibility guard or manual expansion is recommended.

Inherited named owners, nondebug noncode sections, COMMON and the point-owner
guard are preserved. The production source and both relevant headers match
their frozen inputs. The producer reports zero fake-scan leads; this reviewer
did not rerun that scan or compile another object.

Final disposition: **HOLD**, zero exact credit, no second shape. Reopening
needs a separately evidenced complete source/context mechanism that closes
the remaining x87 schedule and satisfies the helper-owner constraint. The
authentic later helper call alone does neither. IDA_AUTOMATION_UNAVAILABLE.

Reproducer and durable evidence: `review.py`, `review.json`, `helper-raw.asm`,
`hcea-collision_test_line-{sym,lines}.txt` in this directory. Producer inputs:
`../damage/helper.c`, `helper.obj`, `helper.patch`, `helper-review.json`,
`raw-proof.json`, `primary.json`, `ghidra_0076c230.{c,asm}`. Archived comparison:
`C:/halo-worktrees/opus5-150k-house-clean-20260914/scratch/workers/damage/park_rebase_candidate.obj`.
