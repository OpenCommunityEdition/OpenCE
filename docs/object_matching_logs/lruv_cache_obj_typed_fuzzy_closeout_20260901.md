# `lruv_cache.obj` typed fuzzy closeout

Date: 2026-09-01

## Result

This closeout starts from pushed canonical checkpoint
`ec4ae894a10ac440566b41a017abf4f94ea3501a`. The unit remains
`16 exact / 2 residual / 0 unwritten`; all 1,180 report-counted data bytes
remain exact. Both residuals now have coherent typed implementations and a
measured evidence boundary, so they are parked without claiming byte equality.

The earlier `_lruv_block_new` reconstruction and its park remain unchanged.
This lane tightens `_lruv_debug_to_file` to the strongest admissible source
shape and removes the final address-derived names from the file inventory.

## Source and provenance

HaloCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c` independently identifies the
debug routine's semantic topology: print `cache->name`, compute failed
allocation pages through the cache byte-to-page helper, keep age unsigned,
and clamp the displayed age. Aerocatia/demon commit
`edb96c7614b80b4a80a3b0cee5c5fb7c7804a3c7` corroborates the same behavior.
Those projects are semantic oracles, not claims of January source provenance.

The retained source therefore:

- passes `cache->name` to `%s`, rather than relying on an offset-zero struct
  pointer representation;
- declares `age` as `unsigned long`;
- calls the correctly named private `lruv_cache_bytes_to_pages` helper;
- uses `MIN(age, 9999)` without a compensating cast;
- names the two private functions in the source inventory as
  `lruv_cache_bytes_to_pages` and `lruv_cache_verify`, not `_code_<address>`.

There is no `_ReadWriteBarrier`, volatile access, register hint, inline
assembly, forced-inline annotation, fake dependency, inactive-union overlay,
raw address identifier, candidate-only helper, or manual byte control.

## Measurements

The PID-safe complete-TU gate reports `16E / 2R / 0U`, so all sixteen exact
siblings survive. A one-unit objdiff report gives:

| Function | Target | Candidate | Relocations | Target hash | Candidate hash | Objdiff |
| --- | ---: | ---: | ---: | --- | --- | ---: |
| `_lruv_debug_to_file` | 464 | 464 | 20 / 20 | `fb7c7a22cacb060e9e3fa2e5a57942daa52e259dccc6b7796bf744bf28d291bb` | `8b2776c4666f22765aba53a34d4bff1f09d4223456d38fb1ee53b3ae69b355aa` | 91.38323% |
| `_lruv_block_new` | 1,312 | 1,312 | 48 / 48 | `d008a8c97b45d969b3d824f6e3daa6caa018c04e9092b80c4ec64d84e3ab954b` | `9420f4204689f729b2a0ad60ec74481598e2c207d13e312eccdf559f614e3b5b` | 94.18722% |

The typed debug source improves the unit's ordinary fuzzy score from
96.644485% to 96.83662%. Its symbolic relocation-target multiset matches all
20 target relocations. The three raw identity differences are only the split
target's undefined shared owners for `""`, `" "`, and `"\n"` versus the
candidate TU's local literal COMDATs. The block allocator's 48 raw relocation
targets also match as a multiset; its late call order differs after register
allocation diverges.

## Evidence boundary

The accumulated LRUV record includes fifty-five source-level experiments,
the two Demon-oracle variants, the unsigned-age one-shot, and a sweep of twelve
unique historical source blobs. The inadmissible historical size-equal body
used `_ReadWriteBarrier`; it remains rejected. The current typed versions are
the strict-best admissible semantic source, and further declaration, lifetime,
fallthrough, helper, ternary, register-hint, and compiler-option permutations
did not produce byte equality.

Both residuals are therefore classified as register allocation, not semantic
gaps. Reopen them only for authenticated January source/local records or a
natural same-compiler source/lifetime donor that changes the first divergent
callee-saved-register cycle. A higher fuzzy score alone is not sufficient.

## Validation before integration

- Complete PID-safe LRUV gate: `16E / 2R / 0U`.
- Target-owned report data: 1,180 / 1,180 bytes.
- `_lruv_block_new` fingerprint: unchanged.
- Fake-match scan: zero findings.
- Address-name, barrier, volatile, and assembly scan: clean.
- JSON parse and `git diff --check`: required at integration.
- Final park-manifest, full build, semantic, regression, test, and treemap
  validation belong to the canonical integration checkpoint.
