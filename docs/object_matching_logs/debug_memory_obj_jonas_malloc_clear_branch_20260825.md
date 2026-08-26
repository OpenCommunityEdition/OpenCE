# `debug_memory.obj` malloc clear-branch one-shot (2026-08-25)

## Frozen scope and provenance

This bounded wave starts from clean cumulative integration commit
`afded1992c18ab95816e221ee37e4424bb7aa71f` in isolated branch and worktree
`jonas/debug-memory-malloc-clear-20260825`. The only permitted production
source is `source/cseries/debug_memory.c`; this new Jonas ledger is the only
other permitted tracked path. Every header, configuration, semantic exception,
parked record, object option, Units path, and other source file is frozen. The
gated Units shared-header package and all protected Units neighbors are outside
this wave.

Exactly one fixed `_debug_malloc` candidate may receive one ordinary
code-producing compile. There is no post-emission spelling change, retry,
tuning pass, alternate candidate, compiler change, or flag change. The pinned
compiler is XDK 3911 CL 13.00.9254.1 with the unchanged Halo flags
`/O2 /Oy- /DDEBUG /Dxbox`.

The frozen starting identities are:

- source Git blob `61f564e6b90fc9f8ceb5d3e0f7e0a12d5c57f07e`;
- configuration Git blob `8825927921a7abf9ad1a30521c3c80df265cdb3a`;
- compiler SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
- January split-object raw SHA-256
  `bcf575ff9e4dc3749ba66beb41991f62985262fccc3c643f06251d14ba942244`;
- cumulative base-object raw SHA-256
  `51360b620030da36e1434a02df34b1990731f990ddc7cb15caecd489602da3e2`;
  and
- ignored baseline manifest
  `build/audit/debug_memory_malloc_clear_baseline_20260825.json`, 637,986
  bytes, SHA-256
  `e878e928a0cfe49a556cac52a5af202abf61539c57c9e63f779f1d70034d20f2`.

The immutable copied baseline object is
`build/audit/debug_memory_malloc_clear_baseline.obj` with the same base-object
hash. A one-target Ninja dry run reported no work before this ledger or source
candidate existed.

## January target and protected translation unit

January and the cumulative base both emit `_debug_malloc` as a 272-byte padded
COMDAT with 259 meaningful bytes and 20 relocations, but their normalized code
is nonexact:

| Owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| January `_debug_malloc` | 259 | 272 | 20 | `6b3e89dbbabe53f5da1ca66d8f5d43406314b3a2008581326aac5fa7c4273782` |
| Base `_debug_malloc` | 259 | 272 | 20 | `7aaea4d6a086363835bb7e81b4730c3dfa0241baf0c6cc2347a6b0510a89b842` |

The first raw instruction divergence is at `+0x05`. The function graphs then
remain structurally aligned under a persistent register-role exchange: January
keeps `size` in EDI and the allocation/user pointer in ESI, while the base
keeps `size` in ESI and the pointer in EDI. The current source's terminal
ternary loads `clear` into BL and lowers without a branch through
`setne`, `dec`, and `and 0xCA`.

January instead loads `clear` into AL, tests it, pushes `size` before the
branch, pushes zero on the true path or `0xCA` on the false path, and joins at
one common `push pointer` and one `_csmemset` call. Its `_csmemset` relocation
is at `+207`; the base's is at `+208`, and the four following data relocations
are each exactly one byte later in the base. The first 15 relocations through
`_code_0007cfc0` otherwise align. This is the ordinary VC7 tail-merged shape
of two calls that differ only in the fill byte. Eliminating the late BL live
range also gives a concrete allocation cause for January's otherwise coherent
EDI/ESI role choice.

The baseline manifest freezes all 18 target function records, all 18 emitted
base function sections, all 27 target-owned runtime data owners, and all 112
base symbols. The following 13 functions are already strict exact and must
remain unchanged:

- `_debug_memory_manager_initialize`;
- `_code_0007ccf0`;
- `_code_0007cd40`;
- `_code_0007cd90`;
- `_code_0007cdc0`;
- `_check_memory_status`;
- `_local_random`;
- `_code_0007ce40`;
- `_debug_check_memory`;
- `_debug_dump_memory_for_file`;
- `_debug_dump_memory_by_file`;
- `_debug_free`; and
- `_debug_dump_memory`.

All 27 January-owned runtime data sections are already exact: 1,073 logical
bytes and 1,120 padded bytes. C1 must preserve those owners and every other
function. The object currently claims 13/18 functions and 1,726/2,856
meaningful code bytes (1,792 padded accepted code bytes). A successful shot
projects 14/18 functions, 1,985 meaningful code bytes, and 2,064 padded
accepted code bytes. It cannot complete or admit the object; the other four
residuals remain `_code_0007cf50`, `_code_0007cfc0`, `_code_0007d060`, and
`_debug_realloc`.

## Claude documentation, history, and tooling reconciliation

All durable Claude material that names this object, this function, or the
relevant comparison and code-generation tooling was screened before freezing
C1. Reconstruction commit `2fc59b8c1b9f31861e60aa27c528ff0a5e32d56f`
introduced `_debug_malloc` with the same retained ternary. Blame at the last
Claude frontier still assigns that line to `2fc59b8c`; the later path commits
`9100401a`, `66faa534`, `fbb90eb9`, and `666b1f03` concern the dump/free
work, not malloc experiments. Searches on the readable Claude heads
`finish-hs`, `libcmt-stream-map`, `multiplayer-sounds-closeout`,
`near-complete-objects`, and `untried-objects` find no `if (clear)` history;
the ternary's introduction is the only matching pickaxe event.

Claude's `research/residual_frontier.json` classifies `_debug_malloc` as a
272/272-byte pure-codegen residual with equal relocation counts and identities,
`UNKNOWN` category, `unclassified` subtype, `recipe: null`, and confidence
`NONE`. It supplies no conflicting causal claim or tested candidate. The
durable `debug_memory.obj` checkpoint records the residual but no prior
experiment or do-not-repeat entry.

Claude's tail-merge-family note records 15 flag variants as no improvement and
confirms the baseline flags. Its only named `debug_memory` member is
`_code_0007cf50`, a different private-convention false positive; it does not
name or dispose of `_debug_malloc`. The Claude code-generation dictionary,
flag-provenance census, register-tie material, corrected register census, and
unwritten-recipe inventory contain no `_debug_malloc` candidate, alternate
flag, or whole-object claim. The general exact-match playbook recognizes that
common calls can be tail-merged, but is treated only as compiler-mechanism
context rather than function-specific proof. Hardened `tools/coff_compare.py`,
full-owner regression comparison, and relocation identity remain the
acceptance authorities; similarity, raw byte views, semantic equivalence, or
comparator exceptions grant no credit.

The committed source body introduced the same ternary and never recorded an
explicit branch. A census of all 10 unique durable `debug_memory.c` blobs found
six ternary bodies, three bodies absent, and one forwarding stub, with zero
explicit-branch forms. Fifteen reachable historical snapshots and 1,376
accessible workspace, Claude, Codex, and Jonas worktree copies likewise found
only the ternary among emitted `_debug_malloc` bodies and no explicit
`if (clear)` form. Named prior allocation-cluster and small-final worktrees also
retain the ternary. Thus C1 is genuinely novel within locally available
provenance, not a repeated miss.
Offline partial-promisor fetch failures during broad `git log --all -S/-G`
searches do not create positive evidence and do not weaken the complete local
reachable-blob and worktree census.

## Frozen candidate C1

C1 replaces only the current conditional-value call:

```c
csmemset(pointer, clear ? 0 : 0xCA, size);
```

with two natural calls in an explicit branch:

```c
if (clear)
{
	csmemset(pointer, 0, size);
}
else
{
	csmemset(pointer, 0xCA, size);
}
```

Under cdecl's right-to-left argument order and VC7's ordinary tail merge, the
common `size` argument can be pushed before the branch and the common pointer
argument and call can be sunk to the join, exactly matching January's visible
topology. There is no signature, header, caller, literal-owner, or runtime-data
change, and `_debug_malloc` has no same-translation-unit inlining dependency.

C1 is readable, defined, typed C. It uses no assembly, `volatile`, barrier,
pragma, intrinsic, attribute, forced/no inline, `register`, raw offset/address,
pointer pun, inactive-union read, undefined behavior, synthetic anchor,
object-byte patch, comparator exception, or flag/tool change.

## Acceptance, rejection, and closeout gates

After this ledger and the exact C1 source exist, the real compiler receives
one `/Zs` parse-only invocation, proven not to modify the immutable baseline
object. Exactly one ordinary Ninja edge may then compile
`build/base/source/cseries/debug_memory.obj`. The unchanged first-shot object
is copied under ignored `build/audit/` before inspection.

Acceptance requires `_debug_malloc` to reproduce January's meaningful and
padded sizes, normalized hash, symbol properties, and ordered relocation
semantics. It must retain one `_csmemset` relocation and 20 total relocations;
two calls, the existing `setne/dec/and` lowering, or any different topology is
a rejection. The complete baseline manifest must retain all 13 accepted
functions, and a baseline-to-shot comparison must show every other code and
target-owned runtime owner unchanged in normalized bytes, extent, flags,
ownership, and ordered relocations. No unexpected constant, storage, or symbol
owner may appear.

Any difference fails closed: C1 is inverse-reverted wholesale with no retry or
tuning. In particular, no branch-polarity, braces, `goto`, fill temporary,
`boolean` widening, flag, or statement-order variant follows without genuinely
new disassembly or provenance evidence. Only this ledger records a rejection.
If exact, the source and initial ledger are committed, followed by full Halo
and libcmt builds, fresh
ordinary, semantic, and progress reports, semantic/admission/parked audits,
the complete tooling suite, policy/scope and JSON checks, and a clean committed
regression snapshot. A verified literal one-object delete/rebuild must
reproduce the first-shot runtime result. Replay evidence is then appended in a
ledger-only commit and repeated from corrected HEAD. No amend, push, rebase,
history rewrite, or worktree removal is authorized.

## Production result

The real XDK `/Zs` invocation passed and left the live baseline object raw
SHA-256
`51360b620030da36e1434a02df34b1990731f990ddc7cb15caecd489602da3e2`
unchanged. The candidate source then remained frozen at Git blob
`83a1667639309b75ccb80ec174cdeb80de919951`. A Ninja dry run exposed exactly
one CL action, and exactly one ordinary production edge compiled
`build/base/source/cseries/debug_memory.obj`:

```text
[1/1] CL build\base\source\cseries\debug_memory.obj
debug_memory.c
```

There was no direct code-producing CL command and no second candidate compile.
The unchanged first-shot object is preserved at ignored path
`build/audit/debug_memory_malloc_clear_first_shot.obj`, raw SHA-256
`5f64376d2d18cb54eebaddac83542d9de04f883d5069c0f37f02a5efd8ebd9b9`.

C1 failed closed. It recovered January's visible branch topology and the entire
ordered relocation layout, but not January's register allocation:

| Owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| January `_debug_malloc` | 259 | 272 | 20 | `6b3e89dbbabe53f5da1ca66d8f5d43406314b3a2008581326aac5fa7c4273782` |
| First-shot `_debug_malloc` | 259 | 272 | 20 | `27935350374ae6dcb5d299da2016d6bb5e33429aa188a86c10c1e8c747347b3d` |

Both objects now place all 20 relocations at identical addresses with identical
types and symbolic targets. In particular, both call `_code_0007cfc0` at
`+181`, push the common size before the clear branch, select immediate zero or
`0xCA`, join at one pointer push, and carry the sole `_csmemset` relocation at
`+207`. All four following `_data_002dcd0c` relocations also align at
`+219`, `+225`, `+234`, and `+241`. Thus the source-level tail-merge inference
was correct, but relocation and control-flow identity are insufficient for
strict code identity.

January still assigns `size` to EDI and the allocation/user pointer to ESI.
The first shot retains the base's opposite assignment: `size` in ESI and the
pointer in EDI. The distinction starts in the prologue at `+0x05`, persists
through allocation, header initialization, fill, accounting, and return, and
also changes the semantically equivalent maximum-size comparison from
January's `cmp ecx,eax; jge` to the first shot's `cmp eax,ecx; jle`. The
explicit branch therefore corrected the local CFG without perturbing VC7's
earlier allocator choice. There is no source-supported second lever in this
wave.

The one-unit regression gate retained all 13 inherited exact functions in
`still_exact`, reported no `newly_exact`, and named only `_debug_malloc` in
`changed_nonexact`. Its sole whole-object failure outside that requested COMDAT
was ordinary compiler metadata: anonymous `.debug$S` grew from 186 to 194
bytes between the copied cumulative baseline and the fresh isolated compile.
The objects otherwise retain the same section and symbol shape; every other
code owner and every runtime constant/storage owner is unchanged. Debug
metadata grants no runtime credit.

C1 was inverse-reverted wholesale with no branch-polarity, braces, `goto`,
temporary, declaration, flag, or statement-order retry. The working source
again hashes to baseline Git blob
`61f564e6b90fc9f8ceb5d3e0f7e0a12d5c57f07e`. This result retires the explicit
clear-branch candidate and shows that the target's EDI/ESI allocation cannot be
inferred from the fill topology alone; another production wave requires
genuinely new source or compiler provenance.

Because no implementation or metadata change survives, there is no exact
function gain, progress or admission change, full-build closeout, or
committed-state implementation replay. `debug_memory.obj` remains 13/18 exact
functions, 1,726/2,856 accepted meaningful code bytes, 1,792 padded accepted
code bytes, and 1,120/1,120 exact padded data bytes. This documentation-only
record is the complete outcome of the one-shot.
