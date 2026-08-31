# Audit: functions credited byte-exact that are implemented in assembly

Found incidentally 2026-08-16 while reading `source/camera/director.c` for an
unrelated reconstruction. The initial audit parked the affected functions so
assembly transcriptions were not credited as C reconstructions. Subsequent
source-credibility work recovered `object_shadows.code_0012b870`,
`action_converse_perform`, and `director_initialize_for_new_map` in ordinary C.
On 2026-08-30 the four Active Camouflage transcriptions were also deleted after
stock-XDK owner evidence exposed their address-placeholder premise as false.
Three assembly implementations now remain parked: `stristr` and two Particles
functions.


## The standing rule

The campaign rule is *no asm, no volatile, no casts intended to alter codegen,
no byte-forcing, readable code only* — and, separately, *never credit anything
that is not byte-identical*. At discovery, the first rule was violated in nine
functions across five units, and the two rules interacted badly: an assembly
transcription will always be byte-identical, so the strict comparator cannot
distinguish a genuine C recovery from a transcribed listing. Three such
implementations remain, explicitly parked and excluded from credit. The gate
is not a defence here; only reading the source is.

## Historical inventory at discovery

Nine functions contained inline, whole-body, or naked `__asm` and compared
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

`director_initialize_for_new_map` was the clearest case when this inventory was
opened. It was not a C function with an asm hint — the loop body, assert,
switch arms and pointer walk were all hand-written asm, including `_emit 07ch /
_emit 006h` to plant a raw `jl` opcode, and comments claimed the original TU
"keeps the zero constant in bx". That transcription was removed on 2026-08-30;
see the ordinary-C recovery below.

The former `__declspec(naked)` cases were unambiguous by construction: a naked
function has no C body at all.

The inline cases (`action_converse_perform`, `director_initialize_for_new_map`)
were the subtler risk, because each function read as C at a glance while the
assembly was buried mid-body. Both are now ordinary C: Action Converse remains
an honest fuzzy park, while Director new-map is strict exact. The remaining
inventory is visibly whole-body assembly.

## Initial resolution (owner chose: park them)

All nine functions were initially parked in `config/parked.json` under a new
class **`asm-implemented`**, so they no longer counted as C reconstructions.

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

## Ordinary-C recovery: Director new-map initialization

On 2026-08-30, all eight assembly blocks were removed from
`source/camera/director.c::director_initialize_for_new_map`, including the raw
branch-byte emission, manually staged registers, direct structure offsets,
private-call setup, and pointer walk. The retained source is a coherent typed C
routine: select the authenticated game/editor mode, clear `initialize_camera`,
reset each local player's `camera_change_pause`, `bored_time`, and `bored`
state, then call `director_choose_camera(local_player_index, TRUE, FALSE)` and
`director_initialize_variables(local_player_index)`.

This shape is independently corroborated by the January control flow and data
offsets, HCEA's Director reconstruction, and the stianeklund and pastudan
histories. The authenticated `director_game_mode` constants, full Director
layout, and field names replace numeric switch labels, unknown-member names,
and the prior overlapping pseudo-array layout. Ten adjacent address
placeholders were mapped to authenticated semantic Director helper names. The
remaining 64-byte accessor, whose original name was unavailable, is named
`director_get` by the repository's getter convention and implemented as a typed
static helper.

The former transcription was 208 padded bytes with 19 relocations and matched
January by construction. The first ordinary-C checkpoint was an honest
128-byte/ten-relocation residual because the private camera-selection callees
were still absent. Defining the authenticated private helper cluster as
ordinary `static` C restored the original translation-unit context naturally:
`director_set_camera`, `director_initialize_variables`,
`director_rotate_cameras`, `director_choose_game_perspective`,
`director_choose_camera_game`, `director_choose_camera_editor`,
`director_choose_camera_script_camera_record`, and `director_choose_camera`
all compare strict exact. VC7 again inlines the 80-byte wrapper into the
new-map loop without an inline directive, and
`director_initialize_for_new_map` now reproduces the target's 208 padded bytes,
19 relocations, and normalized SHA-256
`70b72e43999f76618be8ead0ece3d3a9facba546b477b5245a1f0d2f88e9c6c6`.

The Director park is therefore removed. No assembly, raw emission, volatile or
register forcing, optimizer directive, fabricated branch, direct structure
offset, or byte patch was used, and the January inline schedule still emits no
`point_from_line3d` COMDAT. Full measurements and provenance are recorded in
`docs/object_matching_logs/director_obj_jonas_ordinary_c_credibility_recovery_20260830.md`.

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

Unparking an `asm-implemented` entry means replacing its `__asm` with C that
gates exact. Replacing the asm with honest nonexact C is still a credibility
recovery, but the function must remain parked under the correct residual class.

## Recovery attempt, 2026-08-16 (stristr + the four naked thunks)

At that point, **nothing was changed**: replacing a working asm body with
non-matching C would have traded a byte-exact function for a residual. The
Active Camouflage conclusion was later invalidated by the owner recovery below;
those four bodies are now deleted rather than preserved as fake exact source.

### Correction, 2026-08-30: the four `active_camouflage` owners are stock XDK wrappers

The earlier calling-convention proof is withdrawn. It treated csplit's
`_code_00148dd0` through `_code_00148e00` address placeholders as original
symbol names. They were not. The target bodies, relocation destinations, and
contiguous family match the stock XDK 3911 `D3DINLINE` owners exactly:

| Former placeholder | Correct owner | Callee |
|---|---|---|
| `_code_00148dd0` | `_IDirect3DDevice8_SetVertexShaderConstant@16` | `_D3DDevice_SetVertexShaderConstant@12` |
| `_code_00148de0` | `_IDirect3DDevice8_SetVertexData2s@16` | `_D3DDevice_SetVertexData2s@12` |
| `_code_00148df0` | `_IDirect3DDevice8_Begin@8` | `_D3DDevice_Begin@4` |
| `_code_00148e00` | `_IDirect3DDevice8_End@4` | `_D3DDevice_End@0` |

Direct hardened comparison against the naturally emitted Motion Sensor donor
proves exact padded bytes and relocation identity for all four. The corrected
stdcall decorations also explain the target's `ret 4`; there is no impossible
cdecl ABI. The adjacent 432-, 80-, 544-, and 96-byte sections independently
match the XDK render-state and texture-stage-state wrapper family, confirming
the source provenance and order.

The naked handwritten copies and their four `asm-implemented` park entries
were removed. Active Camouflage now leaves these target wrappers unwritten
until the real typed Direct3D callers naturally instantiate them. No artificial
emission anchor or stand-alone replacement is allowed.

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

## Historical options considered

These are the same shape as the existing `matrix4x3_multiply` **vendored-asm
park**, which the project already treats as *not* a real match. The consistent
options are:

1. **Park them** — move to `config/parked.json` with class `asm-implemented`, so
   they stop counting toward the exact tally, exactly as `matrix4x3_multiply`
   does. This is the option that matches existing precedent.
2. **Re-reconstruct in C** — treat each as unwritten and find the real source
   shape. `stristr` and the small naked helpers look tractable. The Director
   recovery above demonstrates that honest fuzzy C is preferable to retaining
   a transcribed exact body even when missing TU context prevents admission.
3. **Leave and document** — accept them as knowingly-transcribed, but record it
   so the headline "N functions byte-exact" is not read as "N functions
   reconstructed in C".

Option 1 was the initial blanket decision. It remains the current state for
Director, `stristr`, and the two Particles owners. Object Shadows moved to
option 2 with strict-exact ordinary C; Action Converse moved to honest fuzzy C;
and the Active Camouflage placeholders were exposed as misnamed stock-XDK
owners, so their transcriptions and obsolete parks were deleted. Future moves
must preserve this distinction between exact evidence and credible source.

## Not affected

`cseries::csstrcasecmp` contains **no** asm — an earlier crude scan in this
session mis-attributed `stristr`'s asm block to it. `csstrcasecmp` is a genuine
C function and a genuine 19-byte residual (an esi/edi mirror, see
`research/reloc_identity_residuals.md`).
