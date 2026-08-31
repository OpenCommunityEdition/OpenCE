# `libs/libcmt/woutput.obj` — `_woutput` reconstruction

**Status: 5 of 6 symbols byte-exact. `__woutput` within 4 bytes with the exact relocation count; residual is a stack-slot colouring difference.**
**Lane:** `claude/libcmt-stream` · **Object status in `config/config.json`:** `NonMatching`

**Toolchain:** XDK 3911 `CL.Exe` 13.00.9254.1 · **Flags:** `/O1 /Gy /I"xbox/include"`
**Start state:** 0.00% — object `MISSING`, no source.

| symbol | target | ours | exact |
|---|--:|--:|:--:|
| `_code_001d23d8` (`write_char`) | 37 B / 1 rel | 37 B / 1 rel | **yes** |
| `_code_001d23fd` (`write_multi_char`) | 37 B / 1 rel | 37 B / 1 rel | **yes** |
| `_code_001d2422` (`write_string`) | 61 B / 1 rel | 61 B / 1 rel | **yes** |
| `_code_001d245f` (`get_int_arg`) | 9 B / 0 rel | 9 B / 0 rel | **yes** |
| `_code_001d2468` (`get_int64_arg`) | 12 B / 0 rel | 12 B / 0 rel | **yes** |
| `__woutput` | 1932 B / 30 rel | 1928 B / **30 rel** | no |

---

## 1. What the object is

The CRT's wide printf formatting engine. As in the original CRT, `woutput.c` is a
thin wrapper that compiles `output.c` with the wide configuration:

```c
#define _UNICODE 1
#define UNICODE 1
#include "output.c"
```

This lane therefore adds **both** files. `libs/libcmt/output.c` holds the engine
and the five statics; `libs/libcmt/woutput.c` is the two-line wrapper. The
`output` unit is left `MISSING` — the narrow object is not claimed here, but the
source is in place for it.

## 2. Structure recovered from the target

`_woutput(FILE *stream, const wchar_t *format, va_list argptr)`, frame `0x450`:

| slot | meaning | slot | meaning |
|---|---|---|---|
| `[ebp-4]` | flags | `[ebp-0x2c]` | hexadd |
| `[ebp-8]` | textlen | `[ebp-0x30]` | no_output |
| `[ebp-0xc]` | text | `[ebp-0x34]` | mbtowc loop count |
| `[ebp-0x10]` | precision | `[ebp-0x38]` | state |
| `[ebp-0x14]` | charsout (return value) | `[ebp-0x3c]` | capexp |
| `[ebp-0x18]` | prefixlen | `[ebp-0x40]` | wchar |
| `[ebp-0x1c]` | bufferiswide | `[ebp-0x44]` | mbtowc retval |
| `[ebp-0x20]` | fldwidth | `[ebp-0x50]` | double arg |
| `[ebp-0x24]` | `prefix[2]` | `[ebp-0x450]` | `TCHAR buffer[512]` |
| `[ebp-0x28]` | `tempchar[2]` | | |

Confirmed details:

* **`BUFFERSIZE` = 512.** Integer conversion writes backwards from `[ebp-0x251]`,
  which is `&((char *)buffer)[511]` — pins both the buffer size and the fact that
  digits are emitted as **bytes** even in the wide build, then converted through
  `mbtowc` at output time when `bufferiswide == 0`.
* **Flags:** `FL_SIGN` 1, `FL_SIGNSP` 2, `FL_LEFT` 4, `FL_LEADZERO` 8, `FL_LONG`
  0x10, `FL_SHORT` 0x20, `FL_SIGNED` 0x40, `FL_ALTERNATE` 0x80, `FL_NEGATIVE`
  0x100, `FL_FORCEOCTAL` 0x200, `FL_WIDECHAR` 0x800, `FL_I64` 0x8000 — each read
  off a specific `or`/`test` (e.g. `or byte ptr [ebp-3],8` = 0x800 for `w`).
* **`'C'` and `'S'` default to `FL_SHORT`** in the wide build (`test word ptr
  [ebp-4],0x830` then `or ...,0x20`), i.e. the ISO-inverted forms.
* **`_putwc_lk` returns `wint_t` (16-bit)** — the target's `cmp ax,0xffff` versus
  `cmp eax,0xffff`. Declaring the return as `int` was a 1-byte miss in
  `write_char`; fixing the type made it exact.
* **`NORMAL_STATE:` is a label inside `case ST_NORMAL`**, jumped to by the `'I'`
  handler in `ST_SIZE`. That is why the target lays the `ST_NORMAL` body between
  `ST_SIZE` and `ST_TYPE` rather than first.
* **`_cfltcvt_tab` indices:** `[0]` conversion, `[1]` cropzeros, `[3]` forcdecpt.
  The dispatch offsets are relocation fields, so index choice does not affect the
  normalized comparison — they are set to the conventional CRT values.
* **The five statics get custom register calling conventions** from VC7 (e.g.
  `write_char` takes `f` in EAX and `pnumwritten` in ESI). This is compiler-chosen
  for statics with fully known call sites and is not expressible in source — it
  fell out correctly once the bodies and call sites were right.

## 3. `__lookuptable` / `_rdata_002b5c08`

The target references the state table through **two different symbols**:

```
.text+0x48   movsx eax, byte ptr [eax + _rdata_002b5c08]   ; FIND_CHAR_CLASS
.text+0x52   movsx eax, byte ptr [ecx + eax*8 + ___lookuptable]  ; FIND_NEXT_STATE
```

These are the same array. `__lookuptable[ch - ' ']` has an effective address of
`table - 0x20`, which lands before the symbol, so csplit anchored it to the
preceding data symbol `_rdata_002b5c08`. A rebuild naturally emits both through
`__lookuptable` with different addends.

This does **not** block a strict match: `section_infos_equal` compares size,
relocation **count**, and normalized bytes (relocation fields masked) — it does
not compare the symbol a relocation names. Count and positions are what matter.

## 4. Remaining work on `__woutput`

**1928 B vs 1932 B, and the relocation count matches exactly (30/30).**
Aligning the two instruction streams while normalising away stack-slot numbers
and jump displacements leaves **123 of 592 instructions** misaligned, down from
206 when this alignment method was first applied.

Five real source bugs were found and fixed by that alignment:

1. **`strlen` was being called, not inlined.** The target has an inline
   `mov cl,[eax]; inc eax; test cl,cl; jne` loop. Adding `#pragma intrinsic(strlen)`
   removed the call — this was the extra 31st relocation, and is why the counts
   now match.
2. **`__nullstring` / `__wnullstring` are arrays, not pointers.** The target does
   `mov eax, OFFSET __nullstring` (address); declaring them `char *` produced
   `mov eax, [__nullstring]` (a load of the pointer's value).
3. **The double is fetched as a struct, not a bare `double`.** The target copies
   8 bytes with two integer moves rather than `fld`/`fstp`, which is what
   `va_arg(argptr, _CRT_DOUBLE)` produces (`typedef struct { double x; }`).
4. **Wide-string length is a pointer difference** (`sub; sar 1`), not a counter
   incremented in the loop.
5. **The number text is a `char *`.** Writing the `FL_FORCEOCTAL` push-back as
   `*(char *)(--text)` on a `_TCHAR *` decremented by **two** bytes; the target
   decrements by one (`dec dword ptr [ebp-0xc]`).

Also corrected:

* `wchar` is a single function-scope local shared between the `'c'` case and the
  `mbtowc` output loop (the loop takes its address), and that loop is a
  `do { ... } while (count > 0)` rather than `while (count--)`.
* the `'s'` narrow path does **not** reset `bufferiswide` — it relies on the
  `ST_PERCENT` reset, and only the wide branch sets it. Removing that redundant
  assignment took the mismatch from 204 to 194 instructions and the size from
  1936 to 1929.
* `text` is the CRT's `union { char *sz; wchar_t *wz; }`. This is codegen-neutral
  here but is the authentic shape and makes the octal push-back
  (`*--text.sz = '0'`) express the target's one-byte decrement directly.

### The integer fetch uses two variables, not one

The single largest win. January's code reads:

```
5c7  test dword ptr [ebp-4], esi   ; FL_I64
5ca  mov  ebx, eax                 ; number = value, assigned ONCE
5cc  mov  edi, edx
5ce  jne  0x5d2
5d0  xor  edi, edi                 ; else zero the high dword
```

The fetched value stays in EAX:EDX across the whole size/sign dispatch and only
becomes `number` at the end. That is not one variable — it is a signed `__int64`
temporary carrying the fetch and the negation, then a separate `number`:

```c
__int64 l;
unsigned __int64 number;

if (flags & FL_I64)          l = get_int64_arg(&argptr);
else if (flags & FL_SHORT)   l = ... (short / unsigned short);
else                         l = ... (int / unsigned int);

if ((flags & FL_SIGNED) && l < 0) { l = -l; flags |= FL_NEGATIVE; }

if (flags & FL_I64)  number = (unsigned __int64)l;
else                 number = (unsigned long)l;
```

Writing it with a single `number` made every fetch branch store into the
variable's home register pair; splitting it dropped the mismatch from **194 to
125 instructions**. Note the trailing `if/else` is required — rewriting it as
"assign then conditionally truncate" regresses to 160.

Two smaller shapes followed: `text.sz = ++tp` after the digit loop, and setting
`bufferiswide` before the `wchar` fetch in the `'c'` case (123).

### The dominant residual: a stack-slot COLOURING difference

Not a simple swap — both builds pack two variables into one slot, but they pick
**different pairs**:

| slot | January | ours |
|---|---|---|
| `[ebp-8]` | `textlen` **+ `radix`** (shared) | `precision` |
| `[ebp-0xc]` | `text` | `text` **+ `radix`** (shared) |
| `[ebp-0x10]` | `precision` | `textlen` |

`radix` is live only between the `ST_TYPE` case that sets it and the digit loop
that reads it; both `textlen` and `text` are dead across that span, so either may
legally share the slot with it. January colours `radix` against `textlen`; we
colour it against `text`. Every other slot in the frame agrees exactly.

Traced by dumping every `[ebp-N]` reference and attributing it: `[ebp-8]` in the
target carries `radix` at 0x36c/0x51c/0x545 and `textlen` at 0x15/0x2a2/0x2cd/
0x653/0x69f, which is only consistent with slot sharing.

Verified inert against it:

* **declaration order** — five permutations including swapping `textlen` and
  `precision` directly and moving `radix` last. MSVC assigns these by live-range
  colouring, not declaration order.
* **`radix` type** — `unsigned int` regresses to 173.
* **compiler flags** — fifteen sets. `/O1 /Gy`, `/O1`, `/O1 /Gy /Oi`, `/G6`,
  `/Gf`, `/GF`, `/Gs`, `/Op`, `/Oxs /Gy` all give the identical 123; anything
  touching `/Ob` or dropping `/Og` regresses badly (130 with 38 relocations, or
  worse).

### Remaining shape regions (11)

After normalising slots and jump displacements, 123 of 592 instructions are
misaligned across 11 regions. All are register-residency or constant-materialisation
choices, not source shape:

* January materialises `0` into EDI once in the float block and compares against
  it three times (`xor edi,edi` + `cmp SLOT,edi`); we use immediate compares.
* January keeps `flags` memory-resident and does `or byte ptr [ebp-3],1` for
  `FL_NEGATIVE`; we hold it in EBX and write back.
* January stores `text` then reloads it after the NULL check; we keep the value
  live in a register across the branch.
* `inc esi` versus `lea ecx,[esi+1]` after the digit loop — same knock-on.

The 4-byte size gap is the net of these: we are missing four one-to-three byte
instructions (a reload, an `xor`, an `inc`, a `mov`) and carry four extra of
similar size.

### Live-range experiments (all inert or worse)

The remaining shape regions were attacked directly, on the theory that changing
live ranges might flip the colouring. None moved the result off 123:

| attempt | result |
|---|--:|
| `*tp = digit; tp--;` instead of `*tp-- = digit;` | 123 |
| `digit` declared inside the digit loop | 123 |
| sign check nested (`if (SIGNED) { if (l < 0) ... }`) | 123 |
| `tp = (char *)buffer + BUFFERSIZE - 1` (drop `charbuffer`) | 123 |
| `while (--precision >= 0 \|\| number != 0)` | 125 |
| `(int)(unsigned long)(number % radix)` | 123 |
| named 64-bit `rem` temp for the remainder | 157 |
| copy-then-remainder via a 64-bit temp | 175 |
| truncation as `number &= 0xffffffff` | 158 |
| truncation as assign-then-`(unsigned long)` | 160 |

The named-remainder attempts were motivated by January's `mov [ebp-0x44],ebx` at
0x618, which stores the *high half* of the 64-bit remainder — a plausible tell
for a named temporary. Both forms regressed sharply, so that store is the
allocator spilling, not a source-level variable.

### Assessment

Twelve genuine source defects were found and fixed by the alignment method, and
they took the object from unbuildable to 5/6 symbols byte-exact with the exact
relocation count and a 4-byte size gap. What remains has the same signature as
the campaign's parked objects: a live-range colouring decision inside the
register allocator, reachable by neither source shape nor flags. It is closer
than any of those parks (Chicago, hashtable, orbiting_camera, errors), but it is
the same class of wall.

**This is not credited as matching.** The object stays `NonMatching`.

## 5. Build integration

`config/config.json` already declared `libs/libcmt/woutput.c` (index 700). This
lane adds the source pair and flips that object `MISSING` → `NonMatching`. The
`libcmt` project's `/O1 /Gy` recipe was established by the `stream` work in this
same lane and is what makes these objects buildable at all.

Canonical integration revalidates this object through the normal Ninja build,
the focused campaign gate, and the parked-evidence validator; no direct-CL-only
exception remains.

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
