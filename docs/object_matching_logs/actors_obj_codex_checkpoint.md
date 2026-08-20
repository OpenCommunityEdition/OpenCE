# `actors.obj` Codex exact-setter checkpoint

## Scope and result

- Integration base: `cc7b74b732369c5d3ddd659bd03855c10a912aae`.
  That base already contains the corrected `actors.h` ABI from `81e69388`.
- Historical commit `35aa1c363117bbe907cd1bfffc36a76f48729fc9`
  was used only as a source lead. Its four readable typed setters were rebuilt
  independently against the current January target; no other donor source,
  header, configuration, or documentation change was imported.
- The untouched base was 17/76 strict-exact functions, 1,016 meaningful code
  bytes, 1,152 padded bytes, and 72 relocations. The final forced XDK 3911
  build is 21/76, 1,274 meaningful bytes, 1,440 padded bytes, and 80
  relocations.
- The strict gain is four functions and 258 meaningful bytes. The object stays
  honestly `NonMatching`; no whole-object admission or parked credit is
  claimed.

## Strict function evidence

The hardened comparator agrees on padded bytes, relocation count and identity,
and normalized bytes for each added function. Their target and rebuilt public
owners also agree at COFF storage class 2 and function type 32.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_unit_control_crouch` | 57 | 64 | 2 | `8ee39009874b2d5e8cbb3db2aca3eefb42d0f6e56ca8f3a7aa2f261ecbb5f850` |
| `_actor_unit_control_primary_trigger` | 81 | 96 | 2 | `7d2c0bc6e6a8b49a39f01d9a0cce6827439c176302aa472c9c6b4b272bcc0250` |
| `_actor_unit_control_secondary_trigger` | 63 | 64 | 2 | `c32dd71b0f2b8e605dc0446fc6ebebbe31299c7478716adf68156b6bdfde1ff8` |
| `_actor_unit_control_exact_facing` | 57 | 64 | 2 | `223360249e08e2749dab433a0fd0757f33a58f7f19efb039a2ebc4cd1dd7dd50` |

Each body obtains the typed `actor_datum` through `actor_get`, updates the
named control flag or analog-trigger field, and ends with an explicit
`return;`. There is no assembly, volatile scheduling device, force-inline
annotation, optimizer pragma or barrier, undefined behavior, byte forcing,
or raw-offset datum access.

## Whole-TU, data, and symbol audit

The pre-edit regression manifest freezes all 17 accepted functions and every
rebuilt non-code and symbol-ownership record. All 17 baseline functions remain
strict exact after the additions. The candidate's 15 non-code sections (412
logical bytes) are fingerprint-identical to the baseline, so this checkpoint
changes no data and claims no data credit.

The fail-closed pre-edit check reports the four functions as `NEWLY_EXACT`, as
expected. It also reports three accepted-function fingerprint changes and a
symbol-set change. Those findings were inspected rather than waived:

- `_actor_unit_control_jump`, `_actor_unit_control_throw_grenade`, and
  `_actor_unit_control_stop_animation_impulse` retain identical padded size,
  normalized bytes, relocation addresses and destinations, section flags,
  COMDAT selection, and public owner. Only their numeric COFF section indices
  move as the four naturally ordered function sections are inserted.
- The symbol inventory grows from 75 to 84 records: four public function
  owners, four corresponding `.text` section symbols, and the ordinary
  `__fltused` undefined symbol required by the real-valued primary-trigger
  parameter. Apart from the same numeric section-index shifts for the three
  existing setters, no ownership record changes.
- No `DATA_CHANGED`, lost exact function, compiler/configuration drift, or
  changed nonexact-function finding is present.

A clean post-commit snapshot followed by deletion and forced rebuilding of
`actors.obj` is the stable same-path proof for the reviewed new symbol shape.

## Corrected-header and frozen-object boundary

The integration base already includes the `81e69388` correction and its
same-path old/new declaration A/B: 26 direct `actors.h` consumers plus two
additional frozen units were byte-identical after normalizing only the COFF
timestamp. That recorded proof includes `vehicles`, `ai_debug`, `ai_script`,
`actions`, and `units`.

This checkpoint changes only `actors.c` and this new Codex ledger. It does not
modify `actors.h`, any frozen source, configuration, semantic/park ledger, or
pre-existing Markdown, so that consumer proof remains applicable.

## Reproducibility and gates

- Source SHA-256:
  `8e6c2cf4acf64143d1a81a2ce239bfee6abb82d28db48b0b5d7049adf3ceb98c`.
- Corrected header SHA-256:
  `7d5fd4146ab86f65eae81608ecb6c3f43fcecb7095a01aab6f0ff8fea94485c6`.
- January target object SHA-256:
  `632a2b344e779e5d58e4d9e8e881700d8d106d51fdc3b74cc731aff9e7c3d9b9`.
- Forced rebuilt object SHA-256:
  `2b87719a6a19c4d280fca49e3a8997c58e8b1c16df47722394bde36445ddbfc8`.
- Full `halobetacache_build` and `libcmt_build` pass.
- Semantic audit: 470 units, 3,845 functions evaluated, 3,704 semantic
  exact, 3,768 accepted exact, and zero unit errors.
- Campaign progress: 375/833 complete objects, 3,757/11,060 exact functions,
  459,500/2,198,102 exact code bytes, and 1,803,160/4,176,062 matched data
  bytes.
- Admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, and zero invalid.
- Tool suite: 179/179 tests pass.

No tracked file is deleted, no user-frozen object is edited, no existing or
Claude-authored Markdown is changed, and nothing is pushed.
