# `libs/libcmt/atof.obj` Jonas typed-cursor one-shot (2026-08-25)

## Scope and frozen provenance

This bounded greenfield one-shot starts from clean cumulative integration
commit `b092890a74c9cb6b228b93d1f77b29baef4781cc` in the isolated worktree and
branch `jonas/libcmt-atof-cursor-20260825`. The only permitted production
source is the new `libs/libcmt/atof.c`; the only other initially permitted
tracked path is this Jonas ledger. Exactly one fixed natural-C candidate, C1
below, may receive one ordinary code-producing compile. There is no source
spelling change, retry, tuning pass, alternate candidate, compiler change,
header edit, or configuration option after that emission.

The pinned compiler is XDK 3911 CL 13.00.9254.1. The unchanged `libcmt`
project flags are `/O1 /Gy`; `/Oi` is explicitly forbidden in this wave.
Current `config/config.json` blob
`8825927921a7abf9ad1a30521c3c80df265cdb3a` records index 830,
`libs/libcmt/atof.c`, as `MISSING`. The cumulative tree owns no source or
candidate object for this unit, so there is no pre-edit function manifest to
protect. This is governed by the documented greenfield regression procedure.

## January target and accepted exact donor

January owns one external function COMDAT and no runtime data:

| Function | Size | Relocs | Normalized SHA-256 |
|---|---:|---:|---|
| `_atof` | 92 | 4 | `c8b933c4b68c82b91281c63a8bce21e07757cd54438b0b67eaaf1f5b080ae600` |

The four relocations are authoritative and ordered:

- offset `0x0c`, `DIR32`, `___mb_cur_max`, addend zero;
- offset `0x1a`, `REL32`, `__isctype`, addend zero;
- offset `0x24`, `DIR32`, `__pctype`, addend zero;
- offset `0x4f`, `REL32`, `__fltin2`, addend zero.

The target function symbol is external, type `0x20`, at section offset zero.
The target owns no `.data`, `.rdata`, or `.bss`, no aliases, no private helper,
and no data relocations. The exact historical donor is commit `4d293c53`,
source blob `f30276d47be0eb0b8905fd25130cbc879803633a`, recovered from authenticated
Microsoft/XDK CRT source. Its readable locale-aware parsing control flow is
authoritative, but its admission added `/Oi` so that `strlen` became an
intrinsic expansion. That configuration is excluded from this wave.

The decisive target length region is:

```text
+0x36  8b c6          mov eax, esi
+0x38  8d 50 01       lea edx, [eax+1]
+0x3b  8a 08          mov cl, [eax]
+0x3d  40             inc eax
+0x3e  84 c9          test cl, cl
+0x40  75 f9          jne +0x3b
+0x42  6a 00          push 0
+0x44  2b c2          sub eax, edx
+0x46  6a 00          push 0
+0x48  50             push eax
```

This is the ordinary postincrement-cursor form: the compiler precomputes
`string + 1`, advances the end cursor through the terminator, and subtracts
the precomputed base to form the nonterminator length.

## Claude/Codex reconciliation and novelty proof

The historical exact ledger `docs/object_matching_logs/libcmt_atof_obj.md` at
`4d293c53` records only these source/build experiments: standard `strlen`
under `/O1 /Gy` (85 bytes and five relocations because `_strlen` remains a
call), the same source with the public header, and the same source under
`/O1 /Gy /Oi` (the exact 92-byte target). Its do-not-repeat section advises
against a handwritten scan but preserves no emitted cursor candidate,
fingerprint, disassembly, or rejection result.

Reachable-history searches show `4d293c53` is the only commit that ever owns
`libs/libcmt/atof.c`. Searches for `end - string - 1` and `while (*end++)` in
that path return no commit, and every materialized source copy retains
`strlen(string)`. C1 is therefore genuinely untried in all durable local
Claude/Codex source and experiment records. The old advisory is honored as a
warning, not misreported as empirical exhaustion.

## Frozen one-shot C1

C1 recreates the authenticated donor exactly except for these fixed changes:

```diff
-#include <string.h>
@@
 double __cdecl atof(
 	char const *string)
 {
 	struct floating_point_input result;
+	char const *end;
@@
-	return _fltin2(&result, string, strlen(string), 0, 0)->double_value;
+	end = string;
+	while (*end++)
+	{
+	}
+
+	return _fltin2(
+		&result,
+		string,
+		(int)(end - string - 1),
+		0,
+		0)->double_value;
 }
```

There is no `/Oi`, object-specific option, header change, or other source
variation. The prediction is exactly 92 bytes, four target relocations, target
normalized SHA-256 `c8b933c4...`, and zero runtime data. The potential gain is
92 matched code bytes, one exact function, and one complete object.

## Policy and house-style audit

C1 is readable typed C. The two pointers remain within the same input string
object, so their subtraction is defined; the explicit conversion documents
the `_fltin2` length parameter. The authenticated locale-aware whitespace
branch, unsigned-byte classification, typed conversion record, function
parameters, calling conventions, and explicit return are unchanged.

C1 uses no assembly, `volatile`, compiler barrier or intrinsic, optimization
pragma, attribute, forced/no inline, `register`, raw field or address offset,
address integerization, pointer/union pun, inactive-union access, undefined
behavior, synthetic anchor, object-byte patch, comparator exception, fuzzy
credit, or flag/tool change. It touches no tag/object access, Units source,
protected Units neighbor, or deferred Units shared-header package.

## Acceptance, rejection, and closeout gates

After this ledger and exact C1 source exist, the real XDK compiler receives a
`/Zs` syntax-only invocation that emits no object. Exactly one ordinary Ninja
edge may then compile `build/base/libs/libcmt/atof.obj`. The first-shot object
is preserved unchanged under ignored `build/audit/` before inspection.

Because the generator omits a base edge while an object is `MISSING`, index
830 is staged as `NonMatching` only to generate and invoke that ordinary
unchanged `/O1 /Gy` edge. It adds no object option. A rejection restores
`MISSING`; an exact result promotes the same entry to `Matching`, so the final
accepted diff is still the single authorized `MISSING`-to-`Matching` change.

Acceptance requires the exact function size, symbol type/storage/offset,
section flags, normalized hash, and all four relocation addresses, types,
destinations, and addends above; the candidate must own no runtime data,
alias, or helper. Any difference fails closed. If C1 misses, the new source is
removed wholesale immediately, with no retry, spelling change, partial
retention, flag/header/config change, or second compile. Configuration remains
`MISSING`, and only this ledger receives a rejection commit.

If C1 is exact, only index 830 changes from `MISSING` to `Matching`, with no
object options. Full Halo and libcmt builds, fresh ordinary/semantic/progress
reports, semantic/admission/parked audits, all 179 tooling tests, JSON,
source-scope, prohibited-construct, and `git diff --check` gates must pass.
The implementation and initial ledger are committed, then a literal one-object
delete/rebuild must reproduce the first-shot evidence and pass a clean
post-admission regression snapshot/check. A ledger-only replay commit,
corrected-HEAD replay, and final Ninja dry run close the wave. No amend, push,
rebase, or worktree removal is authorized.

## Production result

The real XDK `/Zs` pass succeeded and emitted no object. Index 830 was staged
as `NonMatching` only long enough to generate the ordinary unchanged
`/O1 /Gy` edge. Exactly one normal Ninja action then compiled
`build/base/libs/libcmt/atof.obj`; there was no direct code-producing compiler
invocation and no second Ninja compile. The unchanged first-shot object is
preserved at ignored path
`build/audit/libcmt-atof-cursor-first-shot/atof.obj`, with phase-local raw
SHA-256
`9242bc861eda4beeea034bb6662c48e14d991f59bd8029746451f28a57ef78f6`.
The emitted C1 source blob was
`8282581a482cf81886e13d0d97adfad467c79e44`.

C1 did not reach strict equality. It emitted 90 bytes and four relocations,
with normalized SHA-256
`48b127ab0356ea5287850f83f3a631e4db3358f2333ef3a1ec357f436c62cd1c`,
rather than the target's 92 bytes and
`c8b933c4b68c82b91281c63a8bce21e07757cd54438b0b67eaaf1f5b080ae600`.
The first three relocations remain exact at `0x0c`, `0x1a`, and `0x24`, with
the required types, destinations, and zero addends. The `__fltin2` `REL32`
relocation is at candidate `0x4d` rather than target `0x4f`; its destination
and addend remain correct.

The first true divergence is the candidate's removal of January's precomputed
`string + 1` subtraction base:

```text
target +0x36: mov eax, esi
target +0x38: lea edx, [eax+1]
target +0x3b: mov cl, [eax]
target +0x3d: inc eax
target +0x3e: test cl, cl
target +0x40: jne +0x3b
target +0x44: sub eax, edx

C1     +0x36: mov eax, esi
C1     +0x38: mov cl, [eax]
C1     +0x3a: inc eax
C1     +0x3b: test cl, cl
C1     +0x3d: jne +0x38
C1     +0x41: sub eax, esi
C1     +0x45: dec eax
```

Thus the typed postincrement cursor preserves the intended length but VC7
chooses a two-byte-shorter subtract-then-decrement topology. The final call
and epilogue are displaced by two bytes. This is a source-shape miss, not a
relocation-identity, symbol, or ownership ambiguity.

The candidate still owns the correct external `_atof` symbol, type `0x20`, at
offset zero. Its `.text` flags are the target's `0x60101020`. It owns no
`.data`, `.rdata`, `.bss`, runtime alias, helper, or data relocation; the only
additional sections are ordinary `.drectve` and `.debug$S` compiler metadata.

Per the frozen contract, C1 was inverse-reverted wholesale immediately after
the strict miss. `libs/libcmt/atof.c` is absent again. Index 830 is restored to
`MISSING`, `config/config.json` filters to its authoritative baseline blob
`8825927921a7abf9ad1a30521c3c80df265cdb3a`, and no object option, semantic
exception, or parked entry was added. No alternate spelling, cast change,
flag/header variation, retry, or second code-producing compile occurred. This
ledger is the only retained tracked change.
