# `tiff_file.obj` recovered envelopes and data ownership (2026-08-27)

## Result

This wave restores the strongest documented ordinary-C reconstruction of
`source/bitmaps/tiff_file.obj` after an integration regression. The unit
remains honestly `NonMatching` at 1/3 strict functions, but both residual
functions again have January's exact padded size and relocation count, and the
complete 512-byte BSS owner is now strict exact.

The unit's ordinary data result advances from 412/924 to 924/924 bytes. The
repository gains 512 matched data bytes. Strict code totals remain unchanged
at 268/619 whole objects, 4,338/8,246 functions, and
562,375/1,922,669 padded bytes.

## Provenance and bounded scope

- Starting integration commit: `841f8e2b8e9b1ee2d4e1b1438dd224cc5ed3cb31`.
- Regressed source blob: `0856f17b7942aa925fee017b05d81b79db3ed00d`.
- Retained source blob: `8d9e1ad1e8179b59d7d2089376d31efd9c16dc05`.
- Retained source SHA-256:
  `eedcea03a27688f3267dce8a581191834fb4831fc3554d1282dd6159d4651811`.
- Preserved source artifact before code-neutral prototype formatting:
  `work/tiff-file-closeout-atomic/source/bitmaps/tiff_file.c`, blob
  `ca1acb6d1b3963ac35357318b224627b89efd088`.
- January object SHA-256:
  `af7ebe9529bf233229c81c99690ca3ddd381dfaaa1225a03930f578d1f3394a6`.
- Candidate object SHA-256:
  `617f6430a2287f37b10af7f9801a2b031afe9dea64f3f69bfbcf7f2418c7a911`.

All reachable `tiff_file_obj.md` ledgers were reviewed before integration,
including the atomic closeout record and its do-not-repeat matrix. The source
artifact is the retained result of that measured work, not a new tuning
experiment. Existing Claude-authored ledgers are not edited.

Only `source/bitmaps/tiff_file.c` and this new ledger change. No header,
configuration, compiler flag, semantic exception, parked entry, comparator,
tool, target object, or unrelated source changes.

## Recovered source facts

The retained source restores the evidence-backed topology recorded by the
prior measurements:

- `_bss_0031c320` is a natural initialized 512-byte array, causing VC7 to own
  the zero-filled BSS section rather than emit an undefined COMMON symbol.
- `_tiff_export` uses nested open/allocation success bodies, short pixel-loop
  counters, short TIFF tag locals, and the measured direct R5G6B5 channel
  expressions.
- `_tiff_import` uses short `x` and `y`, nested file/open/orientation/planar
  bodies, shared cleanup exits, the measured bounds order and clamp ternary,
  and `_snprintf` so the COFF relocation resolves to January's `__snprintf`.
- Function declarations were formatted with one parameter per line. This is
  code-neutral and brings the retained file under the current house rules.

The source contains no assembly, `register`, `volatile`, pragma, intrinsic,
attribute, barrier, forced inline, raw address, pointer/integer
reconstruction, type pun, undefined behavior, synthetic anchor, byte forcing,
object patch, comparator exception, or compiler substitution. No raw tag or
object access occurs in this translation unit.

## Strict measurements

| Owner | Target bytes | Candidate bytes | Target/candidate relocs | Target normalized SHA-256 | Candidate normalized SHA-256 | Result |
| --- | ---: | ---: | ---: | --- | --- | --- |
| `_bss_0031c320` | 512 | 512 | 0/0 | `076a27c79e5ace2a3d47f9dd2e83e4ff6ea8872b3c2218f66c92b89b55f36560` | same | exact |
| `_tiff_get_bounds` | 112 | 112 | 6/6 | `8490c88a00c5d6cb26bf2991c5977406e00a739173a246d0ecd5ae1f9dad604a` | same | exact |
| `_tiff_export` | 1,056 | 1,056 | 37/37 | `bb3cfd23e19a770c7007ddea8af93a4f868d72eed00f65f24706df1a986b14aa` | `ab6499a38e42782360f9b6db387e823e9093167184f9137fe9759ee6d8c60a28` | residual |
| `_tiff_import` | 1,344 | 1,344 | 51/51 | `58ebbc3be5af4535adc2efa460f05275fb970e8c1f9f30ff292f640765a08091` | `6f9acb1e23ddee52f80030f9220f12f6660728d6199abb00b12736ef2fb03cda` | residual |

Before this wave, `_tiff_export` emitted 1,024 bytes and `_tiff_import`
emitted 1,280 bytes. The retained source removes both size defects without
regressing the exact sibling. The two residual hashes remain uncredited.

The prior ledgers characterize the remaining boundaries. Export retains
register residency and expression-scheduling differences. Import retains one
EBX/EDI register rotation after the first 145 byte-identical instructions.
The exhausted declaration, scope, register-hint, holder, barrier, copy,
channel, division, case-order, and live-range families are not retried.

## Frozen compile and validation

The final source first passed the isolated strict gate. A Ninja dry run then
showed exactly one production edge, which was executed once:

```text
[1/1] CL build\base\source\bitmaps\tiff_file.obj
tiff_file.c
```

The first-shot object is preserved at
`build/audit/tiff_file_recovered_envelopes_first_shot_20260827.obj` and has
the same SHA-256 as the production candidate object.

- Full `halobetacache_build` and `libcmt_build`: pass.
- Split, ordinary report, semantic report, and strict progress: pass.
- `tiff_file.obj`: 1/3 functions, 112/2,512 strict padded code bytes, and
  924/924 ordinary data bytes.
- Semantic audit: 470 units, 4,469 functions evaluated, 4,347 semantic exact,
  115 hidden exact / 64,889 hidden bytes, 4,375 accepted, and zero unit
  errors.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: three active, zero stale, zero invalid.
- Tooling suite: 194 passed, with only the known unwritable pytest-cache
  warning.
- `build/report.json` SHA-256:
  `ccfd4f60b481c3d69e4364ed82adea58111d98b90ba20895bd5ade29d4c5430e`.
- `build/semantic_report.json` SHA-256:
  `a43bff6a8db264cbebfba09073c40690edcf88a06463f635bcab8561fa631ae3`.
- `git diff --check`: clean apart from Git's line-ending notice.

The unit remains `NonMatching`; this wave claims the exact sibling, exact BSS
owner, exact residual envelopes, and no more.
