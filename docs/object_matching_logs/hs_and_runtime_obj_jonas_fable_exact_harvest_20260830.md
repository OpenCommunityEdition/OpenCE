# `hs.obj` / `hs_runtime.obj` lawful exact harvest (2026-08-30)

## Scope and provenance

This bounded harvest starts from canonical commit
`f93c899f4aeaa86544e86d8ede7d08a45a971b90` in isolated branch/worktree
`jonas/hs-runtime-fable-wake-20260830`. Only `source/hs/hs.c`,
`source/hs/hs_runtime.c`, and this ledger are admitted. The implementation
hunks came from Fable commits `79275995c484a2637aa5db570030d42a9e02685d`
and `c8ff585ee941bd8454e991d7512b5b36ba0f161a`; every unrelated donor hunk was
excluded. Units, Vehicles, Matrix Math, AI Debug, configuration, object flags,
parked records, tools, and all other source files remain frozen.

Before transplanting, the repository instructions, matching methodology,
exact-match acceleration playbook, campaign tooling README, every existing HS
object ledger, and Fable's durable scratch notes were read in full. January PC
COFF remains the only byte and relocation authority; the donor commits are
candidate provenance, not acceptance evidence.

The harvest does not introduce or recover any address-derived function or
global name. Several owners retain pre-existing csplit labels such as
`_code_000ba090`; those labels are matching identities only and are not claimed
as authentic Bungie names. The semantic evidence identifies that private owner
as the HS wake helper, but a broad naming migration is outside this body-only
wave. No new prototype or global declaration is added.

## Exact owners

A source-identical baseline manifest was frozen for each object. Both edited
objects then received one code-producing compile, and every intended owner was
strict-exact on the first result: normalized meaningful and padded bytes,
ordered relocation address/type/resolved destination/addend, and symbol shape
all agree with January.

| Object / owner | Meaningful / padded bytes | Relocations | January normalized SHA-256 |
| --- | ---: | ---: | --- |
| `hs.obj::_code_000b2110` | `80 / 80` | `3` | `ce8daf774706f399466a829183b8abf10f4e3b2238ece7dbd5dff630aac88831` |
| `hs.obj::_code_000b2160` | `80 / 80` | `3` | `a8a9d4039e70c77e3af5d62f15a2d77becba741b1643151273bcdf240c493bbe` |
| `hs.obj::_code_000b2a40` | `96 / 96` | `3` | `debc7bc3cd9698584383df7e3f435c62603fead3e2818e0ede96c18cd3b8504d` |
| `hs_runtime.obj::_code_000ba090` | `176 / 176` | `10` | `013468216e81ef09d2efadc103f2f24b13e086b7aae02d0a9750e5dd59b9d886` |
| `hs_runtime.obj::_code_000ba290` | `32 / 32` | `0` | `edad63c921e96086fc19a9508f72f918ba0d80645bb595a3c38d0bbe793202ac` |
| `hs_runtime.obj::_code_000ba2c0` | `32 / 32` | `0` | `d59469372c5e43c468a74db2b54fed0bf9f75c2ac0ba32505134ad3811aa26c0` |
| `hs_runtime.obj::_code_000ba390` | `128 / 128` | `10` | `5b133e16432ec0ec7dab5026d0eb4375421d4d22de51370b0225e596a51b726f` |
| `hs_runtime.obj::_hs_wake_by_name` | `32 / 32` | `2` | `fdcd889eee29c1d27bacfaf3fedd1c1eca106728bd101a0639c3db0077cb79a3` |
| `hs_runtime.obj::_hs_evaluate_wake` | `240 / 240` | `23` | `0222f00e062bac1949fef5d25f1f3952e3ad2fef75f8876c6fd812990dd2ffd4` |

The wave adds 896 strict-exact padded code bytes and 54 exact relocation
records. `hs.obj` advances from 442/448 to 445/448 strict-exact functions and
from 28,656/29,728 to 28,912/29,728 strict-exact padded bytes.
`hs_runtime.obj` advances from 24/66 to 30/66 strict-exact functions and from
1,776/15,040 to 2,416/15,040 strict-exact padded bytes.

The successful source mechanisms are ordinary, semantic C:

- the existing real-argument evaluator macro preserves single-precision
  argument topology for the two HUD wrappers;
- three `real` locals avoid unintended double promotion in the convolution
  wrapper;
- a distinct union result local gives VC7 the January short-to-real conversion
  register lifetime without pointer or union punning;
- removing a redundant syntax-node local restores the translation-unit-private
  wake-helper register convention and closes both callers;
- actual and desired object masks are loaded in January order; and
- the outer syntax-node index local in `hs_evaluate_wake` frees the January EAX
  lifetime while preserving the typed `datum_get` traversal.

The surviving additions use no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, optimizer barrier, forced inline, raw offset/address,
pointer or union pun, inactive-union read, undefined behavior, synthetic data
anchor, object patch, comparator exception, or compiler/tool exception. They
preserve the existing January inline schedule and do not emit a
`point_from_line3d` COMDAT.

## Remaining boundary

`hs.obj` remains partial only at `_hs_compile_and_evaluate` (592 padded bytes),
`_code_000b33d0` (128), and `_code_000b3f50` (96). `hs_runtime.obj` retains 35
unwritten January owners plus emitted `_code_000b9a10` (144 padded bytes), whose
machine code is equivalent but whose relocation anchor spelling differs. This
wave claims no whole-object admission and does not reopen those boundaries.

## Validation

- both frozen regression manifests pass with every inherited exact owner still
  exact and all nine intended owners newly exact;
- a full Halo/libcmt/semantic/progress Ninja build completes all 570 actions;
- semantic progress evaluates 4,986 functions across 470 units, with 4,875
  semantic-exact, 4,885 accepted-exact, zero unit errors, and zero ordinary-C
  rejections;
- the complete tooling suite passes: `212 passed`;
- semantic, admission, and parked audits pass; admission reports zero
  contradicted or revoked candidates, and parked validation reports 13 active,
  zero stale, and zero invalid records;
- the added-line prohibited-construct scan is empty; and
- `git diff --check` passes apart from the repository's existing Windows
  line-ending notices.

The source is therefore retained as a lawful, independently measured partial
improvement, not as an admission shortcut or a claim that inherited synthetic
names are canonical recovery.
