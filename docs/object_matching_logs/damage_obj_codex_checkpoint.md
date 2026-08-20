# `damage.obj` Codex checkpoint

This Codex-owned ledger records a bounded, source-only recovery in
`source/objects/damage.obj`. The unit remains `NonMatching`: 19 of its 34
functions and all target-owned data remain open. No pre-existing or
Claude-authored Markdown was edited or deleted.

## Scope and provenance

- Integration base: `6fb0a3d8fe36294e2bb3d41ab23519f233cb1c8d`.
- Historical donor: `6d90e8e9aec1cce8bc127a3cfd3a853760303acd`.
- Donor `damage.c` blob: `138e904acf7a14457edc6d2145301a3975a10a63`.
- The retained delta is exactly the two public, already-declared vitality
  helpers. No header, data declaration, configuration, semantic ledger, or
  park changed, and no nonexact donor work was retained.
- The user-frozen `vehicles`, `ai_debug`, `ai_script`, `actions`, and `units`
  objects are outside this lane and untouched.

## Strict code result

The untouched base measured 13/34 exact functions and 507 meaningful code
bytes. The candidate measures 15/34 and 675 meaningful code bytes: a strict
gain of two functions and 168 bytes. Both functions are ordinary objdiff
exact and independently semantic-COFF exact; no exception ledger is used.

| Exact function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_object_get_actual_body_vitality` | 84 | 96 | 3 | `4514a53371a45877cdabddb820056af0bc444119fb22d42240ec94ab6444c47d` |
| `_object_get_actual_shield_vitality` | 84 | 96 | 3 | `79186eac1d5a4f2fe306a73f7e19bc581d669130661c0d7f0661fcfa44d0b1bc` |

Each function has the target's three relocation identities and addresses:
two calls to `_object_get_and_verify_type` and one call to
`_game_difficulty_get_team_value`. All 13 functions accepted at the base
remain ordinary and semantic exact.

## Data, symbols, and regression boundary

No source-owned data was added or changed, and this checkpoint claims no data
credit. The target's 760 non-code bytes comprise 76 bytes of BSS and 684 bytes
of RDATA and remain open.

A clean fail-closed whole-TU snapshot was captured from the authoritative
base before the source change. The candidate comparison identifies exactly
the two helpers as `NEWLY_EXACT`; no previously nonexact function changed.
Adding two code sections necessarily expands the rebuilt COFF symbol table
and changes compiler section-number fingerprints for six already-exact
relocation-bearing functions and the shared `1.0f` constant COMDAT. Those raw
ownership deltas are not suppressed or credited as data. Ordinary objdiff
and independent semantic comparison confirm that all 13 pre-existing exact
functions remain exact. A clean post-commit snapshot and forced rebuild are
used as the reproducibility baseline for the resulting 15-function state.

## Source quality and gates

The retained implementation is readable typed C. It uses no assembly,
volatile scheduling device, forced inline, optimizer pragma/barrier,
undefined type-punning, byte forcing, or raw layout cast. Parameters follow
the one-per-line convention.

The full Halo and libcmt build graphs pass. Semantic audit reports 470 units,
3,857 functions evaluated, 3,717 semantic exact, 3,781 accepted exact, and
zero unit errors. Campaign progress is 375/833 complete objects,
3,770/11,060 exact functions, 460,654/2,198,102 exact code bytes, and
1,803,160/4,176,062 matched data bytes. Halo progress is 273/468 objects,
3,603/7,574 functions, 447,740/1,770,166 code bytes, and
1,797,968/3,923,451 data bytes.

Admission reports zero candidates and zero revocations, with only the
inherited `source/shell/shell_xbox` contradiction. Park validation reports
three active, zero stale, and zero invalid. The complete tooling suite passes
179/179 tests.

The unit remains `NonMatching`; no GitHub push is performed.
