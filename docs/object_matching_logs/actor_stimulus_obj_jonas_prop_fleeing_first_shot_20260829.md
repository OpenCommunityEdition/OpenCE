# `actor_stimulus.obj` prop-fleeing first-shot boundary (2026-08-29)

## Scope and authority

This ledger records the single frozen production candidate for
`source/ai/actor_stimulus.obj::_actor_stimulus_prop_fleeing`. The candidate
was a strict miss and was removed in full. Production
`source/ai/actor_stimulus.c` is restored to Git blob
`82a3e128abbbaa536bdc1134310acd383f609cda`; no source or header change is
retained. The object remains at eight exact functions, zero residuals, and
14 unwritten functions.

The lane began at local campaign commit
`46db5d26afe102b6f4c35886f8b442a771b816f7`. Before editing, both existing
object ledgers were read completely:

- `actor_stimulus_obj_jonas_six_body_20260821.md`;
- `actor_stimulus_obj_jonas_public_wave2_20260821.md`.

The applicable campaign, object-ledger, audit, tooling, tree, and authenticated
Stian `CLAUDE.md`/`AGENTS.md` instructions had already been read in this
isolated worktree. `git log --all -G/-S` found no earlier body or emitted
candidate for this function. January PC COFF is authoritative. Target object
`build/split/source/ai/actor_stimulus.obj` has raw SHA-256
`cd7250bae74664672ed7c7cd20f3063b81b892472524e1d773884f9c2662a19b`.
The normal production edge uses the unchanged XDK 3911 Microsoft C/C++
compiler and repository flags.

## Authenticated source and ABI evidence

The HCEA tree at commit
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` contains the public body
`src/blam/ai/actor_stimulus_prop_fleeing.c`, Git blob
`aa7393b1e375ac1465a58d9f20704e03ca7142c9`, payload SHA-256
`5ee2346b7c1380cb539df5efd75dfb8ff931ee94a2e85dc5bdcd38d8975e2960`.
It authenticates the natural semantics and complete control topology:

- resolve the actor, actor definition, and observed prop;
- require a friendly prop, the definition's panic-in-groups flag, and an
  expired flee-with-friends disable time;
- seed desire from the definition's friend-killed panic chance;
- accept either the actor-emotion helper or the global-seed random test;
- preserve an already stronger panic and otherwise begin from the actor's
  current target prop;
- when the observed prop owns an actor that is fleeing from a valid prop,
  translate that unit back to an active prop for the current actor;
- store friend-fleeing panic type 2 and the chosen prop.

January disassembly and canonical typed declarations fix every PC-specific
detail: lookup order is actor, definition, prop; definition `flags2` is at
`+0x04` with bit 5; the panic chance is at `+0x2A0`; actor flee disable time
is at `+0x39C`; panic type/prop are at `+0x308`/`+0x30C`; actor target prop is
at `+0x270`; action/flee prop are at `+0x6C`/`+0xB8`; and prop actor/unit are
at `+0x1C`/`+0x18`. All 14 target relocations resolve to ordinary public
symbols. There is no private helper or anonymous-data ABI dependency.

The frozen candidate was readable C89 with one parameter per line, explicit
terminal `return;`, named enum values, named typed fields, and compile-time
layout checks. It used no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw address or byte offset, pointer/integer
reconstruction, representation pun, undefined behavior, byte forcing,
compiler change, tool exception, or admission exception.

## Frozen one-shot result

After a non-emitting `/Zs` parse/type check, exactly one normal production
object edge ran. The preflight left the baseline object unchanged at raw
SHA-256
`673772e03d2c6dcc1414418fccecf1ebc3b80a1f4d4dacf87ed6aef3d6753b07`.
The emitted candidate was immediately preserved as
`build/audit/actor_stimulus_prop_fleeing_first_shot_20260829.obj`, raw
SHA-256
`6706a89e8baaa03fe5b2db897af612ec51facddd3903b7d997411ee72d66f0d4`.

| Measure | January target | Frozen candidate |
| --- | ---: | ---: |
| Meaningful instruction extent | 263 | 263 |
| Padded COMDAT extent | 272 | 272 |
| Relocations | 14 | 14 |
| Normalized SHA-256 | `13df393dee019d54339f963e17edd5d85610d11ce48fe5c2e357240b22f65da1` | `63e6d570e8620cfab0b7fb0a86dd90a44a34f375f1304928d68eaa1cc7e0c4f1` |

Thirteen relocation addresses and all 14 relocation types, identities,
destinations, and addends are equal. The only address difference is the
second `_prop_data` DIR32 relocation: January places it at `+0xD5`; the
candidate places it at `+0xD4`. Normalized bytes differ at 20 positions,
confined to `+0xBE..+0xD8`.

The decisive boundary is an isolated VC7 instruction-scheduling/lifetime
choice after the nested `actor_get` call. January first cleans the two actor
lookup arguments from the stack, compares `state.action`, then reloads
`state.action_data.flee.flee_prop_index` from the returned actor pointer in
`EAX`. The natural typed HCEA spelling lets VC7 preload that flee-prop value
into `ECX`, then clean the stack and compare the action. Both streams converge
before the `prop_get` result is consumed and retain the exact 272-byte extent
and complete relocation topology, but their `+0xBE..+0xD8` instruction bytes
are not identical.

Closing this residual would require another source spelling or lifetime/
scheduling experiment. The lane permits neither tuning nor a second emitted
candidate. The body therefore receives no credit and must not be retried
without new authenticated PC source evidence that independently fixes the
original local-variable expression and evaluation order.

## Restoration and gates

The candidate body, local prototype, includes, enum values, and layout checks
were inverse-removed immediately. The restored source hashes to the exact
committed blob named above. A normal single-object rebuild removed
`_actor_stimulus_prop_fleeing` again. Hardened padded-COFF and ordered-
relocation comparison reconfirmed every inherited exact sibling:

- `_actor_stimulus_clear`;
- `_actor_stimulus_suspicion`;
- `_actor_stimulus_prop_sighted`;
- `_actor_stimulus_bumped`;
- `_actor_stimulus_environmental_noise`;
- `_actor_stimulus_was_surprised`;
- `_actor_stimulus_maneuvering`;
- `_actor_stimulus_vehicle_eviction`.

Focused campaign gate: eight exact, zero residual, and 14 unwritten of 22.
Final validation passed:

- complete `halobetacache_build` plus `libcmt_build` and report regeneration;
- semantic audit: 470 units, 4,917 functions evaluated, 4,800 semantic exact,
  131 hidden exact / 78,940 hidden code bytes, 4,810 accepted exact, and zero
  unit errors;
- strict progress: 384/833 objects, 4,771/11,060 functions,
  606,187/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- object admission: zero candidates, zero contradictions, and zero
  revocations;
- parked validation: 12 active, zero stale, and zero invalid;
- protected Units `_unit_preprocess_node_orientations`: exact at 1,920 padded
  bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- tooling suite: 212/212 tests passed;
- `build/report.json` SHA-256:
  `19402f6c01804c9d844537e23194000ab2a656cd1a76bc7d32f7c7768a3af302`;
- `build/semantic_report.json` SHA-256:
  `f1e19b7ef56c296d953cc7c7db7bba7f1f379444c230a4a4541bb3ba1b6391de`;
- production source clean and the ledger the only tracked delta.
