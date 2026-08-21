# `ui_widget.obj` Jonas child-count leaf (2026-08-21)

## Result and fixed boundary

This Jonas-owned source-only wave starts from exact canonical commit
`5c8b8b60aad377f230996bc4b6cb1440dfb23710` on isolated branch
`jonas/ui-widget-accessor-trio-20260821`. The first and only natural
code-producing candidate compile contained exactly these three public
ordinary-C bodies:

```c
int widget_instance_count_children(struct widget_instance *widget);
struct widget_instance *widget_instance_get_topmost_parent(
	struct widget_instance *widget);
int widget_instance_get_child_index_from_parent(
	struct widget_instance *widget);
```

`widget_instance_count_children` was independently strict immediately and is
retained. The other two bodies missed, were removed wholesale, and were not
retried or tuned. `ui_widget.obj` therefore advances from 3/102 to 4/102 exact
functions and from 3/23,042 to 32/23,042 meaningful code bytes. It remains
`NonMatching`. The strict net gain is 29 meaningful bytes, 32 padded bytes,
zero relocations, and no data credit.

| Retained body | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_widget_instance_count_children` | `0x000D3430` | `29 / 32` | 0 | `ae15477fb628a00d4f51a719438f430179585e7069e2e464dee76e2b0d39b60f` |

The one-shot rejected results are evidence, not parked or retained code:

| Removed body | Meaningful / padded | Relocations | January hash | One-shot candidate hash |
| --- | ---: | ---: | --- | --- |
| `_widget_instance_get_topmost_parent` | `27 / 32` | 0 | `e37a00fdbeef379adf03454d98aa38a350f86acb27363f974d40f72b5752c58a` | `69c5f654d82e480d40d2135477c33515fb0e97b496286eb56e94af34bddcd7b5` |
| `_widget_instance_get_child_index_from_parent` | `52 / 64` | 0 | `81079871cf6e6b8ab8ee5e80b2e62ef9b8386b364310ca28ab28c4d146b51640` | `b599286d95a9077bf30e98b06bd5c08ea64cca50b9b493d1aac5c90f2dc4a48c` |

The topmost-parent miss differs only in the source register selected for its
second `parent` load. The child-index miss has the same padded size and
relocation shape but a different legal register and early-return schedule.
Neither difference was pursued after the bounded compile.

## Immutable provenance

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/interface/ui_widget.obj`: 111,969 bytes, SHA-256
  `783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`.
- Exact-HEAD baseline `source/interface/ui_widget.c` Git blob:
  `00be5db0409d884e8dff4188c9915f5c90841ed9`, 21,666 payload bytes,
  payload SHA-256
  `8ef1cbac87591828fc18ca5b0b3dc298b7a178f31b49a3814cd4b25ce54c938f`.
- The retained staged translation-unit payload is Git blob
  `7b07bd7e1dd54672d8a0b8a13e512c883978d656`, 22,270 bytes, with SHA-256
  `b3edac25ffbe2d0cf7a3252fb37560e337923ea36a7d1a06268023efd895abfd`.
  The clean committed-state replay below will authenticate it again from the
  implementation commit rather than treating staged evidence as committed.
- Public `source/interface/ui_widget.h` is unchanged at blob
  `2155a5a4d1bfbb9a5e209557da01a2e189b2a207`, 4,165 bytes, payload SHA-256
  `62614b4c834f1a0fed3f66b3787ce88816924d7900fb3b23e28148d600344c5c`.
- The first three-body candidate object was 1,814 bytes with SHA-256
  `3b9cbb8217e296921b13a01a7cce2b9ecb3c68d9ed4b20d60fa4b9dd8edc2062`.
  The retained pre-commit object is 1,449 bytes with SHA-256
  `5d717663473f6d8a7ead7b57f289f5fdbd88ba796a0814d95260f075c60b68f5`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The repository's unchanged natural flags are `/O2 /Oy- /DDEBUG /Dxbox`.

The clean local HCEA source authority is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Every cited file is directly
tree-reachable from that commit; hashes below cover exact Git-blob payloads.

| HCEA evidence | Git blob | Bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| child-count body | `1b2bd8f2d53f548623c447b197322a7e07e5a05f` | 389 | `d79b19a146d8b3f51c36ef2df831da3fd5109361d8426cda3409c7f19739f3c6` |
| topmost-parent rejected body | `f9cec202d6aa87a6ed24eac49b1b64573eaf88b8` | 327 | `e9d73a2010e6ce4f43c8d2f2677a398ed881301c5f0413b25c4ebbb840f67c11` |
| child-index rejected body | `d2145a70a97b21d0501aec10fa4df6e4b1148421` | 449 | `73d0e88709f8f04a5701bf40cd7c4886ee83eaf4516cc0e5aa21a58ae8122a46` |
| widget-instance layout | `bc3768fbe77d14f223f8c650689b0d4724741404` | 2,650 | `3426b8f6ef249be0be3168f84b552371ef72382732626f313b54b4b3d331fe27` |

HCEA establishes readable source intent and field identity. January PC COFF
remains the byte, ABI, padding, and acceptance authority. No Stian source or
reconstruction was needed.

Canonical history contains no earlier committed definition of any screened
body and no matching ledger for this set. `git log --all -S` finds each name
only in the initial symbol-list/header text. The recent `ui_widget.c` change
`f6a5e7d7` added only the disjoint exact no-op bodies `_code_000d4680` and
`_code_000d4690`. Four registered July/August widget worktrees were clean and
stale, all had the same old `ui_widget.c` blob, and none contained a screened
body. No active collaboration lane owned this translation unit at dispatch.

## Typed ABI, callers, and layout

The three January symbols are external COFF function symbols of type `0x20`
and storage class 2. Their leading-underscore names have no stdcall suffix.
Each reads its sole pointer argument from `[ebp+8]`, returns through `EAX`, and
ends in plain `ret`, proving ordinary cdecl rather than a private or register
ABI.

January has no direct caller of the retained child-count leaf. Its public
header declaration and complete callee establish the contract independently.
For the screened rejects, topmost-parent has REL32 callers from
`ui_widget.obj:_code_000d7480+0xDB` and event-handler functions
`_code_000d9550+0xDF`, `_code_000de190+0x9A`, `_code_000de630+0x61`, and
`_code_000dfb50+0x1B`; child-index is called from
`_code_000d9550+0xF8`. Current reconstructed event-handler source uses the
same typed pointer/result signatures.

The committed `source/interface/ui_widget_event_handler_functions.c` blob
`c0af6f6d425958500e02672a73f4608771801ad2` is 207,676 bytes with payload
SHA-256 `2bb6cea45ce598e6224dbab66ff68549f07a1b9c0c6c2d52a5a4d73c55c13654`.
Its readable `struct widget_instance` owns `next` at `+0x2C`, `parent` at
`+0x30`, and `child` at `+0x34`, exactly agreeing with the c168 layout blob
and January loads.

The retained source copies only that readable typed prefix into
`ui_widget.c`. A translation-unit-local forward declaration before
`ui_widget.h` makes the existing public prototype and later complete prefix
share one tag without changing the public header. Child count initializes an
ordinary `int`, follows `widget->child`, and advances only through
`child->next`. It returns zero for a null widget. No cast, offset literal,
overlay, pun, raw load, helper call, global, or storage access is involved.

## One-shot and storage boundary

The complete typed prefix and all three natural donor bodies were fixed before
emission. A VC7 `/Zs` parse-only prerequisite pass exited zero and was proven
not to create `ui_widget.obj`. Exactly one natural three-body compiler action
then ran. Child count was strict in that object; both misses were removed in
full. There was no alternate spelling, declaration, local lifetime,
expression, control flow, compiler option, or retry.

The retained object defines exactly four external code owners: the new leaf
and inherited `_ui_widgets_safe_to_load`, `_code_000d4680`, and
`_code_000d4690`. It also preserves the sole inherited runtime non-code owner,
two-byte `.data` symbol `_dashboard_abort_error`. It defines no new writable
storage or runtime constant and has no undefined external. Relative to the
exact-HEAD baseline object, `.data` remains two bytes with zero relocations and
SHA-256 `ca2fd00fa001190744c15c317643ab092e7048ce086a243e2be9437c898de1bb`;
`.drectve` remains 42 bytes with SHA-256
`1bf8b9d961b573933e6b8f24821b55ae9c81a53e2c2e7fa81841fd557014cad8`.
Only compiler debug/section metadata and the accepted code COMDAT change. No
target data is claimed.

The retained source contains no assembly, raw address or byte-offset
dereference, pointer/integer reconstruction, cast overlay, pointer or union
pun, inactive-union access, undefined signed overflow, representation trick,
private/register ABI, volatile scheduling device, force-inline annotation,
optimizer pragma or barrier, synthetic anchor, or byte-forcing expression.

## Pre-commit validation

Only `source/interface/ui_widget.c` and this new Jonas ledger are tracked
changes. No shared header, storage, configuration, semantic exception, parked
record, completion label, existing Markdown, or tracked path deletion is
changed. The protected five remain untouched.

- Full `halobetacache_build` plus `libcmt_build`: pass, 569/569 fresh-worktree
  Ninja actions. There is no candidate-source warning.
- Direct hardened comparison: the retained leaf and all three inherited
  bodies are independently strict. The two rejected symbols are absent.
- Semantic audit: 470 units, 4,152 functions evaluated, 3,577 missing-base
  symbols, 4,012 semantic exact, 101 hidden exact / 61,876 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 local symbols
  skipped, 4,073 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 4,060/11,060 exact functions,
  482,602/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched
  data bytes.
- Halo progress: 273/468 objects, 3,893/7,574 functions,
  469,688/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- `ui_widget.obj`: 4/102 functions and 32/23,042 meaningful code bytes. It
  remains nonmatching and claims no data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, changed-path, deleted-path, source-policy, protected-five,
  storage, and rejected-body absence checks are clean.

The clean committed-state snapshot, forced object deletion/rebuild, regression
check, direct comparator, ownership replay, and committed source-payload
authentication will be recorded additively only after the implementation and
this ledger are committed. No push, amend, or history rewrite is performed.

## Clean committed-state replay

Implementation-and-ledger commit
`c310c6b79c1c3e9d777912427eaf7f477ba1585a` was clean before replay. Its
committed `source/interface/ui_widget.c` payload is exactly Git blob
`7b07bd7e1dd54672d8a0b8a13e512c883978d656`. Re-reading that blob from the
commit with `git cat-file blob` produced 22,270 bytes and SHA-256
`b3edac25ffbe2d0cf7a3252fb37560e337923ea36a7d1a06268023efd895abfd`.
Thus every changed source payload is authenticated from committed Git state,
not from working-tree bytes. The shared header remained exact baseline blob
`2155a5a4d1bfbb9a5e209557da01a2e189b2a207`.

A one-unit regression snapshot was written from that exact clean commit to
ignored artifact `build/regression_ui_widget_child_count_20260821.json`. It
reported `SNAPSHOT_WRITTEN`, recorded commit
`c310c6b79c1c3e9d777912427eaf7f477ba1585a`, and contained only
`source/interface/ui_widget`.

The candidate object path resolved to
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\ui-widget-accessor-trio-20260821\build\base\source\interface\ui_widget.obj`.
The resolved path was explicitly proven to begin beneath the isolated
worktree root before deletion. The object was 1,449 bytes with pre-replay
whole-file SHA-256
`5d717663473f6d8a7ead7b57f289f5fdbd88ba796a0814d95260f075c60b68f5`.
That single file was deleted, and a second existence check returned false.

The untouched production Ninja graph then rebuilt the same path through one
`[1/1] CL` action with the unchanged natural compiler, flags, and include
graph. There was no source change, rejected-body resurrection, candidate
retry, or alternate build edge. The immediate regression check returned
`ok: true`, no failures, no warnings, `changed_nonexact: []`, and
`newly_exact: []`. Its `still_exact` set is exactly
`_ui_widgets_safe_to_load`, `_widget_instance_count_children`,
`_code_000d4680`, and `_code_000d4690`.

Direct hardened January comparison after replay returned `all_equal: true`
for that complete four-body inventory. It reproduced the new leaf's 29
meaningful bytes, 32 padded bytes, zero relocations, and normalized SHA-256
`ae15477fb628a00d4f51a719438f430179585e7069e2e464dee76e2b0d39b60f`,
as well as all three inherited no-op bodies. The rebuilt object remains 1,449
bytes with phase-specific whole-file SHA-256
`451639d39eea939b37a7dfd1990058c2554c5a704131f90b73dbd95ca666fb10`;
the whole-file difference is expected compiler header metadata and is not an
acceptance criterion. The January object remained SHA-256
`783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`.

An independent post-rebuild COFF ownership parse found exactly the four
documented external code owners plus inherited two-byte data owner
`_dashboard_abort_error`. It found no other defined runtime owner and no
undefined external. The only final tracked change after replay is this
additive ledger section. Nothing is pushed, amended, or history-rewritten.
