# `biped_limp_noodle.obj` Codex checkpoint

This Codex-owned ledger records two strict footholds in the otherwise
unwritten `source/units/biped_limp_noodle.obj`. The object remains
`NonMatching`; no Claude-authored Markdown is changed.

## Proven functions

Historical commit `f553dfa1b03a88ea195ac47da789b4fbb2341fda`
identified the January getter as an unconditional return of the constant 20.
The `long` return type is material: it emits the target's full-width
`mov eax, 0x14; ret`, whereas a narrow return type changes the instruction.
The no-argument definition uses the repository's explicit `void` form.

The January target and its CodeView line table place the axis repair routine
at source line 535. Its three same-line assertions fail closed on null
`forward`, `left`, and `up` pointers. It normalizes the forward and up axes,
falls back to the global cardinal axes when needed, and rebuilds the basis as
`left = up x forward`, `up = forward x left`, then
`left = up x forward`, normalizing and repairing each rebuilt axis. The local
HCEA source was useful as a semantic oracle, but the January x86 proof was
authoritative: the middle cross-product operand order had to be corrected to
reach strict equality.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_biped_limp_noodle_get_max_relaxation_iterations` | 6 | 16 | 0 | `158d54cf5a2b420bd569e99ead990d116e32e38ad91b0f64ec6fce5f141bc5ea` |
| `_validate_real_vector3d_axes3` | 492 | 496 | 27 | `61124db1c1f059a8e97bfcd2c1828a7dccb4384a436085017ae8034bafcd8723` |

A forced XDK 3911 rebuild agrees with the January target in normalized bytes,
padded size, and strict relocation identity for both functions. A clean
post-commit replay is recorded in the final handoff.

## Honest remaining scope

Four functions remain absent and uncredited:

| Function | Meaningful target bytes |
| --- | ---: |
| `_code_0018ed90` | 1,244 |
| `_code_0018f270` | 1,955 |
| `_code_0018fc10` | 695 |
| `_biped_limp_noodle_relax_nodes_onto_environment` | 244 |

The target also owns 44,936 non-code bytes (44,808 BSS and 128 RDATA). This
checkpoint does not claim partial object/data ownership. Typed experiments for
the orientation helper and relaxation wrapper preserved their target relocation
counts but remained nonexact (688/704 and 240/256 padded bytes respectively),
so both were reverted and emit no code or data. The aggregate BSS layout and
HCEA descriptions remain future oracle inputs, not accepted source evidence.

No configuration, park, symbols file, frozen-object path, or other Markdown is
changed. No assembly, volatile scheduling device, forced inline,
optimizer pragma/barrier, undefined type-punning, or byte forcing is used.

## Reproducibility and gates

- Source SHA-256: `19e77266552eed62dff49aa3b1d0c7d9d352098caaacb995ff1f19e008f51668`.
- January target object SHA-256:
  `7a2586435d62a69b29e51f7328c56dabe230677c87d0aa573605eae380d8e9b8`.
- Forced XDK object rebuild and full Halo/libcmt build graph: passed.
- Semantic report: 470 units, 3,690 functions evaluated, 3,548 semantic
  exact, 3,631 accepted exact, and zero unit errors.
- Progress: 374/833 complete objects, 3,625/11,060 exact functions,
  438,598/2,198,102 exact code bytes, and 1,802,644/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, zero invalid.
- Tool suite: 179/179 tests passed.
- A post-commit snapshot, forced deletion/rebuild, and clean regression check
  are recorded in the final handoff.
