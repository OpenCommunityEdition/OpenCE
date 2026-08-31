# `actor_perception.obj` Boolean-ABI strict integration

## Scope

- Integration base: `fa102b086bebdf6f874145025a05323c8f9d9d3f`.
- Target unit: `source/ai/actor_perception`.
- Audited donor commit: `79275995c484a2637aa5db570030d42a9e02685d`.
- Donor source blob: `6116434971a402509893ddf297957249308068bf`.
- The donor commit was not cherry-picked. Only the acknowledged-prop Boolean
  ABI was reconstructed, then rewritten around the shared typed datums,
  owning headers, and January enum spellings.

## Source and semantic evidence

The remaining mismatch in `actor_perception_become_acknowledged` was not
treated as a byte-shaping opportunity. January emits `setne byte ptr` into a
one-byte stack local and later reloads that byte. HCEA independently declares
the corresponding local as `uint8_t had_orphan`; its acknowledged-stimulus
path also carries `reappearance` and `expected` as byte Booleans. Pastudan and
Stian recoveries corroborate the byte-sized result/out parameters.

The resulting logic is coherent: a prop outside the becoming-unacknowledged
through acknowledged state range records whether an orphan exists, copies the
orphan's persistence fields, redirects references, deletes the orphan, marks
the prop acknowledged, and forwards the two Boolean facts to the stimulus
path. No dead branch, inverted condition, indeterminate read, or otherwise
nonsensical construct is used to obtain the target bytes.

The retained source also applies the current house rules:

- `boolean had_orphan`, `boolean expected_acknowledgement`, and
  `boolean reappearance` express the byte ABI semantically;
- `_prop_state_becoming_unacknowledged` and `_prop_state_acknowledged` replace
  raw state values `2` and `3`;
- `struct prop_datum` from `props.h` replaces the incompatible TU-private prop
  overlay in the acknowledged transition;
- public/cross-TU prototypes live in `actors.h` and `props.h`, grouped under
  their owning source-file headings, and duplicate use-site declarations were
  removed from `actors.c`, `props.c`, and `actor_perception.c`;
- the implementation contains no assembly, `volatile`, optimizer pragma,
  byte forcing, raw offset access, undefined type punning, or added inline
  helper.

## Strict result

`actor_perception_become_acknowledged` is strict-exact:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_actor_perception_become_acknowledged` | 272 | 8 | `fda37b86f7b961ab608089c6e4af22228c7529d3caf3635b3756a038d3aaac49` |
| `_actor_perception_acknowledge` | 208 | 15 | `34d1357aba4fe64c9b4a9827faea666eba8e5a02a3cafcdccba886c959d59737` |
| `_actor_expected_acknowledgement` | 384 | 14 | `7ef8c0e451193ae25b5f2678e163ac268c15608b248bb6edb3921f8b6d6f5557` |

The target object SHA-256 remains
`19704943ab15e64d0bebdfa0688a5779f3efca899bf8631f1f529a995da379e4`.
The rebuilt candidate object SHA-256 is
`0735ca6ee893aa1da7abb1cbaadfeabeef1293211a714a738993683d889feb0d`.

The strict board moves Actor Perception from 32/44 functions and 10,480
padded bytes to 33/44 functions and 10,752 padded bytes. Direct target/base
comparison reports no lost exact function. The owning-header cleanup also
preserves `actors.obj` at 29 exact functions and `props.obj` at 10 exact
functions. Candidate function-symbol and code-COMDAT inventories gain no new
helper; the sole code-section change is the acknowledged transition shrinking
from its former 288-byte nonexact body to the target's 272-byte body.

## Verification

- Forced VC7 rebuilds passed for `actor_perception.obj`, `actors.obj`, and
  `props.obj`.
- The complete `halobetacache_build` and `libcmt_build` passed.
- The current strict semantic audit evaluated 4,986 functions in 470 units,
  found 4,875 semantic-exact and 4,885 accepted functions, and reported zero
  unit errors.
- Park validation reports 13 active, zero stale, and zero invalid entries.
- Admission reports no new completion candidate. Its sole contradiction is
  the already-known `action_converse` completion label after the ordinary-C
  fuzzy park.
- The repository test suite passes 212/212.
- A baseline regression manifest identifies exactly one runtime change:
  `_actor_perception_become_acknowledged` becomes strict-exact. Every formerly
  exact function retains exact code and relocations, and all runtime non-code
  sections remain unchanged.

Moving the public declarations into the owning headers renumbers eleven
compiler-private `$L...` COFF labels in two otherwise byte-and-relocation-exact
functions. The fail-closed regression tool therefore reports two evidence
changes plus a symbol-inventory change. A direct inventory diff proves that
all eleven removed symbols are replaced one-for-one at identical section
offsets, types, and storage classes; only their compiler-generated names
change. This is recorded rather than countered with artificial blank-line or
declaration shaping, which would violate the source-authenticity rule.
