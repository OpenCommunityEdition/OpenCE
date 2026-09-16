# `input_xbox.obj` structural park pass — opus5 next-150K house-clean lane, wave n3a (2026-09-15)

## Scope, baseline and outcome

- Worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/next-150k-house-clean-20260915`,
  HEAD `6ebd05a96`, lane baseline `ae10935da`.
- Owned source: `source/input/input_xbox.c` only. No header, config, build, tool or other source was touched and
  nothing was committed. Both remaining non-exact rows are parked in `config/parked.json`, so every measurement was
  made in scratch; the real file is byte-unchanged (`git diff --stat -- source/input/input_xbox.c` empty).
- **Baseline correction.** The wave prompt listed `== exact 24 residual 3` with
  `_input_update_analog_button_state` residual. That is stale: the w3d park reopen landed in the lane baseline
  commit `ae10935da`, so at this HEAD the measured real-file gate (with the `_point_from_line3d` guard) is

      == exact 25  residual 2  unwritten 0

  and `config/parked.json` holds only two input_xbox entries. There is no third row and no credit available for it.
- Final real-file gate: identical to the baseline, `== exact 25 residual 2 unwritten 0`.
  `scratch/parkcheck.py`: 2 parks, 0 drift. `tools/fake_match_scan.py`: 0 review leads.
- Nothing landed; nothing proposed. Both rows were re-censused from January's bytes and both park premises were
  found structurally refuted, but what survives the refutation is a C2 register-allocation tie, which laws_w2 R11
  and brief section 5 both forbid spending further on.

| function | January padded / relocs / sha16 | real file | best scratch body | outcome |
| --- | --- | --- | --- | --- |
| `_input_get_device_states` | 1232 / 28 / `56316233dc2ab683` | 1280 / 32 / `b7cded6529564446` | 1216 / 28 (`n0`, = w2 `fn/d2.c`) | NOT LANDED — 1 instruction / 1 encoding byte |
| `_input_update_keyboard_devices` | 640 / 49 / `80bcd2f76c52c728` | 640 / 51 | 640 / 49 `[sha]` (`n0`, = w2 `fn/k3.c`) | NOT LANDED — L3 pin + the same L2 tie |
| `_input_update_analog_button_state` | 48 / 0 / `5850a17e99380c54` | **EXACT** | — | SKIPPED-EXACT (landed at `ae10935da`) |

Ledgers read: all 13 `input_xbox_obj_*.md`, in particular `input_xbox_obj_device_keyboard_pollers_wave_20260904.md`
(owns both park criteria), `input_xbox_obj_opus5_150k_w2_20260914.md` (shapes d1-d6, k1-k4) and
`input_xbox_obj_opus5_250k_w3d_20260915.md` (hd1, e1-e6, hd_d6, kA/kB/kE/kF and laws L1-L3) — w3d ran the identical
structural park-pass method on these same two rows earlier today. Also `scratch/w2/laws_w2.md` (R11 rejects the
`input_xbox d2/k3` zero-credit park remeasures by name), `scratch/w3/laws_w3.md`, and the next-150K worker brief.
`git log --all -- source/input/input_xbox.c`: 23 commits, latest `ae10935da`.
`branch_sweep.py source/input/input_xbox`: 18 unique blobs; the current file is the best (exact 25).

Worker notes with the full tables: `scratch/workers/n3_input_xbox.md`.
Scratch: `scratch/workers/n3_input_xbox/` (`sub.py`, `mk.sh`, `real.obj`/`real.gate.txt`, the combined structural
base `n0.c`/`n0.obj`/`n0.gate.txt`, `full.txt` = the complete aligned instruction dump).

---

## 1. `_input_get_device_states` — park premise refuted; one instruction remains

### Recorded park criterion (quoted)

> Typed January Xbox gamepad/memory-unit hotplug and state poller reconstructed with genuine XDK 3911 API/layout
> declarations, semantic private owners, named device flags and exact natural lookup-table ownership. First natural
> pointer/count implementation retains frame, register, table-hoisting and scheduling differences: 1280 versus 1232
> padded bytes and 32 versus 28 relocations. This is not a proven narrow compiler tie or a byte-exact claim. …
> Reopen for new authentic ABI/type/control-flow evidence, not equivalent-expression trials.

### Census (run before any shape)

**tinfo.** January has 28 relocations. `.bss` `_input_globals` at **+282** (`raw_gamepad_states`), **+316**
(`gamepad_handles`), **+332** (`gamepad_states`); the two lookup tables `_gamepad_analog_button_indices` (@0x259)
and `_gamepad_binary_button_masks` (@0x2c7) are referenced at `symbol+0` — January builds no displaced table base.
There is no relocation to `_update_ticks` or `_input_update_analog_button_state`: January inlines both, as we do.

**Frame census.** January `sub esp,0x30`; the real body `sub esp,0x44` — five extra cells. January's slots:
`[ebp-8]` insertions, `[ebp-0x10]` removals, `[ebp-0x14]`/`[ebp-0x18]` the memory-unit pair, `[ebp-4]`
`device_change_flags`, `[ebp-0xc]` the hot-plug count, `[ebp-0x30]` the `XINPUT_STATE`; then in the poll loop the
four homed induction variables `[ebp-0xc]` = raw_gamepad_states, `[ebp-4]` = gamepad_states, `[ebp-8]` =
gamepad_handles, `[ebp-0x10]` = count, all four rehomed in the loop tail at 0x492-0x4b4. The real body additionally
materialises and homes two strength-reduction table bases; that is exactly the five-cell surplus.

**Relocation census by target** (`relocdiff --count-by-target`, real body, 28 vs 32):

| target | January | ours |
| --- | ---: | ---: |
| `_input_globals+282` | 1 | 0 |
| `_input_globals+284` | 0 | 1 |
| `_input_globals+316` | 2 | 4 |
| `_input_globals+332` | 1 | 2 |
| `_input_globals+340` | 0 | 1 |
| every other target | equal | equal |

### Verdict on the premise: REFUTED

The premise attributes the frame, table-hoisting and relocation gaps to "the first natural pointer/count
implementation". They are a source defect, not a property of natural source: the pointer-walk spelling builds
`&array[0].field` bases (`+284`, `+340`) and re-materialises `+316`/`+332`. The w2 body `fn/d2.c` — indexed `for`
loops over `input_globals.<array>[gamepad_index]`, with the gamepad insert/remove work in its own block —
reproduces January's frame **and** its relocation census exactly. Re-measured this wave as `n0`
(`fn/d2.c` + `fn/k3.c` spliced into the current real file, sha256
`e329841cb9cb7f4320ffe642b0b197ec5743962256f58027996b7c7df2d2c05c`): gate `== exact 25 residual 2 unwritten 0`,
row `residual 1232 _input_get_device_states [size 1216!=1232, sha]` — the `[relocs]` tag is gone and no exact row
is lost.

### What remains: one instruction

`alndiff` of `n0.obj` reports target 400 instructions versus ours 385 plus 15 trailing pad nops — 385 aligned
instructions of which exactly **one** differs, a register-role swap inside the binary-button loop's inlined
`update_ticks`:

    T 2c0 mov bl,[ebp-0x2c]   2c3 lea eax,[esi+0x18]  2c6 mov edx,masks  2cb mov esi,8
    T 2d0 test byte [edx],bl  2d2 je   2d4 movzx ecx,[eax]  2d7 inc ecx  2d8 cmp ecx,0xff   (81 F9, 6 B)
    T 2e9 mov [eax],cl  2eb inc eax   2ec inc edx  2ed dec esi  2ee jne
    O 2c0 mov bl,[ebp-0x2c]   2c3 lea ecx,[esi+0x18]  2c6 mov edx,masks  2cb mov esi,8
    O 2d0 test byte [edx],bl  2d2 je   2d4 movzx eax,[ecx]  2d7 inc eax  2d8 cmp eax,0xff   (3D, 5 B)
    O 2e8 mov [ecx],al  2ea inc ecx   2eb inc edx  2ec dec esi  2ed jne

January puts the pointer induction variable in EAX and the helper's 32-bit temp in ECX; we reverse them. The entire
1217-versus-1216 meaningful gap (and therefore the 16-byte padded gap) is the one encoding byte between
`cmp ecx,imm32` and the EAX short form `cmp eax,imm32`. Everything else matches, including the `mov bl,[ebp-0x2c]`
wButtons hoist before the loop, the ESI counter, the EDX mask pointer, all four homed IVs and the entire
`fix_dead_zone` tail.

### Why there is no source lever (evidence, not assertion)

- In the **analog**-button loop immediately above, our build already matches January byte for byte with the roles
  January wants: IV in EAX (`lea eax,[esi+8]`, used at `[eax-8]`, `[eax]`, `[eax+8]`), helper temp in ECX. Only the
  **binary** loop — whose body is nothing but the inlined helper call — swaps.
- Nine distinct spellings of that loop have now been measured across three waves and all swap identically: the
  pointer/count `while` in the real file today, an indexed `for` (d1/d2), a separate `short` index (e1), the mask
  test with operands swapped (e2), `buttons[button_index + FIRST_GAMEPAD_BINARY_BUTTON]` (e3), a `byte *` base with
  `[button_index]` (e4, 1248), a one-use pointer local bound before the call (e5), a pointer/count `while` rewrite
  (e6, 1248), and the `- FIRST_GAMEPAD_BINARY_BUTTON` index base (d6). `fn/d6.c` reaches 1232 only by acquiring
  three new hunks: it loses the BL wButtons hoist (`mov al,[ebp-0x2c]` reloaded inside the loop) and gains a second
  EAX/ECX flip at +0x330 — confirmed here by `alndiff` of `hd_d6.obj`.
- Both plausible origins for the IV (a genuine source-level pointer variable, and a strength-reduced index
  expression) give the same swap, so the "which temp was created first in the IL" explanation has no source handle.
- **New hypothesis tested and refuted this wave.** Under the A1 static-linkage law, a `static` `update_ticks` could
  inline with different IL. `scratch/pdb200-current/pdb_symbols.json` contains the January public `_update_ticks`
  (and `_fix_dead_zone`), so the helper has external linkage exactly as our source declares it. Dead end; recorded
  so no later wave re-spends it.

Classification: **C2 register-allocation tie** (w3d law L2), not structural. Zero new source shapes were spent this
wave: the w2/w3d budget of 13 is already spent and brief section 5 caps post-tie work at two.

**Not proposed.** Re-baselining the park from 1280/32 to 1216/28 is a zero-credit park remeasure that also changes
size — rejected by laws_w2 **R11** by name for this exact body (`input_xbox d2/k3`) and reaffirmed in laws_w3 B.
No coupled A8 re-baseline is required either: the real file is untouched and `parkcheck.py` reports 0 drift.

**Reopen criterion.** A measured source lever that moves which of EAX/ECX the C2 allocator gives to a pointer
induction variable versus an inlined helper's 32-bit temp, in a loop whose body is only that helper call — not an
equivalent-expression permutation. Then re-gate `scratch/workers/n3_input_xbox/n0.c` (equivalently w3d `hd1.c`),
which is one instruction from strict exact.

---

## 2. `_input_update_keyboard_devices` — relocation premise refuted; pin + the same tie remain

### Recorded park criterion (quoted)

> Typed January debug-keyboard hotplug, key-tick aging and SDK keystroke queue/ring handling using the genuine XDK
> keystroke type, semantic modifier/key constants and exact private ASCII/virtual-key tables. Natural source reaches
> the 640-byte target extent but retains scheduling and relocation differences (51 versus 49); no exact or general
> equivalence claim. … Reopen only for new authentic source or ABI/control-flow evidence.

### Census

**Frame.** No `sub esp` gap; every slot agrees. `[ebp-8]`/`[ebp-4]` are insertions/removals in the hot-plug block;
the `struct key_stroke key` then occupies `[ebp-8]` (modifier_flags), `[ebp-7]` (ascii_code), `[ebp-6]` (key_code),
and the `XINPUT_DEBUG_KEYSTROKE` sits at `[ebp-4]`. No slot is missing or extra on either side.

**Relocations by target** (49 vs 51): `_input_globals+568` (`key_ticks`) **5 vs 6** and `_input_globals+672`
(`key_latches`) **2 vs 3**; every other target equal. January walks **one** induction variable over `key_ticks` and
reaches `key_latches` through the constant displacement `+0x68` (`mov cl,[eax+0x68]`, `movzx ecx,[eax]`).

### Verdict on the premise: REFUTED for the relocation half

The 51-versus-49 gap is not scheduling; it is a `long key_code` loop that keeps a second array base alive. w2's
`fn/k3.c` (a `short key_code` `for` loop indexing both arrays, with the ascii store as a ternary) reproduces
January's single induction variable and measures **640 / 49** — January's exact size and exact relocation count.
Re-measured in `n0` this wave: `residual 640 _input_update_keyboard_devices [sha]`. The park's "extent already
reached" half stands; only `[sha]` is left.

### What remains

`alndiff` against `n0.obj`: target 193 instructions, ours 197, in two hunks.

1. **Byte-constant unification (w3d law L3).** Ours emits `push ebx` in the prologue plus `mov ebx,0xff`, then
   `cmp eax,ebx` / `mov eax,ebx` in the tick loop and `mov [ebp-7],bl` for the ascii `NONE` arm. January
   shrink-wraps `push ebx` to +0x27 and `pop ebx` to +0xd7 (EBX is needed only in the hot-plug block) and
   rematerialises the constant three times: `cmp ecx,0xff`, `mov ecx,0xff`, `mov byte [ebp-7],0xff`. w3d's `kA`/`kF`
   proved the cause is the *third* use of the byte value 0xFF — two inside the inlined `update_ticks` plus
   `key.ascii_code = NONE`; remove it and VC7 rematerialises exactly as January does. The third use is a genuine
   `NONE`, so no admissible spelling avoids it. Checked this wave: `struct key_stroke::ascii_code` is `byte`
   (`source/input/input.h:188`), so `(byte)NONE` and a bare `NONE` are the same IL constant 255 — consistent with
   w3d's cast-free `kB`, where the pin stayed. A `char` field would give a distinct IL constant, but
   `mov byte [ebp-7],0xff` is identical for both widths, so there is no January byte evidence for a header packet.
2. **The same L2 EAX/ECX swap** in the inlined `update_ticks`. `kA` shows it survives even when the pin is removed,
   so this row cannot close on the pin alone.

Zero new source shapes were spent (w2 k1-k4 plus w3d kA/kB/kE/kF = 8; `kE` proved the `short key_code`).

**Not proposed** (laws_w2 R11: the `k3` park remeasure was already rejected as zero credit).

**Reopen criterion.** (a) a source lever that stops VC7 unifying a byte constant across a loop and a later byte
store — or January-side evidence that `key_stroke::ascii_code` is `char` — **and** (b) the L2 allocator lever the
`_input_get_device_states` row needs; then re-gate `scratch/workers/n3_input_xbox/n0.c`.

---

## 3. Conclusion for the orchestrator

1. `input_xbox.obj` is **25 exact / 2 residual / 0 unwritten** at this HEAD, not 24/3. Please correct the board row
   for `_input_update_analog_button_state`; it is exact and unparked since `ae10935da`.
2. Both remaining park premises are structurally refuted, and both structural fixes already exist in scratch and
   measure at January's frame and relocation counts (`n0.c`). Neither is landable or proposable today: R11 forbids
   the zero-credit remeasure, and the surviving difference in both rows is the same C2 allocator role swap.
3. `_input_get_device_states` is the single cheapest large row known in this object — **one encoding byte**
   (1232 padded) behind a lever that nine measured spellings have failed to move. If any future wave measures a
   source lever for "pointer IV versus inlined-helper temp" register roles, this row and half of
   `_input_update_keyboard_devices` close immediately from `n0.c`.
