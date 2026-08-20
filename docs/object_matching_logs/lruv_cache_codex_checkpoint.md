# `lruv_cache.obj` Codex checkpoint

Date: 2026-08-20

This is a Codex/Jonas-owned checkpoint ledger. It does not replace or modify
any Claude-authored research or Markdown.

## Scope and provenance

- Integration baseline: `018aa03f36fc0deb3e9e47c73adc4133c9c6a165`.
- Isolated lane: `jonas/lruv-cache-closeout-20260820`.
- Unit: `source/memory/lruv_cache`; configuration remains `NonMatching`.
- Target: January 2002 `cachebeta.exe`, MD5
  `89D83CCFD47617654F3A1D3731D53E3B`.
- Compiler: XDK 3911 CL `13.00.9254.1`, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the unchanged `/O2 /Oy- /DDEBUG /Dxbox` flags and repository include
  closure.
- GPL Demon commit `edb96c7614b80b4a80a3b0cee5c5fb7c7804a3c7` was used only as a
  semantic/control-flow oracle. No Demon source was copied or retained.
- The five frozen objects (`vehicles`, `ai_debug`, `ai_script`, `actions`, and
  `units`) are outside this lane and were not touched.

## Baseline and collision audit

The integration worktree was clean at the requested commit. The forced current
rebuild disproved the reported one-residual state: this exact baseline has
16/18 strict functions, not 17/18. The stronger historical lane
`codex/lruv-cache-full-admission-20260729` at `345a83af` is not an ancestor of
the integration baseline. Its size-equal `_lruv_block_new` uses
`_ReadWriteBarrier()` to keep two returns distinct, so it is inadmissible under
the current no-barrier rule and was not transplanted.

Nine older LRUV branches were already checked out in separate worktrees. The
only worktree reported modified was `agent/lruv-cache-closeout`; its worktree,
index, and HEAD blobs are all exactly
`80da9682f45588d03732b467bd5dd54ea4a92f36`, so the status is line-ending/stat
noise rather than a competing source edit. This lane therefore has no
meaningful source collision.

`config/parked.json` has no LRUV entry. There is no stale park to remove unless
the complete object first becomes strictly admissible.

## Hardened strict inventory

- 16/18 real functions are strict-exact.
- 1,984/3,760 padded target function bytes and 1,880/3,640 meaningful bytes
  are carried by those exact functions.
- 124/192 target function relocations are carried by exact functions.
- All 26 target-owned external RDATA COMDATs are strict-exact: 1,117/1,117
  bytes, zero relocations, and matching owner/storage metadata.
- The target owns no DATA, BSS, or common definition in this unit.
- Five candidate-only literal COMDATs total 16 bytes (`""`, `" "`, `"\n"`,
  `"name"`, and `"cache"`). They are target-undefined split owners, not
  target-owned credit. Candidate-only `.debug$F`, `.debug$S`, and `.drectve`
  metadata are likewise excluded from target-owned credit.

The two residuals are:

| Function | Target/base bytes | Relocs | Target hash | Base hash | First divergence |
|---|---:|---:|---|---|---|
| `_lruv_debug_to_file` | 464/464 | 20/20 | `fb7c7a22cacb060e9e3fa2e5a57942daa52e259dccc6b7796bf744bf28d291bb` | `c63c1864e6fe9be26a5208b89ff574ae8ebacd322079ea1917590503705a08a1` | `+0x6`: target begins the callee-saved cycle with ESI; current begins it with EBX. |
| `_lruv_block_new` | 1312/1296 | 48/48 | `d008a8c97b45d969b3d824f6e3daa6caa018c04e9092b80c4ec64d84e3ab954b` | `302c6f8b64f32700677445abb761181c03ca7ff648b5223f45a8a780e849a4a8` | `+0x11`: target saves ESI before assigning the cache role; current assigns EBX first. |

## Demon-oracle experiment matrix

All variants preserved the 16 exact siblings and all 26 target-owned data
sections. Neither produced a strict gain, so both were reverted completely.

| ID | Independently reconstructed source shape | Size | Relocs | Candidate hash | Decision |
|---|---|---:|---:|---|---|
| D01 | Readable nullable-name report loop with one common output statement, typed cache-name access, and the existing byte-to-page helper. This tests the exact Demon semantic/CFG lead while retaining January asserts, callbacks, strings, types, and an explicit final return. | 464 | 20 | `b87d63029ca16665ea0c7bb479c3e4e772842c3c05210f5e22556af5c0ebe366` | Reverted: strict nonmatch; the natural fallthrough is not the January output CFG. |
| D02 | Keep the target-like shared-output labels, but use the typed offset-zero cache name and existing byte-to-page helper corroborated by Demon. | 464 | 20 | `8b2776c4666f22765aba53a34d4bff1f09d4223456d38fb1ee53b3ae69b355aa` | Reverted: closer arithmetic scheduling is not strict equality and grants no credit. |

The original current source rebuilt back to both baseline hashes after the
reverts. No GPL-derived implementation remains in the worktree.

## Residual classification and do-not-repeat list

- `_lruv_debug_to_file` is a broad VC7 callee-saved-register/stack-coloring
  cycle. Equal size and relocation count are insufficient. The Demon natural
  fallthrough, typed offset-zero name spelling, helper reuse, and their
  combination do not close it.
- `_lruv_block_new` is still a source-topology/register-allocation residual at
  this integration tip. The historical size-equal branch is evidence only:
  its barrier violates the current rules, and same-size output would still not
  be strict equality.
- Do not retry nullable-name fallthrough, final name ternaries, typed-name-only
  spelling, helper-only page conversion, declaration-order/register hints, or
  any barrier/volatile/assembly/forced-inline/pragma/undefined-behavior/byte
  forcing technique.

Reopen only for January-authoritative source/local records, a strict same-shape
donor, or a legal typed-C lifetime/control-flow fact that directly changes the
first divergent register cycles. A fuzzy score, equal size, relocation-count
equality, or semantic plausibility is not sufficient.

## Disposition and validation

The object remains honestly `NonMatching` with two residual functions. No
configuration, park manifest, symbols, source/header file, frozen object, or
pre-existing Markdown is changed.

- Forced clean XDK rebuild of `lruv_cache.obj`: passed.
- Full `halobetacache_build` and `libcmt_build`: passed.
- Strict whole-object census: 16/18 functions; both residual measurements
  restored exactly to baseline; 26/26 target-owned RDATA sections exact.
- Semantic report: 470 units, 3,686 functions evaluated, 3,543
  `semantic_exact`, 3,625 `accepted_exact`, zero unit errors.
- Progress and ownership validation: passed; 272/468 Halo objects remain
  credited at this baseline.
- Admission audit: zero candidates, zero revocations; only the pre-existing
  `shell_xbox` contradiction remains.
- Parked manifest: three active, zero stale, zero invalid; no LRUV entry.
- LRUV regression check: all 16 baseline-exact functions and all target-owned
  sections retained; no changed nonexact function after the reverts.
- Tool suite: 179/179 tests passed.
