# The C2 named hot addresses are profiler artifacts (Jonas, 2026-08-27)

Research record. No matching credit is claimed and no production source changed;
`units.obj` stays 175/10/4 code and 128/0/1 data.

This was an attempt to run the decode that
`docs/object_matching_logs/c2_backend_attribution_20260826.md` §5 named as the
campaign's strongest next lever — "breakpoint `0x107304d1` (the unambiguous dag
function) on a compile large enough to exercise it" — against
`_unit_throw_grenade_release`. **The decode cannot be run as specified, because
`0x107304d1` is never executed.** Five of the seven named addresses in that
ledger are not code entry points at all.

## 1. Measured: five of seven named addresses take zero hits

Breakpoints planted in `C2.Dll` (fixed base `0x10700000`, so the documented VAs
are literal) over one full `source/units/units.c` compile, `dbg32.exe`:

| named address | ledger attribution | hits |
| --- | --- | ---: |
| `0x107304d1` | dag(26), "no competing module" — the designated target | **0** |
| `0x1072a4dd` | dag(10) | **0** |
| `0x1070c2e9` | factor(7) — the `code_00198fd0` target | **0** |
| `0x10780825` | color(15) | **0** |
| `0x10732284` | color(7)/dag(6) | **0** |
| `0x1072b4b4` | dag(21) | 250 (capped) |
| `0x10701000` | arena allocator | 223 |

The instrument is sound: on the same run the two live addresses cap out, and the
loader message confirms `c2 loaded @0x10700000, planted 7 bps`. The same table
holds on an isolated single-function compile.

## 2. Root cause: `func_hits` keys are int3 bucket labels, not addresses

`eip_profile.py` buckets every sample to the nearest preceding int3 boundary:

```python
def func_starts():                      # every byte after an int3 run
    ...  if text[i] == 0xCC and text[i+1] != 0xCC: starts.append(...)
fh[starts[bisect.bisect_right(starts, eip) - 1]] += 1
```

So a key is a **label for a region**, and its sample count is the traffic of the
whole region — which for `0x107304d1` spans `0x107304d1`-`0x107319b1`, 5,344
bytes. The label itself is merely the first byte after the padding; in
POGO-split code that byte is usually reached only by a branch, and at
`0x107304d1` it is a `jmp` that nothing jumps to. `0x1072b4b4` and `0x10701000`
fire only because those particular boundaries happen to coincide with real
entries.

This also re-explains the earlier "every static-map breakpoint took zero hits"
result. Hot/cold splitting is real and documented, but it is not the whole
story: **the replacement addresses inherited the same defect** — they were never
executable addresses either.

## 3. The regions are hot; the labels are not

`bpsweep.py` (new, in the scratch lab) resolves this. It disassembles a region,
takes the **backward-branch targets** — guaranteed instruction boundaries, and
loop heads, so they execute if the loop runs — and breakpoints them eight at a
time. Against an isolated `_unit_throw_grenade_release` compile, inside the very
region labelled `0x107304d1`:

```text
0x10730881  93    0x1073089d  93    0x10730946  93
0x10730a27  88    0x10730a4b  83    0x10730a53  83
0x1073076d  32    0x107306e4  30    0x10730988  30
... 30 of 48 candidates executed
```

So the dag region is active for this function. Only the label was dead. Any
future decode must sweep for real addresses first; **do not breakpoint a
`func_hits` key.**

`eip_raw.py` (new) is the other half of the fix: it reuses the sampler but emits
the raw EIP histogram instead of the bucketed one, so its addresses are directly
breakpointable. Over 25 isolated compiles (53.4M samples, 2.79M in C2) the
hottest true address is `0x1070c7bb` — which is `0x1070c2e9 + 0x4d2`, i.e.
inside the factor bucket but 1,234 bytes away from the label the ledger told the
next lane to breakpoint.

## 4. There is no instruction scheduler in this compiler

`C2.Dll` 13.00.9254.1 contains exactly 48 `__FILE__` path strings:

```text
Common: error getflags ioin
P2:     coff coffemit color dag dlp ehexcept emit except factor fg getattr
        globdf globlopt globopt hash ide inline list ltcg lur main misc mod
        optimize p2pragma p2symtab pogocg reader sizeopt stack tuple
P2/x86: cgintrin ehgen fppeeps gzmd lower lowerflt mdlist mdmisc sehgen
P2/wvm: codew lowerw mdlistw mdmiscw sehgenw
```

plus four `.cpp` (`utccom`, `dll`, `imeta`, and one more). **The string "sched"
does not occur anywhere in the image, in any casing.** There is no `sched.c`, no
scheduling pass, and exactly one peephole module, `x86/fppeeps.c`.

This retires the campaign's framing. The `unit_place`, `throw_grenade_release`
and `euler` residuals have been described as "scheduler ties" and chased through
a "dag.c scheduler"; there is no scheduler to find. Instruction order is decided
during **`dag.c`'s DAG-driven emission** (with `x86/fppeeps.c` as the only
peephole stage). That is a different kind of decision — an evaluation-order
choice over a DAG, not a ready-list priority — and it should be described and
attacked as such.

## 5. `0x1072b4b4` re-decoded: why its hit counts were never the verdict

The one dag address that does fire is now fully read:

```text
0x1072b4a9  cmp byte [eax+8], 1        ; scan a list threaded on +0
0x1072b4af  cmp [eax+0x1c], ebp
0x1072b4b4  mov eax, [eax]             ; <-- the named address: the next step
0x1072b4b8  jne 0x1072b4a9             ;     of a SEARCH loop
0x1072b4c9  or byte [ebp+6], 8         ; commit: mark node done,
0x1072b4d5  mov [ebp+0x14], <list>     ; unlink from list threaded on +0x14,
0x1072b4d8  mov <list>, ebp            ; push onto the list at [esp+0x1c]
```

The named address is the **next-pointer step of a search**, so its count is the
length of the scanned list — confirming, mechanically, the earlier warning that
its counts track enumeration rather than decisions. The commit at `0x1072b4c9`
fires 29 times for a 264-instruction function, so it is block-level, not
per-instruction, and is not the instruction-ordering decision either.

## 6. Byproduct: an isolated reproduction of the residual

`mk_single.py` builds a translation unit containing exactly one function
definition (keeping every declaration, typedef, global and prototype; removing
only top-level function bodies, which are the blocks delimited by a `{` and `}`
at column 0 — struct and initialiser blocks end in `};` and are untouched). The
kept function is given external linkage because an unreferenced `static` is
eliminated; that is measured-equivalent here (probe `v_extern`).

The result, `tgr_only.c`, is **968 lines and reproduces the residual exactly**:
784 bytes, 26 relocations, normalized sha `5643bd9b8da40736…`, identical to the
full-TU carrier-body probe, with the same two transpositions. Every C2
breakpoint hit during its compile therefore belongs to
`_unit_throw_grenade_release`. This is a far better decode substrate than gate
mode over a full compile and should be the starting point for any future
attempt, on this or any other owner.

## 7. Status and the corrected next lever

No owner became exact and no source changed. The decode as specified is void.
What replaces it:

- Re-sweep with `bpsweep.py` from the real executed addresses listed in §3, on
  `tgr_only.c`, to find the `dag.c` emission-order code — **not** a ready-list
  priority, which does not exist.
- The honest prior for that work is now lower than the previous ledger implied:
  ~200 source variants are already measured inert for this residual (see
  `units_obj_jonas_carrier_body_frontier_20260827.md`), so the decode's value is
  mainly in settling whether a source-visible lever exists at all.
