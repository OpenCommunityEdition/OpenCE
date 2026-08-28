# `ai_communication.obj` conversation-start wave

Date: 2026-08-28
Owner: Jonas / Codex
Status: partial object, strict gain admitted

## Scope and baseline

This wave starts from published commit
`e145d3a0654fb687b65b73ea50a63a677887fc43`, where
`source/ai/ai_communication.obj` had 23 exact, 0 residual, and 25 unwritten
functions, covering 3,984 of 25,696 padded code bytes. The inherited exact
code set and typed `.rdata`, `.data`, and `.bss` owners are frozen acceptance
boundaries.

The retained implementation is only the independently emit-able public
`ai_conversation` entry point. The private line-begin, line-perform, line-end,
and update functions remain an atomic research cluster because VC7 assigns
private calling conventions from their real in-translation-unit call graph.
No residual private body, proof caller, dead emission anchor, ABI annotation,
compiler directive, or byte-forcing construct is retained.

This wave also applies the authenticated public declaration correction from
Claude-history commit `f9782194dea35c5c71528acc29310e3a211d3c5b`:
`ai_communication_get_player_rating` takes a `long unit_index`, not a `real`.
The declaration is formatted under the current one-parameter-per-line house
rule. The only current source caller already passes
`actor->meta.unit_index`. January's private communication caller likewise
pushes the datum index bits directly.

## Accepted result

One function becomes a strict target-owner COFF match:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_ai_conversation` | 336 | 25 | `506dd2e57d64a49e266f40de5ea5f4feb319d6f6a6ac1ced88b6a764b1b2a348` |

Final strict object result: 24 exact, 0 residual, and 24 unwritten functions;
4,320 of 25,696 padded bytes. The net gain is one function, 336 padded bytes,
and 25 relocations. Exact-set comparison reports no inherited loss and only
`_ai_conversation` as newly exact.

The rebuilt data boundaries remain strict exact:

| Owner section | Bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| main `.rdata` | 6,336 | 36 | `7a24cfa7b71aa91b354208ee7a7dd45663c0c37e1025af5f87fbe5835bd17dbc` |
| main `.data` | 276 | 69 | `d489001ff88ed206b56f44bd0789a8332510f33bf40a312a7b2e5d60c3fe3e03` |
| target-owned `.bss` | 16 | 0 | `374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb` |

## Source-shape findings

- The exact entry point uses the typed scenario conversation block, obtains
  the runtime datum through `code_00031d50`, and dispatches the three begin
  outcomes through `code_00034020` without manufacturing a private helper
  body.
- A function-scope `result` Boolean and a branch-local `continue_trying`
  Boolean reproduce January's four-byte frame and complete branch topology.
  Every tag access uses `TAG_BLOCK_GET_ELEMENT`, and the function has an
  explicit terminal return.
- Removing the residual `code_00032030` body while retaining its `static`
  declaration fails VC7 with C2129. Externalizing it compiles, but changes the
  exact update caller to cdecl (`push`, `call`, `add esp`) and makes
  `_ai_conversation_update` residual at its first call site. This proves that
  `_code_00031e80`, `_code_00032030`, `_code_000322c0`, and
  `_ai_conversation_update` must be admitted together through their genuine
  static call graph. The isolated exact public entry point does not share that
  dependency.
- The corrected `long` rating prototype changes no exact function in either
  `ai_communication.obj` or `ai_debug.obj`. On the current unrelated residual
  `ai_debug_render_actor`, it changes the candidate envelope from 24,960 to
  24,944 bytes against January's 24,976-byte target; the authenticated Claude
  history records why the integer ABI is nevertheless the correct source type
  and why an older exact state required a separate melee-control fix as well.

## Audit artifact

The generated object remains local evidence and is not committed:

- `build/audit/ai_communication_conversation_start_integration_exact_20260828.obj`
  - raw SHA-256:
    `1366649124984AD68DDF9B107EAA0C60EC40F532E4F31A5E26A9633641515774`

Source SHA-256 identities before commit:

- `source/ai/ai_communication.c`:
  `3BDCD5CBA406253B73769D2192A3A23BBA636229CCF7F2A623B36303AB5D8984`
- `source/ai/ai_communication.h`:
  `31A5611D3C628730FA1E6A6A6F3813AEFE35DBC4E5C9A52045E1D01F6535ADDA`

## Validation

- Strict whole-unit gate: 24 exact, 0 residual, 24 unwritten.
- Hardened direct comparison: `_ai_conversation` is equal at 336 padded
  bytes, 25 relocations, normalized bytes, ordered relocation addresses,
  types, destinations, and addends.
- Exact-set comparison: all 23 inherited exact functions retained, no loss,
  and exactly `_ai_conversation` newly exact.
- Full `halobetacache_build`, `libcmt_build`, `semantic_progress`, and
  `progress` targets pass.
- Semantic audit: 470 units, 4,625 functions evaluated, 4,494 semantic exact,
  4,521 accepted exact, and 0 unit errors.
- Object admission: 0 candidates, 0 contradicted, and 0 revoked.
- Parked audit: 13 active, 0 stale, and 0 invalid.
- Tooling tests: 205 passed.
- Strict campaign board: 271/619 objects, 4,475/8,246 functions, and
  602,567/1,922,669 padded bytes.
- `build/report.json` SHA-256:
  `3F07CD4F39D452FFC33555759D6AA01BAD6B75E88CB2F3D33063AE3722622297`.
- `build/semantic_report.json` SHA-256:
  `A2A45084B4F78DC05AEDD4DED5239577A736E43B67911BDB9C5987085D73ED2F`.

## Provenance and doctrine

The implementation and admission decision were reconciled against:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/halo/CLAUDE.md`
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`
- `tools/campaign/README.md`
- every preceding `ai_communication.obj` matching ledger through public wave
  5
- HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`
- Stian Halo commit `fc11d861865735877d5b8b4c408b1a0c06dde3cf`
- Claude-history commit `f9782194dea35c5c71528acc29310e3a211d3c5b`

Donors supply semantic names, types, and candidate topology. January's Halo
CE Xbox COFF remains the only admission authority.

## Continuing frontier

The private conversation driver remains preserved in its isolated worktree.
Its 432-byte line-begin helper, 48-byte line-end helper, and 608-byte update
driver are exact only with the real 656-byte line-perform body present; the
line-perform body is currently a same-size, same-relocation scheduling
residual and remains unretained. Speech research continues through the
genuine 912-byte inbound caller so the already proven 416-byte private speech
helper can be admitted without a proof-only wrapper.
