# `unit_dialogue.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

Lane `C:\halo-worktrees\opus5-150k-house-clean-20260914`, baseline `ae10935da`.
Production source was NOT modified. `git diff --stat -- source/units/unit_dialogue.c` is empty.

## State

| measurement | baseline | final |
| --- | --- | --- |
| gate | exact 12 residual 2 unwritten 0 | exact 12 residual 2 unwritten 0 |
| `_unit_describe_speech` | 272 / 15 / `2b01a81debf36cda` (target 272 / 265 meaningful / 15 / `7d9ee7e3b923460a`) | unchanged |
| `_unit_make_damage_sound` | 784 / 18 / `a72cd2b9285abc4f` (target 784 / 18 / `87952366bca787dc`) | unchanged |

Parks: both OK, drift 0.

## `_unit_describe_speech` — census

- **Frame:** neither build allocates a frame beyond the saved registers (no `sub esp`); every local is
  enregistered, so there is no slot census to write.
- **Relocations:** 15/15, identical targets and order. The five literal relocations differ only as
  csplit external-versus-`defined-noncode` spellings and compare equal symbolically.
- **Code:** January 115 real instructions + 2 nops; ours 116 real instructions, no padding. The ENTIRE
  divergence is one loop-rotation hunk in the `abbreviated` arm:

      January  0x5e mov eax,esi / 0x60 test eax,eax / 0x62 je 0x8a ...
               0x73 inc eax / 0x74 mov esi,eax / 0x76 jmp 0x60        (unconditional back edge)
      ours     0x5e test esi,esi / 0x60 mov eax,esi / 0x62 je 0x8c ...
               0x73 inc eax / 0x74 mov esi,eax / 0x76 jne 0x64 / 0x78 jmp 0x8c

  Our VC7 rotates the loop: it peels the entry test above the `scan = sound_name` copy and re-uses
  `inc eax`'s ZF as the bottom test. January's does not. Everything after 0x78 is byte-identical,
  shifted by two. Both builds agree on the two `break` exits jumping to the same join, so the
  two-exit `while`/`break` topology is confirmed rather than in question.

### Shapes (2 — the sha-only budget), both scratch `--source`, siblings 12/12 EXACT in each

| id | shape | result |
| --- | --- | --- |
| U1 | top-tested `while (scan) { scan = strchr(scan, backslash); if (!scan) break; scan++; sound_name = scan; }` | 272/15, **byte-identical to HEAD** |
| U2 | `for (scan = sound_name; scan; sound_name = scan) { ... }` (A25: an increment-clause statement is emitted after the body, forming a latch block) | 272/15, **byte-identical to HEAD** |

All three admissible loop spellings — `while (TRUE)` with a leading break, a top-tested `while`, and a
`for` with an increment clause — produce the same bytes. The rotation is source-invariant here (A43
family: a late machine pass, no source lever).

Reopen criterion: only a control that stops VC7 folding the loop-entry test into the back edge, i.e. a
spelling whose last flag-setting body operation does not determine the loop condition. Do NOT retry the
three spellings above; all are measured byte-identical at this HEAD.

## `_unit_make_damage_sound` — SKIPPED-EXHAUSTED

`laws_w2` section D lists this function with five shapes spent and a clear tie verdict; the park
evidence (2026-09-12 packet) records the complete January-authoritative, HCEA-cross-checked
reconstruction, and the extents and all 18 relocation identities already match. No w2/w3 law fires and
brief section 5 bars a sha-only spend without a donor, a forced-register fact or a new law. No shape
spent.

## Duplicate prevention

`branch_sweep` (8 blobs): best historical row `exact 9 residual 5 unwritten 0`; current 12/2/0. No donor.

## Disposition

`NonMatching`, 12/14, both parks retained, no credit claimed.
