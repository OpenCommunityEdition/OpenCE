# Units animation-update declaration closure (Jonas, 2026-08-24)

## Result and bounded scope

This declaration-only wave starts from cumulative Units commit
`b0ef2704c3f1a90f9edea9440d661000440ca548`. It adds the missing
translation-unit-local external prototype for `animation_update_internal`
to `source/units/units.c` and changes no function body:

```c
short animation_update_internal(
	short animation_type,
	long animation_graph_index,
	struct animation_state *state,
	long *sound_index);
```

The sole affected code owner, `_code_0019b160`, is strict on the first
edited-source object:

| January owner | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_0019b160` | 73 / 80 | 4 | `31a801fff65c190a04e8e767f047a25415ca243ab6d107343563467de1979ec1` |

The direct ordinary-function census advances from 164 exact / 13 emitted
nonexact / 12 absent to 165 / 12 / 12 across all 189 January owners. Removing
this one gain from the final exact set reproduces all 164 inherited exact
owners. Direct comparison of every common code owner proves that
`_code_0019b160` is the only changed code section.

The object remains `NonMatching`. No function body, shared header, protected
source, configuration, storage owner, semantic exception, parked-function
record, or completion label is changed. The only tracked paths in this wave
are `source/units/units.c` and this new ledger.

## Immutable authority and toolchain

- January `source/units/units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- January `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- Cumulative base commit: `b0ef2704c3f1a90f9edea9440d661000440ca548`.
- Cumulative base Units source blob: `be4e36f0b88ca8f7ef27c01a6036dd3aedb83986`.
- Frozen retained source blob before commit:
  `725b1cd35f667fc64e4f5b3fd17fcfe5a8d18219`.
- Immutable cumulative baseline object: 140,865 bytes, SHA-256
  `49f66a5a3f0e384325c5a63a46e62b4d133310182dae308a4314021499893fc9`.
- Immutable first-shot object: 140,882 bytes, SHA-256
  `7255882655eec4e6ea755242b7c6bf3c9c67f7b997d173320a732f10e4f5d27b`.
- XDK 3911 `CL.Exe` 13.00.9254.1: 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Authenticated csplit v0.0.2 and objdiff-cli v3.3.1 SHA-256 values:
  `708f957c37caa66df90ade35d1e65e2617e2cc2c8c2ccf68d8bf5cb20e752`
  and `090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677`.

The generated production rule remains ordinary VC7 C compilation with the
repository's `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` boundary and unchanged
include graph. No compiler option, tool binary, generated configuration, or
target object was altered.

## Claude documentation and provenance audit

This wave follows the complete Claude documentation/tooling audit recorded in
the immediately preceding
`units_obj_jonas_update_animation_private_closure_20260824.md` ledger. That
audit covered the specialized `units_obj_update_animation.md` history through
UUA-42, the consolidated `units_obj.md`, all linked animation/private-helper,
lifetime, compiler-provenance/QFE, crossbuild, donor-atlas, HCEA, and final
topology records, plus the campaign reproduction, compile, gate, hunt, c2
configuration, backend-map, and IR-layout tooling.

For this fresh wave, the specialized and consolidated ledgers, the retained
support source, the January COMDAT, and the authenticated Claude carrier at
`deb5554ab8d16da80da934e2aa7081bc7d1f594f` were rechecked at the exact
declaration boundary. The prior support wave explicitly reserved this missing
prototype for a separate declaration-only emission. The carrier's complete
Units source blob is `124f29e32497c091c4163b5faafc58dbf8890161` and contains the
same typed `short(short, long, animation_state *, long *)` declaration. It was
used as provenance only; none of its unrelated barriers, raw layout accesses,
attributes, register hints, or other forcing constructs was imported.

## Measured blocker and source justification

Before this wave, the helper already had the readable typed body recovered by
the update-animation support closure. Its 80-byte candidate and January
section had the same four relocation identities and addresses:

| Offset | Type | Destination |
| ---: | --- | --- |
| `+0x0E` | `REL32` | `_animation_update_internal` |
| `+0x21` | `DIR32` | `_global_forward3d` |
| `+0x27` | `DIR32` | `_global_origin3d` |
| `+0x3A` | `REL32` | `_object_impulse_sound_new` |

The only pre-wave mismatch was normalized hash
`8370a5fea4a740e91e7719ab45af4ac987a0d8c9274d9ecfcaca18835c612ff3`
versus January's `31a801fff65c...`. With no visible prototype, legal C89 let
VC7 apply an implicit-int declaration at the call site. January and the
authenticated fixture schedule `add esp,0x10` before retaining the short
result and reloading `sound_index`; the implicit-declaration object deferred
that cleanup past both moves. The explicit typed declaration reproduces the
January sequence without touching the body.

This is ABI/type recovery backed by the emitted object and Claude provenance,
not a source-shape or scheduling guess. The declaration is file-local in
visibility of use but correctly declares an external callee; making the
function itself `static`, changing a public header, or changing the body would
misstate ownership or exceed the evidence.

## Scratch boundary and sole candidate emission

The source diff was frozen at exactly five added prototype lines. `git
diff --check` passed. A production-flag VC7 `/Zs` parse-only invocation passed
and proved that the pre-existing object remained byte-identical during syntax
validation.

The next and only code-producing action on the edited source was the normal
generated edge:

```text
[1/1] CL build\base\source\units\units.obj
```

The first-shot object was copied immutably before adjudication. The target and
candidate helper agree in all 80 padded bytes after relocation normalization,
all four relocation positions/types/destinations/addends, and the normalized
SHA-256 above. There was no retry, declaration variation, body edit, source
tuning, or compiler-option change.

A fresh-worktree regression-manifest setup had earlier refreshed the untouched
base Units object before the source edit while discovering an unavailable
download edge. That was an unmodified baseline build, not a candidate. The
immutable cumulative baseline copy remained the comparison authority, and the
edited source still received exactly one production emission after `/Zs`.

## Whole-object and data preservation

The complete direct code census is 165 exact / 12 emitted nonexact / 12 absent.
The inherited exact set is preserved without addition or removal other than
`_code_0019b160`; no inherited nonexact code section changed.

Across all 129 January non-code owners, both baseline and first shot are 125
exact / zero present-nonexact / four absent. There is no exact-data gain or
loss and no common target non-code mutation. The prototype changes ordinary
compiler debug/type metadata only; it adds no runtime storage and changes no
January-owned data section.

## Full pre-commit gates

The complete validation boundary passed:

- full Halo plus libcmt targets passed and were current; the already-adjudicated
  Units candidate was not recompiled;
- strict semantic audit: 470 units, 4,234 functions evaluated, 3,495 missing
  base symbols, 4,092 semantic exact, 114 hidden exact / 64,705 hidden code
  bytes, 36 ordinary-only, 35 ordinary structural, one ordinary rejection,
  12 local symbols skipped, 4,153 accepted exact, and zero unit errors;
- campaign progress: 375/833 objects, 4,127/11,060 functions,
  500,368/2,198,102 code bytes, and 1,835,088/4,176,062 data bytes;
- Halo progress: 273/468 objects, 3,960/7,574 functions,
  487,454/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes;
  libcmt remains 102/212 objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes;
- object admission: zero candidates and zero revocations; the sole
  contradiction is the inherited unrelated `source/shell/shell_xbox` label;
- parked-function validation: three active, zero stale, zero invalid;
- complete tooling suite: 179/179 tests pass.

## Committed-state replay plan

No committed-state replay is claimed in the implementation commit. After the
source and this initial ledger are committed by Jonas Volman and the worktree
is clean, a fresh one-unit accepted-state snapshot will pin that commit. The
generated Units object path will be resolved and proven inside this worktree,
deleted with literal-path semantics, rebuilt once by its ordinary Ninja edge,
and checked immediately. Direct code/data censuses, the helper fingerprint,
and a final Ninja dry run will be repeated before one additive ledger-only
replay commit. No push, amend, rebase, history rewrite, or worktree removal is
performed.
