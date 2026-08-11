# `source/networking/telnet_console.obj` — reconstruction

**Status: CLOSED — all 17 of 17 symbols byte-exact under the strict comparator
(5/5 functions, 1344/1344 `.text` bytes, all 11 string literals, `.bss`).
Was 3/5 functions, 624/1344 bytes when §5 below was written.**
**Lane:** `claude/libcmt-stream` · **Object status:** `Matching`

**Toolchain:** XDK 3911 `CL.Exe` 13.00.9254.1 · **Flags:** the game set, `/O2 /Oy- /DDEBUG /Dxbox`
**Start state:** the source was a pure skeleton — symbol-map header and empty
section comments, no code. Every symbol was missing.

| symbol | target | ours | exact |
|---|--:|--:|:--:|
| `_telnet_console_dispose` | 80 B / 7 rel | 80 B / 7 rel | **yes** |
| `_telnet_console_print` | 192 B / 18 rel | 192 B / 18 rel | **yes** |
| `_telnet_console_process` | 352 B / 33 rel | 352 B / 33 rel | **yes** |
| `_bss_00456d00` | 140 B / 0 rel | 140 B / 0 rel | **yes** |
| all 11 string literals | — | — | **yes** |
| `_telnet_console_initialize` | 224 B / 18 rel | 224 B / 18 rel | **yes** (§6) |
| `_code_001203c0` | 496 B / 30 rel | 496 B / 30 rel | **yes** (§6) |

---

## 1. The globals block

Byte-exact, which confirms the layout. Recovered by reading the **relocation
addends** — every reference into the block resolves to one of four offsets
(0x0, 0x4, 0x8, 0x88):

```c
struct telnet_client
{
	struct transport_endpoint *endpoint;              /* +0x04 */
	char buffer[128];                                 /* +0x08 */
};

struct telnet_console_globals
{
	struct transport_endpoint *listening_endpoint;    /* +0x00 */
	struct telnet_client clients[1];                  /* +0x04 */
	boolean initialized;                              /* +0x88 */
};
```

4 + 132 + 4 = 140. **`MAXIMUM_TELNET_CLIENTS` is 1**, proven by the scan loop in
`telnet_console_process`: it strides `add eax,0x84` (132 = one client) from
`globals+4` with the bound `globals+0x88`.

Defined `= {0}` so the symbol lands in `.bss` owned by this TU rather than
becoming COMMON — the `thread_win32` precedent.

## 2. Facts recovered from the disassembly

* **Port 23** (`mov word ptr [ebp-6],0x17`), `address_length` 4.
* **`create_transport_endpoint(0x12)`** — the endpoint type constant.
* **`_code_001203c0` takes its client pointer in ESI.** VC7 gives statics with
  fully-known call sites a custom calling convention; the call site does
  `push count; push buffer; mov esi, OFFSET globals+4; call`. Written as an
  ordinary third parameter, the compiler reproduces this on its own.
* **The character switch** uses a byte index table at `+0x1e0` and a 4-entry jump
  table at `+0x1d0`. Decoding both gives the cases exactly: `c=4` disconnect,
  `c=8` backspace, `c=10`/`c=13` execute via `hs_compile_and_evaluate`, other
  values in 4..13 skip. Printable (`isalnum`/`ispunct`/space) appends.
* **`read_endpoint` into a 32-byte stack buffer**, with the returned count
  `push`ed *before* the branch so it serves as the argument to either
  `code_001203c0` or `transport_error_to_string`.

## 3. Source-shape findings

Five real corrections, each verified by the gate:

1. **`print` tests its write result once, at a shared join.** January writes
   `"\r\n"`, then the string, then optionally the pending input buffer, and tests
   **once** at a join reached both by fall-through and by the empty-buffer branch.
   A short-circuit `a>0 && b>0 && (!buf[0] || c>0)` adds a branch and shifts every
   later offset. Using a `result` local and testing at the join made it exact.
2. **`process` indexes the globals directly, it does not cache a `client`
   pointer.** The three missing relocations were all references to
   `bss_00456d00+0x4/+0x8/+0x88`. Rewriting the second half to use
   `bss_00456d00.clients[0]` throughout recovered them.
3. **`process`'s greeting branch is written failure-first.** January's layout is
   `jg <assign>` falling through into the delete; writing `if (write>0) {assign}
   else {delete}` produces the opposite layout. Inverting to
   `if (write<=0) {delete} else {assign}` made the function exact.
4. **`initialize` uses `if (endpoint) {...} else {error}`, not an early return.**
   The positive form places all three error blocks out of line at the end, in
   reverse order, with the bind and listen paths tail-merging into a shared
   `delete` + NULL + return (`jmp 0x97`). The early-return form emits them inline.
5. **The switch case order is `10/13`, `8`, `4`.** MSVC lays case bodies out in
   source order, and January's are execute → backspace → goodbye. Writing them in
   ascending value order reverses the layout: this alone took `_code_001203c0`
   from 219 to 120 differing instructions.

Also: **the `transport_address` is zeroed inline in an inner scope.**
`csmemset(&address, …)` emits a real call (+2 relocations, 208 B); declaring
`= {0}` at function scope emits the zeroing before the globals `csmemset`. An
inner block opened after the endpoint check puts it where January has it.

## 4. `transport.h` — union member order

`struct transport_address_data`'s union originally listed `byte bytes[16]` first.
With that order, `struct transport_address address = {0}` makes MSVC initialise
the union's *first member* — so it emits a byte store for `bytes[0]` followed by a
misaligned blob fill (`[ebp-0x17]`, `[ebp-0x13]`, …). January's fill is six clean
4-aligned dword stores.

Reordering the union to put `unsigned long long_words[4]` first restores the
aligned fill and matches January. The union order is not observable except
through codegen. The admission lane forced clean rebuilds of both current exact
consumers: all 5 `transport_address.obj` functions and all 100
`ui_widget_event_handler_functions.obj` functions remained strict-exact.

## 5. The residual as it was diagnosed — SUPERSEDED BY §6

**This section's class verdict was wrong.** Both functions were adjudicated
here as register-allocation ties; both closed from source (§6). The measured
negatives below remain valid negatives and are kept. Two specific claims are
corrected: (a) the two functions were *not* "identical modulo register naming"
— `_code_001203c0` had two `push 0` where January has `push eax` (a 2-byte
code-size difference hidden by section alignment padding, which also displaced
the jump table), and (b) the register assignment was not a tie — it was
downstream of a live range the source failed to create, exactly the house-rules
§6 pattern "read the live ranges, never chase the registers".

### `_code_001203c0`

January puts `index` in EDI and `size` in EBX; we do the reverse. A mechanical
EBX↔EDI rename of our disassembly aligns **132 of 172** instructions, and the
residue shows it is *not* a clean swap: at the execute case both builds use EDI
for the buffer pointer (`lea edi,[esi+4]`). January therefore **reuses EDI for
both `index` and the buffer pointer**, spilling `index` and reloading `size` from
`[ebp+0xc]` each iteration; we keep the two in separate registers. Same
instruction sequence, different live-range packing.

Verified inert against it:

| attempt | ndiff |
|---|--:|
| baseline | 120 |
| `index` declared before `result` | 120 |
| `length` at function scope | 120 |
| `character` and `length` both at function scope | 120 |
| named `client_buffer` local in the append path | 119 |
| cached `limit` local for the bound | 119 |
| `buffer[index]` directly, no pointer variable | 119 |
| `size>index` | 119 |
| `index<size && result` (result tested second) | 275 |
| `while` form | 198 |

The one-instruction wins are metric noise, not progress: `size>index` emits
`cmp ebx,edi; jle` where January has `cmp edi,ebx; jge`, so `index<size` is the
faithful form and is what the source keeps.

### `_telnet_console_initialize`

January materialises `0` into **two** registers — ECX for the three address
dwords, EDX for `[ebp-8]` and `[ebp-4]` — because ECX is consumed by
`lea ecx,[ebp-0x18]` for the `&address` push. We keep one zero register and use
EDX for the `lea`, so the three error-path reloads then rotate EDX/EAX/ECX
against our EAX/ECX/EDX. The extra `xor edx,edx` is exactly the two bytes our
version spends on trailing padding — both functions are 224 bytes.

Verified inert or worse: field assignment order (`port` first), explicit
`address_type = 0` (133), `address_type = 0` before the other fields (133),
declaring `address` at function scope (115).

### Class (as adjudicated then — retracted in §6)

Same as the `woutput` residual — a register-allocation / live-range packing
decision with no source or flag lever found. Not credited; the object stays
`NonMatching`.

## 6. Close-out — both "register ties" fell to source levers (2026-08-10)

Diagnosed by a relocation-masked lockstep walk of both residuals against the
target (register- and slot-wildcarded tiers), which separated the genuine
divergences from the permutation noise the earlier ndiff metric mixed together.

### `_code_001203c0`: January passes the write result to the error report

At **both** write-failure sites January has `push eax` where we had `push 0`:
the original passes the failed `write_endpoint` result to
`transport_error_to_string`, not a literal `(short)0` — the same idiom
`telnet_console_process` already proved with its `(short)count`. Spelled

```c
length = write_endpoint(client->endpoint, character, 1);
if (length<=0)
{
	error(2, "failed to write to telnet client ('%s')",
		transport_error_to_string((short)length));
	...
}
```

(and likewise at the overflow-message site, reusing the `length` local), the
function became **byte-exact in one step**. The named result's live range now
crosses into the error arm, and that alone flipped the entire EBX/EDI
assignment and the prologue push schedule into January's shape — the "120
differing instructions" were all downstream of these two calls. The tell in
the disassembly was 1-byte `push eax` (0x50) vs 2-byte `push 0` (0x6A 00)
with every later branch target off by one, and section size still 496 because
alignment padding absorbed the difference. **Size-exactness can hide a real
code-size delta; diff the padding tail too.**

(The earlier §5 claim that January "reuses EDI for both `index` and the buffer
pointer" was an artifact of the mechanical rename: January converts `index`
to the character pointer in place (`add edi, eax`) exactly as we do, and the
append-path buffer pointer lands in EBX. The execute path's
`lea edi,[esi+4]` matched byte-for-byte on both sides while meaning different
things — jan's EDI held the dead character pointer, ours the dead size copy.)

### `_telnet_console_initialize`: `{{0}}`, not `{0}`

January's zero-fill of the `transport_address` local splits **exactly at the
member boundary**: `[-0x18]` immediate + `[-0x14..-0xc]` from ECX cover the
16-byte `struct transport_address_data address` member, then
`lea ecx,[ebp-0x18]` consumes ECX, then `[-8]`/`[-4]` (the trailing
`address_length`/`port`/`address_type` scalars) come from a freshly
materialised EDX. Two zero sources for one initializer means two fill
operations in the IR. **VC7 groups a `= {…}` zero-fill by brace level**:

- `= {0}` → the `0` initialises the deepest first scalar
  (`long_words[0]`, the immediate store) and the *rest of the whole struct*
  is one fill — one zero register (ours, wrong);
- `= {{0}}` → member 0's fill and the implicit zeroing of the remaining
  members are *separate* fills — the allocator can hand the address `lea` the
  first fill's dead register and materialise a second zero for the tail
  (January, exact).

One character of source. The three error-path register rotations §5 describes
were pure cascade from this and closed with it.

### Verification

Whole unit rebuilt via ninja (CL path patched per the known checkout issue)
and every one of the 17 target sections verified with the strict comparator
(`section_infos_equal`): **17/17 EXACT**, 5/5 functions, 1344/1344 `.text`
bytes. `configure.py progress` validation (parked/semantic/ownership gates)
passes. Status flipped `NonMatching` → `Matching`.

The §5 compiler A/B result (all three local VC7 suites byte-identical) stands
— correctly, as it turns out: the residual was never compiler variance.

### House-rule audit

Every parameter remains one-per-line, both no-argument `void` parameter lists
use the project form, and every void function ends in an explicit `return;`.
The source contains no assembly, volatile codegen forcing, alignment trick,
compiler-flag override, or raw typed `tag_get`/`object_get` cast. The early
returns in `telnet_console_print`, `telnet_console_process`, and
`code_001203c0` are measured control-flow requirements of the exact January
sections; rewriting them into one exit changes the emitted branches and is the
documented exception to the preferred single-return rule.

> Note: `ninja` cannot launch `xbox/bin/vc7/CL.Exe` in this checkout
> (`CreateProcess failed`); pre-existing, reproduces in `campaign-integration`.
> Verification used CL directly with the exact cflags ninja generates.

## Compiler A/B — all three local VC7 suites produce identical bytes

Prompted by the `units.obj` compiler-provenance work
(`units-campaign-current-20260802/docs/object_matching_logs/units_obj_compiler_provenance.md`
and `units_obj_compiler_qfe_hunt.md`), the local worktrees were re-censused for
distinct compiler binaries. **Three** distinct `CL.Exe` images exist, not one:

| suite | copies | `CL.Exe` SHA-256 (16) | note |
|---|--:|---|---|
| 13.00.9254.1 | 830 | `483e00c47bb08d69` | project standard |
| 13.00.9210 | 4 | `38955691c987c64d` | genuinely different `C1`/`C2` back end |
| vsnet-beta2-rc4-9254 | 1 | `cd6b42c2e4f14140` | reports 9254 but is a distinct image |

This object was rebuilt with all three complete suites (each with its matching
`C1.dll` / `C1xx.dll` / `C2.dll`), same flags, same source. **All three produce
byte-identical output**, with the identical residual.

That is a stronger park than "only one compiler is available": the residual is
now shown not to be compiler-version variance at all. It also corrects an
earlier claim in this log that only a single CL binary existed locally — that
was wrong, and the conclusion it supported (a QFE-level codegen difference)
is not supported by this test.
