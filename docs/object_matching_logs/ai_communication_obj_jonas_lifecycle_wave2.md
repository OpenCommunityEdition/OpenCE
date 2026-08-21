# `ai_communication.obj` Jonas lifecycle wave 2

## Result

This bounded wave starts from authoritative integration commit `e450d186` and
adds four independently strict lifecycle/conversation leaves to
`source/ai/ai_communication.obj`. The object advances from 5/48 to 9/48 strict
functions while retaining all 6,612 previously recovered typed-data bytes and
all 22 section owners.

| Accepted function | Meaningful code | Padded section | Relocations | Normalized SHA-256 |
|---|---:|---:|---:|---|
| `_ai_communication_dispose_from_old_map` | 13 | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| `_ai_communication_packet_new` | 83 | 96 | 5 | `786d860ed9d85c9e31a08095fa263e9a475af9c6cc0ca163f375ec67284e75b8` |
| `_ai_conversation_advance` | 129 | 144 | 9 | `12e41cfb13047b8617e9ba19f131b897291dc245ea183d23ed6b22aa486ea918` |
| `_ai_conversation_stop` | 138 | 144 | 10 | `6209aa75bea24d51f6c8f1232450b8ec99f484ff96836c0542e26c8159de52da` |
| **Wave delta** | **363** | **400** | **26** | |

The hardened comparator proves padded bytes, relocation count/order/type,
semantic relocation destinations and addends, and normalized bytes. The four
definitions are ordinary readable C. There is no assembly, volatile access,
force-inline directive, pragma, compiler barrier, raw-address reconstruction,
type pun, overflow match, synthetic anchor, or byte/code-generation forcing.

## Signature and semantic evidence

The HCEA source/PDB supplies the public signatures, conversation-datum prefix,
scenario conversation shape, and lifecycle intent. The January split object is
the final code and relocation authority.

- `ai_communication_dispose_from_old_map` invalidates the existing
  conversation data array through `data_make_invalid`.
- `ai_communication_packet_new` applies the authentic assertion, clears the
  typed packet, and initializes its four sentinel fields to `NONE`.
- `ai_conversation_advance` and `ai_conversation_stop` use the established
  `ai_conversation_datum_header` prefix. That single shared type now names the
  authentic accessed fields at offsets `0x00..0x13`; offset `0x0A` is kept as
  the unused `byte reserved0A[2]`, not invented as a word field.
- The scripting iterators use typed scenario block access. Advance sets
  `told_to_advance`; stop calls `ai_conversation_finish` with both Boolean
  arguments false.
- The print guard reads frozen `ai_debug.__unknown3C[99]` as its declared
  `char` truth value. A compile-time assertion proves the resulting `+0x9F`
  address, independently corroborated by the frozen
  `hs_globals_external.c` mapping for `ai_print_conversations`.

The shared prefix rename is source-compatible with `action_converse.c`; all
nine functions in that translation unit remain strict after a forced rebuild.

## Fail-closed candidate wave

Exactly six independent candidates were compiled once. Four survived the
strict gate above. The other two definitions and declarations were removed in
full, with no code-generation tuning:

| Rejected function | Target padded / relocs | Candidate padded / relocs | Target SHA-256 | Candidate SHA-256 |
|---|---:|---:|---|---|
| `_actor_communication_team` | 64 / 3 | 64 / 3 | `e381bc05013c03f5e5b56f8cc1dbeba503ed14c30c28872daebeb67c00afbda6` | `d40a945bdcd92ef1aa839ed445b554a4f279587f7f3fb0c917e027bae93bc42d` |
| `_ai_conversation_status` | 272 / 6 | 288 / 6 | `39f1ffa6d82dc98ef2bf97366e49ccbe328b33b9cf789d9f949406aa616af9bc` | `d94c131f64f6807974f1d54c8b6cbd394e8d241306332e6bff67da00f3da1639` |

No rejected body remains, and none of the five reply-filter bodies rejected by
the preceding wave was retried.

## Preservation

The five pre-wave strict functions remain independently exact:
`_ai_communication_dispose`, `_ai_communication_get_type_name`,
`_ai_communication_get_type_by_name`, `_code_00031550`, and
`_ai_conversation_line`.

| Existing typed section | Bytes | Relocations | Owners | Normalized SHA-256 |
|---|---:|---:|---:|---|
| main `.rdata` | 6,336 | 36 | 19 | `7a24cfa7b71aa91b354208ee7a7dd45663c0c37e1025af5f87fbe5835bd17dbc` |
| main `.data` | 276 | 69 | 3 | `d489001ff88ed206b56f44bd0789a8332510f33bf40a312a7b2e5d60c3fe3e03` |

All 105 relocations, owner names/offsets/types/storage classes, and the 6,612
checkpoint bytes remain strict. The accepted functions naturally introduce
only their exact assertion/console literals; no new owned data definition is
added.

The frozen `vehicles.obj`, `ai_debug.obj`, `ai_script.obj`, `actions.obj`, and
`units.obj` sources, configuration, and existing ledgers are untouched. A
forced rebuild after the shared-header cleanup proves all 172 defined code
sections and all 431 non-debug runtime non-code sections strict-identical to
the clean frozen baseline; the five directive sections are identical as well.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: all nine retained `ai_communication` functions
  and all nine `action_converse` functions pass.
- Strict semantic report: 470 units scanned, 3,960 functions evaluated, 3,881
  accepted exact, zero unit errors.
- Progress: Halo 3,701 credited functions and libcmt 167; the combined code
  total rises by exactly 363 meaningful bytes and four functions.
- Object-admission audit: zero candidates and zero revocations, with only the
  inherited `source/shell/shell_xbox` contradiction.
- Parked-function validation: 3 active, 0 stale, 0 invalid.
- Tooling suite: 179/179 tests pass.

A clean post-commit forced replay rebuilds every shared-header consumer and
rechecks the strict object evidence before handoff.
