# `hs_runtime.obj` wake-staging one-shot (2026-08-25)

## Frozen scope and provenance

This bounded source-only wave starts from clean cumulative integration commit
`226f4aeec7d7912844c5efc97aa57d5ce53aebbb` in isolated branch/worktree
`jonas/hs-runtime-wake-staging-20260825`. The only permitted production
source is `source/hs/hs_runtime.c`; this Jonas ledger is the only other
permitted tracked path. Every header, configuration, semantic exception,
parked record, object option, Units path, and other source file is frozen. The
gated Units shared-header package and its protected neighbors are outside this
wave.

Exactly one frozen `_hs_evaluate_wake` source candidate may receive one
ordinary code-producing compile. There is no post-emission spelling change,
retry, tuning pass, alternate candidate, compiler change, or flag change. The
pinned compiler is XDK 3911 CL 13.00.9254.1 with the unchanged Halo flags
`/O2 /Oy- /DDEBUG /Dxbox`.

The frozen identities are:

- baseline source Git blob / raw SHA-256
  `73261b3dec4b32e61c12166c8d89ffb2879aea38` /
  `8e0abfbe84f43a96d3e26713e4b38d952a640d504f990b7cf61811a7cf5af4cf`;
- candidate source Git blob / raw SHA-256
  `9e530c4eb1fb4c44aa673ef5abb31dca0f8eb969` /
  `5e01234c4407ef3b9a6c34610123285b113f12d6a79804021359b4da6be4828b`;
- configuration raw SHA-256
  `2829306ccd9c648d86972cb84e8d7fb8de793b779e6c560c89e415c5b4c79f6c`;
- compiler SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- January executable SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`;
- January split-object raw SHA-256
  `8ead2e539de118ecb284e82abe96fbb636b66800bbe623c2f8a306546a4b8ea6`;
- isolated source-identical baseline-object raw SHA-256
  `db1107eb9d38917e28ffd5a63ed6fae3ce549b83d3b4046884aeaeb7b74d48f7`;
  and
- frozen regression manifest
  `build/audit/hs_runtime_wake_staging_baseline_20260825.json`, 1,557,332
  bytes, SHA-256
  `db784331ca9776e14bda18acb5e44b38839adb4ff18cf6e48e9d48b6dbca9f86`.

The isolated support setup compiled the unchanged baseline source once before
the candidate existed, copied it to
`build/audit/hs_runtime_wake_staging_baseline_20260825.obj`, and reached a
one-target no-work state. Whole-object raw hashes are artifact sentinels only;
strict authority is normalized function bytes plus ordered relocation
address, type, resolved destination and addend, symbol shape, and complete
runtime non-code ownership.

## January target and private-ABI cluster

The candidate addresses one connected three-function residual:

| Owner | January meaningful / padded | Base meaningful / padded | Relocations T/B | January normalized SHA-256 | Base normalized SHA-256 |
|---|---:|---:|---:|---|---|
| `_hs_evaluate_wake` | `228 / 240` | `229 / 240` | `23 / 23` | `0222f00e062bac1949fef5d25f1f3952e3ad2fef75f8876c6fd812990dd2ffd4` | `2f56ac0d943726091e7e3a3c416945c85f827acb049d6eb2083dd7a2f7b3616a` |
| `_code_000ba090` | `175 / 176` | `175 / 176` | `10 / 10` | `013468216e81ef09d2efadc103f2f24b13e086b7aae02d0a9750e5dd59b9d886` | `ae8e4ef31f29adb7bba9c1bdadadc3655f5e6016e2c7bfbbe99791a109b89933` |
| `_hs_wake_by_name` | `31 / 32` | `33 / 48` | `2 / 2` | `fdcd889eee29c1d27bacfaf3fedd1c1eca106728bd101a0639c3db0077cb79a3` | `ee982c5d261731b0f1aa0a06d2bccd665ea5add415ab2b381d5298dfa0d4c6c9` |

January establishes a translation-unit-private EDI convention for
`_code_000ba090`. The helper's target and base differ only by a whole-function
EDI/EBX rename: its thread pointer remains ESI, every relocation and branch is
already exact, and all four uses of its private `thread_index` formal use EDI
in January versus EBX in the base. January's `_hs_wake_by_name` retains
`_code_000ba1a0`'s return in EDI and calls the helper without the base's extra
EBX preservation. January's evaluator retains the final syntax node in ESI,
receives `_code_000ba140`'s wake index in EDI, and calls the helper directly;
the base instead uses EDI for the node and transiently saves EBX around the
call.

The complete manifest freezes all 66 target function records, all 31 emitted
base functions, all 42 emitted base non-code sections, and all 168 base
symbols. It records 25 accepted functions and 1,717 accepted meaningful code
bytes. The connected exact sentinels are `_code_000ba140`,
`_code_000ba1a0`, and `_code_000b9f20`; all 25 accepted owners and the
complete data/symbol shape remain protected. If the private-ABI cascade closes
all three residuals, the object projects to 28/66 accepted functions and 2,151
accepted meaningful code bytes. The object would remain incomplete because 35
target functions are still unwritten and three other emitted functions remain
nonexact.

## Claude and cross-build evidence

All durable Claude documentation naming this object, function, or relevant
allocator tooling was screened before freezing the shot. Claude commit
`fae4cc9a` classifies `_code_000ba090` in
`research/residual_frontier.json` as a 176/176-byte `UNKNOWN`,
`unclassified`, pure-codegen residual with exact relocation identities,
`recipe: null`, and confidence `NONE`. The corrected register census
independently identifies it as a pure EDI/EBX rename.

Claude's register-tie reachability work proves that integer register assignment
is source-sensitive: definition order, initializer structure, and named
live-range boundaries can change VC7 allocation. It also cautions that the
specific lever must be tested in the full function because whole-function
pressure controls the result. No Claude ledger, durable branch, or historical
`hs_runtime.c` blob records this staged candidate or a prior production
attempt. Reconstruction commit `0e111d9f` introduced the current nested
lookup and reachable history retains it.

The separate local HCEA repository at commit
`c89106c4964f2df3a98ef7b4035d6750372797cd`,
`src/hs_evaluate_wake.c:18-28`, independently reconstructs the lookup as
sequential `call_expression`, `name_node`, and `script_argument` values.
HCEA targets a 2011 Halo CEA prototype on Xbox 360 PPC, is LLM-assisted, and
does not claim complete binary accuracy. It therefore supports semantic source
topology only; it is not January-PC spelling, register, or byte provenance.
January COFF remains the sole emission authority.

The direct alternative of initializing `thread` in
`_code_000ba090`'s declaration was rejected before compilation. It acts
inside the helper but is semantically identical to the current immediate
assignment and is likely compiler-inert. HCEA has a broad declaration-
initializer analogue in `src/hs_wake.c`, but it uses direct array arithmetic
and a substantially different staged body rather than January's `datum_get`
topology; it supplies no evidence for that initializer as a January-PC
emission lever. The staged evaluator is the only frozen candidate because it
introduces cross-build-supported live-range boundaries in a caller that
participates in the same private-ABI fixed point.

## Frozen candidate C1

C1 replaces only the nested syntax lookup with three sequential assignments to
the existing typed `script_name_node` local:

```c
script_name_node = datum_get(
	hs_syntax_data,
	thread->stack->expression_index);
script_name_node = datum_get(
	hs_syntax_data,
	script_name_node->data);
script_name_node = datum_get(
	hs_syntax_data,
	script_name_node->next_node_index);
```

The data flow is unchanged: fetch the current call expression, follow its
`data` index to the function-name node, then follow that node's
`next_node_index` to the script argument. The candidate adds no local, cast,
signature, helper, literal, storage, or runtime-data owner. It is readable,
defined, typed C and uses no assembly, `volatile`, barrier, pragma, intrinsic,
attribute, forced/no inline, `register`, raw offset/address, pointer or union
pun, inactive-union read, undefined behavior, synthetic anchor, object-byte
patch, comparator exception, or flag/tool change.

## Acceptance, rejection, and closeout gates

The frozen source must pass one XDK `/Zs` syntax-only invocation without
changing either baseline object. A Ninja dry run must then expose exactly one
`CL build/base/source/hs/hs_runtime.obj` edge. Exactly that edge may receive
the wave's sole ordinary candidate compile, after which the first-shot object
is copied immediately under ignored `build/audit/` before inspection.

Acceptance requires the three projected owners to reproduce their January
meaningful/padded sizes, normalized hashes, and ordered relocation
address/type/destination/addend records. The two public callers must reproduce
their January symbol properties, while private `_code_000ba090` must preserve
the baseline static symbol tuple: csplit's external target owner is synthetic
and is not linkage provenance. The complete baseline manifest must retain all
25 accepted functions and every other code, runtime non-code, and symbol owner.
No unexpected constant, storage, or symbol may appear.

Any difference fails closed: C1 is inverse-reverted wholesale with no retry or
tuning, and only this ledger records the rejection. No separate staging-local,
callee-initializer, declaration-order, linkage, flag, or scheduling variant
follows without genuinely new source or compiler provenance. If exact, the
source and initial ledger are committed; full Halo and libcmt builds,
ordinary/semantic/progress regeneration, semantic/admission/parked audits,
the complete tooling suite, policy/scope and JSON checks, and a clean committed
regression snapshot follow. A literal one-object delete/rebuild must reproduce
the first-shot result, followed by a ledger-only replay commit and a second
literal replay from corrected cumulative HEAD. No amend, push, rebase, history
rewrite, or worktree removal is authorized.

## Production result

The first syntax-only invocation was support-invalid: its manually transcribed
include list accidentally omitted the existing `source/math` directory, so
the compiler stopped in `scenario.h` with C1083 before reaching the candidate.
It produced no object and both live and immutable baseline-object hashes
remained
`db1107eb9d38917e28ffd5a63ed6fae3ce549b83d3b4046884aeaeb7b74d48f7`.
No source, flag, or candidate change followed. The corrected `/Zs` invocation
used the exact Ninja include list and passed.

The frozen candidate then remained at source Git blob
`9e530c4eb1fb4c44aa673ef5abb31dca0f8eb969`. A Ninja dry run exposed exactly
one CL action, and exactly one ordinary production edge compiled
`build/base/source/hs/hs_runtime.obj`:

```text
[1/1] CL build\base\source\hs\hs_runtime.obj
hs_runtime.c
```

The first-shot object was copied before inspection to ignored path
`build/audit/hs_runtime_wake_staging_first_shot_20260825.obj`, raw SHA-256
`6d35d7f9adbea0cb23a4c4888ce08e488fe96faadfa1c62065db6408a950d4ab`.

C1 failed closed because it is completely compiler-inert. All three projected
owners retained their base padded sizes, relocation records, and normalized
hashes:

| Owner | January normalized SHA-256 | Baseline and first-shot normalized SHA-256 | Result |
|---|---|---|---|
| `_hs_evaluate_wake` | `0222f00e062bac1949fef5d25f1f3952e3ad2fef75f8876c6fd812990dd2ffd4` | `2f56ac0d943726091e7e3a3c416945c85f827acb049d6eb2083dd7a2f7b3616a` | unchanged nonexact |
| `_code_000ba090` | `013468216e81ef09d2efadc103f2f24b13e086b7aae02d0a9750e5dd59b9d886` | `ae8e4ef31f29adb7bba9c1bdadadc3655f5e6016e2c7bfbbe99791a109b89933` | unchanged nonexact |
| `_hs_wake_by_name` | `fdcd889eee29c1d27bacfaf3fedd1c1eca106728bd101a0639c3db0077cb79a3` | `ee982c5d261731b0f1aa0a06d2bccd665ea5add415ab2b381d5298dfa0d4c6c9` | unchanged nonexact |

The hardened baseline-to-shot comparison reports the same 73 sections and 168
symbols, identical semantic section records, identical symbol tuples, and no
code, non-code, relocation, ownership, or debug collateral. The raw objects
differ only in the COFF timestamp bytes at offsets `0x04` and `0x05`.
The complete regression gate passed with empty failures and warnings, all 25
inherited accepted functions in `still_exact`, and empty `newly_exact` and
`changed_nonexact` lists.

The candidate was inverse-reverted wholesale. Because patch application changed
the worktree line-ending representation, the unchanged source was then copied
mechanically from the clean cumulative worktree; this restored both the
baseline Git blob
`73261b3dec4b32e61c12166c8d89ffb2879aea38` and raw SHA-256
`8e0abfbe84f43a96d3e26713e4b38d952a640d504f990b7cf61811a7cf5af4cf`.
The immutable baseline object was copied back to the live output and its
timestamp restated without changing its bytes; the live and immutable hashes
again both equal
`db1107eb9d38917e28ffd5a63ed6fae3ce549b83d3b4046884aeaeb7b74d48f7`,
and a one-target Ninja dry run reports no work.

No implementation survives, no matching credit is claimed, and no full build,
progress regeneration, admission audit, or replay is warranted. Sequential
rebinding of the existing syntax-node pointer is closed to retries without new
source or compiler provenance.
