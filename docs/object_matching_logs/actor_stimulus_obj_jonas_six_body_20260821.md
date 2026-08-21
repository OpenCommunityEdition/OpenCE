# `actor_stimulus.obj` Jonas six-body wave

## Result

This bounded source-only wave starts from authoritative integration commit
`8de9760a24e3109243b4fd4626c0d1feed876725`. Six HCEA-backed actor-stimulus
bodies were emitted together in one natural candidate compile. Four were
independently strict and are retained. The two nonexact bodies and all of
their support-only declarations and layout checks were removed immediately,
without a spelling, declaration, control-flow, or compiler-control retry.
`actor_stimulus.obj` advances from 0/22 to 4/22 exact functions and remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_actor_stimulus_clear` | 39 | 48 | 3 | `423fad324c96d351a525cf09e9c88eca512d5c82cf2924da92b68c3db08750f4` |
| `_actor_stimulus_suspicion` | 81 | 96 | 2 | `54abeaa1d93b8065aa64ced4e465fabf26bcd4d09b3e28f0ea9aa94b3663db76` |
| `_actor_stimulus_was_surprised` | 31 | 32 | 2 | `18d367e5e41494aa6e8d9aead99667d70c91b31eff2a75829c73ab6ab0c6d868` |
| `_actor_stimulus_vehicle_eviction` | 31 | 32 | 2 | `34bb70138441c64d84e7bd782bbf1ba498eb73ca6b1a46a390eeb3f6289cc842` |
| **Wave gain** | **182** | **208** | **9** | |

The hardened COFF comparator proves identical padded bytes and exact
relocation addresses, types, destinations, and addends for all four retained
functions. The object now reports 182/4,350 meaningful code bytes and four
exact functions. The target's remaining eighteen functions and all 204
non-code bytes stay outside this wave's credit.

## Provenance and signatures

- Baseline `source/ai/actor_stimulus.c` blob:
  `f0ce604cd0c7be78cc0d2c591f2247cc01ee8d00`.
- Retained source blob/SHA-256:
  `09470e37ec25f66954ce6d54665d67c114827e59` /
  `88bfe03af3a4c128e1bc286e13363690e7996932559362006e339d9b4abaac3d`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `cd7250bae74664672ed7c7cd20f3063b81b892472524e1d773884f9c2662a19b`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Its exact-name
  `actor_stimulus_clear.c`, `actor_stimulus_surprise.c`,
  `actor_stimulus_suspicion.c`,
  `actor_stimulus_enter_combat_perceived_enemy.c`,
  `actor_stimulus_was_surprised.c`, and
  `actor_stimulus_vehicle_eviction.c` files supply the natural typed bodies.
- The preserved January-PC reconstruction independently corroborates the
  accessed actor offsets and branch/store semantics. January PDB labels and
  the split COFF remain authoritative for symbol identity, code extent,
  bytes, padding, and relocation ownership.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, with the repository's unchanged XDK 3911 command and flags.

The screened signatures are:

```c
void actor_stimulus_clear(long actor_index);
void actor_stimulus_surprise(
    long actor_index,
    short surprise_level,
    long prop_index,
    real_vector3d const *surprise_vector);
void actor_stimulus_suspicion(
    long actor_index,
    short suspicion_combat_status,
    long suspicion_timer);
void actor_stimulus_enter_combat_perceived_enemy(
    long actor_index,
    long prop_index);
void actor_stimulus_was_surprised(long actor_index);
void actor_stimulus_vehicle_eviction(long actor_index);
```

HCEA establishes the parameter meanings and natural behavior; current
`actors.h` supplies the typed `actor_datum`, stimulus record, `actor_get`,
and `_actor_data` ownership. January codeflow independently proves the signed
short comparisons/stores and complete 32-bit datum handles/timer.

## Authenticated retained behavior and layout

Source-local compile-time checks bind every retained access to the January
layout: `actor_stimulus_data` is 0x64 bytes; `actor_datum.stimuli` begins at
`+0x2EC`; `vehicle_eviction` is at `+0x2ED`; `was_surprised` is at `+0x2F0`;
and the suspicion status/timer are at `+0x34A` and `+0x34C`.

`actor_stimulus_clear` resolves the actor through the ordinary typed
`actor_get` path and clears the complete named stimulus record with
`csmemset`. `actor_stimulus_suspicion` replaces status and timer when the
incoming signed-short status is higher, and extends an equal-status timer
with `MAX`. The two Boolean leaves set their named fields to `TRUE`. All four
bodies use one parameter per line and an explicit terminal `return;`.

The final candidate COFF contains only `.drectve`, `.debug$S`, and the four
code COMDATs. It defines no `.rdata`, `.data`, `.bss`, COMMON symbol, or global
storage, so it claims zero of the target's 204 non-code bytes.

## One-shot rejection record

The first compiler invocation stopped during parsing because the math headers
used `byte` before its owning `cseries.h` include. No object was emitted.
Adding that owner include first was the permitted parse-only correction; the
next invocation emitted the sole candidate object. No body or declaration
was changed between object-producing attempts.

| Rejected function | Target meaningful / padded / relocs | Target hash | First natural candidate |
| --- | ---: | --- | --- |
| `_actor_stimulus_surprise` | `96 / 96 / 2` | `cb774545048c7ad0b1f9ff51e2a226106db2f494887df8070a71209c5563effb` | Rejected at 112 padded / 2 relocations, SHA `70aa8816843a33be7fa785dfcba6bf7d63fbd1ca75e032429d7b1707ff704eb2`. The natural typed control flow is semantically proven but does not reproduce January's copy/store schedule. |
| `_actor_stimulus_enter_combat_perceived_enemy` | `62 / 64 / 3` | `ccb9feb3be28fe6f7bb5cf52692e9f0da1da3a34081e8a87170e1f988149f101` | Rejected at 64 padded / 3 relocations, SHA `05bfe6d567e9c0cfa66723659dfa03e5f74c654f86ba2ecdea01102f26d4fdfe`. Candidate relocation offsets `+4,+15,+48` differ from target `+4,+16,+51`; January calls the private helper with a register-assisted ABI that an ordinary typed external prototype does not reproduce. |

Both rejected COMDATs are absent from the final object. The rejected
surprise-only offset checks, `props.h` dependency, prop-layout check, private
helper prototype, and transition enum are also absent. Reopen either body
only with independently preserved January source or an authenticated calling
contract; do not retry aggregate-copy spellings or emulate the private ABI.

## Policy, scope, and validation

Only `source/ai/actor_stimulus.c` and this new Jonas-owned ledger are changed.
No shared header, configuration, frozen-five source, semantic exception,
parked record, pre-existing Markdown, or Claude-owned file is edited.

The retained source is readable typed C. It contains no assembly, raw address
or byte-offset dereference, pointer/integer reconstruction, cast or union pun,
inactive-union access, undefined overflow, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, synthetic anchor, or
object-byte forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass (568 actions).
- Direct hardened comparison: all four retained functions pass; both rejected
  symbols are absent from the final COFF symbol table.
- Semantic audit: 470 units, 4,025 functions evaluated, 3,885 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,946 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,933/11,060 exact functions,
  474,550/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds four functions and
  182 meaningful bytes with no data increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.
- The fail-closed pre-wave check identifies exactly the four retained
  functions as `NEWLY_EXACT`, reports `changed_nonexact: []`, and surfaces
  only the expected `.debug$S` and symbol-set changes caused by adding four
  COMDATs. No adjudication or configuration exception is added.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `actor_stimulus.obj` is the final same-path reproducibility
proof. No push is performed.
