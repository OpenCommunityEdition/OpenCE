# PathObstacles: natural circle query and genuine accessor owner (2026-09-06)

## Result

Starting at published `007da7f2a9e41291c40192b84f5ac31d50005af7`, reconstruct
the previously unwritten `obstacles_test_circle` once in natural typed C and
restore the disc accessor to its genuine header owner.

The query is **160 meaningful/padded bytes, four relocations, 96.878784%
fuzzy**. PathObstacles advances from **6 exact / 0 residual / 10 unwritten**
to **6 exact / 1 residual / 9 unwritten**. This is 160 bytes of newly
reconstructed function scope, **zero new exact bytes**. All 6,269 inherited
strict functions survive. Neither PathObstacles nor PathObstacleAvoidance is
claimed complete or whole-object admitted by this packet.

## January and later evidence

January `_obstacles_test_circle` at 0x50A80 scans signed-short disc indices,
skips the signed-short `ignore_disc_index`, and returns the first intersecting
disc or `NONE`. It compares squared XY distance to the square of the sum of
the two radii, including tangent equality. It uses the January 24-byte disc
stride, not the later HCEA 20-byte record. Disc flags, object handles and
height do not participate in this 2D query.

The public ABI is `(const obstacles*, short, const real_point2d*, real)` with
an AX result. January stack reads and later HCEX DIA parameter records agree.
Local later evidence relative to the outer `i-w` workspace:

- `research/tools/DIA2Dump/x64/Release/DIA2Dump.exe -sym obstacles_test_circle`
  against `research/hcea_jun2011_prototype/payload/Halo CE Anniversary (Jun 24 2011)/HCEX_Release.pdb`.
- `research-cache/halocea-full-blobs-20260830/src/blam/bitmaps/obstacles_test_circle.c`,
  snapshot `570c83fd9c365dad6f2a3e7041705d5b84c7847c`.
- Adjacent `obstacles_get_disc.c` corroborates the typed accessor contract;
  January, not the later assertion-free function, supplies the debug behavior.

The loop does not invent a whole-container precondition: nonpositive counts
return `NONE` without reading center, and an ignored disc skips the accessor.
An oversized positive count reaches the real accessor assertion before any
disc geometry is read. Unordered floating comparisons do not count as hits.

## Genuine header restoration

The query's four relocation records identify `path.h` and the assertion
`disc_index>=0 && disc_index<obstacles->disc_count && obstacles->disc_count<=MAXIMUM_DISC_COUNT`
at line 396. The same body already exists as `obstacles_get_disc` in the
avoidance C file, but that placement cannot supply the January inline
schedule to another translation unit.

Move that exact existing body into `source/ai/path.h` as ordinary `__inline`,
remove the C duplicate, and declare the public query in that same genuine
owner. The query calls `obstacles_get_disc` and the existing `point_in_circle`
API naturally. No accessor assertion is copied into the caller; no force-inline,
namespace alias, extra wrapper, assembly, qualifier or compiler control is added.
The existing math aliases elsewhere in PathObstacles are untouched. No
`point_from_line3d` definition or reference is introduced.

The existing avoidance accessor keeps its bytes and four relocations; its
COMDAT selection changes from NODUPLICATES to SELECT_ANY as a normal
consequence of the header-inline definition. The query emits a second natural
SELECT_ANY copy. These copies are not additional January function credit and
are not hidden from the object inventory.

## First natural residual

Target normalized SHA-256:
`3a11e2b7f83efa430dfba1728254845c7707cba32e459854f452a47d527bd433`.
Candidate:
`f07c634dda655b3857287511109246b281ad8f7c784f29868e4e930140d31ef9`.

The entire raw function agrees outside [0x4D, 0x5D). January calculates the
disc pointer, loads disc.radius and adds the parameter radius. The compiler
instead loads the parameter radius first, calculates the pointer and adds
disc.radius. The later distance, comparison, branches, assertions, return
and stack instructions are unchanged. This is a fixed first natural body,
not a search over equivalent expression or declaration spellings. Keep the
honest fuzzy result; reopen only for genuine source/compiler-context evidence.

## Verification evidence

The four frozen phases under `scratch/path-obstacles-circle-{before,owner,first,final}-20260906`
contain all 51 actual header consumers, C/header/config snapshots and complete
current-base/January-split object hashes. The owner-only trial keeps all
strict matches. Of 3,298 inherited runtime sections, 3,239 are unchanged and
57 differ only in compiler-local label spelling. Two changes require explicit
review: the getter's COMDAT selection and the already-fuzzy speech-timer's
two independent stack loads at [0x167, 0x16E). No AI communication source
is changed and no new exact credit is claimed there.

Gate object SHA-256:
`9eecf47e5bb37f7428f95fc8ea56c3fb0e32ab624867b97361741eaa6f61611f`.
First/final ordinary object:
`ae99335deba8e6cddd594d4add96639b3b7f6b9d60c17058930285964f958a63`.
First/final PathObstacles, header and avoidance source hashes respectively:
`cea5c6440f7389a9b2b14b2d32aa75d446f604d5d536b46d7920029cb4b8050c`,
`5b0e569f51633c058c5412b7996ac762b1e5f947381d5a7a4dbb78de02268a15`,
`c2d82a66acf4e223a2c27e9b58fe7fcb771f36c92a0439f718954e0dd5a954f2`.

Independent owner/speech proof and root replay both pass. HCEX DIA also
explicitly marks the getter `WasSpecifiedAsInline=true` and `WasInlined=true`;
January's atlas selects it from `i path_obstacle_avoidance.obj`. This supplies
independent provenance for the header restoration, beyond the assertion path.
The complete 572-object current-base census finds only the two identical
SELECT_ANY getter providers and no undefined getter entries. Neither provider
conflicts with the other; this is not a claim about mixing current objects
with csplit's synthetic selected NODUPLICATES wrapper or a full engine link.

The complete speech-timer section and all 43 relocations/referenced literal
and data sections agree outside the seven-byte block. Its MOV instructions
read distinct frame locations into independent EBX/DI destinations, change
no flags, and have no incoming branch between them. The local at EBP-8 is
initialized earlier; the parameter at EBP+0x10 is read-only. Its existing
fuzzy record is refreshed to 97.76382% with this proof, not blindly rehashed.

Owner-to-first preserves all 3,298 inherited sections and adds only the
query, accessor fallback and two exact assertion literal COMDATs. All 51
first/final ordinary objects and all six frozen C/header/config/target files
are byte-identical. Gate-to-ordinary differs only in the debug filename;
runtime sections and the full symbol inventory agree. Root rechecks all
1,405 current base/split objects against the final manifest.

### Bounded runtime replay

`tools/audit/path_obstacles_test_circle_runtime.py` executes the actual
January, first-gate and final ordinary function bytes. Only display_assert
and nonreturning system_exit are modeled; there is no substituted math,
disc-access or query implementation.

Root and independent replay pass **40 scenarios x two x87 modes x three
roles = 240 executions**, plus two detected in-memory actual-byte negative
controls. Cases include signed-short count and ignore widths, nonpositive
counts, oversized-count assertions, first-hit order, an ignored sole disc
with a null query pointer, all 128 records, exact tangent and one-ULP
inside/outside boundaries, negative radii, signed zero, subnormal distance,
quiet NaN and selected infinity combinations. Whole input/container bytes,
padding, read-only protections, stack/write guards, AX return, ESP/EIP,
nonvolatile registers, DF and x87 control/tag/TOP/status are checked.

The branch-inversion and radius-subtraction mutations fail the result oracle.
No object file is patched. Root's one verifier refinement makes the existing
ignored-disc case use a null query; both replays include that stronger check.
The source is unchanged from its first natural compile.

These are disclosed masked-exception FPCWs 0x027F and 0x037F, not proof of
the game's process default or every floating environment. With unmasked
invalid/denormal exceptions, exchanging FLD and memory-FADD operands can
change trap timing/location. Signaling NaNs, arbitrary pointer aliasing and
exhaustive floats are outside the proof. The function stays fuzzy and gains
no exact credit.

Final Ninja passes; **641 tests plus 26 subtests** pass. Parks are
**297 active / zero stale / zero invalid**. Admission has zero new candidates
or contradictions and retains its five existing rejections. Canonical remains
**919,188 exact code bytes, 6,223 credited functions, 6,269 strict functions,
2,061,024 matched data bytes and 391/833 Matching objects**. Active Opus
implementations and donor worktrees are untouched.

### Reproducible local evidence

Original objects, PDBs and scratch reports remain local, not in the commit.

- `scratch/path_obstacles_circle_owner_independent_20260906.py`, SHA-256
  `915201bafca20f030860faa758c29dfca69439dee4cc3910fa94e019d7c407e9`.
  Independent JSON and `scratch/path-obstacles-circle-owner-root-replay-20260906.json`
  agree: `ab98f8a95f87128fe142cf039fb846ed0c78627cf3ad38bcb57133d018b89ef4`.
- `scratch/path_obstacles_circle_final_artifacts_20260906.py`, SHA-256
  `2292ebb30c98e819bcd90530e814a7473fff7257cb276b48771130f55426eed4`.
  Its final JSON is `acfd378facbff0c7e0ff17ff74d3f668c4543d360b28fc5504f3e714a9e1ab45`.
- `scratch/path-obstacles-circle-runtime-root-20260906.json` and
  `scratch/path-obstacles-circle-runtime-independent-final-20260906.json`
  agree: `5e59a52909e73318e4a40fa6051ae9c8ba60f186efbfc6a058ecb33bb4b6cdef`.
- Runtime tool SHA-256:
  `28ecea49dd639d158dffa8dc5a7baf414b7da241a59eccf4e4006782b6e78851`;
  eight focused tests SHA-256:
  `fb7da0fa6a5ff1476dbebd1beadf54780f0e33df22bc38389d55bf779ace7a86`.
- `scratch/path-obstacles-circle-final-stable-20260906.json`, SHA-256
  `6e0da6d2d5cd6ffe88b05706e01ba587c8efd11e24bda82a12a8983d7cc3c178`.
