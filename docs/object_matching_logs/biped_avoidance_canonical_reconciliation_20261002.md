# Biped and actor-avoidance canonical reconciliation — 2026-10-02

Four specifically approved lane packets were independently reconciled against canonical
`09255f0e789d888217452b4f9e607d6ed21a7ce2` on `jonas/exact-pilots`. Each packet received its own
fresh clean full build and whole-board gate before committing. No whole-lane merge, research
candidate, build-flag change or scoring change was imported.

| Packet | Donor commit | Canonical commit | Meaningful / padded code gain |
| --- | --- | --- | ---: |
| B: sight and placement | `edbdbf729e9b7c0c09557f1ccb45f94ad6c8a06a` | `606d0a24` | 495 / 512 B, two functions |
| J: jump call restoration | `82ea0d5e9bcee8a1a48775aab6f0c351f5ee5e2a` | `164026a7` | zero |
| X: transform call restoration | `41d3b992a5e5c0fbbb5f76c476d27f9c68031e82` | `201442d3` | zero |
| F: test avoidance vector | `c694a43d4e3343eedebac48fe6f2b6052f679778` | this commit | 743 / 752 B, one function |

The imported per-packet logs retain their historical lane measurements and "not pushed" state.
This log supplies the fresh canonical reconciliation; the lane's additional unpublished tools
tests and other research were not imported.

## Fresh strict section receipts

All three sections equal their January split targets under the unchanged hardened COFF gate,
including complete relocation identities. These are normalized section hashes, not executable hashes.

| Function | Meaningful B | Padded B | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_biped_get_sight_position` | 407 | 416 | 19 | `59e27e5e5b6f7a7ca3ec56c3ff8f347e70de4848ad52d8c8dfe257e7ef3dffc8` |
| `_biped_adjust_placement` | 88 | 96 | 2 | `41db59000f9eb782eedfa45b2457e16898749fd2a3bc25a0e687c762181a40d4` |
| `_actor_move_test_avoidance_vector` | 743 | 752 | 16 | `271a1af1480dd3012f9c97fc0922e0c8fbe60e97078b87d304a0eb736e1f1f0b` |
| Total | 1,238 | 1,264 | | |

## Narrow source-policy scope

- B uses the approved An form, coupled jumping constants/crouch brake, two ground-surface call
  restorations and the separately approved placement site. Its three whole-value parentheses are
  per-site inferred spellings, not recovered source. The four tuning names are descriptive; `const`
  is inferred. The MSVC 19.00.24234 gap remains disclosed, not silently closed by tests on other versions.
- J restores only the genuine `biped_jump` call, removes its expansion-only local and admits its two
  vector-to-point casts under the existing site conditions. No new parenthesis or scope is introduced.
- X restores three genuine transform calls, six admitted vector-to-point casts and three byte-inert
  per-site parentheses. Its two guard lines are removed under the explicit narrow ruling, with one
  uncredited helper copy. It was verified separately before F was applied.
- F is the approved one-function body: seven individually evidenced parenthesised scales, twelve
  admitted casts, genuine calls and zero-vector copies, PDB local names, and inferred disjoint scopes.
  `collision_result`'s byte-inert scope placement is inferred too. The twelve scope controls support
  lifetime constraints among the tested forms, not original brace locations. S6 opens the block before
  `origin_offset`'s last use and remains exact. January alone permits one reused block-local variable;
  choosing the separate variables relies on the later first-party records. Neither a universal scope
  impossibility proof nor a first-party lexical-block record is claimed.
- All twenty newly admitted casts were independently ablated one at a time and in their groups using
  canonical's current compiler and unit flags. The listed exact functions remain byte-identical.
  The private-path control also reproduces those sections. Nine size/alignment/member-offset checks
  pass for each unit; a deliberately false assertion fails. The PDB-attested view is distinct from the
  inferred cast spelling, and no const qualifier is removed.
- The general parenthesis and view-cast holds are not lifted. No assembly, pragma, filler, function
  move, synthetic declaration, new type owner or compiler flag is introduced.

## Canonical gates and blast radius

- Halo functions: 7,480 -> 7,482 -> 7,482 -> 7,482 -> 7,483 of 7,573.
- Meaningful code: 1,601,833 -> 1,602,328 -> 1,603,071 of 1,770,166 (+1,238).
- Halo data stays 3,311,591 / 3,922,163; objects stay 396 / 468. No object admission is implied.
- Strict section-owner rows: 7,653 -> 7,656 of 8,252. Every stage has zero inherited exact losses.
- Parks: 61 -> 59. Only the two freshly exact biped entries are retired; no stale/invalid park,
  no park rebaseline, no altered rejection/reopen criterion.
- Admission remains nine review candidates, zero contradicted/rejected/revoked. Fake scan retains
  the same 26 inherited review leads. Whitespace checks pass.
- Tools tests at every clean checkpoint: 1,663 passed, five skipped, 159 subtests passed. The lane's
  1,700 figure is not claimed for this canonical tree.
- All 621 configured objects plus the inherited extra libcmt/chkstk object were fingerprinted.
  B changes only bipeds.obj: the two gains, the disclosed still-residual `_biped_update_physics`
  (88.408554 -> 88.398636%) and the new helper. Two extra raw-fingerprint differences are only
  compiler-local switch-label names; the unchanged strict comparator verifies both exact sections
  and relocation destinations as identical. Raw diagnostics were preserved, not used as credit.
- J is normalized-object-identical to B. X preserves all prior function sections and adds only the
  helper copy to actor_moving.obj. F changes only its exact target and the still-residual
  `_actor_move_vector_avoidance` (4,144 -> 4,128 B, 135 relocations; January 4,144). No definition,
  reference or section is added by F.
- New folded helpers: two 48-byte SELECT_ANY `_point_from_line3d` copies, no relocations, no duplicate
  byte credit. The board has 32 -> 33 -> 34 copies, all strictly equal to January's selected
  action_charge.obj provider. Every January real helper call is preserved per function: ten in
  four biped functions, two in two actor_moving functions; reconstructed inline sites stay inline.
- Current selected-provider checks cover all surplus external definitions in both units, in both
  input orders: 282 ordered probes at B/J and 284 at X/F, all pass. This is bounded duplicate evidence.
- The whole-board diagnostic link is unchanged: 621 objects, 398 distinct unresolved-name tokens,
  exit 1120, only the inherited `_real_local_random` duplicate; no image produced or executed.
- Fresh board-wide /W3: 4,091 before and after, identical by unit/code across all 612 CL targets.
  Fresh /W4 for both units at every stage: 204 before and after (106 bipeds, 98 actor_moving), identical
  warning multisets. These canonical counts differ from the lane and are reported separately.
- Production changes are confined to the two C files and the two park retirements. Tools, scorer,
  verifier/pins, all other configuration and production files are unchanged. Q10 credit remains
  93 records / 661,012 bytes. The unrelated dirty README and untracked research are preserved.
- Lens-flare F3, both collisions texts, the small-function negative candidates, function-order
  recovery, DD2/DD3, ai_debug casts and every other held packet stay out.

Private receipts and controls are in `scratch/astra_biped_avoidance_reconcile_20261002/`.
They contain local build artifacts and are not published. This verification establishes bounded
object/section identity, not an exact executable link or new runtime/playability claims.
