# Audit: functions credited byte-exact that are implemented in assembly

Found incidentally 2026-08-16 while reading `source/camera/director.c` for an
unrelated reconstruction. The audit is now resolved by parking the affected
functions so assembly transcriptions are not credited as C reconstructions.
`object_shadows.code_0012b870` was subsequently recovered in ordinary C and
removed from the parked inventory on 2026-08-29.

## The standing rule

The campaign rule is *no asm, no volatile, no casts intended to alter codegen,
no byte-forcing, readable code only* — and, separately, *never credit anything
that is not byte-identical*. The second rule is being honoured. The first is
not, in nine functions across five units, and the two interact badly: an assembly transcription will
always be byte-identical, so the strict comparator cannot distinguish a genuine
C reconstruction from a transcribed listing. The gate is not a defence here;
only reading the source is.

## Inventory

Nine functions contain inline or naked `__asm` and compare
`EXACT` by `section_infos_equal`:

| unit | function | asm blocks | kind |
|---|---|---|---|
| `ai/action_converse` | `action_converse_perform` | 3 | inline, inside C |
| `camera/director` | `director_initialize_for_new_map` | 8 | inline, inside C |
| `cseries/cseries` | `stristr` | 1 | whole body |
| `effects/particles` | `code_0008fcd0` | 1 | whole body |
| `effects/particles` | `code_0008ff30` | 1 | whole body |
| `rasterizer/xbox/…active_camouflage` | `code_00148dd0` | 1 | `__declspec(naked)` |
| `rasterizer/xbox/…active_camouflage` | `code_00148de0` | 1 | `__declspec(naked)` |
| `rasterizer/xbox/…active_camouflage` | `code_00148df0` | 1 | `__declspec(naked)` |
| `rasterizer/xbox/…active_camouflage` | `code_00148e00` | 1 | `__declspec(naked)` |

Reproduce with:

```
rg -n "__asm" source -g "*.c"
```

then gate each enclosing function.

## Why this matters more than the count suggests

`director_initialize_for_new_map` is the clearest case. It is not a C function
with an asm hint — the loop body, the assert, the switch arms and the pointer
walk are all hand-written asm, including `_emit 07ch / _emit 006h` to plant a
raw `jl` opcode, and comments that say the original TU "keeps the zero constant
in bx". That is a transcription of January's listing, not a reconstruction of
January's source. It will never stop matching, and it teaches the project
nothing about what the original C looked like.

The `__declspec(naked)` cases are unambiguous by construction: a naked function
has no C body at all.

The inline cases (`action_converse_perform`, `director_initialize_for_new_map`)
are the subtler risk, because the function reads as C at a glance and the asm is
buried mid-body.

## Resolution (owner chose: park them)

All nine remaining functions are parked in `config/parked.json` under a new class
**`asm-implemented`**, so they no longer count as C reconstructions.

Parking these required a deliberate change to `tools/parked_functions.py`,
because the manifest was built for the opposite case. Its validator rejects any
entry whose function matches (`"function is now semantically exact"`) — sensible
when a park records a codegen gap, but exactly backwards here: an assembly body
matches *by construction*, and that is the reason for the park. The validator now
inverts the expectation for this class only:

* `asm-implemented` + exact  → **active** (expected state)
* `asm-implemented` + not exact → **invalid**, reason
  `"asm-implemented function no longer matches the target"` — i.e. the
  transcription has drifted from the target and needs attention
* every other class is unchanged

Two regression tests cover both directions
(`test_asm_implemented_entry_stays_active_when_exact`,
`test_asm_implemented_entry_is_invalid_when_not_exact`).

## Ordinary-C recovery: object shadows

On 2026-08-29, `source/objects/object_shadows.c` replaced the naked assembly
owner with a typed static helper:

```
static void *code_0012b870(
	long object_index)
{
	return object_get_and_verify_type(object_index, _object_mask_all);
}
```

Restoring its two in-TU call sites gives VC7 the original private calling
convention naturally. The resulting 16-byte section is strictly exact,
including its single relocation. The same change removed both
`_ReadWriteBarrier` calls from `object_build_shadow`; an ordinary local result
preserves that function's original branch topology. The complete object now
gates 3/3 exact with no assembly, intrinsic, pragma, barrier, or attribute, so
the obsolete `asm-implemented` park was deleted.

Unparking one means replacing its `__asm` with C that gates exact.

## Recovery attempt, 2026-08-16 (stristr + the four naked thunks)

Attempted after parking. **Nothing was changed** — replacing a working asm body
with non-matching C would trade a byte-exact function for a residual, which is
not a recovery.

### The four `active_camouflage` thunks are provably not expressible in C

Their bodies are e.g. `push eax; push ecx; push edx; call
D3DDevice_SetVertexShaderConstant@12; xor eax,eax; ret 4`. Two facts together
rule out every C calling convention:

* the argument registers are **live on entry** (`push eax` with no preceding
  load), so the arguments arrive in registers, and
* the symbol is **`_code_00148e00`** — plain leading-underscore, i.e. `__cdecl`
  decoration — while the body does **`ret 4`**, i.e. callee cleanup.

Measured: compiling the same body as `__stdcall` emits the symbol as
`_code_00148e00@4`, so it no longer matches the target's name at all.

| convention | symbol | cleanup |
|---|---|---|
| `__cdecl` | `_name` | caller (`ret`) |
| `__stdcall` | `_name@4` | callee (`ret 4`) |
| `__fastcall` | `@name@4` | callee (`ret 4`) |
| **target** | **`_name`** | **callee (`ret 4`)** |

No standard convention produces the target's pair. The only C route is VC7's
private convention for a `static` with in-TU call sites — and these four have
**no references anywhere in the tree**, so an unreferenced static would simply be
eliminated (measured earlier in
`research/unwritten_frontier_recipe.md`). They stay parked, and the park is now
backed by a proof rather than a judgement.

### `stristr` is expressible in C but blocked on a spill preference

`stristr` is ordinary `__cdecl` (`_stristr`, caller cleanup), so it is
recoverable in principle, and the C below is semantically correct:

```c
char *stristr(const char *haystack, const char *needle)
{
	char first = *needle++;
	long length;

	if (first)
	{
		length = csstrlen(needle);
		for (;;)
		{
			char character = *haystack++;

			if (!character)
				return NULL;
			if (character==first && !_strnicmp(haystack, needle, length))
				return (char *)(haystack-1);
		}
	}

	return (char *)haystack;
}
```

Progress measured: the naive early-return shape is **85** bytes off; moving the
empty-needle return to the tail (the branch-polarity lever that closed
`unit_get_local_player_index`) takes it to **74**; a `while (*haystack)` loop
reaches **72/96**. The prologue, the `csstrlen` call, the search loop and the
`_strnicmp` call are all in the right places.

The residue is a **spill preference**, the same class as the standing parks:
January keeps the first needle character in **bl** across the function and spills
`length` into the dead `needle` parameter slot `[ebp+0xc]`; we do the reverse,
keeping `length` in ebx and spilling the character to a fresh stack byte
`[ebp+0xf]`. Measured inert: declaration order either way, `register` on either
or both locals, `int`/`long`/`unsigned long` for the length (`int` regresses to
103), scoping `length` inside the `if`, hoisting `character` out of the loop,
`continue`-style and `while`-style loops, and computing the length before the
guard.

So `stristr` is a genuine near-miss, not an impossibility. It stays parked as asm
until a lever flips that spill choice.

### Correction: 74 is closer than 72

A follow-up pass reading the *whole* diff rather than the byte count shows the
72-byte `while (*haystack)` form is **structurally wrong**: it rotates the loop,
duplicating the load/test at the back edge and forcing `lea eax,[esi-1]` where
January uses `dec esi; mov eax,esi`. The **`for (;;)` form at 74/96 is
structurally correct** — its loop body matches January instruction for
instruction (load, `inc`, `test`, `je`, `cmp`, `jne` back-edge, `_strnicmp`,
`test`, `jne` back-edge, `dec`, return). Byte count is the wrong metric here.

With the structure pinned, the residue is one free-register contest. `esi` and
`edi` hold haystack and needle in both builds, and exactly one of
`{first, length}` can have `ebx`, because both live ranges span a call (`first`
across `csstrlen`, `length` across `_strnicmp`). January gives `ebx` to `first`
and spills `length` to `[ebp+0xc]`; we give `ebx` to `length` and spill `first`
to `[ebp+0xf]` — *also* inside the dead `needle` slot, so both compilers reuse
the same storage and differ only in which value wins the register.

Further inert levers measured: `char`/`byte`/`unsigned char` for both the first
character and the loop character, `long`/`unsigned long`/`const long` for the
length, assignment inside the `while` condition, comparing `*haystack` before
incrementing, and computing the length before the guard.

## Options considered (superseded by the above)

These are the same shape as the existing `matrix4x3_multiply` **vendored-asm
park**, which the project already treats as *not* a real match. The consistent
options are:

1. **Park them** — move to `config/parked.json` with class `asm-implemented`, so
   they stop counting toward the exact tally, exactly as `matrix4x3_multiply`
   does. This is the option that matches existing precedent.
2. **Re-reconstruct in C** — treat each as unwritten and find the real source
   shape. `stristr` and the small naked helpers look tractable; the
   `director_initialize_for_new_map` loop is a real piece of work.
3. **Leave and document** — accept them as knowingly-transcribed, but record it
   so the headline "N functions byte-exact" is not read as "N functions
   reconstructed in C".

Option 1 is the chosen current state. Individual entries can move to option 2
only after a C implementation passes the strict gate.

## Not affected

`cseries::csstrcasecmp` contains **no** asm — an earlier crude scan in this
session mis-attributed `stristr`'s asm block to it. `csstrcasecmp` is a genuine
C function and a genuine 19-byte residual (an esi/edi mirror, see
`research/reloc_identity_residuals.md`).
