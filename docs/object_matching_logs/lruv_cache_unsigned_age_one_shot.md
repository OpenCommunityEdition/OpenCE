# `lruv_cache.obj` unsigned-age one-shot

Date: 2026-08-25

This ledger freezes one bounded natural-C experiment for
`_lruv_debug_to_file`. It does not authorize any change to
`_lruv_block_new`, a header, compiler flags, or project metadata.

## Baseline

- Cumulative commit: `eeade5e51b0886b16ad83713df7cc63d16a28552`.
- Isolated branch: `jonas/lruv-debug-unsigned-age-20260825`.
- Source blob: `80da9682f45588d03732b467bd5dd54ea4a92f36`.
- Source file SHA-256:
  `52b99c81f762e589eecaf21a869c1519e5f87ac7789fec1cfe6f714b156623e9`.
- Frozen regression manifest:
  `build/audit/lruv_unsigned_age_baseline.json`.
- Baseline inventory: 16/18 functions are `STRICT_EXACT`; 26 target-owned
  non-code sections are `STRICT_EXACT`; eight candidate-only literal/debug
  sections are `BASE_ONLY` and grant no credit.

The two residual fingerprints are:

| Function | Target meaningful/padded | Target relocs/hash | Base meaningful/padded | Base relocs/hash |
|---|---:|---|---:|---|
| `_lruv_debug_to_file` | 458/464 | 20 / `fb7c7a22cacb060e9e3fa2e5a57942daa52e259dccc6b7796bf744bf28d291bb` | 452/464 | 20 / `c63c1864e6fe9be26a5208b89ff574ae8ebacd322079ea1917590503705a08a1` |
| `_lruv_block_new` | 1302/1312 | 48 / `d008a8c97b45d969b3d824f6e3daa6caa018c04e9092b80c4ec64d84e3ab954b` | 1286/1296 | 48 / `302c6f8b64f32700677445abb761181c03ca7ff648b5223f45a8a780e849a4a8` |

## Provenance and hypothesis

E54/checkpoint D02 proved that typed cache-name access plus the exact
byte-to-page helper preserves `464` bytes and 20 relocations while improving
the normalized debug hash to
`8b2776c4666f22765aba53a34d4bff1f09d4223456d38fb1ee53b3ae69b355aa`.
That source shape is present in commit
`71cc36e5c79dbd946ec39ec8298992e30cedb3a8`, but that commit must not be
transplanted because its `_lruv_block_new` also contains a banned
`_ReadWriteBarrier()`.

The new factor is only the natural unsigned lifetime of `age`. The target
clamp uses `cmp eax, 0x270f` followed by `jb`; the baseline uses a memory
compare followed by `jbe`. The unsigned local was not tested by E1-E55,
D01-D02, or any materialized production LRUV worktree.

## Frozen source edit

Apply exactly these four edits in `source/memory/lruv_cache.c`:

1. `long age;` becomes `unsigned long age;`.
2. The first debug `fprintf` receives `cache->name` instead of the raw
   offset-zero `cache` pointer.
3. The manual allocation page-count calculation becomes
   `allocation_page_count = code_0010cd50(cache, allocation_size);`.
4. `age = MIN((unsigned long)age, 9999);` becomes
   `age = MIN(age, 9999);`.

No other spelling, declaration scope, control flow, or file may change.

## Execution contract

- Run one `/Zs` syntax-only check first.
- Permit exactly one normal Ninja emission of
  `build/base/source/memory/lruv_cache.obj` and preserve that first-shot
  object for measurement.
- Do not retry, permute, tune, or substitute an equivalent expression.
- Admit the source only if `_lruv_debug_to_file` reaches the complete target
  fingerprint, `_lruv_block_new` remains byte-for-byte at its frozen base
  fingerprint, all 16 protected functions remain strict-exact, and all 26
  target-owned data owners remain strict-exact.
- A miss receives no implementation credit: restore the source and record
  only the measured read-only outcome.
- A hit raises the unit to 17/18 but leaves it `NonMatching`; the
  `_lruv_block_new` blocker remains.

## Outcome

Miss; no implementation credit.

- The XDK 3911 `/Zs` check exited zero and emitted no object.
- One and only one normal Ninja compile emitted the candidate object. Its
  whole-file SHA-256 is
  `27cc7f981fb08debaceb1090784d422379ea2eab1b4e88013663770e18a4800d`.
  The preserved first-shot object is
  `build/audit/lruv_unsigned_age_first_shot.obj`.
- `_lruv_debug_to_file` remained `452` meaningful / `464` padded bytes with
  20 relocations and normalized SHA-256
  `8b2776c4666f22765aba53a34d4bff1f09d4223456d38fb1ee53b3ae69b355aa`.
  It is not relocation-equal to the target. This reproduces E54/D02 exactly
  and proves that the unsigned `age` lifetime plus cast removal are
  object-neutral on that scaffold.
- `_lruv_block_new` remained unchanged at `1286` meaningful / `1296` padded
  bytes, 48 relocations, and normalized SHA-256
  `302c6f8b64f32700677445abb761181c03ca7ff648b5223f45a8a780e849a4a8`.
- All 16 protected functions remained strict-exact. All 26 target-owned
  non-code sections remained strict-exact. The generic whole-TU gate also
  reported a change in candidate-only `.debug$S`; that section is one of the
  eight `BASE_ONLY` sections and grants no target-owned credit.
- The experiment produced no newly exact function. The complete four-edit
  source shot was reverted, restoring source blob
  `80da9682f45588d03732b467bd5dd54ea4a92f36`.

Do not retry the E54/D02 scaffold with an unsigned `age`, with or without the
cast in `MIN`. Reopen `_lruv_debug_to_file` only for new January-authoritative
source/local records, a strict donor, or a different legal lifetime fact that
directly changes the first callee-saved-register divergence.
