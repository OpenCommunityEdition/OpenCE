# `cseries.obj` Jonas typed-cursor one-shot (2026-08-25)

## Scope and frozen provenance

This bounded source-only one-shot starts from clean cumulative integration
commit `37e36a8ead9a36d9d28ce535f91d5f3c84508019` in the isolated worktree and
branch `jonas/cseries-two-cursor-20260825`. The sole permitted production
source is `source/cseries/cseries.c`; the only other initially permitted
tracked path is this new Jonas ledger. Exactly one fixed natural-C candidate,
C1 below, may receive one ordinary code-producing compile. There is no source
spelling change, retry, tuning pass, alternate candidate, compiler change, or
header edit after that emission.

The immutable January executable is `cachebeta.exe`, SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
The pinned compiler is XDK 3911 `CL.Exe` 13.00.9254.1, SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
using the repository's unchanged `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` edge.
Pinned csplit is SHA-256
`708f957c37caa66df90ade35ad3d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`;
objdiff-cli is SHA-256
`090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

The authoritative baseline source blob is
`439de05852fb8e1eeb71caabef542e010cf04de0`. The fresh baseline object's
phase-local raw SHA-256 is
`6bcab2af0e5a7edc740c216caf22bb0b01ae5b4dd6d2348cd58be77447821bbe`;
the fresh split object's phase-local raw SHA-256 is
`ad10d47ea181300e3101524c27af9dc930e9411eb6406c878a12ef0fbd8499cc`.
Those whole-object digests are artifact sentinels only because COFF timestamps
and debug paths are worktree-local. Section fingerprints and ordered
relocations are the strict authority.

Before this ledger or any candidate source edit was created, a clean
whole-translation-unit regression snapshot was written at
`build/regression_manifest.json`. It records commit `37e36a8e` and only unit
`source/cseries/cseries`.

## Fresh strict baseline

The object has 25 of 26 target functions strict exact. The sole residual is
`_csstrcasecmp`: target and baseline are both 192 padded bytes; January has
187 meaningful bytes followed by five padding bytes. Both have 11 relocation
entries at decimal offsets `23, 28, 38, 43, 48, 55, 62, 75, 88, 121, 134`,
with identical type, destination, and addend. The target normalized SHA-256 is
`9fb7ecf4631bd2d54ae281b836b5f974242c9527483666ccd00a04fefce84efa`;
baseline is
`65cce7e90a3feca7ef0c315cf5dd3be00f6e8c89842f5aad2b7362f739a2b4f4`.
The first real source error is comparison polarity. After the target-backed
`c1 != c2` correction, the known intermediate fingerprint is
`cbae75b4d55e41a8e8b1ba0f62837c92e20e6d095f39005e199b9c238aa14f8d`;
only the `c1: EDI/ESI` and second-string cursor `ESI/EDI` allocation mirror
then remains.

All other 25 functions are strict exact. All 20 January runtime non-code
owners are also strict exact, 926 bytes total:

- `_global_real_argb_color_table`: 272-byte `.rdata`, zero relocations;
- 18 owned/select-any string COMDATs: 518 bytes total, zero relocations;
- `_global_real_argb_white`: 136-byte `.data`, 34 ordered relocations.

The raw `.rdata` section-symbol alias is not a unique owner and is not used as
an acceptance identity. Compiler-only `.drectve` and `.debug$S` sections do
not replace the runtime owner census.

## Claude/Codex documentation and exact-donor reconciliation

The complete earlier evidence was read before freezing C1:

- `work/cseries-full-admission/docs/object_matching_logs/cseries_obj.md`,
  including its imported `claude/csstrcasecmp-15` results;
- `research/single_function_residual_diagnoses.md` at commit `32245e36`;
- the `csstrcasecmp` case in `docs/matching_methodology.md`;
- the later exact closeout at commit `8d261db5` and its new ledger
  `docs/object_matching_logs/cseries_obj_codex_closeout.md`.

Claude/Codex already measured comparison polarity, removal/addition of the
`register` hint, three declaration orders, integer-width alternatives,
short- and full-lived aliases of `s2`, cursor lifetime partitions, integer
subtraction spellings and directions, loop-increment placement, post-assert
scope, explicit versus macro assertion spelling, and VC7 13.00.9210 versus
13.00.9254.1. None of those is repeated.

Commit `8d261db5` is decisive machine-code provenance: its source blob
`14314637696221a94f0b35f5ccd14f4d831056f7` places a typed second-string
cursor's first definition immediately before the invariant address delta and
emits all 192 target bytes, all 11 target relocations, and target normalized
SHA-256 `9fb7ecf4...`. It cannot be transplanted under current policy. It keeps
the obsolete `register` spelling and computes/carries the offset through
pointer-to-`unsigned long` address arithmetic and a cast-back dereference.
This wave therefore treats it as topology evidence, not admissible source.

The earlier ledger dismissed a natural two-pointer loop as ungrounded but did
not preserve a compiled measurement for one. Every locally materialized
reachable-history `cseries.c` blob and every surviving worktree source copy
was searched: none contains `s1++`, `++s1`, `s1_cursor`, `cursor1`, or a loop
that dereferences a separately advancing first-string cursor. C1 is therefore
genuinely new against all locally available Claude/Codex source and durable
experiment records. Unavailable partial-clone promisor objects are not
silently treated as proof; any recovered contrary record would fail this wave
closed.

## Frozen one-shot C1

C1 makes exactly these changes inside `csstrcasecmp`:

```diff
-\tregister int c1;
+\tint c1;
 \tint c2;
-\tint offset;
+\tconst char *s1_cursor;
+\tconst char *s2_cursor;
@@
-\toffset = s1 - s2;
+\ts1_cursor = s1;
+\ts2_cursor = s2;
 loop:
 \tif (c2 == 0)
 \t\tgoto c2_zero;
-\tif (c2 != c1)
+\tif (c1 != c2)
 \t\tgoto not_equal;
-\ts2++;
-\tc1 = towlower(s2[offset]);
-\tc2 = towlower(*s2);
+\ts1_cursor++;
+\ts2_cursor++;
+\tc1 = towlower(*s1_cursor);
+\tc2 = towlower(*s2_cursor);
```

The cursor definitions intentionally replace the old offset assignment, at
the exact lifetime boundary where the historical exact donor proved the
second-string cursor selects EDI and `c1` selects ESI. C1 expresses the same
lock-step traversal with two typed pointers. The evidence-backed prediction is
that VC7's induction-variable elimination retains one advancing cursor and
forms the invariant base delta, reproducing January's `[ebx+edi]` address
without pointer subtraction, integer address arithmetic, or undefined
cross-array subtraction.

Expected strict result: `_csstrcasecmp` 187 meaningful / 192 padded bytes,
11 relocations at the baseline addresses, normalized SHA-256 `9fb7ecf4...`;
all 25 siblings and all 20/926 runtime data owners unchanged. The potential
gain is 187 meaningful and 192 padded code bytes, completing the object at
26/26 functions. This is a frozen prediction, not an equality claim.

## Policy and house-style audit

C1 is readable, defined C. It uses no assembly, volatility, compiler barrier
or intrinsic, optimization pragma, attribute, forced/no inline, `register`,
raw offset/address, pointer or union pun, inactive-union access, undefined
behavior, synthetic anchor, tool/flag change, object-byte patch, comparator
exception, or fuzzy-only credit. Parameters remain one per line. It touches no
tag/object access and no header, storage owner, Units source, protected Units
neighbor, or deferred Units shared-header package.

The function retains its existing `goto` topology and three explicit returns.
January's back-edge and three distinct return paths prove this is the
documented exactness exception to the preferred single-return rule. No other
function is changed.

## Acceptance, rejection, and closeout gates

After this ledger and exact C1 source exist, the real XDK compiler receives a
`/Zs` syntax-only invocation that emits no object. Exactly one ordinary Ninja
edge may then compile `build/base/source/cseries/cseries.obj`. The first-shot
object is preserved unchanged under ignored `build/audit/` before inspection.

Acceptance requires all of the following:

1. `_csstrcasecmp` exact 192-byte normalized body, target hash, and all 11
   relocation addresses/types/destinations/addends.
2. All 25 inherited exact siblings remain exact.
3. All 20/926 runtime data owners, their storage classes, and the 34 writable
   data relocations remain exact.
4. The frozen regression snapshot reports 25 `still_exact`, only
   `_csstrcasecmp` `newly_exact`, no changed nonexact function, no data or
   symbol-ownership drift, and no failure.
5. Only after those checks, `config/config.json` may change this unit from
   `NonMatching` to `Matching`; no semantic exception or parked entry is
   permitted.
6. Full Halo and libcmt builds, fresh objdiff/semantic/progress reports,
   semantic/admission/parked audits, all 179 tooling tests, policy/scope scans,
   and `git diff --check` must pass.
7. A clean literal object deletion/rebuild must reproduce the first-shot
   section evidence from the committed implementation, followed by a
   corrected-HEAD replay and final Ninja dry run.

If any strict C1 check misses, the complete inverse patch is applied at once,
with no compile, spelling, tuning, or alternate retry. The source must return
to blob `439de058...`; configuration and matching metadata remain unchanged;
only this ledger is updated and committed as a rejected one-shot record. No
amend, push, rebase, or worktree removal is authorized in either outcome.

## Production result

The real XDK `/Zs` pass succeeded and emitted no object; the existing baseline
object's hash and timestamp were unchanged. Exactly one ordinary Ninja edge
then compiled C1. Its unchanged first-shot object is preserved at the ignored
path `build/audit/cseries-two-cursor-first-shot/cseries.obj`, with phase-local
raw SHA-256
`4b818cff128ad406132a86cf8d268a432b84a557ccc0b7aea4065b1ea8be54a7`.
The emitted C1 source blob was
`c6e97be15033b7a01a148d281568ba72acdf6b86`.

C1 did not reach strict equality. It retained the correct 187 meaningful / 192
padded size and 11 relocations, but emitted normalized SHA-256
`8f1984c1a2e68edb4bb09c2a9b8ae8eedd59e348f91d4c735e59deee14fa7d18`
rather than target `9fb7ecf4...`. The first eight relocation entries through
the two initial `towlower` calls remain exact. The first loop `towlower`
relocation is one byte early, candidate `+120` versus target `+121`; the final
call realigns and remains exact at `+134`.

The failure is bounded to the induction-variable anchor selected by VC7:

```text
target +0x70: movsx dx, byte ptr [ebx+edi+1]
target +0x76: inc   edi
target +0x78: call  towlower          ; relocation operand +121
target +0x80: movsx ax, byte ptr [edi]

C1     +0x70: movsx dx, byte ptr [edi+1]
C1     +0x75: inc   edi
C1     +0x77: call  towlower          ; relocation operand +120
C1     +0x7f: movsx ax, byte ptr [ebx+edi]
```

Thus C1 does create the target `c1=ESI` allocation and the one-time
`sub ebx,edi`, but VC7 retains the first-string cursor as the advancing EDI
induction variable and reconstructs the second stream from the invariant
delta. January retains the second-string cursor and reconstructs the first
stream. The five-byte first C1 load and five-byte second C1 load exchange the
target's six-byte/four-byte lengths, so the final call and all tails realign;
the normalized bodies still differ.

The complete candidate-object audit proved all 25 inherited sibling functions
strict exact. All 20/926 runtime data owners remained exact, including all 34
relocations in `_global_real_argb_white`. A no-build comparison against the
pre-emission regression snapshot returned `ok: true`, 25 `still_exact`, only
`_csstrcasecmp` in `changed_nonexact`, no `newly_exact`, no data/symbol drift,
and no failure.

Per the frozen one-shot contract, C1 was inverse-reverted wholesale before any
other compile. The working source again filters to exact baseline blob
`439de05852fb8e1eeb71caabef542e010cf04de0`. No alternate spelling, tuning
pass, partial retention, or second compile occurred. `config/config.json`
remains `NonMatching`; no semantic exception or parked entry was added. The
object remains 25/26 with 926/926 runtime data bytes strict exact, and there is
no implementation or progress credit. This ledger-only rejection commit is
the complete disposition of the C1 wave.
