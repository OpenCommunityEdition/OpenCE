# `render_particles.obj` Jonas partial salvage

## Result

This bounded salvage recovers the two independently emitted helper functions
and the complete owned profile section.  The large `_render_particles` body is
intentionally omitted and remains unmatched.  `source/effects/particles.h` and
the shared shader/build-sprite layouts are untouched.

| Symbol | Result | Meaningful / padded bytes | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_local_player_is_first_person` | strict exact | 161 / 176 | 8 | `b454d5eb3e4762dca5a8d23906bcb86bc05948eab84d0e425c626385cd243eac` |
| `_code_0017bdd0` | strict exact | 47 / 48 | 0 | `e4b2f7805c3ed41ade998a43155e1eac7f5b0619a42b929451fb53b003c40abc` |
| `_data_0030e180` | strict exact | 1,528 | 1 | `806638334fff1341360b4dec7db0987871f376bc3999e26ff4bcd3a1755d2955` |

The profile section's relocation names the 17-byte `"render_particles"`
select-any string.  csplit leaves that symbol undefined in this split object
and assigns its image owner to `hs_globals_external.obj`; VC7 emits the normal
local select-any COMDAT copy.  Cross-object comparison proves that copy exact
at 17/17 bytes, zero relocations, and normalized SHA-256
`30037c1d6427e0e26bd575ce99b938f28cf1f5c1f26208ed07129ead46cea343`.
The semantic-data ledger entry is therefore a measured objdiff attribution
correction, not a status override.  `render_particles.obj` remains
`NonMatching`.

## Source and policy audit

The first-person helper uses the repository's typed player datum, unit datum,
parent-unit definition, tag block, and unit-seat accessors.  The nested typed
`unit_definition_get` expression is required by January's tag-get relocation
order; naming an extra definition local moved that call and compiled 16 bytes
short, so that rejected draft was restored before admission.

The sort callback now has the standard
`int __cdecl(void const *, void const *)` ABI and immediately binds typed
`rendered_particle const *` locals.  This policy correction preserves the
exact 48-byte body and its definition/cluster/first-person key order.  The
private sort record has an asserted eight-byte size.  The perspective value is
a named enum constant.

All parameters occupy their own lines, both functions have explicit final
returns, and the source contains no assembly, volatile scheduling device,
force-inline directive, optimizer pragma or barrier, raw-address access,
union punning, undefined aliasing, byte forcing, or synthetic anchor.  No
shared particle or shader header was changed.

## Regression and validation

The pre-edit fail-closed manifest was captured from clean authoritative commit
`dd371124269b80d25227a42501ae5ef883f618e0`.  Its candidate comparison reports
only the two reviewed `NEWLY_EXACT` functions, the newly owned profile/string
sections and their symbol inventory, plus compiler debug metadata.  It reports
no changed nonexact function and no lost accepted evidence.  A clean-commit
snapshot followed by a forced rebuild is stable.

Complete validation on the isolated worktree:

- strict comparator: both helpers and `_data_0030e180` pass size, normalized
  bytes, relocation address/type/destination/addend, and ownership checks;
- clean full Halo and libcmt build: pass (574 build/report edges);
- semantic audit: 470 units, 3,923 functions evaluated, 3,845 accepted exact,
  zero unit errors;
- campaign progress after this lane: 375/833 objects, 3,832/11,060 functions,
  463,956/2,198,102 code bytes, and 1,833,776/4,176,062 data bytes;
- object-admission audit: zero candidates and zero revocations; only the
  inherited unrelated `shell_xbox` contradiction remains;
- parked-function audit: 3 active, 0 stale, 0 invalid;
- tooling tests: 179/179 pass;
- `git diff --check`, JSON validation, and forbidden-construct/scope audit:
  pass.

The five frozen large objects were force-built but not edited.  A hardened
runtime A/B against the authoritative dd371 build proves identical function
sets and strict code/relocations for all 172 emitted functions, identical 431
runtime non-code sections, and identical 18 COMMON owners across
`vehicles.obj`, `ai_debug.obj`, `ai_script.obj`, `actions.obj`, and
`units.obj`.  Cross-worktree differences are confined to compiler debug-path
records.

No existing or Claude-authored Markdown was edited or deleted.  No object
status was flipped, and nothing was pushed.
