# `ai_communication.obj` conversation-driver wave

Date: 2026-08-28
Owner: Jonas / Codex
Status: partial object, strict gain admitted

## Scope and baseline

This wave starts from published commit
`98cb738b81b7a4b8c88e2fa3751234b5f068257b`, where
`source/ai/ai_communication.obj` had 24 exact, 0 residual, and 24 unwritten
functions, covering 4,320 of 25,696 padded code bytes. The inherited exact
code set and typed `.rdata`, `.data`, and `.bss` owners are frozen acceptance
boundaries.

The retained change completes the genuine private conversation-driver call
graph: line begin, line perform, line end, and update. It preserves the exact
public `ai_conversation` entry point admitted in the preceding wave. No proof
caller, dead emission anchor, ABI annotation, compiler directive, residual
body, or byte-forcing construct is retained.

## Accepted result

Four functions become strict target-owner COFF matches:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_code_00031e80` | 432 | 16 | `b903d5c15fe893b476f8bc52ca1b0e6308e402f15e76cc37c5d80484264a8157` |
| `_code_00032030` | 656 | 17 | `0b4beb68bed0a2442a401d35e959d7f18c384d8e101d7c106503d9ba3de97e98` |
| `_code_000322c0` | 48 | 4 | `c6ea7e21337b2380f482ffcfa220f859e1240a10edd908b78baad9ca3111ce39` |
| `_ai_conversation_update` | 608 | 27 | `50654da723ce578e3a7abea60c3c9c517cff26ae27888b89afa54c83fa80fe66` |

The inherited `_ai_conversation` remains exact at 336 padded bytes and 25
relocations, with normalized SHA-256
`506dd2e57d64a49e266f40de5ea5f4feb319d6f6a6ac1ced88b6a764b1b2a348`.

Final strict object result: 28 exact, 0 residual, and 20 unwritten functions;
6,064 of 25,696 padded code bytes. The net gain is four functions, 1,744
padded bytes, and 64 relocations. Exact-set comparison reports no inherited
loss and only the four functions in the table as newly exact.

The rebuilt data boundaries remain strict exact:

| Owner section | Bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| main `.rdata` | 6,336 | 36 | `7a24cfa7b71aa91b354208ee7a7dd45663c0c37e1025af5f87fbe5835bd17dbc` |
| main `.data` | 276 | 69 | `d489001ff88ed206b56f44bd0789a8332510f33bf40a312a7b2e5d60c3fe3e03` |
| target-owned `.bss` | 16 | 0 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

## Source-shape and ABI findings

- The four private functions are a genuine atomic VC7 cluster. With the
  656-byte line-perform body absent, externalizing it changes the update
  caller from VC7's private EAX convention to cdecl (`push`, `call`, and
  caller cleanup). Retaining the complete in-translation-unit call graph is
  therefore required for exact codegen.
- The exact line-perform body uses a block-scoped speech item, preserving the
  target stack lifetime and register schedule. The update driver uses an
  outer `if (!line_advance)` guard and a shared terminal return; its told
  branch retains the source-level `if`/`else` fallthrough that produces the
  target topology.
- Conversation definitions, scenario participants and lines, runtime datums,
  and object access are represented by typed views and the repository's tag
  and object access macros. Layout assertions bind every relied-on offset.
- Every function has an explicit terminal return where its return type
  requires one. No inline assembly, intrinsic, pragma, volatile/register
  coercion, raw-address access, undefined behavior, or binary byte-forcing is
  used.

## Audit artifacts

Generated objects remain local evidence and are not committed:

- `build/audit/ai_communication_pre_conversation_driver_98cb738b.obj`
  - raw SHA-256:
    `1366649124984AD68DDF9B107EAA0C60EC40F532E4F31A5E26A9633641515774`
- `build/audit/ai_communication_conversation_driver_integration_exact_20260828.obj`
  - raw SHA-256:
    `EE14C486FB8525A8174ACF4D340ABFBDC83AC23109F15491A7011B2E20D0897D`

Source identity before commit:

- `source/ai/ai_communication.c`:
  `FA9257890DA618E96378308023896E1D7F9059B16FF6CE871B8C85E213F5E464`

## Validation

- Strict whole-unit gate: 28 exact, 0 residual, 20 unwritten.
- Hardened direct comparison: all five retained conversation functions are
  equal in padded size, normalized bytes, ordered relocation addresses,
  types, destinations, and addends.
- Exact-set comparison against commit `98cb738b`: all 24 inherited exact
  functions retained, no loss, and exactly four newly exact functions.
- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` targets pass.
- Semantic audit: 470 units, 4,629 functions evaluated, 4,498 semantic exact,
  4,525 accepted exact, and 0 unit errors.
- Object admission: 0 candidates, 0 contradicted, and 0 revoked.
- Parked audit: 13 active, 0 stale, and 0 invalid.
- Tooling tests: 205 passed.
- Strict campaign board: 271/619 objects, 4,479/8,246 functions, and
  604,311/1,922,669 padded bytes.
- Full progress: 556,399/2,198,102 code bytes and 4,490/11,060 functions;
  halobetacache: 543,485/1,770,166 code bytes and 4,323/7,574 functions.
- `build/report.json` SHA-256:
  `49153C08ADB97D8D232C3A9D14A3B14F9026F8B4CDCE258F1651778C10B4146C`.
- `build/semantic_report.json` SHA-256:
  `C6126F272D36FFB4B95C326FD9C778FD69C1C55944BFDA1549B0F82146BD5D02`.

## Provenance and doctrine

The implementation and admission decision were reconciled against:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halo/CLAUDE.md`
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `tools/campaign/README.md`
- every preceding `ai_communication.obj` matching ledger through
  conversation-start wave 6
- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
- Stian Halo commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`
- Demon commit `ef8c62160d3c6cd0112614ca01020350140b432b`
- Claude-history commit `f9782194dea35c5c71528acc29310e3a211d3c5b`

Donors and Claude documentation supply semantic names, types, recovered
topology, and compiler/tooling guidance. January's Halo CE Xbox COFF remains
the sole admission authority.

## Continuing frontier

Speech work continues through its genuine inbound caller: the 416-byte
private speech helper is exact, and its 912-byte caller has reached a
same-size, same-instruction-count final scheduling tie. The actor-looking lane
is being developed independently against the authenticated HCEA topology and
Xbox target tables. Neither residual lane is retained in this wave.
