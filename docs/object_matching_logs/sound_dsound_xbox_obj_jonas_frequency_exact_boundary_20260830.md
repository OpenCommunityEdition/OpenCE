# `sound_dsound_xbox.obj` frequency conversion exact boundary (2026-08-30)

## Outcome

The first and only production candidate makes
`_dsound_frequency_from_pitch` strict byte-exact at 111 meaningful / 112
padded bytes with all 10 ordered relocations exact.  The three inherited
accepted functions also remain individually strict exact under the direct
complete-COFF comparator.

The candidate is nevertheless not retained.  The fail-closed pre/post
whole-translation-unit manifest rejects the target-order insertion because it
changes accepted section fingerprints for `_dsound_angle_from_angle` and
`_dsound_get`, adds four target-authentic `.rdata` COMDAT owners plus the
associated `.debug$F` contribution, and changes the TU symbol-ownership set.
No adjudication or policy exception was supplied.  Production source and the
selected object were restored exactly; this commit therefore adds evidence
only and claims no new accepted function or byte.

## Freshness, provenance, and type evidence

- Base commit: `ce73a1e91062128c7741ecafd7874f4b1c3db83e`.
- Accepted pre-shot source blob: `17417718ecfd94a9180b972ac0c8a3e1fc320031`.
- Frozen candidate source blob: `b1262f18f0aae56c0b9bc3409047514f9c39c89b`.
- Restored source blob: `17417718ecfd94a9180b972ac0c8a3e1fc320031`.
- January split object SHA-256:
  `c7b04e9b80b2a397fb3c5562129c2c5b0681f483a659a05b6076d057fdea7aac`.
- Pre-shot candidate object SHA-256:
  `9c8f535db317f2677daf15aba826eb4b5b9133aa2b26d02ac227fd469b61600b`.
- Frozen first-shot object SHA-256:
  `2c9010118718ff0a323b180e3bed5da81faf221b2c9196a72d6942fe1bc5ada4`.

The four prior `sound_dsound_xbox.obj` ledgers were read in full.  They recover
the sample-rate getter, angle conversion, and DirectSound getter, and freeze
the attenuation-helper closure; none attempts or freezes this frequency
conversion.  A filesystem census found no production definition in the
campaign trees, and the local commit history contains only the symbol
inventory plus the three accepted sibling waves.

Independent Stian and Pastudan reconstructions both supply the same semantic
body: validate a 32-bit sample rate against 22050/44100, multiply its
single-precision conversion by pitch, clamp to the DirectSound frequency
range, and convert to a 32-bit integer.  They are corroborating later-source
evidence only.  January bytes and relocations establish the exact public cdecl
signature and expression topology.  The XDK header provides the named
`DSBFREQUENCY_MIN` and `DSBFREQUENCY_MAX` constants as 188 and 191983.

The applicable campaign/Claude instructions, acceleration playbook, donor
documentation, all current object ledgers, current source/header topology,
call sites, target disassembly, and ordered relocations were audited before
mutation.  The function is link-closed: it requires only the ordinary
`display_assert`, `system_exit`, compiler-generated `__ftol2`, and select-any
literal owners.

## Frozen natural candidate

The sole code-producing candidate was ordinary typed C89:

```c
long dsound_frequency_from_pitch(
	long samples_per_second,
	float pitch)
{
	float frequency;

	match_assert(
		"c:\\halo\\source\\sound\\sound_dsound.h",
		0x36,
		samples_per_second==22050 || samples_per_second==44100);

	frequency = (float)samples_per_second * pitch;

	return (long)PIN(frequency, DSBFREQUENCY_MIN, DSBFREQUENCY_MAX);
}
```

This uses the current named `PIN` macro and XDK limits.  It contains no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, optimizer
barrier, raw address or offset access, pointer/integer reconstruction,
pointer or union pun, undefined behavior, synthetic anchor, byte forcing,
object patch, compiler exception, or comparator exception.

The source passed `git diff --check`, the policy scan, and a no-output `/Zs`
syntax check.  Exactly one normal Ninja compiler edge emitted candidate code;
there was no declaration, expression, condition, lifetime, constant, source
order, flag, or compiler retry.

## Strict function evidence

January and the candidate emit the same complete instruction stream: two
sample-rate compares and the line-`0x36` assertion, `fild` of the 32-bit
sample rate, multiplication by the float pitch, the `PIN` macro's lower and
upper x87 comparisons, and two natural `__ftol2` return tails.

| Function | Meaningful / padded | Relocations | Normalized SHA-256 | Direct result |
| --- | ---: | ---: | --- | --- |
| `_dsound_frequency_from_pitch` | 111 / 112 | 10 | `54ffb1b0ac8762eeccddcdf7e8f6d9557f34afce76fa23ac276a9051816a3984` | newly exact |
| `_sound_samples_per_second` | 77 / 80 | 6 | `e8b9fca56aaa9d922301b77d72c6debc700ceb7de1d899966324a31b29e24547` | still exact |
| `_dsound_angle_from_angle` | 18 / 32 | 2 | `ea6279c6d854d47a52aaff3dd5306713f73870f27fe26028f56f70f200e2386f` | still exact |
| `_dsound_get` | 18 / 32 | 2 | `384e7aada8b4cd93cf43c6609bf1a179a34d1843d6afb2fb90a8cd29c5f6c103` | still exact |

The frequency relocation sequence is exact at offsets `+25`, `+30`, `+35`,
`+42`, `+57`, `+72`, `+77`, `+85`, `+100`, and `+105`: source-path string,
assert-expression string, `display_assert`, `system_exit`, two lower-limit
literal references, `__ftol2`, two upper-limit references, and the final
`__ftol2`, all with exact types, owners, destinations, and addends.

## Fail-closed whole-TU boundary

The baseline manifest was captured from the clean accepted source and exact
pre-shot object.  Its candidate check reports `_dsound_frequency_from_pitch`
as `NEWLY_EXACT`, and `_sound_samples_per_second` as `still_exact`, but fails
retention on:

- changed accepted evidence for `_dsound_angle_from_angle` and `_dsound_get`;
- new/changed `.rdata` owners for the exact assertion path, assertion text,
  `__real@433c0000`, and `__real@483b7bc0`;
- the associated `.debug$F` change; and
- a changed complete symbol-ownership set.

These are natural target-authentic additions, not wrong function bytes, but
the campaign's fail-closed full-section policy does not permit them to alter
already accepted TU evidence without an explicit reviewed ownership
transition.  The candidate was therefore inverse-restored rather than
retained on function-only evidence.

Do not repeat or tune this function body: it is already strict exact.  Reopen
only as part of a reviewed whole-TU target-order completion wave that can
admit the external function and its four data COMDATs while re-proving every
inherited section, or if the campaign supplies an explicit ownership
adjudication for this measured transition.

## Restored-state validation

After restoring source and object, the whole-TU gate passes with exactly the
three inherited functions `still_exact`, no newly exact entry, no changed
nonexact sibling, no failure, and no warning.  Complete Halo and libcmt builds
pass.  The final gates report:

- semantic audit: 470 units, 4,953 functions evaluated, 4,836 semantic exact,
  4,846 accepted exact, 131 hidden exact / 78,940 hidden code bytes, and zero
  unit errors;
- progress: 384/833 objects, 4,807/11,060 functions,
  610,244/2,198,102 meaningful code bytes, and
  1,856,050/4,176,062 data bytes;
- Halo progress: 282/468 objects, 4,640/7,574 functions, and
  597,330/1,770,166 meaningful code bytes;
- admission: zero candidates, zero contradictions, zero revocations;
- parks: 12 active, zero stale, zero invalid;
- tooling tests: 212/212 pass under `python -m pytest -q` (a direct `pytest`
  invocation was discarded after its launcher omitted the repository root
  from `sys.path` and failed during collection without running tests);
- protected Units `_unit_preprocess_node_orientations`: strict exact at 1,920
  padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- `build/report.json` SHA-256
  `a5c84248c185f532cad281eacedf7d54cc741918cae4ff1a24791f313f0a10ea`;
  and
- `build/semantic_report.json` SHA-256
  `e94ecc24b1f5565187a91d1fc59b61521a4072878ca3c0fabc15e5285a411a63`.

Only this additive ledger is retained.  No source/header/configuration/tooling
file is changed, no accepted credit is claimed, and nothing is pushed.
