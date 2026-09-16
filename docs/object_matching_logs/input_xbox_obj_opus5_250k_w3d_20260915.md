# `input_xbox.obj` structural pass — opus5 250K house-clean lane, wave w3d (2026-09-15)

## Scope and baseline

- Worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/250k-house-clean-20260915`.
- Owned source: `source/input/input_xbox.c` only. No header, config, build, tool or other source was touched and
  nothing was committed. All three non-exact rows are parked in `config/parked.json`, so every body was built and
  measured in scratch only; the real file is byte-unchanged (`git diff --stat -- source/input/input_xbox.c` empty)
  and its final gate is identical to the baseline.
- Baseline and final real-file gate (with the `_point_from_line3d` guard): **`== exact 24  residual 3  unwritten 0`**.
- Park drift on the real file: none (`scratch/parkcheck.py`, 3 parks, 0 drift).

| function | January padded / relocs / sha16 | real file | wave outcome |
| --- | --- | --- | --- |
| `_input_get_device_states` | 1232 / 28 / `56316233dc2ab683` | 1280 / 32 / `b7cded6529564446` | NOT LANDED — 1 allocator hunk (best scratch 1216 / 28) |
| `_input_update_analog_button_state` | 48 / 0 / `5850a17e99380c54` | 48 / 0 / `78f85bb297cc663f` | **PARK-REOPEN PROPOSED — strict EXACT** |
| `_input_update_keyboard_devices` | 640 / 49 / `80bcd2f76c52c728` | 640 / 51 | NOT LANDED — pin + the same allocator hunk |

Ledgers read: all twelve `input_xbox_obj_*.md` (in particular the w2 park re-investigation
`input_xbox_obj_opus5_150k_w2_20260914.md` and `input_xbox_obj_device_keyboard_pollers_wave_20260904.md`, which
owns the three park criteria), `scratch/w2/laws_w2.md` (R11 rejects the w2 `d2`/`k3` zero-credit park remeasures;
section D lists `_input_update_analog_button_state` as a spent-tie row), `scratch/w3/laws_w3.md`, the 100K handoff.
`git log --all -- source/input/input_xbox.c` (22 commits; latest `e123a1d82`). No branch holds a better blob.

Worker notes with the full probe tables: `scratch/workers/w3d_input_xbox.md`.
Scratch: `scratch/workers/w3d_input_xbox/` (`lab.py` micro-TU compiler using input_xbox.c's own cflags, `run.sh`
whole-TU gate, `sub.py` body splicer, `lab1.c`-`lab5.c` probes, `fn/*.c` bodies, `*.gate.txt`/`*.obj`).

---

## 1. `_input_update_analog_button_state` — EXACT (48 B), park reopen proposed

### The one-instruction gap

January's standalone copy and ours differ in exactly one instruction out of 30:

```
 0 test dl,dl      2 je 0x15       4 cmp al,0x20    6 jae 0xc
 8 xor al,al       a jmp 0xe       c sub al,0x20      <-- January;  ours: add al,0xe0
 e cmp al,[ecx]   10 jbe 0x25     12 mov [ecx],al   14 ret
15 cmp al,0xbf    17 jbe 0x1d     19 or al,0xff     1b jmp 0x1f
1d add al,0x40    1f cmp al,[ecx] 21 jae 0x25       23 mov [ecx],al  25 ret
```

The layout proves the source arm order already in the file (`if (value < OFFSET) … else …` in both paths), and the
release path's `add al,0x40` / `or al,0xff` prove `+= 64` and `= UNSIGNED_CHAR_MAX`. The park's recorded blocker was
"only `sub al,32` versus compiler-canonical `add al,-32`", with the criterion *"Reopen only for new authentic
compiler/source evidence"*.

### New evidence: a measured VC7 byte-width law (new, not a re-run of the w2 search)

Every earlier wave respelled the **subtrahend**. Twelve such spellings were re-measured in a micro-TU built with
`input_xbox.c`'s own cflags (`scratch/workers/w3d_input_xbox/lab2.c`, probes `pa`-`pl`): `value -= 32`,
`value = value - 32`, a `const byte` local, a file-scope `static const byte`, an `enum` constant, `32u`,
`(byte)(value-32)`, `+= -32`, `(int)32`, `0x20`, `sizeof(int)*8`, `(byte)(value-32u)` — **all twelve emit
`add al,0xe0`**. The subtrahend is inert; that direction is closed.

The destination is not. `lab3.c` / `lab4.c` controls:

| probe | source | emitted |
| --- | --- | --- |
| `q_ret8` | `byte f(byte v){ v -= 32; return v; }` | **`sub al,0x20`** |
| `q_branch8` | the same `v -= 32` inside an `if/else` | `add al,0xe0` |
| `q_branch32` | the same shape at 32-bit width | **`sub eax,0x20`** |
| `q_twice` | `v -= 32; v -= 32;` | `sub al,0x40` |
| `q_bigconst` | `v -= 200` | `add al,0x38` |
| `q_mem8/16/32` | `*p -= 32` | `add …,0xe0 / 0xffe0 / -0x20` |
| `b1`-`b5`, `b8`-`b10` | eight further branch shapes that write back into `value` | `add al,0xe0` |
| **`b7`** | `byte result; if (c) result = value - 32; else result = 0; return result;` | **`sub al,0x20`** |

**Law (L1, byte self-update canonicalisation).** At byte width VC7 13.00.9254 rewrites a *self-update*
`x = x - C` into `add x, (256-C)`; it keeps `sub x, C` when the difference is assigned to a **different** variable
(after copy propagation, the function result counts as one). 32-bit arithmetic keeps `sub` in both cases, and a byte
constant above 127 always becomes `add` with the complement. Detector: *January `sub r8,imm` where ours has
`add r8,imm` with the high bit set means the source assigned to a second variable.*

In-tree corroboration: the **current real** `_input_get_device_states` body, which binds the reading to its own
local `byte value` before calling the helper, already emits `sub al,0x20` at its inlined copy of this helper
(`scratch/workers/w3d_input_xbox/realeq.txt`, +0x2B9).

`lab5.c` then showed that the four natural placements of the result variable (function scope, per-arm scope,
declare+initialise via a ternary, press arm only) are **byte-identical** and reproduce every other instruction of
January's body — the bytes do not discriminate them, so the house rules pick.

### Landed body (`fn/h1.c`)

```c
static void input_update_analog_button_state(
	byte *threshold,
	byte value,
	boolean pressed)
{
	byte new_threshold;

	if (pressed)
	{
		if (value < ANALOG_BUTTON_PRESS_THRESHOLD_OFFSET)
		{
			new_threshold = 0;
		}
		else
		{
			new_threshold = value - ANALOG_BUTTON_PRESS_THRESHOLD_OFFSET;
		}

		if (new_threshold > *threshold)
		{
			*threshold = new_threshold;
		}
	}
	else
	{
		…the release path with the same named result…
	}

	return;
}
```

The helper maintains a hysteresis envelope for one analog button: it derives the threshold implied by this frame's
reading and moves `*threshold` towards it (up while held, down while released). Naming that derived quantity and
leaving the incoming `value` parameter unmodified is ordinary defensive C, and it is what January's bytes require.
No cast, no `volatile`, no redundant store, no unused local (three uses per arm), no change to the arm order,
constants or control flow.

### Checks — candidate `scratch/workers/w3d_input_xbox/candidate_reopen.c`

sha256 `c13f310311128753a30fb961b9d874ba98a4614fdd3fa8e081f871801b81beb2` (the current real file plus only this body).

1. Whole-TU gate: `== exact 25  residual 2  unwritten 0`; the row-by-row diff against the baseline listing shows
   **exactly one** changed row, `residual 48 _input_update_analog_button_state -> EXACT 48`. No other row moved.
2. `_point_from_line3d` emitted-symbol guard passed.
3. `scratch/parkcheck.py`: `_input_get_device_states` OK (1280 / 32 / `b7cded65…` — unchanged; the helper edit does
   **not** drift it), `_input_update_keyboard_devices` OK (640 / 51), and
   `_input_update_analog_button_state` drifts to the January target values, as a reopen must.
4. Owner census vs `build/base` and `build/split`: 52 candidate owners vs 52 base and 50 target; the only delta is
   the compiler-generated jump-table label `$L8245` -> `$L8246` (present in neither January object).
   `target owners missing in candidate: []`. No new or lost real code/data/BSS/COMMON owner.
5. `tools/fake_match_scan.py`: 0 review leads, same as HEAD.
6. CRLF line endings preserved (1229 CRLF, 0 lone LF).

**ORCHESTRATOR PROPOSAL (park-reopen).** Remove the `config/parked.json` entry for
`source/input/input_xbox::_input_update_analog_button_state` and apply the `fn/h1.c` body in the same commit.
Park criterion satisfied: "new authentic compiler/source evidence" — a measured byte-width codegen law with ~20
controls, not an equivalent-expression trial.

---

## 2. `_input_get_device_states` — structural gap closed, one allocator hunk left (NOT landed)

### Census (done before any shape)

- **Frame.** January `sub esp,0x30`; the real body `sub esp,0x44` — five extra cells. January's slots:
  `[ebp-8]`/`[ebp-0x10]` gamepad insertions/removals, `[ebp-0x14]`/`[ebp-0x18]` the memory-unit pair, `[ebp-4]`
  `device_change_flags`, `[ebp-0xc]` the hot-plug count, `[ebp-0x30]` the `XINPUT_STATE`; then in the poll loop three
  homed pointers `[ebp-0xc]` = `raw_gamepad_states` (+282), `[ebp-4]` = `gamepad_states` (+332), `[ebp-8]` =
  `gamepad_handles` (+316) and `[ebp-0x10]` = count. The real body additionally builds and homes two table bases
  (`mov eax,_gamepad_analog_button_indices; sub eax,_input_globals+332; …; sub eax,0x18`).
- **Relocations.** 32 vs 28. The four extras are those two hand-built table bases plus `_input_globals+284`/`+340`
  bases where January uses `+282`/`+332`.
- **Diagnosis.** A missing/extra-local + wrong-cursor problem, i.e. structural, exactly as the tags said. w2's
  `fn/d2.c` — index `for` loops over `input_globals.<array>[gamepad_index]` with the insert/remove work in its own
  block — already fixes both: **1216 / 28** with `sub esp,0x30` exact.

### This wave

With the new helper body the second of d2's two recorded blockers disappears (`sub cl,0x20` at +0x29F now matches),
leaving a single hunk in the binary-button loop's inlined `update_ticks`:

```
T 2c3 lea eax,[esi+0x18] … 2d4 movzx ecx,[eax] 2d7 inc ecx 2d8 cmp ecx,0xff … 2e9 mov [eax],cl 2eb inc eax
O 2c3 lea ecx,[esi+0x18] … 2d4 movzx eax,[ecx] 2d7 inc eax 2d8 cmp eax,0xff … 2e8 mov [ecx],al 2ea inc ecx
```

The entire 16-byte size gap is the one-byte difference between `cmp ecx,imm32` (6 B) and `cmp eax,imm32` (5 B).

Shapes measured this wave (all with the helper fix in place):

| shape | spelling | result |
| --- | --- | --- |
| `hd1` | w2 `fn/d2.c` + `fn/h1.c` | 1216 / 28, one hunk |
| `e1` | binary loop in its own block with a separate `short binary_button_index` | identical |
| `e2` | mask test operands swapped | identical |
| `e3` | `buttons[button_index + FIRST_GAMEPAD_BINARY_BUTTON]` | identical |
| `e4` | `byte *binary_button_ticks` base indexed by `button_index` | 1248, worse |
| `e5` | tick pointer bound to a local before the call | identical |
| `e6` | binary loop as a pointer/count `while` walk | 1248, worse |
| `hd_d6` | w2 `fn/d6.c` + helper | 1232 (right size) but three extra hunks: `wButtons` reloaded inside the loop instead of hoisted into BL, plus a second EAX/ECX flip at +0x330 |

Classification: **C2 register-allocation tie** (law L2 below), not structural. `hd_d6` also confirms that January
hoists the `wButtons` low byte into BL before the loop, which `d2` reproduces and `d6` does not.

**Not proposed.** Re-baselining this park from 1280/32 to 1216/28 would be a zero-credit park remeasure, and
laws_w2 **R11** already rejected exactly this (`input_xbox d2/k3`). The helper reopen does **not** drift this park
(`parkcheck` reports it OK), so no coupled A8 re-baseline is required either.

**Reopen criterion.** A measured source lever that moves which of EAX/ECX the C2 allocator gives to a pointer IV
versus the inlined `update_ticks` 32-bit temp, in a loop whose body is only that call; then re-gate
`scratch/workers/w3d_input_xbox/hd1.c` (1216 / 28 / `0138629fbb74299e`).

---

## 3. `_input_update_keyboard_devices` — structural gap already closed by w2, two ties left (NOT landed)

### Census

- **Frame.** No `sub esp` gap; slots agree (`[ebp-8]`/`[ebp-4]` insertions/removals, the `key_stroke` at
  `[ebp-8..-5]`, the `XINPUT_DEBUG_KEYSTROKE` at `[ebp-4]`).
- **Relocations** (`relocdiff --count-by-target`, real body): 51 vs 49 — `_input_globals+568` **6 vs 5** and
  `_input_globals+672` **3 vs 2**; every other target is equal. January walks **one** induction variable over
  `key_ticks` (+568) and reaches `key_latches` (+672) through the constant displacement `+0x68`
  (`mov cl,[eax+0x68]`, `movzx ecx,[eax]`). w2's `fn/k3.c` (a `short key_code` `for` loop indexing both arrays)
  already reproduces that: **640 / 49**. The structural row is therefore closed by an existing body; what remains is
  `[sha]`.

### This wave

| shape | spelling | result |
| --- | --- | --- |
| `kA` | **diagnostic only** — ascii else-value `0` instead of `NONE` | the `mov ebx,0xff` pin disappears, `push ebx` moves to January's +0x27 and `pop ebx` to +0xD7, 197 -> 195 instructions, only the EAX/ECX hunk left |
| `kB` | ascii store as `key.ascii_code = keystroke.Ascii; if (table[...] == NONE) key.ascii_code = NONE;` | identical to k3 |
| `kE` | `long key_code` instead of `short` | 656 / 51, much worse — proves `short key_code` |
| `kF` | named `boolean down = input_globals.key_latches[key_code];` | identical to kA |

`kA` identifies the mechanism of the recorded "constant-pinning" blocker: the two in-loop uses of
`UNSIGNED_CHAR_MAX` inside the inlined `update_ticks` and the later **byte** store `key.ascii_code = NONE` are
unified by VC7 into one enregistered 0xFF (`cmp eax,ebx`, `mov eax,ebx`, `mov [ebp-7],bl`), which additionally
forces `push ebx` into the prologue instead of shrink-wrapping it to the hot-plug block. Remove the third use and
VC7 rematerialises the constant exactly as January does. January performs all three 0xFF uses and does not unify
them; the else-value is genuinely `NONE`, so there is no admissible spelling that avoids it.

`kA` also shows the **same** EAX/ECX role swap as `_input_get_device_states`, so this row cannot close on the pin
alone.

**Not proposed** (R11: the w2 `k3` park remeasure was already rejected as zero-credit).

**Reopen criterion.** (a) a source lever that stops VC7 unifying a byte constant across a loop and a later byte
store, **and** (b) the allocator lever the device_states row needs; then re-gate `hk3.c` (640 / 49).

---

## 4. Laws proposed for the wave file

**L1 — BYTE SELF-UPDATE CANONICALISATION (closing; closed 48 B first probe).** At byte width VC7 rewrites
`x = x - C` into `add x,(256-C)`; `sub x,C` appears only when the difference is assigned to a different variable
(or is the function result). 32-bit arithmetic keeps `sub` either way; a byte constant `> 127` always becomes
`add`. Twelve subtrahend spellings and ten branch shapes were controls. Detector: January `sub r8,imm` against our
`add r8,imm` with the high bit set ⇒ the source names a second variable.

**L2 — SOLE-STATEMENT LOOP HELPER ALLOCATION TIE (diagnostic).** In a loop whose body is only an inlined helper
taking (pointer, condition), January gives EAX to the pointer IV and ECX to the helper's 32-bit temp; our build
reverses them in all nine spellings measured across two functions (index loop, separate index variable, operand
swap, index-base swap, one-use pointer local, pointer/count `while`, `long` index, named boolean local, and the
pointer-walk body in the real file today). Where the loop body has other work that also reads through the IV
(the analog-button loop) the assignment matches. Costs one byte per `cmp r32,imm32`. No source lever found.

**L3 — BYTE-CONSTANT UNIFICATION ACROSS A LOOP (diagnostic).** Two in-loop uses of an int constant whose low byte
equals a later byte-store constant are unified into one enregistered value and push the callee-saved register into
the prologue instead of shrink-wrapping it; removing the third use restores rematerialisation. Our-only behaviour
in `_input_update_keyboard_devices`; no admissible source lever (the third use is a genuine `NONE`).
