# `key_agreement.obj` — opus5 next-150K wave n1 (2026-09-15)

## Scope and result

Worker n1, lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, frozen baseline `ae10935da`.
Target row: the PARKED `_complete_key_exchange` (416 padded / 406 meaningful / 17 relocations).
All work was scratch-only; `source/bungie_net/common/key_agreement.c` was **not modified**.

Gate before and after (with `--forbid-emitted-symbol _point_from_line3d`, guard passed):
`== exact 9  residual 1  unwritten 0`. Park drift: `parks 1 drift 0`. Fake-match scan: 0 leads.
Newly exact: **none**.

## Duplicate prevention

Read in full: `key_agreement_obj_fable_small_families_20260901.md` (including its P1/P2 parked-sibling probe
table and do-not-repeat list), `key_agreement_obj_jonas_message_predicate_boundary_20260830.md`,
`config/parked.json` entry, `scratch/w2/laws_w2.md` (A18), `scratch/w3/laws_w3.md` (A24, A34, A45).
`git log --all --oneline -- source/bungie_net/common/key_agreement.c` = 15 commits, none newer than the
ledgers. `branch_sweep.py key_agreement` = 11 unique blobs; the best historical state is `exact 7`, against
`exact 9` at HEAD, so there is no historical body to mine.

## Census (this row is NOT a scheduling tie)

The park is classified `instruction-scheduling`. That classification is **refuted** by the census.

- **Frame.** January `sub esp,0x30`; ours `sub esp,0x30`. Every `[ebp±N]` slot agrees: `[ebp-4]`
  packet_version, `[ebp-8]` encoded_packet_size, `[ebp-0x10]` finalize_packet, `[ebp-0x18]` key,
  `[ebp-0x20]` private-key scratch, `[ebp-0x28]`/`[ebp-0x30]` initiate_packet. Both builds additionally reuse
  the `msgptr` parameter home `[ebp+0xc]` for `packet_type` — laws_w3 A24 fires identically on both sides,
  because `msgptr` is copied into ESI on the dominating entry path (`mov esi,[ebp+0xc]` at +0x0b). No slot is
  missing or added on either side.
- **Relocations.** `relocdiff --count-by-target` reports 17 target / 17 candidate / **0 differing rows**;
  every address, type, owner and addend is identical (0034, 0039, 003e, 0045, 009d, 00a2, 00bc, 00e3, 00e8,
  0104, 0117, 012c, 0136, 013e, 0156, 0164, 017d). No call and no global is missing or added.
- **Instructions.** 167 target / 167 ours. Exactly two hunks, both inside `0x4f..0x74`:

      A)  T  4f  xor ecx, ecx / 51 mov cx,ax / 54 shr cx,4 ... 5f lea edx,[ecx-2]
          O      mov cx,ax / shr cx,4 / movzx edx,cx ... sub edx,2
      B)  T  6e  movsx ax,  byte ptr [eax + esi - 1]
          O      movsx eax, byte ptr [eax + esi - 1]

  Everything before 0x4f and everything from 0x74 to the end is byte-identical.

## Evidence

- The out-of-line `_key_agreement_get_packet_type` (strict EXACT, 80 B) returns in **AL only**
  (`mov al, byte ptr [ecx + edi - 1]`), so its return type is signed `char` — not `byte` (that would be
  `movzx`) and not `short`/`word` (that would need the full AX defined).
- January **inlines** that helper at this call site (no call relocation to it) and folds away its line-0x4D
  assert under the dominating `message_type == _message_type_packet` test. Our build inlines it identically.
- laws_w2 A18 / laws_w3 A34 (value-width facts): a 16-bit `movsx ax` destination proves a 16-bit assignment
  target; an `xor r32,r32` placed before a 16-bit partial definition proves the 32-bit form of that object is
  read afterwards (here `lea edx,[ecx-2]`).

## Shapes tried (5)

| id | one-factor shape | result |
| --- | --- | --- |
| k1 | drop the first `message_size` store; `encoded_packet_size.value = GET_MESSAGE_SIZE(*msgptr) - sizeof(word)` | WORSE (164 insns). The caller computes at 32 bits (`movzx ecx,ax; shr ecx,4`) and **loses the CSE with the inlined helper**, which then re-derives the size itself (`shr ax,4`). Four extra hunks. |
| k2 | `word message_size` and no `(word)` cast | byte-identical to k1. VC7 widens an unsigned-short local whose every use is 32-bit. REFUTES "make the size local unsigned". |
| k3 | `packet_type.encoded = key_agreement_get_packet_type(msgptr)` (assign the union's short member) | **Closes hunk B's opcode and its register choice**: ours becomes `movsx ax, byte ptr [eax+esi-1]`, reusing EAX exactly as January does. Sole residue: January stores `mov dword ptr [ebp+0xc], eax`, ours `mov word ptr [ebp+0xc], ax`. 165 insns; hunk A unchanged. |
| k4 | `packet_type.value = (word)key_agreement_get_packet_type(msgptr)` | Right value semantics, wrong allocation: `movsx cx, byte ptr [...]` plus an extra `movzx eax,cx`; the argument pushes then shift. WORSE. |
| k5 | full-word init idiom: `encoded_packet_size.value = 0; .encoded = GET_MESSAGE_SIZE(*msgptr); .value -= sizeof(word);` and `packet_type.value = 0; .encoded = helper(...)` | MUCH WORSE (160 insns). The two zero initializers pin a zero register in EDI and rewrite the whole assert prologue (`cmp esi,edi` replacing every `test`). REFUTED. |

Candidate files: `scratch/workers/n1_key_agreement_prime_numbers_etc/k1.c` … `k5.c` with their `.obj`.
No candidate lost a baseline-EXACT row (all five gate `exact 9 residual 1`).

## New evidence banked for the park

1. The park class `instruction-scheduling` is **wrong**. Frame and all 17 relocations are identical; there are
   exactly two width hunks and nothing is scheduled differently. Suggested reclass: `value-width`.
2. `packet_type` is assigned through a **16-bit** member — k3 proves both the `movsx ax` opcode and the EAX
   reuse — yet January stores the full **dword** with a zero upper half. The January object is therefore four
   bytes wide, written with a zero-extended 16-bit value, i.e. `(word)(char)`. `.encoded` yields the value but
   a word store (k3); `.value = (word)…` yields a dword store but a different register (k4). No spelling
   tested produces both.
3. The incoming-size expression **must stay 16-bit-typed**: every 32-bit form (k1, k2) loses the CSE with the
   inlined helper's own `word message_size`. January additionally pre-zeroes ECX, which is VC7's def-side
   zero-extension for an object whose 32-bit form is read.
4. The unified "initialize the full word, then write the encoded member" idiom is refuted (k5): it pins a zero
   register and destroys the prologue.

## Reopen criterion (replaces the park's)

Reopen when a source form makes **both** (a) the `packet_type` assignment a 16-bit narrowing stored as a dword
with a zero upper half, and (b) the incoming `GET_MESSAGE_SIZE(*msgptr)` value a 16-bit CSE with the inlined
helper whose 32-bit zero-extension sits at the **definition** (`xor ecx,ecx`) rather than at the use.
k1, k2, k4 and k5 are measured negatives. k3 is the closest body (one differing instruction) and is the
correct starting point for the next attempt.

## Disposition

`_complete_key_exchange` remains parked at 416 / 17 / `fb0142cb223b631a`, unchanged. Real file untouched;
`git diff --stat` over it is empty. Newly exact: none. Worker notes:
`scratch/workers/n1_key_agreement_prime_numbers_etc.md`.
