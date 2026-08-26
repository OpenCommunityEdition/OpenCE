# Transport endpoint Winsock two-leaf rejection (2026-08-26)

## Result

This bounded wave tested `_transport_dispose` and
`_transport_network_available` in the first and only ordinary code-producing
candidate compile. Neither function matched January exactly. Both candidate
bodies and their three required includes were removed wholesale; the owner
translation unit is byte-identical to its pre-wave source and no code, data,
configuration, status, exception, or parked-function change is retained.

The result is therefore zero accepted functions, zero accepted code bytes,
zero accepted relocations, zero data bytes, and no change to the object's
`NonMatching` status.

## Immutable evidence boundary

The complete precompile evidence packet and first artifact remain preserved
under `build/audit/transport-lifecycle-20260826/`.

- Base commit: `34793596bffce0f6a4341f89d06e06afcae23f37`.
- Owner TU:
  `source/bungie_net/network/transport_endpoint_set_winsock.c`.
- Frozen candidate source Git blob:
  `2c36c1a5b527d6b449d26d27396ec59b676fe755`.
- Frozen candidate source SHA-256:
  `be4c3e3437d60d8405582fd87275eea9ebdef9975e559e00517b693b98fadab5`.
- January object: 14,232 bytes, SHA-256
  `912eb719656c8e64376d86499b22a7505144ca7e4a485b960d472ad1420713c1`.
- Empty pre-wave object: 541 bytes, SHA-256
  `205e99970f4d684f7071a3a799684c4e046a3a0ca2a157611d33f38ea6661b77`.
- First candidate object: 2,063 bytes, SHA-256
  `5ab9bcb6f23e7a72d32303b1bf4e64949557ca48f4733e64b9faa663d8b1f4d4`.
- Microsoft compiler 13.00.9254.1 SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Natural flags: `/nologo /c /O2 /Oy- /DDEBUG /Dxbox`, XDK 3911, and
  the unchanged repository include graph.

VC7.1 `/Zs` passed before the sealed ordinary compile. No source byte changed
between the seal and emission. No retry was performed after the failed first
artifact.

## Strict comparison

The hardened comparator checked padded size, normalized bytes, relocation
count, COFF relocation type, destination identity, and addend.

| Function | Target meaningful / padded | Candidate padded | Relocs | Target SHA-256 | Candidate SHA-256 | Result |
| --- | ---: | ---: | ---: | --- | --- | --- |
| `_transport_dispose` | 41 / 48 | 48 | 4 / 4 | `3711768c555070d3283baaee32930eae990a21d6b0f267716d353c99ce483a7f` | `4a72141af54193456cd99b4b34d00297c9db2a2a948dd832a45c566204ef0eed` | reject |
| `_transport_network_available` | 8 / 16 | 16 | 1 / 1 | `73954fa0da7600bd1a60ddb3387eafabb788779227be715ec886a2fb1bdfe783` | `eef4bf49c6b243bcaae881c7f8494b0d0f6d9e65505d6dbe7d8138d1f2b12b2b` | reject |

`_transport_dispose` preserved the four destination identities but emitted a
different instruction topology. January relocations occur at function offsets
`+0x01`, `+0x0D`, `+0x12`, and `+0x1B`; the candidate emitted them at
`+0x01`, `+0x0F`, `+0x14`, and `+0x1A`. The first destination is
`_transport_initialized`, the middle two are `_WSACleanup@0` and
`_XNetCleanup@0`, and the last is `_transport_initialized` again. Equal
padded size and destination count do not overcome the byte and relocation
address mismatch.

`_transport_network_available` repeated the previously rejected compiler
shape: the candidate masked EAX, while January masks AL. Its lone REL32 call
still targets `_XNetGetEthernetLinkStatus@0`, but the normalized bytes remain
nonexact. The evidence-backed pre-mask cast did not change VC7.1 emission.

## Storage ownership

The candidate left `_transport_initialized` undefined and defined no BSS or
COMMON owner. Its XDK include closure also emitted inline-support data symbols
`_D3DTEXTUREDIRECTENCODE`, `_D3DSIMPLERENDERSTATEENCODE`, and
`_D3DPRIMITIVETOVERTEXCOUNT`; none is claimed or retained.

January's eight-byte BSS section containing `_transport_initialized`,
`_transport_global_client_active`, and `_transport_global_key_depth` remains
unclaimed. This wave makes no runtime-data ownership assertion.

## Source restoration and policy

After rejection, the owner TU was restored to Git blob
`8fa59b46aa300c12a9e0e63b36c08b2be97763fd` and working-tree SHA-256
`73bc26f42e5ae734de3555b6e73a9aab8131beb3be1a60c383bc2dae6d962159`.
That identity is the exact pre-wave skeleton. Only this ledger is retained as
a tracked change.

The rejected source used ordinary typed C and the required no-argument layout,
but source style is not matching evidence. No same-wave source-shape tuning,
compiler-control experiment, declaration perturbation, or second ordinary
compile was attempted. Reopening either leaf requires genuinely new binary,
prototype, compiler, or cross-build evidence and a newly frozen wave.

The semantic donor and tooling context considered for the frozen decision
included the Stian Halo C source, HCEA source and type material, the January
cross-build bridge, the earlier typed-tiny-leaf rejection, and the Claude
policy documents at:

- `C:/halo-worktrees/claude-ai-conversation-2/AGENTS.md`;
- `C:/halo-worktrees/claude-ai-conversation-2/CLAUDE.md`;
- `C:/halo-worktrees/claude-ai-conversation-2/docs/verification_policy.md`;
- `C:/halo-worktrees/claude-ai-conversation-2/docs/references/prototype-inference.md`;
- `C:/halo-worktrees/claude-ai-conversation-2/docs/references/kb-update-policy.md`.

Those sources remain provenance and semantic evidence only. January PC bytes,
relocations, symbol ownership, and natural VC7.1 output remain authoritative.
No push, amend, or history rewrite is performed.
