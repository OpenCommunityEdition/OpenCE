# `collision_debug.obj` fixed-point closeout (2026-08-30)

## Result

`source/physics/collision_debug.obj::_collision_debug_render` was audited from
canonical base `594f504332d8a148febd3d518dd553d871fba80e`.  No production candidate was
compiled and no source, header, build, or match-metadata file was changed.  The
function is already at a durable legal-C fixed point: the complete E2-E10
experiment matrix is preserved on commit `cd3798e02` and the current canonical
public-donor audit requires genuinely new January-specific provenance before a
new code-producing shot.

The ordered fallback, `source/objects/widgets/lightning.obj`, is independently
frozen by its complete-donor atomic one-shot.  Repeating either target would not
be a fresh candidate and would violate the campaign's frozen-wave rule.

## January packet and current ownership

- `_collision_debug_render`: 4,182 meaningful bytes, 4,192 padded bytes,
  278 ordered relocations;
- normalized target SHA-256:
  `5a365dfa90e8300c345e596646f4d2a0be40c6a2527b4b15e747a3de364990d3`;
- raw January object SHA-256:
  `42c2ecc104a5b785c97ff9e0bfe1787f52e7b1405ed6e1fe3bacbaf4f0c7047b`;
- untouched base object SHA-256:
  `a1ba240cc574203bb5c80221ad1746fed66eadd61350e3c4f08ea6b9a14a7ac7`;
- recovered target data: 28,978/29,278 bytes, including exact BSS
  28,962/28,962 and DATA 16/16;
- current campaign inventory: 0/1 code functions, 0/4,192 code bytes.

The exact data reconstruction introduced by `c335fd8e2` remains untouched.
The BSS packet contains 1,200 cached normals, 1,200 cached points, a 1,200-bit
cache mask in 38 dwords, width and height, and two phantom-BSP flags.  The
target also owns the 0x60-byte cube-vertex RDATA and its strings/constants.

## Exhausted legal-C families

The historical `collision_debug_obj.md` ledger at `cd3798e02` records the
complete bounded E2-E10 matrix.  The tested readable, typed source families
cover:

- simple row-major spray indexing;
- aggregate and separate-array representations of the proven BSS layout;
- explicit row/bit strength-reduction values;
- explicit nested 30/40-iteration downcounters;
- HaloCEA-derived vector initialization and scaling orderings;
- ignore-object local lifetime versus direct global references;
- single-result versus explicit true/false bit-vector updates;
- compiler-derived versus explicit loop induction.

The closest legal candidates remain split across two incompatible outcomes:

- E6 reaches the exact 0x1060 padded envelope but emits 280 relocations and
  normalized SHA-256
  `204dd9912e56111b5bd0fe2294134fab78a9a3338410fb10c9df434aca2f01e4`;
- the best 278-relocation family is 0x1050 padded, 16 bytes short.

The first persistent register-allocation divergence appears near `+0x8B` and
continues through the spray loops.  This is not a local scheduling tie.  The
matrix already spans the plausible legal-C cache-layout, loop-counter,
initialization, update, and lifetime surfaces; another spelling-only shot is
not justified.

## Donor, history, and worktree census

The full repository history and registered worktrees were searched before this
closeout.  The only complete experimental ledger is the clean historical
`collision-debug-atomic-20260813` branch (`cd3798e02` ledger, `11aa7cc45`
parked-state tip); its production source is restored and contains no retained
body.  `collision-debug-large-20260820` contains only the already integrated
runtime-data recovery.

The current canonical audit at
`docs/object_matching_logs/collision_debug_obj_jonas_public_donor_audit_20260826.md`
records the public-repository searches.  HaloCEA supplies the external-global
data definitions but no render/spray body.  Stian, Pastudan, punpckhdq,
Aerocatia, Baboon, Nimbus, and the available Claude/Fable material provide no
new body or local-lifetime provenance.  The punpckhdq source is an empty symbol
and section inventory only.

## Ordered Lightning fallback

The remaining `lightning.obj` pair was already compiled exactly once from its
complete HaloCEA/Stian donor cluster and restored after a strict miss:

- private `_code_00124c70`: target 240 padded / 22 relocations / hash
  `2fe97798519acba821968a88f76c5a3ac64a62c86a9919327b6ed89c8599f17d`;
  candidate 240 / 22 / hash
  `f31a3152a95637d3f7e9f1bbada7d9c01bbcb9a88193ccf7c741c3fde5e3dbaa`,
  with two independent instruction-scheduling swaps;
- `_lightning_submit`: target 2,576 padded / 80 relocations; candidate 2,512 /
  81 / hash
  `e03280899a3ed771bf2ada305b4729aedd25a945285852312eda06ae795cdf2b`.

The current canonical ledgers
`lightning_obj_jonas_complete_donor_one_shot_20260828.md` and
`lightning_obj_jonas_offset_helper_fixed_point_20260826.md` explicitly prohibit
repeating that donor adaptation without new January-specific provenance.

## Reopen criteria

Reopen Collision Debug only after obtaining an original or cross-build spray
body, debug local-variable records, or other January-specific evidence that
resolves the competing loop-counter/cache lifetimes.  Reopen Lightning only
with provenance that explains the helper scheduler choices and the broad
submit-body residual.  Do not reopen either target for source-order sweeps,
compiler coercion, raw access, asm, volatile/register forcing, barriers,
pragmas, undefined behavior, byte patches, or comparator exceptions.

## Verification

The untouched production edge was built only to authenticate the current base
and split packets; it was not a candidate compile.  Final restored-state gate
results:

- full `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass;
- semantic report: 470 units, 4,954 functions evaluated, 4,837 semantic exact,
  131 hidden exact, 0 unit errors, 0 ordinary rejected;
- progress: 27.77% all matched, 33.75% `halobetacache`, 23.47% `libcmt`;
- admission audit: 0 candidates, 0 contradicted, 0 revoked;
- parked audit: 12 active, 0 stale, 0 invalid;
- tooling tests: 212 passed;
- protected Units `_unit_preprocess_node_orientations`: strict exact, 1,920
  padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `git diff --check`: pass.

The expected inherited compiler diagnostics in unrelated `vehicles.c`,
`scenery.c`, and `network_game_manager.c` remain baseline warnings.  This lane
introduced no new diagnostic and no exact-code or exact-data loss.
