# `biped_limp_noodle.obj` Codex checkpoint

This Codex-owned ledger records a strict first foothold in the otherwise
unwritten `source/units/biped_limp_noodle.obj`. The object remains
`NonMatching`; no pre-existing or Claude-authored Markdown is changed.

## Proven function

Historical commit `f553dfa1b03a88ea195ac47da789b4fbb2341fda`
identified the January getter as an unconditional return of the constant 20.
The `long` return type is material: it emits the target's full-width
`mov eax, 0x14; ret`, whereas a narrow return type changes the instruction.
The no-argument definition uses the repository's explicit `void` form.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_biped_limp_noodle_get_max_relaxation_iterations` | 6 | 16 | 0 | `158d54cf5a2b420bd569e99ead990d116e32e38ad91b0f64ec6fce5f141bc5ea` |

A forced XDK 3911 rebuild agrees with the January target in normalized bytes,
padded size, and zero-relocation ownership. A clean post-commit replay remains
required and is recorded in the final handoff.

## Honest remaining scope

Five functions remain absent and uncredited:

| Function | Meaningful target bytes |
| --- | ---: |
| `_code_0018ed90` | 1,244 |
| `_code_0018f270` | 1,955 |
| `_validate_real_vector3d_axes3` | 492 |
| `_code_0018fc10` | 695 |
| `_biped_limp_noodle_relax_nodes_onto_environment` | 244 |

The target also owns 44,936 non-code bytes (44,808 BSS and 128 RDATA). This
checkpoint does not guess their types or claim partial object/data ownership.
Recovered HCEA semantic descriptions exist for the validation, orientation,
and relaxation routines; they are future oracle inputs, not byte-exact source
evidence, and must be reconstructed and gated independently.

No configuration, park, symbols file, frozen-object path, or existing
Markdown is changed. No assembly, volatile scheduling device, forced inline,
optimizer pragma/barrier, undefined type-punning, or byte forcing is used.

## Reproducibility and gates

- Source SHA-256: `ab330c0dc61b92592b7d08be229dee4101e73064b9db1b46c60b0c771a353ece`.
- January target object SHA-256:
  `7a2586435d62a69b29e51f7328c56dabe230677c87d0aa573605eae380d8e9b8`.
- Forced XDK object rebuild and full Halo/libcmt build graph: passed.
- Semantic report: 470 units, 3,689 functions evaluated, 3,547 semantic
  exact, 3,630 accepted exact, and zero unit errors.
- Progress: 374/833 complete objects, 3,624/11,060 exact functions,
  438,106/2,198,102 exact code bytes, and 1,802,644/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, zero invalid.
- Tool suite: 179/179 tests passed.
- A post-commit snapshot, forced deletion/rebuild, and clean regression check
  remain required and are recorded in the final handoff.
