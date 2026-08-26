# `thread_win32.obj` exact closeout

## Result

The January 2002 `thread_win32.obj` is complete under the XDK 3911 and
hardened COFF gates.

- translation unit: `source/bungie_net/common/thread_win32.c`;
- compiler: XDK 3911 CL 13.00.9254.1;
- flags: `/O2 /Oy- /DDEBUG /Dxbox`;
- result: 9/9 functions, 892/892 meaningful code bytes, 960 padded code
  bytes, and 62 relocations;
- runtime data: 1704/1704 bytes: 160 bytes of exact strings and padding plus
  1544 bytes of logical BSS; and
- target object SHA-256:
  `c1c57fecec87fbb2822269bf6144501d02bdac88baeabcd3314633f794432206`.

| Function | Meaningful / padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_0006fc30` | 40 / 48 | 2 | `b812580cd3f3a51857eea79e7c400e2c90d6cb18eb084dfc6dd0f68ee0209089` |
| `_code_0006fc60` | 59 / 64 | 3 | `3b67f561243fc178ef970c0e26668272cdfd5b49b5f94800b85cbeeb4e051c6f` |
| `_create_thread` | 237 / 240 | 14 | `16e3913a39b5114ae051449aeca7ba35617303f5bb69196f6e6e2c68a0ad2ccb` |
| `_thread_has_exited` | 79 / 80 | 5 | `df2022740f864481ff3f69dbf24462cdf30dba67511e9ee5406dba5553616710` |
| `_dispose_thread` | 103 / 112 | 9 | `50a3c125e2d61a6e0b8fe9de4e715998392ec7d7bd7e732c3eec4cc5721244a2` |
| `_create_mutex` | 132 / 144 | 10 | `56979efa415931b2f3e8863bca78d35ddec9b4b1d5bb66f1c521550f12316c95` |
| `_take_mutex` | 81 / 96 | 5 | `73acc544cb6723d7adcde7102eb4f4f50160f67577e3003fc65296f3c1c539b0` |
| `_release_mutex` | 54 / 64 | 5 | `a07e6ba4ed6a0a917f5dfca3d44aa86c6b3159e6437d7462265b4bf5c48bf748` |
| `_dispose_mutex` | 107 / 112 | 9 | `ceb7fa5b3ab518f45d1b164834475e469db7e933a32b79d0e32bb91834ecb571` |

Every relocation address, type, destination owner, addend, function type, and
storage class agrees with the January target.

## Reopen provenance

The decisive new donor is upstream merge commit
`fc47f9a15a0aea7f0081394c4d66db5c68ab5323`, from
`https://github.com/punpckhdq/halo/pull/51`. Two independent upstream CI
reports established its new topology as an exact ordinary match under the
same compiler configuration:

- PR-head run `32986103544`, artifact `9613347017`, report ZIP SHA-256
  `441f2cb6e63e0d2c9a5871fab979f8b779c7369981a5397dfdf097abf1092724`;
- post-merge run `32987365550`, artifact `9613378994`, report ZIP SHA-256
  `32a71bd65b577625dc503ef6a09080e43a6193ef6615b33cb8eff78c7ac67887`.

Both reports show 9/9 functions, 892/892 code bytes, and 1704/1704 data bytes,
with `.text` 960, `.rdata` 160, and `.bss` 1544 complete. Only `report.json`
was uploaded, not the candidate object, so these reports justified reopening
but did not replace local hardened comparison.

The Claude-authored object analyses consulted in full are
`C:/halo-worktrees/opus-thread-win32/CREATE_THREAD_TIE_ANALYSIS.md` and
`C:/halo-worktrees/opus-thread-win32/THREAD_WIN32_CHECKPOINT.md`. The related
Claude repository and tooling guidance reviewed before mutation are
`C:/halo-worktrees/claude-ai-conversation-2/AGENTS.md`, `CLAUDE.md`,
`docs/verification_policy.md`, `docs/references/prototype-inference.md`, and
`docs/references/kb-update-policy.md`. Those documents correctly record the
old source family's terminal NULL-store code-generation tie, approximately
180 exhausted variants, conservative prototype recovery, binary authority,
and the requirement for targeted disassembly and strict validation. The PR
supplied a genuinely new legal-C topology and therefore satisfied the old
ledger's reopen criteria.

The HCEA thread sources and headers under
`research-cache/halocea-cseries-20260820` were used only to authenticate the
32-entry thread and mutex pools, 8-byte thread records, 40-byte mutex records,
word flags, `LPTHREAD_START_ROUTINE`, the unused thread ID, and the 16 KiB
stack. January remains the authority for control flow, names, and ownership.

## Rejected four-file port

The upstream package changed `config/config.json`, `config/symbols.json`,
`source/bungie_net/common/thread.h`, and
`source/bungie_net/common/thread_win32.c`. It was tested once as a frozen
first-shot package. The preserved candidate is
`thread-win32-closeout-20260826/build/audit/thread-win32-closeout-first-shot-20260826/first-candidate.obj`,
SHA-256
`4e2ce03b3001e36745c29cc89d2622fe126546d0bb1038cc5bca8486fe5b2af2`.

That candidate made `_create_thread` exact and reproduced the BSS bytes and
layout, but failed strict owner identity:

- the upstream `_mutex_count`, `_thread_pool`, and `_mutex_pool` definitions
  were source-static, producing COFF storage class 3 instead of January's
  external storage class 2; and
- the mutex allocator expressed its one-past destination through
  `_mutex_pool + 0x524`, while January uses `_bss_0031cd30 + 0x24`.

Those relocations resolve to the same linked address, but strict comparison
does not equate different owners merely because their addresses coincide.
The static globals, helper renames, header replacement, and symbol renames
were therefore rejected rather than admitted through an exception.

## Accepted hybrid and evidence waves

The admitted hybrid retains the cumulative helper names, combined external
`struct thread_globals bss_0031c728`, exact external one-past owner
`_bss_0031cd30`, existing `thread_reference` and `mutex_reference` layouts,
empty header scaffold, and all eight already-exact function bodies.

Only the proven `_create_thread` topology was transferred: `word flags`, the
typed Windows thread procedure, distinct `unsigned long unused_thread_id`, a
combined allocation and `CreateThread` condition, ordinary low/normal/high
priority selection, close-and-null behavior on priority or resume failure,
one shared output write, and one final Boolean return. The January reserved-
slot behavior remains unchanged on failure; this closeout does not introduce
a robustness fix.

### Wave 1: `_create_thread`

The surgical source passed a production-flag `/Zs` parse and was compiled
exactly once. The preserved first candidate is
`thread-win32-create-thread-closeout-20260826/build/audit/thread-win32-create-thread-surgical-20260826/first-candidate.obj`,
SHA-256
`06f12a9673bd9994a73567ecf967924aa28464fd92e3ce2bb64bc1588bce4d23`.
It made `_create_thread` strict exact while retaining the eight cumulative
exact functions and their January relocation owners.

### Wave 2: BSS definition

The first artifact then proved that the remaining data gap was only the
tentative COMMON representation already diagnosed in the Claude checkpoint.
The combined global was changed to the ordinary explicit-zero definition
`= {0}` so this translation unit owns the BSS. This new, data-only packet also
passed `/Zs` and was compiled exactly once. Its preserved first candidate is
`thread-win32-create-thread-closeout-20260826/build/audit/thread-win32-bss-definition-20260826/first-candidate.obj`,
SHA-256
`49629923ad41147690903cacea302b55a36fb2dbb52fbde53b0df415daa5e40`.
All nine function hashes and relocation identities remained unchanged.

## BSS and semantic representation

The final owner is external `_bss_0031c728`, storage class 2, type 0, logical
size 1544, with zero relocations. Its normalized logical-zero SHA-256 is
`3bb37955629333a7bd053bf97693bcd51627120a57695a9cb0989e21ce9bc6c5`.
The aggregate owns the mutex counter at `+0x000`, a four-byte layout gap, 32
thread references at `+0x008`, and 32 mutex references at `+0x108`; it ends at
`+0x608`, immediately before `_bss_0031cd30`.

csplit stores the target's zero payload physically while VC7 emits fileless
BSS. The semantic-data record is limited to that physical-representation
difference. Logical size and bytes, section flags and alignment, external
owner spelling, storage class, and every code destination are exact. It does
not waive the rejected static/external owner mismatch. The seven assertion
and format strings account for the remaining 160 exact data bytes.

## House-rule and policy audit

The accepted source places every parameter on its own line, uses `void` on its
own line for no-argument functions, and gives every void function an explicit
`return;`. It retains typed structures and `LPTHREAD_START_ROUTINE`; no tag or
object-access macro conversion is implicated.

The closeout introduces no assembly, `volatile`, `register`, barrier, pragma,
forced inline, undefined type-punning, raw address, pointer/integer cast, byte
patch, compiler-flag change, or fabricated ownership alias. The retained
legacy one-past helper body was already strict exact and was not edited. The
upstream header was not imported.

## Isolated validation

The ordinary report proves `thread_win32` complete at 9/9 functions,
892/892 code bytes, and 1704/1704 data bytes. Canonical isolated outputs are:

- `build/report.json`: 1,575,048 bytes, SHA-256
  `ecb1060a5e4f7ff5ddf5c74b65fc26da77f0c4f060b7dc51502931390b3956af`;
- `build/semantic_report.json`: 2,987,720 bytes, SHA-256
  `ac400d0d3af2e8d8cfc54a9560407b6011ee5aec30d47fa2cd95a2b2d8521c5b`.

The semantic scan evaluates 4267 functions: 4126 are semantic exact, 114 are
hidden exact for 64,705 bytes, 4187 are accepted exact, and there are zero
unit errors. Campaign progress is 376/833 complete objects, 4161/11060
functions, 502,965/2,198,102 code bytes, and 1,836,756/4,176,062 data bytes.

Admission reports zero candidates and zero revocations, with only the
inherited unrelated `source/shell/shell_xbox` completion-label contradiction.
Parks remain three active, zero stale, and zero invalid. All 179 tooling tests
and 15 selected JSON parses pass. The complete Halo and libcmt object graphs
and the `all_source` dry run report no pending work. `git diff --check`, the
source-policy scan, and the protected-path audit pass; none of the eight Units
package protected paths is changed. No push, history rewrite, or protected
Units edit is part of this closeout.

## Clean committed-state replay

The implementation and admission tip is
`76d2f34a73855fb955c87b473f6d0e64b85f5fc8`. A clean accepted-state
manifest was captured there: 207,627 bytes, SHA-256
`2136be218ed874d257799c03292d8ef482f48ec7cadb89180c35a8029927d2e2`.
It records all nine functions as `STRICT_EXACT`, the seven string sections as
`STRICT_EXACT`, and the BSS as the narrowly identified `SEMANTIC_EXACT`
physical-representation case.

The pre-replay live object was preserved at 4,929 bytes with SHA-256
`49629923ad41147690903cacea302b55a36fb2dbb52fbde53b0df415daa5e40`.
Its resolved path and hash were checked before deletion. A Ninja dry run then
showed exactly one production compiler edge for `thread_win32.obj`; the clean
committed source was rebuilt once and preserved at 4,929 bytes with phase-
specific SHA-256
`baa1df51376e755598a2a358fdfa82740d3b89ef2fbe4005f906de7e9a94a657`.

The whole-TU manifest replay has no failures or warnings, empty
`newly_exact` and `changed_nonexact` arrays, and all nine functions under
`still_exact`. Direct hardened comparison again proves every function in the
table, the exact BSS logical hash and flags, and the normalized BSS symbol
inventory: external `_bss_0031c728` at offset zero plus undefined external
`_bss_0031cd30`. Regenerated ordinary and semantic reports reproduce the
isolated sizes, hashes, progress counts, and zero-error semantic counts above.

This replay-record update changes documentation only. The cumulative
fast-forward and same-path replay are recorded after integration.

## Cumulative integration and replay

The authoritative `jonas/units-integration-20260824` branch was cleanly
fast-forwarded from `6ad4b7185af77f492cb7468b30931c7d2c620f92` through the
thread commits to `2f7a8169`. The thread-specific cumulative-base diff is
exactly `config/config.json`, `config/semantic_data_matches.json`, this ledger,
and `source/bungie_net/common/thread_win32.c`; it has no deletion and no
intersection with the eight protected Units package paths.

The previously stale cumulative object was preserved before replacement:
4,835 bytes, SHA-256
`5c93b00edb416154adce90e4fbb416aa424afbe1f7ca27feec3e2bf13a738edf`.
After configuration regeneration, its resolved path and hash were checked,
the object was deleted, a dry run showed exactly one production compiler
edge, and the accepted source was rebuilt once.

A cross-worktree manifest comparison then intentionally failed closed on the
absolute-versus-relative compiler command spelling and the expected CodeView
source-path record. Direct runtime comparison was already exact, but neither
diagnostic was waived. A clean same-worktree cumulative manifest was captured
at `bd3b48ddbf797ef6b80e627d7a49eb535d75b328`: 207,795 bytes, SHA-256
`993cfffd48d2f02c15d1869375f2caf2857b7934c3abd399488ff17c74d9d547`.

The live preimage for the final same-path replay was preserved at 4,911 bytes,
SHA-256
`f7104ad522ec6228a346e952bd695b4568092e63b71460478735dc8fbc9de895`.
Its path and hash were checked before deletion. The dry run showed exactly one
absolute-path XDK compiler edge, and the final replay object was produced once
at 4,911 bytes with phase-specific SHA-256
`3cf2ca0f8f30d6c3629233e17e7994ea44c63155e16fc19928028b203144b8c0`.
The cumulative same-worktree gate then passed with no failures or warnings,
all nine functions under `still_exact`, and empty `newly_exact` and
`changed_nonexact` arrays. Direct comparison again proves all nine functions,
all 62 relocations, the 1,544-byte BSS logical hash and flags, and the complete
normalized BSS symbol inventory.

While this replay was running, the independently authorized Units shared-
interface package was merged as the second parent of `bd3b48dd`. Its changes
to `source/units/units.c`, `units.h`, `source/game/game_engine.c`, and
`game_engine.h` are not part of the thread wave. The combined cumulative
reports therefore include both accepted advances:

- `build/report.json`: 1,575,192 bytes, SHA-256
  `fa4ffd548c1916868d66650cc7037af9d657ce471964f50412d317ee4e3dc667`;
- `build/semantic_report.json`: 2,991,676 bytes, SHA-256
  `040bb97823d28dc80a4b71190791673ed524aed414b07fee6d49a4aea1179f81`.

Combined cumulative progress is 376/833 complete objects, 4167/11060
functions, 506,149/2,198,102 code bytes, and 1,836,756/4,176,062 data bytes.
The semantic scan evaluates 4272 functions: 4132 are semantic exact, 114 are
hidden exact for 64,705 bytes, 4193 are accepted exact, and there are zero
unit errors. `thread_win32` remains individually complete at 9/9 functions,
892/892 code bytes, and 1704/1704 data bytes.

Admission remains zero candidates and zero revocations, with only the
inherited unrelated `shell_xbox` contradiction. Parks remain three active,
zero stale, and zero invalid. All 179 tooling tests and 15 JSON parses pass;
the complete Halo and libcmt graphs and the `all_source` dry run report no
pending work. The worktree is tracked-clean apart from this additive replay
record. No push or history rewrite was performed.
