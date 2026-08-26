# `action_charge.obj` Jonas math-helper recovery rejection — 2026-08-26

## Decision

This frozen wave is **rejected wholesale**. All twelve requested math helpers
reproduced their January padded bytes and relocation identities on the only
ordinary candidate compile, but the candidate also emitted an out-of-line
`_magnitude_squared3d` COMDAT that failed the required linker-level ownership
gate. No production source from this wave is retained and no function or data
credit is claimed from it.

The rejection is useful new evidence: the canonical readable body of
`magnitude3d` is byte-correct after inlining, but spelling it as a call to the
header `magnitude_squared3d` helper causes VC7 to materialize an unwanted
out-of-line copy. A later frozen wave may reopen only with a source topology
that removes that helper reference before compilation.

## Frozen provenance

- Branch: `jonas/action-charge-math-recovery-20260826`
- Base commit: `19ed6902ffa755c9b7c7b01cb926ea8af49d5d14`
- January target object SHA-256:
  `76C4C0B62850FFAA42FE5027C3E68B2CC42F7640C1156342C9A9CC4D1125AFEA`
- Clean pre-wave base object SHA-256:
  `C0683E96025CBAC9980D9779786C89B69164502336E7ABCA23874EB6835C40CE`
- Frozen candidate source Git blob:
  `ffc743d088b2c065a53826300514a4e3122ea500`
- Frozen candidate source physical SHA-256:
  `769194E601F4CC9132B8AB90CB825DE07A452896647328179D7D5CC3AC771C08`
- XDK 3911 compiler SHA-256:
  `483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`
- Splitter SHA-256:
  `708F957C37CAA66DF90ADE35AD3D1E65E2617E2CC2C8C2CCF68D8BF5CB20E752`
- Candidate object: 3,839 bytes, raw SHA-256
  `146B19F6DB3DF4163904B15CD5ABD3AB2E0FBA548CB7F2792E100F3425CE306C`.
- Compiler flags and include search path were unchanged:
  `/O2 /Oy- /DDEBUG /Dxbox`.

The source passed the XDK `/Zs` syntax-only gate. A Ninja dry run then exposed
exactly one `CL build\base\source\ai\action_charge.obj` edge. It was invoked
once and its first artifact was preserved before any comparison. No spelling,
declaration, scheduling, compiler, or flag retry followed.

## Strict-exact evidence from the rejected artifact

The hardened comparator reported `all_equal: true` for all twelve new helpers
and the three inherited accepted functions. The twelve helpers contribute 479
meaningful / 544 padded target bytes and six relocations.

| Function | Padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_square_root` | 16 | 0 | `0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7` |
| `_scale_vector2d` | 32 | 0 | `496d82c0615b4ec0cb4211dd21dfbf507d8959357a60895b9fd4d930b554cf95` |
| `_magnitude_squared2d` | 32 | 0 | `9d8460618a63e41e020b2ce8071486fe278a849abccad44e5ea444da9700d014` |
| `_magnitude2d` | 32 | 0 | `17d586569ec882a34a898e8a589fe5140e8d8b79b85e2fd4e7816dfe6210d3fa` |
| `_normalize2d` | 80 | 3 | `8a5659d5dd494fe2d1d880057d1836185ebfb8e81d9d63362a43e6a2cb287890` |
| `_dot_product2d` | 32 | 0 | `defea20c9a86f0deda350c23260a0e093b08da31a2cbcd5e5c08b17164579415` |
| `_point_from_line3d` | 48 | 0 | `9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741` |
| `_scale_vector3d` | 48 | 0 | `6436919fbdc80b5d5fe76d9c92fa146e5ac7d3b4637c9b75aaa62401f0c54a3b` |
| `_magnitude3d` | 48 | 0 | `114f10e7ee4543446daaaafc859a4ec729ba6c8c74c38825d05bd0f00a42352c` |
| `_normalize3d` | 96 | 3 | `67fbd33f204d0c93cce135fcfd37d4ce5b443642edc6ad99230846b91c893e5b` |
| `_dot_product3d` | 32 | 0 | `d564ef69fd99449fdfc29d59d0d844fb17df91369277eaba9574e7c8c3d2ae68` |
| `_subtract_vectors3d` | 48 | 0 | `1d706c27e470449c1ac16f5c5ff47c98ee2be96426f2b8bb949ba79e9d764cac` |

The inherited sentinels remained strict exact:

| Function | Padded bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_action_charge_begin` | 80 | 3 | `c78c6feeb10fda8857834f5264614aa976498059590ee08304aef25f21af8bf3` |
| `_action_charge_update` | 80 | 2 | `f2591b2de350c20aefef42c38623875dca494bb46a51d06c6dcd5947b035976e` |
| `_real_random` | 16 | 2 | `50eeeb3dc58069325659d8bc0b6f2c17bb9f4fdce278b2f1a37edb64f4fcc60c` |

Both normalize functions reproduced the target relocation offsets, types,
destinations, addends, and local ownership for `0.0f`, `1.0f`, and the
double-precision epsilon literal. The three emitted target-owned `.rdata`
COMDATs were also strict exact: four bytes at
`__real@00000000`, four bytes at `__real@3f800000`, and eight bytes at
`__real@3f1a36e2e0000000`. They receive no credit because the wave is rejected.

## Ownership contradiction

The candidate additionally emitted `_magnitude_squared3d` as a 48-byte,
zero-relocation, select-any code COMDAT. No candidate relocation refers to the
out-of-line copy. Its normalized bytes are strict exact against the January
canonical owner in `source/ai/action_alert.obj`:

- 48 padded bytes;
- zero relocations;
- normalized SHA-256
  `0d04579096eeab3a1c45f87774e1eb20ce4856dbac219f624a855e8c9ae99725`.

That cross-object byte equality is insufficient. The acceleration playbook
allows a candidate-only helper COMDAT only when the ordinary linker discards
it. A direct XDK 3911 link of the January canonical `action_alert.obj` and the
candidate `action_charge.obj` was run in both input orders. Both orders fail:

```text
action_charge.obj : error LNK2005: _magnitude_squared3d already defined in action_alert.obj
action_alert.obj : error LNK2005: _magnitude_squared3d already defined in action_charge.obj
fatal error LNK1169: one or more multiply defined symbols found
```

Thus this is not an admissible discardable-helper representation difference.
The target canonical owner is a no-duplicates COMDAT, and the candidate copy
cannot coexist with it. The wave fails before campaign admission regardless of
the twelve function-byte successes.

## Atomic restoration and reopen criterion

The complete macro/definition package was removed after the ownership failure.
The production source is restored exactly to:

- Git blob `3e76470cf8be066072239845cbf08e37aeffa458`;
- physical SHA-256
  `3B798CC7F02BCC50741E88D6C735E9F12780DC4AE645BC69DD48262A6E6D67EB`.

Only this rejection ledger is retained. There is no config, comparator,
compiler, protected-file, source, or matching-label change and no push.

The link failure is genuinely new evidence. A new wave may test one frozen
topology: keep the twelve exact external helper bodies, but express
`magnitude3d` directly as the already-inlined three-component squared-magnitude
expression so the header `_magnitude_squared3d` body is never referenced and
VC7 has no reason to materialize the extra COMDAT. That is a new evidence wave,
not a retry of this rejected source.
