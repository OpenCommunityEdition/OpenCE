# ActionObey shared-schema residual proof (final)

Date: 2026-09-05

Scope: the two `runtime_or_owner_changed_REVIEW` sections selected by the frozen
58-consumer shared-schema audit.  This proof reads only the frozen before/after
inventories and objects.  It does not read live objects or source, compile,
run a gate, refresh a park, or decide packet ownership.

## Frozen inputs

- Before inventory SHA-256:
  `90becfaff10da7f9e549e43d28385da41c37675d51d3c9319347bb7b71600253`
- After inventory SHA-256:
  `b7fef54d4e8030efd58299c5dd290f7e3e9a0ba0c80e8bd8aaa70a929d95c831`
- Infection objects:
  `8bb55ad955ced508c5db6cd768ef437575b3b6e6a83f2140757282854197f531`
  -> `80caf88f6bc3db25ecffa0488ac480eba0997618be066f5d2cfca6b8fd76de62`
- Communication objects:
  `329718faba04602a5809c17849c23f824ee483b4f854c58bc62d11c7c3e97141`
  -> `be4f557e04d2b99155594435035bef9705c6e317e85162efe9494d1f5909ed5e`

## Result

Both residuals preserve their public ABI, COFF owner/linkage/type/value,
section flags/alignment/COMDAT selection, relocation identities and topology,
control-flow topology, and observable behavior for a valid call.

### `_infection_swarm_control`

- Size remains 3,360 bytes and all 103 relocations map exactly.  Thirty-eight
  relocation source offsets move with the shortened encodings.  The six
  changed internal table destinations map as
  `1254->1252` three times, `1701->1697` twice, and `1793->1789` once; the
  table slots themselves remain fixed.
- All 130 direct branches and both indirect table jumps map exactly.  No branch
  enters either alignment/trailing-padding interior.
- The only executable changes are three `AND EAX, imm32` encodings becoming
  their sign-extended `imm8` forms: `0x0000fffe -> 0xfffffffe` twice and
  `0x0000fff7 -> 0xfffffff7` once.  Exhaustive evaluation of all 65,536 16-bit
  flag inputs at all three sites (196,608 comparisons) is exact.
- The first two sites explicitly zero EAX before loading AX.  At the third
  site, upper EAX is not assumed zero: only AL is tested and AX is stored;
  `TEST AL,1` overwrites the `AND` flags before the branch, and each successor
  fully defines EAX before any full-width read.
- The exact allowed padding bytes are asserted, not merely described: old
  `LEA ESP,[ESP]` becomes `JMP 0x940` plus skipped `LEA EBX,[EBX]`; the old
  post-`RET` `NOP` becomes post-`RET` `LEA ECX,[ECX]`.
- After masking relocation slots, the complete 80-byte `[0xcd0,0xd20)` table
  and tail are byte-identical, SHA-256
  `eb8ad26cf1b215d3d0731e00af0d53823547fa6cd4254298bafa1996d58f96c1`.
- No runtime/data section was added or lost, no COMMON changed, no definition
  transition occurred, and the complete function-owner name inventory is
  unchanged.  Shifted compiler-local labels are fully accounted for by the
  same offset map.

### `_ai_communication_update_speech_timers`

- Size remains 672 bytes and all 43 relocation records are byte-for-byte
  identical.
- The only change is the order of adjacent loads in `[0x167,0x16e)`:
  `DI <- word [EBP+0x10]` and `EBX <- dword [EBP-8]`.  They read distinct
  stack slots, write disjoint registers, and do not touch flags or memory.
- Bytes before `0x167` and from `0x16e` onward are identical.  All 27 direct
  branches are identical, two enter at the block boundary `0x167`, the jump at
  `0x165` rejoins at `0x16e`, no branch targets the strict interior, there are
  zero indirect jumps, and the block contains zero relocations.
- No runtime/data section was added or lost, no COMMON changed, no definition
  transition occurred, and the complete function-owner name inventory is
  unchanged.

This is a frozen-object equivalence proof for these two deltas, not a blanket
exception for future schema packets or other compiler-shape changes.  It does
not replace the root owner's admission, park, build, or stable-gate decision.

## Artifacts

- Hardened proof script SHA-256:
  `b9aa2a4c8b4509c368b9bbd96d5907abd4944202b635edc29c852807e2df74b0`
- Final v2 JSON SHA-256:
  `2a673deafe4714f0ad74ff074feb24d8cb34cb4f87fe6485fbca89d5911c2e4f`
- Preserved preliminary v1 JSON SHA-256:
  `b44e5ed4c6d5e469a0ae821fbd4b91aa6af04393242238d0df7e4670081226c8`
