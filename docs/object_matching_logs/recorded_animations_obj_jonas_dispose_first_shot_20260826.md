# `recorded_animations.obj` Jonas disposal first shot (2026-08-26)

## Frozen pre-production packet

This bounded wave starts from clean cumulative commit
`278d55f9e57f8ccac6d5ced80c22fa29db688f79` on isolated branch
`jonas/recorded-animations-dispose-20260826`. It is intentionally outside the
approval-gated Units/shared-header package and changes no protected path.

The selected owner is `_recorded_animations_dispose`. It is absent from the
current base, is final-link closed, and is the first missing direct dependency
needed by the otherwise source-authenticated `_game_dispose` chain. The frozen
source packet is ordinary typed C:

```c
extern void *animation_threads_debug;

void recorded_animations_dispose(
	void)
{
	if (animation_threads_debug)
	{
		match_free(
			"c:\\halo\\SOURCE\\cutscene\\recorded_animations.c",
			123,
			animation_threads_debug);
		animation_threads_debug = NULL;
	}

	return;
}
```

No header, storage definition, build file, semantic exception, or parked
record is in scope. Production source scope is limited to
`source/cutscene/recorded_animations.c`. The missing public header prototype is
deferred to the future `_game_dispose` caller wave, where its consumer can be
frozen coherently. The authenticated target-symbol label
`_animation_threads_debug` is added at decimal file offset `4414636` in
`config/symbols.json`; this names the existing target-owned BSS slot without
changing linked runtime bytes or creating source-owned storage. This ledger is
the only other tracked path in the wave.

## Baseline and target authority

- Baseline source Git blob:
  `1da9a38950def1486c043e8d15c5a2bb773097fe`.
- January split `recorded_animations.obj` SHA-256:
  `cef157f5d4aa20f06b5e14dc659f44cc75ba29d11274faa4ec74166ee51990f0`.
- Untouched branch-local baseline object SHA-256:
  `b0c18d6ca612bdfa65e3c557957af8cfa1dd1f632a652b0d2fbd0b0cf936f0db`.
- Pre-symbol frozen regression manifest:
  `build/audit/recorded-animations-dispose-first-shot-20260826/pre-symbol-manifest.json`,
  SHA-256
  `4468c47a3e1263057e593d9b102910e0cc5791beaccb49a95018c889f3654855`.
- Label-normalized pre-source regression manifest at clean preflight commit
  `1b41ef13523486ea09ce04500ff76b574ed00d41`:
  `build/audit/recorded-animations-dispose-first-shot-20260826/pre-manifest.json`,
  SHA-256
  `3ebbdc32be3ccbaaa70c302bf8170939b4d8721c8f0537ec9b8d2bfc1dfad270`.
- Label-normalized January split object SHA-256:
  `e21a14fcbba6f5ccbcad6dc97e8d195c29dba6113ef543e18aebbddcf3f8a5a9`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1 for
  80x86, `CL.Exe` SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

Before the authenticated label is added, csplit represents the pointer slot at
absolute `0x435CAC` as `_debug_recording+4`. January initialization, the
`animation_threads_debug` assertion literal, the HCEA knowledge base, and the
same-build reference all authenticate that slot as the distinct pointer
`animation_threads_debug`. Regenerating only the split target after the label
addition must preserve every byte while spelling both pointer relocations as
`_animation_threads_debug+0`.

The target owner is 36 meaningful / 48 padded bytes with four relocations and
normalized byte SHA-256
`b9b64bb030325c73231a51ab101f24b1d8ce3ec37789c76d2686751f0cc07ed0`:

| Offset | Type | Destination |
| ---: | --- | --- |
| +1 | DIR32 | `_animation_threads_debug` (pre-label csplit spelling `_debug_recording+4`) |
| +12 | DIR32 | `??_C@_0CO@FNHDDCJP@c?3?2halo?2SOURCE?2cutscene?2recorded@` |
| +18 | REL32 | `_debug_free` |
| +27 | DIR32 | `_animation_threads_debug` (pre-label csplit spelling `_debug_recording+4`) |

The target literal is exactly
`c:\halo\SOURCE\cutscene\recorded_animations.c`. The target line argument is
`123` (`0x7B`). The existing `match_free` macro expands to the declared `_debug_free`
call without any compiler control. `_debug_free` is already accepted strict
exact, so this owner has complete direct-callee closure. The retained
`_recorded_animations_dispose_from_old_map` is already strict exact at 13
meaningful / 16 padded bytes, two relocations, and normalized SHA-256
`9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc`.

Semantic source corroboration is the HCEA donor at commits `f0b06e55` and
`61165c93`, which authenticates the `animation_threads_debug` name and natural
free/null body. The clean Stian donor at commit
`41c10616b69b982700e0913f21a5137807a03d0e` supplies corroborating source at
`research-cache/stian-halo-cseries-20260820/src/halo/cutscene/recorded_animations.c`.
January PC COFF/disassembly remains byte and relocation authority.

## One-shot and acceptance boundary

The pre-symbol snapshot succeeded before any tracked change. The authenticated
split label was then regenerated and proven to preserve every normalized code
section hash and relocation count while respelling twelve relocations across
seven functions from the same absolute `_debug_recording+4` destination to
its correct `_animation_threads_debug+0` identity. A second clean pre-source manifest
froze that label-normalized target before any production source change. The
frozen source will receive one real VC7 `/Zs`
parse-only check, which emits no object, followed by exactly one normal Ninja production compile of
`build/base/source/cutscene/recorded_animations.obj`. The first generated
object will be preserved immediately under the ignored audit directory before
comparison. There will be no spelling, source-shape, control-flow, or tuning
retry in this wave.

Retention requires all of the following:

1. `_recorded_animations_dispose` has exact padded bytes and complete
   relocation address/type/destination/addend identity;
2. `_recorded_animations_dispose_from_old_map` remains strict exact;
3. the emitted path literal has exact content/owner identity;
4. the pre-wave regression check has an empty `changed_nonexact` list and its
   only newly exact owner is `_recorded_animations_dispose`; expected physical
   COMDAT, debug, section-index, and symbol-inventory changes caused by adding
   that owner must be explained and every inherited accepted function must be
   proved directly exact; and
5. a later clean committed-state snapshot and replay report no failure or
   warning, with both accepted functions under `still_exact`; and
6. source-policy, scope, build, semantic, admission, park, test, and replay
   gates all pass.

A strict comparison or inherited-evidence failure rejects the production
source edit wholesale. The generic pre-wave gate is deliberately fail-closed
on physical translation-unit expansion, so its reviewed structural result is
not treated as an adjudication or exception. The first artifact and this
ledger record the result, and no second candidate compile is allowed without a
genuinely new evidence-backed wave.

The body uses readable typed C with `void` on its own parameter line, an
explicit terminal return, a direct typed external, and the repository memory
interface. It contains no assembly, `volatile`, `register`, barrier, pragma,
intrinsic, attribute/compiler control, raw address or byte offset,
pointer/integer or union pun, undefined behavior, synthetic anchor, or object
byte forcing.

## Result

The frozen packet matched in the first and only production candidate compile.
`recorded_animations.obj` advances from 1/16 to 2/16 strict functions and from
13/2,684 to 49/2,684 meaningful code bytes. It remains `NonMatching` and
receives no matched-data credit: the report records 0/894 matched data bytes.

| Strict function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_recorded_animations_dispose` | 36 | 48 | 4 | `b9b64bb030325c73231a51ab101f24b1d8ce3ec37789c76d2686751f0cc07ed0` |
| `_recorded_animations_dispose_from_old_map` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| **Strict total** | **49** | **64** | **6** | |

The exact 46-byte source-path COMDAT has zero relocations and normalized
SHA-256
`f198026ae4ef733095b3ce37b1c45da8073398075268c62d147a465dcd3f5c48`.
It is direct COFF evidence for the call-site owner and literal spelling; it is
not a semantic-data exception and grants no data-byte credit.

## First-shot discipline and strict evidence

The retained source passed a real VC7 production-flag `/Zs` parse-only check.
The baseline object remained byte-for-byte unchanged with SHA-256
`b0c18d6ca612bdfa65e3c557957af8cfa1dd1f632a652b0d2fbd0b0cf936f0db`
after that non-emitting check. The selected-object dry run named exactly one
compile edge, and the ordinary production edge then ran once:

```text
[1/1] CL build\base\source\cutscene\recorded_animations.obj
recorded_animations.c
```

The first generated object was preserved immediately at
`build/audit/recorded-animations-dispose-first-shot-20260826/first-candidate.obj`.
It is 1,439 bytes with SHA-256
`e7c394f65485c8c0d850604b5cb1ce4444d36b77628bb0c81b7aebab3daf0863`.
There was no declaration, spelling, control-flow, source-shape, or compiler-
tuning retry.

Direct hardened comparison proves both complete padded function sections,
their normalized bytes, and all ordered relocation address/type/destination/
addend tuples. `_recorded_animations_dispose` has DIR32 relocations at `+1`
and `+27` to `_animation_threads_debug`, a DIR32 relocation at `+12` to the
exact path COMDAT, and a REL32 relocation at `+18` to `_debug_free`.
`_recorded_animations_dispose_from_old_map` retains its DIR32 relocation at
`+1` to `_bss_00435ca4` and REL32 relocation at `+7` to
`_data_make_invalid`. The direct path-COMDAT comparison is independently
equal. `_debug_free` is already accepted strict exact, so final-link semantic
closure is complete under the established target-owned-data model.

The generic label-normalized pre-wave manifest check exits closed with
`_recorded_animations_dispose` as the sole `NEWLY_EXACT` owner and an empty
`changed_nonexact` array. Its five expected physical-expansion failures are:

- accepted-evidence/index change for
  `_recorded_animations_dispose_from_old_map`;
- `.debug$F|anonymous=1` and `.debug$F|anonymous=0` changes;
- insertion of the exact path `.rdata` COMDAT; and
- symbol-set change.

Direct comparison proves the retained old-map function unchanged. The target
and base runtime `.bss` and initialized data ownership receive no new source
owner or credit. No regression adjudication, semantic exception, comparator
change, or parked entry was added. A clean post-commit manifest is required
for the warning-free reproducibility replay below.

## Retained source and policy

The retained C blob is `cf840f9dfb7a70c5b80a1cd6dd4534b778bb8898`.
Its canonical CRLF form is 3,593 bytes with 135 CRLF endings, zero bare LF
endings, and SHA-256
`f82995c3ea089387b7fa1612ec9b1b162b5c4ec8bfcc1ec7ed641a1d41e78150`.
The untouched header remains Git blob
`9f1c3375bf1c4ce0ae4594de8c2195b124006dbe`; its 440-byte physical form has
SHA-256
`ee5e708f08eeb13dc7a611388b85cefd5eb53eca2b25247758167067c1c9e4ab`.

The retained definition is the frozen packet above. It is readable typed
C89-style code and contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute/compiler control, barrier, raw address or raw byte
offset, pointer/integer reconstruction, cast or union pun, undefined behavior,
synthetic anchor, object-byte forcing, or source-owned runtime storage. The
header remains untouched so the strict-complete `game_state.obj` consumer is
not rebuilt prematurely.

## First-shot validation

The complete Halo and libcmt object graphs pass, and they plus the
`all_source` dry run report no pending work. Canonical outputs are:

- `build/report.json`: 1,574,896 bytes, SHA-256
  `0a750e16200e329ee8196686c843b3d59338719528deb8f00b72d28880e1f2f5`;
- `build/semantic_report.json`: 2,987,040 bytes, SHA-256
  `0fd25c994525fa6d318a805cffe17ebb1e3c1cf431354e0e7a6e412e99799298`.

Strict campaign progress is:

- all categories: 375/833 complete objects, 4,160/11,060 exact functions,
  502,728/2,198,102 meaningful code bytes, and
  1,835,212/4,176,062 data bytes;
- Halo: 273/468 complete objects, 3,993/7,574 exact functions,
  489,814/1,770,166 meaningful code bytes, and
  1,830,020/3,923,451 data bytes;
- libcmt: 102/212 objects, 167/476 functions, 12,914/55,015 code bytes,
  and 5,192/8,637 data bytes.

The semantic audit scans 470 units and evaluates 4,267 functions: 4,125
semantic exact, 114 hidden exact / 64,705 hidden code bytes, 36 ordinary-only,
35 structural, one rejected, 12 local skips, 3,462 missing, 4,186 accepted
exact, and zero unit errors.

Admission is zero candidates and zero revocations, with only the inherited
unrelated `source/shell/shell_xbox` completion-label contradiction. Its
384-byte JSON has SHA-256
`f0aa45c22cc604fb74abc388b06dd79a27e7a0b498a5d28837a9e785680c9a36`.
Parks remain three active, zero stale, and zero invalid; the 3,950-byte JSON
has SHA-256
`cfac03aea5be607c19a6a40bcdbc1164e71ffb0179602af4bd37b3fd50e26d1d`.
All 179 tooling tests pass, and all 14 selected tracked/configuration/
canonical-report JSON documents parse.

Across the complete wave, tracked scope is exactly `config/symbols.json`,
`source/cutscene/recorded_animations.c`, and this ledger. There are no
deletions. The complete `source/units/**` tree and all eight protected paths
have zero intersection with the wave. `git diff --check` and the source-policy
scan pass. No push, amend, rebase, history rewrite, or worktree removal is
performed.

## Clean committed-state replay

The implementation-and-ledger commit is
`8abd4eada92f3b56a9f476418a5e45ee0e95b0dc`, authored and committed by
Jonas Volman. It follows the separate authenticated-symbol preflight commit
`1b41ef13523486ea09ce04500ff76b574ed00d41`, retains C blob
`cf840f9dfb7a70c5b80a1cd6dd4534b778bb8898`, leaves the public header
unchanged, and was tracked-clean before replay.

A fresh accepted-state manifest was captured at that clean commit. It is
292,054 bytes with SHA-256
`d3c335aa7622a193ed8d913499d656bae26a32c5f3f4958d208a96fb1b592717`.
Its immediate check has no failures or warnings, empty `newly_exact` and
`changed_nonexact` arrays, and exactly the two strict-table functions under
`still_exact`.

The post-validation object was preserved as `pre-committed-replay.obj`:
1,439 bytes, SHA-256
`e7c394f65485c8c0d850604b5cb1ce4444d36b77628bb0c81b7aebab3daf0863`.
The resolved production object path was proved identical to the expected
absolute path and inside the isolated worktree. That literal file was deleted,
and absence was verified. The selected-object dry run named one edge, and the
unchanged committed source then rebuilt through exactly one ordinary action:

```text
[1/1] CL build\base\source\cutscene\recorded_animations.obj
recorded_animations.c
```

This is a reproducibility replay, not a candidate or tuning pass. The 1,439-
byte committed replay object has phase-specific SHA-256
`20cbac9f51003a72f95bf49fdd38099d2d4a87bfaa78ebf2fe492271915e9a6b`.
Direct hardened comparison again proves both complete padded functions, their
six ordered relocations, and the exact path COMDAT with every normalized hash
in the strict tables above. The differing whole-file COFF hash is confined to
phase-specific compiler/debug metadata and changes no admitted evidence.

The accepted-state manifest check after replay again has no failures or
warnings. It retains both functions under `still_exact` and empty
`newly_exact` and `changed_nonexact` arrays. The label-normalized split target
is 12,543 bytes with SHA-256
`e21a14fcbba6f5ccbcad6dc97e8d195c29dba6113ef543e18aebbddcf3f8a5a9`.

The canonical report and semantic-report sizes, hashes, object/function/byte
counts, and semantic counts reproduce the first-shot values. Admission
reproduces zero candidates / zero revocations plus the inherited unrelated
`shell_xbox` contradiction and the same 384-byte file/hash. Parks reproduce
three active / zero stale / zero invalid and the same 3,950-byte file/hash.
All 179 tooling tests and all 14 selected JSON parses pass again. The complete
Halo-plus-libcmt graph and `all_source` dry run report no pending work, and the
tracked worktree is otherwise clean before this additive replay record.
