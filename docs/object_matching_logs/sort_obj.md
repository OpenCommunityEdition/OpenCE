# `source/cseries/sort.obj` — MATCHING

**Status: byte-exact. All four functions, whole object.**
**Lane:** `claude/libcmt-stream` · **Object status in `config/config.json`:** `Matching`

**Toolchain:** XDK 3911 `CL.Exe` 13.00.9254.1 · **Flags:** the game set, `/O2 /Oy- /DDEBUG /Dxbox`
**Start state:** the source was a pure skeleton — symbol-map header and empty
section comments, no code.

| symbol | target | ours | exact |
|---|--:|--:|:--:|
| `_code_00080360` (2-byte selection sort) | 96 B / 0 rel | 96 B / 0 rel | **yes** |
| `_code_000803c0` (4-byte selection sort) | 80 B / 0 rel | 80 B / 0 rel | **yes** |
| `_qsort_2byte` | 336 B / 1 rel | 336 B / 1 rel | **yes** |
| `_qsort_4byte` | 320 B / 1 rel | 320 B / 1 rel | **yes** |

`section_infos_equal` passes for all four; `.text` sections are identical with
relocation fields masked. Normalized SHA-256: `21df9a493f4dd024`,
`c98ad64dbdb87dbc`, `c09f5aead7542561`, `1cedc780c4faa025`.

---

## 1. What the object is

An iterative quicksort specialised for 2-byte and 4-byte elements, with an
explicit 30-deep bounds stack and a selection-sort cutoff at 8 elements. The two
`_code_*` symbols are the selection sorts; they take `hi` in **EAX** under VC7's
custom calling convention for statics with fully-known call sites.

The comparator takes **values**, not pointers: the selection sort does
`xor eax,eax; mov ax,[ebx]` before pushing, i.e. zero-extension — so the 2-byte
element type is `word` (unsigned short), not `short`. `texture_page.c`, itself an
exact object, declares the callee as `boolean (*)(short, short)`; that prototype
disagrees with this definition but is ABI-compatible (both push 4 bytes), and the
zero-extension in the target is decisive for what `sort.c` itself uses.

`compare(a, b)` returns true when `a` sorts **after** `b` — proven by the
selection sort using it to find the maximum and swap it to the high end.

## 2. Source-shape findings

Five corrections were needed; each was read directly off the disassembly.

1. **Swap order.** The target loads *both* elements, then stores to `*best`
   first: `dx=*hi; cx=*best; *best=dx; *hi=cx`. That is
   `temp = *best; *best = *hi; *hi = temp;` — the mirror of the natural
   `temp = *hi; *hi = *best; *best = temp;`. This alone made both selection
   sorts exact.
2. **Unsigned comparisons throughout.** The target uses `jb` for the
   `element_count < 2` guard, `ja` for the cutoff test, and `shr` (not `sar`) for
   the midpoint. So `element_count` and the running `size` are **unsigned long**.
3. **`lo+1 < j`, not `lo < j-1`.** The target computes `lea eax,[esi+2]` and
   compares against `j`; writing the algebraically identical `lo < j-1` produces
   `lea eax,[edi-2]` with the operands reversed.
4. **The larger-partition test is a byte-basis comparison.** The target computes
   raw byte differences with no shift and no mask:

   ```
   e0  mov eax, edi     ; j
   e2  sub eax, esi     ; j - lo, in BYTES
   e4  mov edx, ecx     ; hi
   e6  dec eax
   e7  sub edx, ebx     ; hi - i, in BYTES
   e9  cmp eax, edx
   eb  jl  <right-larger>
   ```

   Written on `word *` pointers, any element-count form makes MSVC either emit
   two `sar`s or cancel them and mask (`and eax,0xfffffffe`) — 16 bytes too many.
   Only an explicit byte-basis comparison reproduces the plain `dec`:

   ```c
   if ((char *)j-(char *)lo-1>=(char *)hi-(char *)i)
   ```

   Note `j-1-lo >= hi-i`, `j-lo > hi-i` and the above are all integer-identical;
   only the third matches the emitted code.
5. **`depth = 0` is assigned before `hi`.** The target stores `[ebp+0xc]` (which
   the optimiser reuses for `depth` once the `element_count` parameter is dead)
   at 0x1f and `hi` at 0x26. Ordering the source the other way emits the two
   stores swapped. This was the last 3 instructions.

## 3. Notes

* `dword` is not a `cseries.h` type; the 4-byte variant uses `long`.
* Both `qsort` bodies are identical modulo element type, and both went exact from
  the same five fixes — a useful confirmation that the shapes are right rather
  than coincidental.
* The bounds stacks are `lo_stack[30]` / `hi_stack[30]`, fixed by the frame size
  (`sub esp,0xf4` = 4 + 120 + 120) and the `[ebp+eax*4-0x7c]` / `[ebp+eax*4-0xf4]`
  addressing.

## 4. Current-integration revalidation (2026-08-13)

The completed source was recovered from Claude's local `claude/libcmt-stream`
campaign commit `bf44b00d3e97f5781068b85d35d86f6d8751226e`, then cherry-picked onto the
current authoritative integration base `66af3489`. This was an audit and
admission lane, not a fresh reconstruction: reusing the already-proven work
avoided repeating the source-shape search above.

This checkout successfully rebuilt the object through Ninja with XDK 3911
`CL.Exe` 13.00.9254.1 and the configured `/O2 /Oy- /DDEBUG /Dxbox` flags. The
hardened comparator then reproduced the complete hashes and relocation evidence:

| symbol | padded bytes | relocations | normalized SHA-256 | strict result |
|---|---:|---:|---|:---:|
| `_code_00080360` | 96 / 96 | 0 / 0 | `21df9a493f4dd0244dfd9e3bdf4c1cfc68b33b8fa11f2e6bc8e30a76102b9205` | exact |
| `_code_000803c0` | 80 / 80 | 0 / 0 | `c98ad64dbdb87dbc89d00c9ff63b6ae4b1ec6685291add335aac30724f7032d4` | exact |
| `_qsort_2byte` | 336 / 336 | 1 / 1 | `c09f5aead7542561d64a5e7b41b4ba79c23bfbdf1cd45ed8c713fafa99dab200` | exact |
| `_qsort_4byte` | 320 / 320 | 1 / 1 | `1cedc780c4faa02599391876115607221a735e6d1cd51f7bcecb98eeca37dd3c` | exact |

Both quicksort relocations match in address, type, destination symbol, and
addend. The January object owns only these four `.text` COMDAT sections; it has
no `.data`, `.bss`, or `.rdata` to reconstruct. The candidate's `.drectve` and
`.debug$S` sections are the ordinary configured compiler artifacts and contain
no game-owned data.

The fail-closed TU regression gate reported all four symbols as `still_exact`,
with no changed non-exact functions, warnings, or failures. The full 466-object
`halobetacache_build` succeeded, ordinary and semantic progress generation
completed with zero unit errors, and all 173 tooling unit tests passed.

### House-rule audit

The implementation is readable C: no assembly, `volatile`, `__forceinline`,
undefined behavior, byte patching, special flags, or comparator exceptions are
used. Every parameter is on its own line, every function has an explicit final
`return;`, and the private helpers are correctly file-local. The two public
quicksort functions also retain their direct `element_count < 2` early return.
This is the narrow exception to the preferred single-return style: it is the
January-proven control-flow shape, and replacing it with an artificial shared
exit would change otherwise exact code. No tag/object access macros apply to
this generic cseries utility.
