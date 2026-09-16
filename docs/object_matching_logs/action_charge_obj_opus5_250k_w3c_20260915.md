# `action_charge.obj`: setup store-position closure proof and perform linkage re-measurement (Opus 5 250K house-clean lane, wave w3c, 2026-09-15)

## Result

- **Nothing landed.** `git diff -- source/ai/action_charge.c` is empty; the object is unchanged at
  `== exact 20  residual 2  unwritten 0`, identical row for row to the baseline listing.
- `_action_charge_setup` (944): re-measured at **291 instructions vs January's 291 with exactly ONE aligned
  difference** — the position of `mov byte ptr [ebp-1],0` (January `+0xe4`, ours `+0x138`). This wave adds a
  **construction proof that no non-redundant source spelling can place that store at `+0xe4`**, so the row is
  definitively owner-blocked on the laws_w3 A42 / R12 redundant-store ruling, not on a missing shape.
- `_action_charge_perform` (3248): the `[size 3216!=3248]` gap is **fully attributed** to a single cross-jump —
  January emits the `direction = prop->actor_to_prop;` copy twice, ours once. The linkage lever was reproduced and
  a strictly better-motivated route for it was found (`p2.c`: delete the per-TU rename package and the 11 local
  helper bodies, take the helpers from `math/real_math.h`), producing **byte-identical** output to the previously
  declined `__inline` form. Neither reaches exact (3264), so neither is landable; the residual is 59 real
  allocation blocks.

## 1. Baseline and provenance

- Worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/250k-house-clean-20260915`.
- Gate WITHOUT the `_point_from_line3d` guard (brief 3b exception: this object is the genuine owner). The guard run
  fails only on that pre-existing owner: one code definition, 48 bytes, strict EXACT, sha16 `9b763841f8519177`.
  No additional candidate-only math owner exists (census below).
- Baseline rows: `residual 3248 _action_charge_perform [size 3216!=3248, sha]`, `residual 944 _action_charge_setup [sha]`,
  20 EXACT. No parks for the unit.
- Read in full: all nine action_charge ledgers (2026-08-21 two-leaf, the three 2026-08-26 math-helper ledgers,
  2026-08-28 public residuals, 2026-08-29 helper reintegration, 2026-09-06 authentic math header owners,
  2026-09-14 fresh-graphs, 2026-09-14 150K w2), `scratch/workers/action_charge.md`, the 100K handoff,
  `scratch/w2/laws_w2.md` and `scratch/w3/laws_w3.md` in full.
- `git log --all --oneline -- source/ai/action_charge.c`: 16 refs. `branch_sweep`: 13 unique blobs, HEAD is the best
  (`exact 20 residual 2 unwritten 0 sizedelta 32`); nothing to reuse.
- Scratch: `scratch/workers/w3c_action_charge/` — `base.c/.obj`, `p1.c/.obj`, `p2.c/.obj`, `setup_aln.txt`,
  `perform_aln.txt`, `perform_aln_full.txt`, `p1_aln.txt`. Worker notes: `scratch/workers/w3c_action_charge.md`.

## 2. `_action_charge_setup` — the shape space is closed

`alndiff` against the HEAD build (`setup_aln.txt`):

```
target 291 instructions, ours 291 instructions
--- delete target[67:68] ours[67:67] @T 0xe4    T  e4  mov byte ptr [ebp - 1], 0
--- insert target[88:88] ours[87:88] @O 0x138   O 138  mov byte ptr [ebp - 1], 0
```

(the three other reported blocks are `__FILE__`/`.rdata` label artifacts of `--source`, not code differences).

January's `[ebp-1]` writes: `+0x45 = 1` (the sunk `boolean result = TRUE;` initializer — no entry store, so A42's
"VC7 does not hoist a FALSE into the entry" detector says `+0xe4` is a *separate* source statement), `+0xbc = 0`
(swarm arm), `+0xe4 = 0`, `+0xea = 0` (inhibited arm), `+0x107 = 0` (notarget arm), `+0x308 = 1` (melee success),
`+0x383 = al` (vehicle arm), read once at `+0x39a`.

Context of the divergence:

```
d9  mov cl, byte ptr [eax + 0xb6]
df  add esp, 8
e2  test cl, cl
e4  mov byte ptr [ebp - 1], 0        <-- between the compare and the jcc
e8  jns 0xfc
ea  mov byte ptr [ebp - 1], 0
ee  mov word ptr [edi + 0x190], 3
```

**A38** (initializer / zero-store placement) fires exactly: a store emitted between an `if`'s compare and its `jcc`
belongs to the block *enclosing* that `if`, not to its body — i.e. the source statement sits before
`if (TEST_FLAG(unit->object.damage_flags, _object_melee_attack_inhibited_bit))`.

**Closure proof (new).** Both bodies emit four zero stores to `result`. In any source where every path assigns
`result` exactly once, the fourth store is the default for the deep melee-attempt path, and its only non-redundant
position is inside that arm — which is the landed H1 shape and emits at `+0x138`. Conversely a store emitted at
`+0xe4` necessarily precedes the inhibited test, where it dominates the inhibited and notarget arms; those arms'
own stores cannot be dropped (w2 measured A1 "pre-store only" at 1024 and A6 "arm stores only" at 1056 — removing
them re-creates nine tail-duplicated epilogues). Hence **strict EXACT requires a store that is redundant with
respect to two arms**, and the eleven alternatives already measured in w2 (A1, A4, A6, R1, R1b, R2, R3, H1, H3,
S2a, S5) are exhaustive of the non-redundant space. No shape was spent this wave; the proof replaces the probe.

The strict-EXACT body already exists: `scratch/workers/action_charge/w2/v3.c` (21/22 object) = the landed file with
`result = FALSE;` moved one source line, from the top of the melee-attempt arm to immediately before the inhibited
`if`. Class: laws_w3 C3 "Byte-proven redundant stores/branches (A42)", alongside `_encounter_test_rule` (tr8) and
`_extract_sequence`.

**Owner ruling requested (re-stated, with the new closure proof): +944 padded strict-exact bytes for one moved
source line, zero sibling and zero park risk.**

## 3. `_action_charge_perform` — the `[size]` gap attributed, and the linkage route improved

Production: 3216 / 84, **983 instructions vs January's 982**, relocation multiset identical.

### 3.1 The 32-byte gap is one cross-jump

January emits the `direction = prop->actor_to_prop;` struct copy twice:

```
6c8  mov eax,[ebp-8]                  <- then-arm of `if (dot_product3d(&direction,&prop->actor_to_prop) < 0.f)`
6cb  fld dword ptr [__real@00000000]     (distance = 0.f, interleaved)
6d1  mov ecx,[eax+0xe0] / 6d7 mov edx,[eax+0xe4] / 6dd mov eax,[eax+0xe8]
6e3  mov [ebp-0x2c],ecx / 6e6 mov [ebp-0x28],edx / 6e9 mov [ebp-0x24],eax
...
707  mov eax,[ebp-8] ... (second copy, different register schedule)  <- inside `if (distance == 0.f)`
```

Ours emits it once at `+0x6e7`: the inlined `dot_product3d(&direction, &prop->actor_to_prop)` strength-reduces the
argument address to `add edx,0xe0` at `+0x6a1`; both copies then become the identical `mov ecx,[edx] …` sequence
and VC7 cross-jumps them. The eight-instruction target block against our one-instruction stub *is* the
`3216 != 3248` delta. The same biased pointer also reorders the inlined dot: January k,j,i (A35(2)
youngest-term-first), ours j,k,i.

**Detector caveat for A35 (new):** a k,j,i vs j,k,i term order can be an addressing-mode artifact of a
strength-reduced `&struct->field` pointer rather than a source expression-order or leaf-age fact. Look for an
`add reg,imm` bias in the candidate before spending an A35 shape on term order.

### 3.2 The linkage lever, reproduced and improved

| Shape | Size / relocs | perform sha16 | Siblings | Owner census | Landed |
| --- | --- | --- | --- | --- | --- |
| HEAD (rename package + 11 local helper definitions) | 3216 / 84 | `2e9beea61ca0c704` | 20 EXACT | unchanged | (in tree) |
| `p1.c` = HEAD + `__inline` on the 11 local definitions (w2 `v4_inline`) | 3264 / 84 | `798a99326c8aff1a` | 20 EXACT | unchanged | no |
| `p2.c` = HEAD with the rename package **and** the 11 local definitions **deleted** (helpers come from `math/real_math.h`) | 3264 / 84 | `798a99326c8aff1a` | 20 EXACT | `+_magnitude_squared3d` (code) | no |

`p1` and `p2` produce **byte-identical** perform sections, so inline *linkage* is the only operative variable; the
local `__inline` copies are not needed to obtain it. `p2` is the better-motivated form: January's map atlas marks
`normalize2d`, `point_from_line3d`, `magnitude3d`, `normalize3d` and `collision_test_line` as inline (`i`) COMDAT
copies, and `p2` removes an A30/R8-class per-TU `#define helper helper_inline` rename package plus 114 lines of
duplicated real_math bodies from a consumer `.c`. Its only cost is one surplus candidate-only COMDAT
`_magnitude_squared3d`, the same real_math.h select-any class as this object's already-disclosed
`_vector_from_points3d`.

Neither reaches 3248, so under brief section 6 (already-written residual: land only if strict EXACT) neither is
landable, and the fresh-graphs lane's decline of the `__inline` route stands. Recorded so a later wave does not
re-litigate the linkage question.

### 3.3 Residual after the lever (59 real blocks, `p1_aln.txt`)

109 aligned blocks, 59 after removing relocation-annotation and branch-offset noise (HEAD base: 67). All are
allocation:

- extra `mov ecx,[ebp-8]` prop reload at `+0x256` (+3 bytes; the only instruction-count difference);
- melee-abort byte pair swapped — January `check_range` at `[ebp-2]`, `berserk_ranges` (and the later `melee`) at
  `[ebp-1]`; January also keeps `check_range` in BL across `game_time_get` where ours keeps `prop` in EBX and
  reloads the flag (`mov al,[ebp-1]; test al,al` at `+0x387`, +5 bytes);
- `_charge_goal_melee` rematerialised as an immediate by January (`cmp word [edi+4],2`, `mov word [edi+4],2`) vs
  pinned in ECX by us (`mov ecx,2` … `cmp word [edi+4],cx`) — the constant-register pinning family, inverted;
- `[ebp-0x14]` vs `[ebp-0x18]` for the tail `unreachable` byte;
- two commutative `fadd` operand orders at `+0x944` / `+0x956` — R3 forbids swapping the source operands for these;
- cdecl-pop (`add esp,N`) placement at `+0x6a2`, `+0x8e7`, `+0x919` (A31(a));
- roughly 35 pure eax/ecx/edx register-name substitutions.

Net 3252 meaningful vs January's 3246.

## 4. Shapes spent and rejected

- setup: 0 this wave (closure proof; the 11 w2 alternatives are exhaustive of the non-redundant space).
- perform: 2 of 5 (`p1`, `p2`), neither exact, neither landed.
- Designed and rejected on paper: setup S1 (split the inhibited else-if into `if`/`else` with one default in the
  else) — yields three zero stores, not January's four, so it cannot be exact.
- Not re-spent (prior do-not-repeat): declaration order / block scope / names / dead initialisers of `leap` and the
  tick outputs; early-return arms; `check_range` / `berserk_ranges` / `abort_range` / `time` / `target_range` /
  `unreachable` declaration order and scope; dot_product3d argument order; `danger + attack` order; moving perform
  before the helper package.

## 5. Checks

- Final real-file gate (no guard): `== exact 20  residual 2  unwritten 0`, listing identical to baseline row for row.
- Guard gate: fails only on the pre-existing genuine `_point_from_line3d` owner (one definition, 48 bytes, EXACT).
- Parks: `scratch/parkcheck.py` → "no parks for source/ai/action_charge".
- Owner census vs `build/base`: HEAD `candidate-only vs base: []`; `p1` `[]`; `p2` `[('_magnitude_squared3d','code')]`.
  Pre-existing non-target owners (`_vector_from_points3d`, `__real@3f000000`, `__real@41f00000`, the two assert
  literals, the D3D tables) unchanged in all three.
- `tools/fake_match_scan.py source/ai/action_charge.c`: 0 review leads.
- `git diff --stat -- source/ai/action_charge.c`: empty.

## 6. Proposals

1. **Owner ruling (setup, A42/R12 class):** admit `scratch/workers/action_charge/w2/v3.c`. One moved source line
   (`result = FALSE;` before the inhibited `if`, arms unchanged) for +944 padded strict-exact bytes. This wave adds
   the proof that no non-redundant spelling can reach exact, so the ruling is the only remaining path for this row.
2. **Census/policy note (perform):** if a later wave closes the allocator items, the linkage prerequisite should be
   taken via `p2.c` (delete the rename package and the local helper bodies) rather than the declined local
   `__inline` form; the orchestrator would then need to accept `_magnitude_squared3d` as a real_math.h surplus
   select-any disclosure, the same class as this object's existing `_vector_from_points3d`.

## 7. Reopen criteria

- **setup:** the owner ruling in section 6.1. Do not re-spend on store placement, declaration order, block scope or
  early-return shapes.
- **perform:** a source-level cause, measured on the `p2` base, for (a) the `[ebp-1]`/`[ebp-2]` byte-pair rank in the
  melee-abort block that also puts `check_range` in BL, and (b) the `+0x256` prop reload. Do not re-spend on
  declaration order/scope of the abort-block locals, dot_product3d argument order, or commutative `fadd` swaps (R3).
