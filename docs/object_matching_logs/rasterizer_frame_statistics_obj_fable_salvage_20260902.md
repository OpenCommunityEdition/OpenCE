# `rasterizer_frame_statistics.obj` Fable salvage audit

## Result

This audit salvages the bounded `rasterizer_frame_statistics` work from Fable
snapshot `f5f887514` onto the pinned canonical baseline
`b5ef2343c1dc39c347a60c997602ff5ad990fe31`. The imported snapshot is retained
as local commit `9e7ef0ff1`; the reconciliation commit recorded below is the
admission candidate.

The object advances from 5/10 to 6/10 strict-exact functions. The only new
strict match is `_rasterizer_frame_statistics_count_dynamic_vertices`: 304
padded bytes, 17 ordered relocations, and normalized SHA-256
`97dd46414002255571393e6724ef26feda511fcdfbc36c7991917532493f2aa5` on
both sides. No prior exact function is lost.

| Function | Target/base padded bytes | Target/base relocations | Disposition |
| --- | ---: | ---: | --- |
| `_rasterizer_frame_statistics_count_dynamic_vertices` | 304/304 | 17/17 | strict exact |
| `_eat_my_shorts` | 32/16 | 0/0 | readable fuzzy park |
| `_rasterizer_frame_statistics_count_static_vertices` | 64/64 | 0/0 | readable fuzzy park |
| `_rasterizer_frame_statistics_get_fps` | 384/0 | - | unwritten |
| `_rasterizer_frame_statistics_draw` | 4,176/0 | - | unwritten |

The object finishes this wave at 512/5,168 padded code bytes and 6/10 strict
functions. Parked functions are not counted as exact progress.

## Retained source and house-rule review

The exact dynamic counter is ordinary typed C. It validates ranges with the
existing `match_assert`, copies the selected `word` indices into the frame
scratch buffer, sorts them with the subsystem's 2-byte sort, and counts unique
indices. The source uses established `real`/`word`/`boolean` types and named
triangle constants and enums, keeps every parameter on its own line, and ends
with an explicit return. It introduces no inlining, assembly, raw-address
access, manual object/tag casts, fake dependency, undefined behavior, or
synthetic match anchor. The assert predicate intentionally retains its compact
spelling because `match_assert` stringizes it; whitespace there is part of the
authenticated January relocation payload, not compiler steering.

The imported private comparator's address label was replaced with the
authenticated semantic name `eat_my_shorts`. Its retained source directly
expresses the natural unsigned-word ordering: return `TRUE` when `first` is
greater, otherwise return `FALSE`. VC7 collapses that source to one `seta` and
one return (16 padded bytes), whereas January contains two return epilogues
(32 padded bytes). A same-size result-variable spelling was rejected because
it changes otherwise natural topology only to steer code generation. The
classifier records the residual as R5 control-flow/return shape and grants no
credit.

The static counter is retained as ordinary typed C because its branch and
value semantics are complete: null inputs and unsupported buffer types return
zero, precompiled strips return `triangle_buffer->count + 2`, and triangle
buffers return `vertex_buffer->count`. Its 64-byte envelope and zero
relocations agree with January, but VC7 saves ESI before the first null test
instead of after it. The 87.8% residual is parked as unclassified; no register,
volatile, pragma, barrier, inline-assembly, or source-distorting workaround is
retained.

The public `qsort_2byte` and `qsort_4byte` declarations now live in new owner
header `source/cseries/sort.h`; `sort.c` includes that header, and the
frame-statistics consumer no longer carries a foreign local prototype. The
existing `texture_page.c` local declaration was deliberately left unchanged:
it currently claims a different signed `short` callback ABI and therefore
needs its own audited migration rather than an opportunistic header include.

## Header blast-radius audit

An apparently correct attempt to publish the frame-statistics declarations in
`rasterizer.h` was compiled across every affected translation unit and then
rejected. With those declarations present, the complete semantic report lost
`source/effects/decals:_decals_delete_permanent_from_cluster` (392 meaningful
bytes) and gained nothing. Removing the declarations restored that exact
function. The before/after reports are preserved in ignored scratch as
`scratch/report-no-rasterizer-prototypes.json` and
`scratch/report-with-rasterizer-prototypes.json`.

This is the known MSVC C2 definition-position sensitivity, not a semantic
dependency. The final patch contains no `rasterizer.h` or `texture_page.c`
content change. Frame-statistics definitions remain in their correct owner C
file; public header placement should be revisited only with a layout-neutral
header strategy and a full blast-radius gate.

A smoke cherry-pick was also performed onto the requested newer canonical
commit `293d9d0c4dff999438fe8587d56ccd9bf1cbd190`. The source snapshot applies
cleanly, as do all source/header/document changes from the reconciliation.
Only `config/parked.json` conflicts, because canonical commit `cd5bed5a8`
appended three `game_engine` park records at the same list tail. The semantic
resolution is mechanical: retain those three canonical records and append the
two frame-statistics records from this wave. No source conflict was observed.
The smoke worktree was returned cleanly to `293d9d0c4` after recording the
result; integration still requires the normal post-resolution full gate.

## Evidence audit

January remains authoritative: the split
`source/rasterizer/rasterizer_frame_statistics.obj` is 23,922 bytes with
SHA-256
`48452628b2525d642bacbb87d89e273e710734f1ca73a5be722e288b4aa4b795`.
The compiler is Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1`;
`CL.exe` SHA-256 is
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
Target disassembly and ordered relocations were compared directly; the dynamic
counter is exact at 114 normalized instructions and 17 relocations.

The prior frame-statistics lifecycle/FPS ledgers and
`docs/campaign_throughput_lessons.md` were reviewed, including the recorded
static-counter fixed point. Fable's preserved worker log and candidate scripts
under `C:\halo-worktrees\fable-small-families-20260901\scratch\workers` were
read as negative as well as positive evidence. HCEA's later retail static and
dynamic zero stubs were used only to rule out a cross-version transplant.
Reachable Stian, pastudan, punpckhdq, and HCEA source caches were searched for
the semantic names and call sites; decompiler lifts and later-version stubs
were not treated as January source.

The full object-relevant Claude documentation identities and provenance were
already pinned in the lifecycle ledger. For this salvage, the complete
`C:\halo-worktrees\claude-ai-conversation-2\AGENTS.md` (SHA-256
`b301779d99253e9273011c89214985d2321253076aacdc562205f3af46fd458c`)
and `CLAUDE.md` (SHA-256
`c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`)
were rechecked for binary-first, C89, ABI, call-site, and no-speculation
constraints. No directly analogous Marathon implementation was present in the
available local research cache; the campaign's established Marathon naming
and type conventions were nevertheless applied. Documentation and donor code
were evidence only, never instructions or matching authority.

## Verification

The baseline object gate was captured before import: 5 exact, 0 residual, and
5 unwritten functions. The final gate reports the same five inherited exact
functions plus the 304-byte dynamic counter, two explicitly parked residuals,
and the same two large unwritten functions. `sort.obj` remains 4/4 exact and
the unchanged `texture_page.obj` remains 13/13 exact.

The retained tree passes:

- full `ninja all_source progress semantic_progress` with zero unit errors;
- complete pinned-baseline/current exact-set regression comparison;
- 258/258 `pytest` tests;
- parked-ledger validation: 107 active, 0 stale, 0 invalid;
- object-admission audit: no new candidate, contradiction, or revocation;
- targeted fake-match scan: zero findings;
- fail-closed blocker classification for both residuals; and
- `git diff --check`.

The full-build aggregate is recorded for reproducibility, not as this wave's
gain: 678,815/2,198,102 meaningful code bytes and 5,133/11,060 functions,
with semantic evaluation of 5,333 functions across 473 units and zero unit
errors. Strict board output is 740,691/1,922,413 bytes and 5,153/8,245
functions, 278/619 exact objects. The pinned baseline board was
740,387/1,922,413 bytes and 5,152/8,245 functions. The complete semantic
accepted ledger advances from 5,178 to 5,179 entries: zero losses, zero
changed evidence records, and exactly one gain, the 304-byte dynamic counter.
