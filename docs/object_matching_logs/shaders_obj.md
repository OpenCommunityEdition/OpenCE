# shaders.obj exact-match log

> **Current disposition (2026-08-11): strict-exact and admitted.** All 20
> functions, all 11 target-owned `.rdata` COMDATs, and the complete 12-byte
> `.bss` pass the hardened comparison and ownership gates. The experiment
> history below is retained so future work does not repeat the rejected
> source-shape search.

## Current-campaign revalidation (2026-08-13)

The completed but unintegrated result was transplanted onto campaign commit
`738deff8` and independently rebuilt with XDK 3911. The hardened comparator
again reported `all_equal: true` for all 20 functions, including exact internal
jump-table destinations and all external relocation identities/addends. A full
466-object `halobetacache_build` completed successfully. The complete current
tooling suite passed 173/173 tests. The ordinary `ninja progress` target could
not be regenerated in this isolated lane only because `cachebeta.exe` is not
provisioned there; no strict comparison or build depends on that executable.

The house/Berth audit was repeated: parameters remain one per line; every
no-argument parameter list uses `void` on its own line; void functions end in
explicit `return;`; typed shader access macros wrap the type-verifying getter;
and the exact BSS uses natural layout rather than alignment pragmas. There is
no assembly, volatile forcing, undefined behavior, byte patch, or flag change.

## Scope and admission standard

Target: January 14, 2002 Xbox `source/shaders/shaders.obj`, compiled with XDK
3911 CL 13.00.9254.1 and the repository flags (`/O2 /Oy- /DDEBUG /Dxbox`).
Credit requires strict `section_infos_equal` equality for all 20 functions plus
the target-owned `.rdata` and 12-byte `.bss`.  No fuzzy score, assembly, byte
patch, undefined behavior, forced inline, alignment pragma, compiler-flag
change, or relocation-destination substitution is admissible.

## Baseline

The committed skeleton had 12/20 strict-exact functions and 287/2,353
meaningful target code bytes.  The missing/non-strict set was:

- `shader_get_vertex_shader_permutation` — target 288 bytes / 18 relocs.
- `shader_is_mirror` — target 80 / 2.
- `shader_is_decal` — target 144 / 10.
- `shader_is_water_decal` — target 80 / 2.
- `shader_ignores_effect` — target 80 / 2.
- `shader_environment_texture_animation_evaluate` — target 336 / 25.
- `shader_texture_animation_evaluate` — target 736 / 35.
- `numeric_countdown_timer_update` — code bytes exact, but its BSS relocation
  destination was wrong.

The target owns no `.data`, eleven external SelectAny `.rdata` string COMDATs
(478 literal payload bytes), and one ordinary 12-byte zero-initialized `.bss`.
The original candidate BSS was only 9 bytes and ordered the hidden previous-time
static before the public on/off byte.

## Proven cross-build/salvage findings

HCEA source and PDB information are used only for names and control-flow
hypotheses; January bytes remain authoritative.  A prior disposable January
lane proved exact readable-C bodies for the four shader predicates and the
environment animation evaluator.  Those bodies were adapted here using typed
`SHADER_GET_*` accessors and January-proven layouts.  The environment evaluator
uses a 32-bit `real time_value`; the later HCEA `double` signature is not valid
for January.

## Current measurements

After rebuilding the adapted bodies:

- `shader_is_mirror`: strict exact, 80 bytes / 2 relocs.
- `shader_is_decal`: strict exact, 144 / 10.
- `shader_is_water_decal`: strict exact, 80 / 2.
- `shader_ignores_effect`: strict exact, 80 / 2.
- `shader_environment_texture_animation_evaluate`: strict exact, 336 / 25,
  normalized SHA-256
  `90887e3c1c540ade41928b8e94cee254e9e4de2731bbe7f1e13ddde79433ccb3`.

This raises the working result to 17/20 strict functions.  The first direct
permutation transcription compiled to 304 bytes / 19 relocs because it emitted
both transparent cases' second typed-access calls independently.  January is
288 / 18 and shares the second access/screen-space tail and the lit-bit tail
between type 5 and type 6.

## Active experiment ledger

1. **Direct macro-expression permutation body — rejected.** 304 / 19 versus
   target 288 / 18.  It preserved semantics but duplicated one helper call and
   did not encode January's shared tails.
2. **Next experiment:** reproduce the previously measured 288 / 18 local-view
   form, then express the two January shared tails explicitly if VC7 chooses
   the wrong block order.
3. **Texture evaluator:** recover January expression/store order from the
   target and compile the HCEA topology as a hypothesis; measure all 35
   relocation identities and x87 ordering before accepting it.
4. **BSS:** test declaration/linkage/source-order shapes in a disposable build;
   accept only a 12-byte BSS with anchors at +0/+4 and previous time at +8,
   without changing any already-exact timer code.

Measured follow-up:

- **Per-case local-view form — best retained.** It reproduces the target size
  and relocation count (288 / 18) and is byte-identical through +0xB4.  Its
  remaining difference is block placement: VC7 emits the generic shared work
  before the Chicago initial calculation, while January emits both initial
  calculations before the shared screen-space and lit tails.
- **One function-scope transparent pointer plus explicit shared labels —
  rejected.** It over-merged the two paths, compiling to 272 / 18 and changing
  the prologue and jump table.  The source-level pointer lifetime was not free;
  it changed allocation before the intended tail window.  Restored the 288 / 18
  local-view form.

### Texture evaluator experiments

- **Preinitialize U/V/R to 1.0 and conditionally overwrite — rejected.** This
  shortened the function to 704 / 35 because VC7 eliminated the three explicit
  default branches.  The target is 736 / 35.
- **Separate input, animation-result, and total locals — near match only.** This
  reproduced 736 / 35 with every relocation address, type, destination, and
  addend exact, but left ten argument-home-slot displacement bytes different.
- **Reuse each channel input in place — accepted.** Assigning the periodic
  result back into `u_input`, `v_input`, and `r_input`, then adding the U/V
  translation offsets back into those same variables, reproduced January's
  slot chains: U at `[ebp+0x28]`, V at `[ebp+0x08]`, and R at `[ebp+0x2c]`.
  This reduced the entire function to two differing bytes.
- **Null-animation defaults ordered V, U, R — accepted.** January stores the
  three 1.0 defaults in that order.  Reversing only U/V in the `else` block
  closed the last two bytes.  Final result: strict exact, 736 / 736 bytes,
  35 / 35 relocations, normalized SHA-256
  `bc0e37db95c846a77a02447a81645f721a1628ff853c41aaa3072bd3b967270b`.

The October executable has the same relocation-masked body as January.  It
also confirms that January has only the zero-angle shortcut; the later HCEA
`360.0f` shortcut and `double` time parameter are not valid for this target.

### Countdown-timer ownership

The accepted representation is one external four-byte millisecond counter
followed by one external naturally aligned state structure containing the
on/off byte and the previous game time.  It emits an exact 12-byte zeroed BSS:

```c
struct numeric_countdown_timer_state
{
	boolean on;
	long previous_game_time;
};

long numeric_countdown_timer_milliseconds = 0;
struct numeric_countdown_timer_state numeric_countdown_timer_on = { 0 };
```

All five timer functions remain strict-exact, including relocation identities;
`numeric_countdown_timer_update` resolves the previous-time field as
`_numeric_countdown_timer_on + 4`.  No `.data`, alignment pragma, or explicit
padding is needed.

## Plain-English status

All eight formerly missing/non-strict functions now reproduce the original
machine code exactly, and the complete BSS layout is exact. The final
permutation breakthrough was a readable asymmetric control-flow spelling:
the generic non-screenspace path keeps a local copy of the lit-bit test, while
the screenspace path and both Chicago paths converge on `test_lit`. VC7
cross-jumps the duplicate generic test into the shared forward tail, yielding
January's block order without assembly, volatile forcing, undefined behavior,
or a compiler flag change.

Final strict totals are **20/20 functions**, **2,528/2,528 padded `.text`
bytes**, and **126/126 relocations**. All eleven target-owned SelectAny
`.rdata` string COMDATs are exact (**478 bytes, zero relocations**), and the
ordinary 12-byte zero-initialized `.bss` is logically exact with external
anchors `_numeric_countdown_timer_milliseconds` at `+0` and
`_numeric_countdown_timer_on` at `+4`. The target splitter stores twelve zero
bytes physically while MSVC correctly emits fileless BSS; the hardened logical
comparison proves identical size, bytes, relocations, flags, and ownership.

Ordinary objdiff under-reports `shader_get_vertex_shader_permutation`,
`shader_is_decal`, and `numeric_countdown_timer_get` because csplit and VC7
spell compiler-local labels differently. The hardened semantic ledger
re-verifies complete normalized bytes and relocation destinations before
crediting those three false negatives.
