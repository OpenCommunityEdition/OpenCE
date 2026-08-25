# Units update-animation private closure (Jonas, 2026-08-24)

## Result and strict boundary

This bounded wave starts from exact cumulative Units commit
`0fab4bb1c991ddb114588e634150a65376bef9cc`. Its natural production closure
contains the public `unit_update_animation` support body and five real private
dependencies. Four private COMDATs were strict on the sole candidate object
and are retained:

| January owner | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_00198050` | 18 / 32 | 0 | `eda63f46a5417149c356fd1b320ca113fb8c9dbf68f135dbdb862b9edf53befc` |
| `_code_00197f90` | 124 / 128 | 7 | `31b479b4dc7053f80c1cab9504b331fd7da20c4701a579b5f1ad3417f4fc3421` |
| `_code_00198070` | 86 / 96 | 4 | `4b6482d862a9d5de668a695986cb613035ba81535cd2a365ac1e93f8fe2e57a1` |
| `_code_0019dff0` | 46 / 48 | 4 | `57f986cef306281dca5882d5882d6f259da3d382d03abb1f1b46d89f1e578c65` |

The gain is four exact target code owners, 274 meaningful bytes, 304 padded
bytes, and 15 exact relocation identities. The complete direct census moves
from 156 exact / 11 emitted nonexact / 22 absent to 160 / 13 / 16 across the
189 ordinary January function owners. Removing the four gains from the final
exact set reproduces all 156 inherited exact owners. There is no inherited
loss and no inherited nonexact-body mutation.

`_code_0019b160` and `_unit_update_animation` are retained only as the real
support topology that emits and calls the four strict private bodies. Both are
explicitly nonexact and receive zero code credit. The object remains
`NonMatching`. No header, protected source, configuration, storage definition,
semantic exception, parked-function record, completion label, or existing
ledger is edited.

## Immutable authority and compiler

- January `source/units/units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- January `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Exact base Units source blob: `45e28394c8073ab4eebea1892855d7665b29ca6a`,
  239,929 raw Git-payload bytes.
- Final retained Units source blob before commit:
  `52dac812e29575758ab8e9a0b6e943620a086012`, 251,430 raw Git-payload
  bytes, payload SHA-256
  `c3a940ea50e38f99695d4a558b94a22690124ce9a91814e2f464e38f4a94fab8`.
- Phase-specific exact-base object copy: 133,262 bytes, SHA-256
  `b72bd579243c49e310b580a01aea2ad55455496952c31b228e1509342747739c`.
  Its source is the same exact `0fab4bb1` payload; the preceding clean replay
  independently recorded the expected metadata-variable whole-file hash.
- Immutable sole production first-shot object: 138,052 bytes, SHA-256
  `8d3474ca185a7dff2c7dc86c2a6018f6f4c66b972f11df4018f26e9bf06b7e88`.
- XDK 3911 `CL.Exe` 13.00.9254.1: 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Authenticated csplit v0.0.2 and objdiff-cli v3.3.1 SHA-256 values:
  `708f957c37caa66df90ade35d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
  and `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

The generated production rule remains ordinary VC7 C compilation with the
repository's `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` boundary. No compiler flag,
tool binary, or generated configuration was changed.

## Claude documentation and tooling audit

All reachable Claude documentation for `units.obj`, the update-animation
body, its private helpers, and its tooling was read as evidence before source
emission. This includes the complete specialized
`units_obj_update_animation.md` history through UUA-42; the complete
consolidated `units_obj.md`; the animation-helper, animation-action,
animation-postprocess, animation-state, private-helper, lifetime-control,
compiler-census/provenance/QFE, crossbuild-provenance, donor-atlas, HCEA PDB
and scope-range, seat-scripting, and final-topology ledgers; and the associated
update-animation dependency-audit and donor-scan research.

The campaign `README`, `mk_repro.py`, `lab_compile.py`, `gate.py`, and
`units_hunt_all.py` tooling was read completely, as were the c2 configuration
generator, backend module map, and IR-layout documentation. The most complete
Claude carrier was authenticated at
`deb5554ab8d16da80da934e2aa7081bc7d1f594f`; its `source/units/units.c` blob
is `124f29e32497c091c4163b5faafc58dbf8890161`, 305,068 raw bytes. It proves
all five private target hashes above but contains unrelated barriers, a raw
unit-layout access, attributes, and other forcing constructs, so it was never
imported wholesale.

UUA-41 was carried forward exactly as documentation requires: the exit-seat
path first calls `object_get_world_matrix(unit_index, &world_matrix)` and then
passes that named matrix to `matrix4x3_transform_vector`. The HCEA dependency
and contribution audits prove this producer relationship. UUA-41 also says
the root remains nonexact and closes the remaining HCEA spellings as
NO-REOPEN; this wave honors that boundary and makes no root matching claim.

## Typed and policy-clean source

The final source is ordinary readable C:

- all five private helpers are `static` and are emitted by real calls from the
  external target support root;
- a two-byte translation-unit-local `unit_animation_update_data` structure is
  size-checked and carries the typed root input;
- the existing named `unit->object.functions_active_flags` field replaces the
  carrier's forbidden raw `+0xB6` byte access;
- existing unit, animation, biped, model, seat, vector, and matrix types are
  used directly; `biped_definitions.h` is included because `/Zs` proved that
  the root's real biped-definition access requires it;
- the exit velocity uses distinct typed `real_vector3d` and
  `real_matrix4x3` locals with the UUA-41 producer call;
- no storage owner, dummy caller, export anchor, linkage promotion, raw
  address, raw layout offset, pointer or union pun, inactive-union read,
  alias violation, assembly, intrinsic, barrier, `volatile`, pragma,
  attribute, explicit `register`, or serialized-byte forcing is present.

The added-line policy scan and `git diff --check` pass. The only tracked
production path is `source/units/units.c`; this new ledger is the only other
tracked path.

## Scratch-first packet and one production emission

The authenticated root plus its five helpers were first isolated with
Claude's `mk_repro.py` and compiled through `lab_compile.py`. The fixture was
then policy-cleaned before comparison: the barrier and pragma were removed,
the raw `+0xB6` carrier macro was replaced by the named typed field, the root
input became the two-byte structure above, and UUA-41's separate matrix
producer was used. All five private bodies remained strict in that isolated
object. The root remained nonexact, as expected.

Production parse-only validation ran before code emission. The first `/Zs`
pass exposed the missing biped-definition type dependency and produced no
object; the required existing include was added. The repeated `/Zs` pass then
succeeded. This completed the typed pre-emission packet without consuming a
candidate compile.

Exactly one natural code-producing Units candidate command followed:

```text
ninja build\base\source\units\units.obj
[1/1] CL build\base\source\units\units.obj
```

The first-shot object was copied immutably before adjudication. All four
retained helpers were strict across padded bytes, normalized hashes,
relocation positions/types, destination identities, and addends. There was no
source-shape, declaration, expression-order, control-flow, or compiler-option
retry after this production emission.

## Transparent `_code_0019b160` blocker

The fifth private dependency is the only private miss:

| Owner | Target | First-shot candidate | Disposition |
| --- | --- | --- | --- |
| `_code_0019b160` | 80 padded / 4 relocations / `31a801fff65c190a04e8e767f047a25415ca243ab6d107343563467de1979ec1` | 80 padded / 4 relocations / `8370a5fea4a740e91e7719ab45af4ac987a0d8c9274d9ecfcaca18835c612ff3` | required nonexact support; zero credit |

All four relocations, including REL32 `_animation_update_internal` at `+0x0E`
and REL32 `_object_impulse_sound_new` at `+0x3A`, are identical. The exact
root cause is the current Units declaration topology. The authenticated
Claude carrier and the strict isolated fixture explicitly declare:

```c
short animation_update_internal(
	short animation_type,
	long animation_graph_index,
	struct animation_state *animation,
	long *sound_index);
```

Current cumulative `units.c` and its included headers do not declare that
external function. VC7's accepted implicit-int call leaves the same result,
size, and relocations but schedules the caller cleanup differently. January
and the strict fixture emit `add esp,0x10; mov esi,eax; mov eax,[ebp-4]` after
the call; the production object emits `mov esi,eax; mov eax,[ebp-4]; add
esp,0x10`. `source/objects/scenery.c` independently carries the same typed
short-return prototype for its own use, corroborating the contract.

The parse-only pass did not reject this C89 implicit declaration. Once the
one production object exposed it, this spent wave was not retried. The helper
cannot be removed because `unit_update_animation` has four real calls to it,
so it remains transparently nonexact support. Adding the missing declaration
belongs to a separate fresh bounded declaration wave.

## Nonexact root and target data boundary

`_unit_update_animation` is 1,280 padded bytes with 64 relocations in both
objects. January's normalized SHA-256 is
`41f12865734551c376b91868f7f815bc36b7a6b575b85d6bf5446ab1688a4d5f`;
the natural first-shot hash is
`5e5e8d9a83d8df7e0ecafb9c691b0ff8f3c7c0f4f4ab427f12639e08c42190be`.
It remains support-only and receives no code credit. Its inclusion does not
mutate any of the 11 inherited emitted-nonexact bodies.

The support root naturally emits two January-exact assertion-string COMDATs:

| Target owner | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `??_C@_0BO@GAMKEDMN@desired_base_seat_index?$CB?$DNNONE?$AA@` | 30 | 0 | `5ff049ed77656128e5a3580a40fa907a29afc1be0a9b6033c2dd62455631c4a2` |
| `??_C@_0DI@EIIFDJIB@desired_state?$DO?$DN0?5?$CG?$CG?5desired_stat@` | 56 | 0 | `f81a0aa8524c6a8385487a7ac661cb66f5a6246d5cf2c8330dd77f6e22b3d6cd` |

The direct target-data census therefore moves from 123 exact / zero
present-nonexact / six absent to 125 / zero / four across all 129 January
non-code owners. This is recorded as strict support-emitted evidence, but the
wave claims no campaign data-progress increase because the containing root is
nonexact and no progress metadata or semantic exception is changed.

Runtime-owner A/B between the exact-base object and first shot finds all 207
baseline non-code identities still present. Every common non-debug acceptance
fingerprint is unchanged. The candidate adds only the two exact `.rdata`
owners above and four ordinary `.debug$F` records; common `.debug$F` and
`.debug$S` changes are compiler metadata. There is no runtime owner removal or
nonexact runtime mutation. The inherited strict 1,564-byte `.data` block and
four-byte `_unit_globals` BSS remain unchanged.

## Full pre-commit gates

The complete validation boundary passed:

- full Halo plus libcmt build: 568/568 edges; the already-adjudicated Units
  object was current and was not recompiled;
- strict semantic audit: 470 units, 4,230 functions evaluated, 3,499
  missing-base symbols, 4,087 semantic exact, 112 hidden exact / 64,056 hidden
  code bytes, 36 ordinary-only, 35 ordinary structural, one ordinary
  rejection, 12 locals skipped, 4,148 accepted exact, and zero unit errors;
- campaign progress: 375/833 objects, 4,124/11,060 functions,
  499,857/2,198,102 code bytes, and 1,835,088/4,176,062 data bytes;
- Halo progress: 273/468 objects, 3,957/7,574 functions,
  486,943/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes;
  libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes;
- object admission: zero candidates and zero revocations; the only
  contradiction is the inherited unrelated `source/shell/shell_xbox` label;
- parked-function validation: three active, zero stale, zero invalid;
- complete tooling suite: 179/179 tests pass.

The two private helpers that ordinary objdiff cannot recognize through their
private relocation topology remain direct strict matches and are included in
the semantic accepted total. This explains why the direct census gains four
while the ordinary campaign progress counter gains two. No matching metadata
was weakened or added to bridge that distinction.

## Committed-state replay plan

No committed-state replay is claimed in the implementation commit. After the
source and this initial ledger are committed by Jonas Volman and the worktree
is clean, a fresh one-unit accepted-state snapshot will pin that commit. The
generated object path will be resolved and proven inside this isolated
worktree, deleted with literal-path semantics, rebuilt once by its normal
Ninja edge, and checked immediately. Direct function/data censuses, support
hashes, runtime-owner fingerprints, and a final Ninja dry run will be repeated
before one additive ledger-only replay commit. No push, amend, rebase, history
rewrite, or worktree removal is performed.

## Clean committed-state replay

Implementation-and-initial-ledger commit
`ad04fa05982260ccb89bcc4edce8daa34fe876f4` was authored and committed by
Jonas Volman and was clean before replay. Re-reading that commit proves:

- retained source blob `52dac812e29575758ab8e9a0b6e943620a086012`,
  251,430 raw Git-payload bytes, payload SHA-256
  `c3a940ea50e38f99695d4a558b94a22690124ce9a91814e2f464e38f4a94fab8`;
- initial ledger blob `48ae5fff2488221d7a5ddae0f755e317330e3ad7`,
  13,355 raw Git-payload bytes, payload SHA-256
  `51caa3369df7c9009e1c465207948a2ed8cbb82e20dbaf501b7cf7510ba50964`.

A fresh one-unit accepted-state snapshot was written with `--no-build` at
that exact clean commit. The ignored manifest
`build/audit/regression_units_uua_private_committed_20260824.json` is
5,931,514 bytes with phase-specific SHA-256
`e992b69a7cf421ab761376488e2c00e950a2205f451d5bc2919457fd21483cb2`
and pins the full implementation commit.

The generated path resolved to
`build/base/source/units/units.obj`, was proven equal to the expected absolute
path, and was proven to remain inside this isolated worktree. The snapshotted
first-shot object was 138,052 bytes with SHA-256
`8d3474ca185a7dff2c7dc86c2a6018f6f4c66b972f11df4018f26e9bf06b7e88`.
Only that exact file was removed with `Remove-Item -LiteralPath`; immediate
verification proved it absent. The normal generated target then ran exactly:

```text
[1/1] CL build\base\source\units\units.obj
```

The replay object is again 138,052 bytes with metadata-phase SHA-256
`0600356cab6a7f729a7076436f9ecc70c1961980dcf5677d4845942407e0b081`.
The raw object-container hash differs through normal COFF emission metadata;
the complete runtime acceptance view is identical.

The immediate committed regression check returned `ok: true`, exactly 160
`still_exact`, zero failures, zero warnings, zero `newly_exact`, and zero
`changed_nonexact`. Independent direct comparison repeated:

- 160 exact / 13 emitted nonexact / 16 absent across the 189 ordinary target
  function owners;
- 125 exact / zero present-nonexact / four absent across all 129 January
  non-code owners;
- all six emitted closure bodies reproduce the first-shot padded bytes,
  normalized hashes, relocation sequences, and support/exact dispositions;
- all 213 first-shot non-code identities are present in the replay, with zero
  addition, removal, or cross-object acceptance-view mutation.

A final generated Units-object Ninja dry run reports `ninja: no work to do`.
Only this additive replay record is changed after implementation; no source,
header, protected path, configuration, storage owner, matching metadata, or
other ledger is modified.
