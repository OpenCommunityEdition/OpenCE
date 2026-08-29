# ai_debug.obj

**Largest object in the tree under 10% complete.** 42,992 `.text` bytes across 60
sections; 11 sections exact, 1,520 bytes, **3.5%**. Next largest under 10% is
`ai_communication.c` at 25,696, so this is 1.7x the field.

## A measurement correction worth knowing

A per-symbol survey reports this object as 67,968 bytes across 61 functions. That
is wrong. `_ai_debug_render_actor_jmptable` is **not a function** -- it is the
switch jump table, and it lives at offset `0x6000` *inside the same `.text`
section* as `_ai_debug_render_actor`. Counting both symbols double-counts 24,976
bytes.

Any progress metric over this tree must de-duplicate `.text` sections by owning
symbol (the symbol whose value is 0), not count symbols. The true figure is
42,992 bytes over 60 sections.

## The shape: one function is 58% of the object

| bytes | function | state |
|--:|---|---|
| 24,976 | `_ai_debug_render_actor` | written, **+32 bytes**, +11 relocations |
| 2,336 | `_code_00041220` | absent |
| 1,168 | `_ai_debug_update` | absent |
| 1,152 | `_code_0003a2e0` | absent |
| 1,120 | `_code_00039e80` | absent |
| 848 | `_code_00039990` | absent |
| 816 | `_ai_debug_render_path_storage` | 816 vs 512 |

`_ai_debug_render_actor` at 24,976 bytes is 58% of the whole object and is
already very close. Closing it alone would take this object from 3.5% to roughly
62%. Its jump table is 400 bytes, so the switch has **100 cases**.

## `_ai_debug_render_actor`: the residual is one frame

The bodies agree far more than the byte count suggests. **4,657 of January's
6,841 instructions match in order (68.1%)**, and the instruction counts are 6,841
against 6,843.

The entire divergence starts at instruction 2:

```
January  sub esp, 0x810     (2064)
ours     sub esp, 0x82c     (2092)
```

**Our frame is 28 bytes too large**, and that one difference accounts for most of
the apparent diff: every `[ebp-N]` local reference shifts (34% of mismatching
instructions touch `ebp`) and every relative `call`/`jmp` target moves with the
+32 bytes (another ~800 mismatches are `call`, `jmp` and `je`). Fix the frame and
a large fraction should collapse at once.

### The missing locals, named

Slot-set differencing against January locates it precisely. Every deep slot is
uniformly +28 in ours (`0x2b0`/`0x2cc`, `0x310`/`0x32c`, `0x410`/`0x42c`,
`0x510`/`0x52c`, `0x710`/`0x72c`, `0x810`/`0x82c`), so the large buffers are
placed correctly and the excess is at the *top* of the frame.

**January has five dword slots we do not have at all: `[ebp-0xf0]`, `[ebp-0xf4]`,
`[ebp-0xf8]`, `[ebp-0xfc]`, `[ebp-0x100]`.** Nineteen references. Their use:

* `[ebp-0x100]`, `[ebp-0xfc]`, `[ebp-0xf8]` take `fstp` then later `fadd` -- three
  consecutive floats, i.e. a `real_point3d` or `real_vector3d` based at `-0x100`.
* `[ebp-0xf0]` is both `fstp`/`fadd`-ed and has its address taken six times
  (`lea edx/eax/ecx, [ebp-0xf0]`), so it is the base of a struct passed to
  callees.
* `[ebp-0xf4]` is an integer, set to a literal `2` at one point.

January also packs sub-dword locals more tightly than we do: it has a `byte` at
`0x31` and `0x75` and `word`s at `0x8e`, `0x92`, `0x96`, where ours sit at `0x41`,
`0x71`, `0xca`, `0xce`, `0xd2`. Both sides use 16 sub-dword slots, so the types
are probably right and only the packing order differs -- likely a consequence of
the missing locals above rather than an independent problem.

## Suggested order of attack

1. Recover the five missing locals in `ai_debug_render_actor` and get `sub esp`
   to `0x810`. Highest leverage in the object by a wide margin.
2. Re-measure before anything else; the relative-displacement noise should drop
   sharply and reveal the real remaining differences.
3. Then the seven absent functions above, which are ordinary writing work.

## Frame recovery, step 1: `mid_point`

The middle of the three marker-drawing branches declared a fifth point:

```c
real_point3d mid_point = base_point;
...
render_debug_line(TRUE, &mid_point, &p3, actor_color);
render_debug_line(TRUE, &p2, &mid_point, actor_color);
```

`base_point` is not modified between the copy and the two uses, so passing it
directly is identical in meaning and costs one `real_point3d` less of frame.
January does exactly that -- there is no fifth point in its layout.

Result:

| | before | after | January |
|---|--:|--:|--:|
| frame (`sub esp`) | 0x82c | **0x820** | 0x810 |
| function size | 25,008 | **24,976** | 24,976 |
| relocations | 1,916 | 1,916 | 1,905 |
| instruction agreement | 68.1% | 68.4% | — |

**The size now matches exactly.** Frame is 16 bytes over and relocations are 11
over, so the function is not yet exact and is not credited; the object stays at
11/60 and 1,520 bytes.

### Next lead: a three-element string table

January keeps a `char const *` array of three entries in the frame, address
taken twice:

```
05984  mov  dword ptr [ebp - 0x114], 0    ; -> " ack "
0598e  mov  dword ptr [ebp - 0x110], 0    ; -> "u/orph"
05998  mov  dword ptr [ebp - 0x10c], 0    ; -> "i/orph"
011b7  lea  eax, [ebp - 0x114]
01264  lea  edx, [ebp - 0x114]
```

with `[ebp-0x104]` and `[ebp-0x108]` holding register spills either side of it.
Neither `u/orph` nor `i/orph` appears anywhere in our source, so that part of the
function is either unwritten or spelled differently -- our version formats those
strings with `sprintf` at lines 1391 and 1398 instead of selecting from a table.
That is the most likely home of the remaining 16 frame bytes and some of the 11
extra relocations.

## Frame recovery, step 2: the frame now matches exactly

Two further recoveries, both evidenced rather than guessed.

**`char string[88]` is `char string[72]`.** Three sibling scopes in the function
declare this buffer. Sweeping the size, only 72 puts `sub esp` on January's
`0x810` -- 72 gives +0, 80 gives +8, 96 gives +24. The buffers hold
`sprintf(string, "<unknown %d>", ...)` and `sprintf(string, ": persistent %d", ...)`,
so 72 is comfortably sufficient and the frame arithmetic identifies it uniquely.

**`DEGREES_TO_RADIANS(50)` is `DEGREES_TO_RADIANS(30)`.** Ours emitted the double
`3febecde60000000` = 0.87266 = 50 degrees where January has `3fe0c15240000000`
= 0.52360 = 30 degrees. Correcting it removed exactly that extra constant and
the corresponding missing one, confirming the diagnosis.

| | start of session | now | January |
|---|--:|--:|--:|
| frame (`sub esp`) | 0x82c | **0x810** | 0x810 |
| function size | 25,008 | **24,976** | 24,976 |
| relocation count | 1,916 | 1,916 | 1,905 |
| surplus constants | 17 | **15** | — |
| missing constants | 6 | **4** | — |

Frame and size both exact. The function is still not credited and the object
stays at 11/60, 1,520 bytes.

### Remaining: a sign convention

The relocation delta is now almost entirely float constants of the wrong sign:

* surplus `__real@bdcccccd` (-0.1f) x9, `__real@bd23d70a` (-0.04f) x3,
  `__real@be99999a` (-0.3f) x3
* missing `__real@3d23d70a` (+0.04f) x3, and one empty string literal

We write negative literals -- `point_from_line3d(&p, &v, -0.1f, &out)` and
similar -- where January uses the positive constant and arranges the negation
another way, so it never materialises the negative form. Each site needs checking
individually, but they are all the same shape and there are only fifteen.

Note on comparing this function: the csplit reference names its jump table
`_ai_debug_render_actor_jmptable` and relocates 97 self-references against
`_ai_debug_render_actor`, where a fresh compile uses internal `$L` labels. Those
differences are artefacts of how the reference was carved out of the executable
and should be filtered out when diffing relocation targets, or they swamp the
real signal.

## Frame recovery, step 3: the negative-constant sites

All fifteen surplus constants come from exactly five `point_from_line3d` calls
that pass a negative scalar -- three at `-0.1f`, one at `-0.04f`, one at `-0.3f`,
three components each.

January does not materialise a negative constant. At the `-0.04f` site it emits

```
fld   dword ptr [eax]        ; global_up3d->i
fmul  dword ptr [0.04f]
fsubr dword ptr [ebx]        ; base - up*0.04
fstp  dword ptr [ebp-0x40]
```

-- a subtraction against the positive constant. Rewriting that one site as an
explicit component-wise subtraction is a strict improvement on every axis:

| | before | after |
|---|--:|--:|
| size | 24,976 (+0) | 24,976 (+0) |
| surplus constants | 15 | **12** |
| missing constants | 4 | **1** |

**The other four sites must not be rewritten the same way.** Measured
individually:

| site | size | surplus | missing |
|---|--:|--:|--:|
| 1646 `-0.04f` | **+0** | 15 -> 12 | 4 -> **1** |
| 989 `-0.1f` | -16 | 15 -> 12 | 4 |
| 1008 `-0.1f` | -32 | 15 -> 12 | 4 |
| 1023 `-0.1f` | -16 | 15 -> 12 | 4 -> **5** |
| 2926 `-0.3f` | -32 | 15 -> 12 | 4 |

Each removes the surplus constant but shortens the function, and 1023 also loses
a relocation January has. So January keeps the call shape at those four and
avoids the negative constant some other way. The likely mechanism, and the one to
try next, is negating a *variable* rather than writing a negative literal --
`point_from_line3d(&p, &v, -offset, &out)` with `real offset = 0.1f` emits `fchs`
and no constant, which is the idiom already seen in `vehicles.c`
(`negative_k = -vehicle->object.forward.k`). That should preserve the call size
while removing the constant.

## State

`_ai_debug_render_actor`: frame `0x810` exact, size 24,976 exact, relocations
1,916 against 1,905, twelve surplus constants and one missing string literal.
Not credited. Object remains 11/60 sections, 1,520 bytes, 3.5%.

### Rejected: negating a variable

The idiom from `vehicles.c` -- bind the magnitude to a local and negate that
rather than writing a negative literal -- does **not** work here:

```c
real offset = 0.1f;
point_from_line3d(&base_point, &offset_vector, offset, &p0);
point_from_line3d(&base_point, &offset_vector, -offset, &p1);
```

MSVC constant-folds `-offset` straight back to a `-0.1f` literal, so the surplus
constant count is unchanged at 12 and the function loses 16 bytes. It works in
`vehicles.c` only because the negated value there is a *runtime* field
(`-vehicle->object.forward.k`), not a constant.

So the four remaining sites resist both rewrites tried: explicit subtraction
(removes the constant, wrong size) and variable negation (folds back, wrong
size). Whatever January does there keeps the call-shaped code size while
sourcing `+0.1f`/`+0.3f`, and it is not yet identified.

## Honest status

**The object is not complete and completing it is a substantial piece of work.**
It stands at 11/60 sections, 1,520/42,992 bytes, **3.5%** -- unchanged in credited
terms by this session's work, because the dominant function is not yet exact.

What did change is that `_ai_debug_render_actor` -- 24,976 bytes, 58% of the
object -- now has an **exact frame and an exact size**, with relocations down
from 1,916 to 1,916 against January's 1,905 and surplus constants down from 17 to
12. Four separate recoveries got it there: the surplus `mid_point`, the `[72]`
buffer size, the 30-degree constant, and the `-0.04f` subtraction site.

Remaining work, in order of leverage:

1. The four negative-constant sites in `render_actor` (see above). Twelve surplus
   constants and one missing string literal stand between it and exact.
2. Then the object still needs **49 absent functions** written, 16,208 bytes,
   led by `_code_00041220` (2,336), `_ai_debug_update` (1,168),
   `_code_0003a2e0` (1,152) and `_code_00039e80` (1,120).

Closing `render_actor` alone takes the object from 3.5% to roughly 62%, so it
remains overwhelmingly the right thing to finish first.

## Frame recovery, step 4: the negation is a shared product

The counts settle what January does. Positive constants match exactly --
`+0.1f` 30 against 30, `+0.3f` 3 against 3, `+0.04f` 3 against 3 -- and January
has **zero** negative constants against our twelve. It is not substituting one
constant for another; it never materialises the negative at all.

At `0x12d4` it emits `fsub st(2)` -- a *register* subtract against a product
still on the x87 stack. January computes `offset*0.1f` once and both adds and
subtracts it:

```
fld [eax]; fmul 0.1f; fadd [base]     ; p0 = base + off*0.1   (product kept)
...
fxch st(2); fsub st(2)                ; p1 = base - off*0.1   (same product)
```

So the source writes both members of the pair as expressions over a shared
subexpression, and MSVC commons it. Expanding **both** calls of a pair does it:

| | size | relocations | surplus | missing |
|---|--:|--:|--:|--:|
| before | 24,976 (+0) | 1,916 (+11) | 12 | 1 |
| pair A expanded (2 sites) | **24,976 (+0)** | **1,909 (+4)** | **6** | 2 |

Expanding only the negative call of a pair is what shortened the function
earlier -- it recomputes the product instead of sharing it.

### Sites B and C need a three-way fusion

Applying the same rewrite to the remaining two pairs removes the last six
surplus constants but shortens the function (B: -32, C: -48), so their shape is
different again. January's site B fuses *three* computations:

```
0119d  fld [eax];  fmul 0.1f;  fadd [ebp-0x18]      ; p1.x -- left on the stack
011a8  fld [eax+4];fmul 0.1f;  fadd [ebp-0x14]      ; p1.y -- left on the stack
011b4  fld [eax+8];fmul 0.1f;  fadd [ebp-0x10]; fstp [ebp-0x8c]   ; p1.z stored
011cf  fld [ebp-0xc]; fmul 0.1f; fld st(2); fadd st(1)            ; p3 = p1 + off*0.1
0120e  fxch st(2); fsub st(2)                                     ; p2 = p1 - off*0.1
```

`p1` itself is never fully written to memory -- two of its three components stay
in x87 registers and feed both dependent points. Our version computes `p1` with
a `point_from_line3d` call that stores it, then reloads. Matching this needs the
`p1` computation expanded together with the pair so the values flow, which is a
three-statement fusion rather than the two-statement one that worked at A.

## State

`_ai_debug_render_actor`: frame `0x810` exact, size 24,976 exact, relocations
1,909 against 1,905, six surplus constants and two missing. Not credited; the
object stays at 11/60 sections and 1,520 bytes.

### Sites B and C: rejected, three-way fusion

Expanding `p1` together with its dependent pair does not work either:

| variant | size | surplus |
|---|--:|--:|
| current (A only) | **24,976 (+0)** | 6 |
| B fused, grouped | 24,944 (-32) | 3 |
| B fused, interleaved | 24,928 (-48) | 3 |

Every expansion of B is *shorter* than January, by 32 to 48 bytes. That is the
key observation and it inverts the earlier reasoning: January's code at this site
is not a tighter fusion we are failing to reach, it is **looser** than what MSVC
produces from any expanded form we can write. Looking at the target again,
`lea eax,[ebp-0x114]` and `push eax / push 1` appear *inside* the float run at
`0x11b7` -- January is interleaving the next `render_debug_line` argument setup
with the point arithmetic, and carrying spills that a clean expansion optimises
away.

So the shape at B and C is not reachable by rewriting the arithmetic alone. The
four rewrites now measured and rejected there are: expanding only the negative
call, negating a bound variable, expanding the pair, and expanding the pair
together with `p1`.

## Where this stands

`_ai_debug_render_actor` -- 24,976 bytes, 58% of the object:

| | session start | now | January |
|---|--:|--:|--:|
| frame (`sub esp`) | 0x82c | **0x810** | 0x810 |
| size | 25,008 | **24,976** | 24,976 |
| relocations | 1,916 | 1,909 | 1,905 |
| surplus constants | 17 | **6** | 0 |
| missing constants | 6 | 2 | 0 |

Frame and size exact; six surplus constants and two missing stand between it and
a match, all of them at the two resistant sites. Not credited, so the object
remains **11/60 sections, 1,520/42,992 bytes, 3.5%**, and beyond this function it
still needs 49 absent functions written.

## Frame recovery, step 5: the shared scaled vector, written through a helper

Site B does yield, but not to raw expressions. The shape is a **shared scaled
vector** built with the existing header helpers, with one of the output points
reused as the scratch:

```c
scale_vector3d(&offset_vector, 0.1f, (real_vector3d *)&p2);
add_vectors3d((real_vector3d const *)&p1, (real_vector3d const *)&p2, (real_vector3d *)&p3);
subtract_vectors3d((real_vector3d const *)&p1, (real_vector3d const *)&p2, (real_vector3d *)&p2);
```

Two things had to be right at once, and the measurements separate them cleanly:

* **The helper calls, not raw arithmetic.** Written as expressions, MSVC commons
  and tightens the code and the function comes out 32 to 48 bytes short. Routed
  through `scale_vector3d` / `add_vectors3d` / `subtract_vectors3d` the size lands
  exactly.
* **`p2` reused as the scratch, not a new local.** A fresh `real_vector3d scaled`
  gives the same size and constants but pushes the frame to `0x81c`. Reusing `p2`
  -- which is overwritten by the subtraction on the last line anyway -- keeps
  `sub esp` on `0x810`. This is the "January reuses locals as scratch" rule from
  `vehicles.c` §10.

| | before | after |
|---|--:|--:|
| frame | 0x810 | **0x810** |
| size | 24,976 (+0) | **24,976 (+0)** |
| relocations | 1,909 (+4) | **1,906 (+1)** |
| surplus constants | 6 | **3** |

### Site C is the same shape but not the same size

Applying the identical rewrite at the `-0.3f` site removes the last three surplus
constants -- `extra` reaches **0** -- but the function comes out 16 bytes short
and relocations drop to 1,903, two *below* January. Three scratch choices were
tried (`p2`, `p0`, raw expressions) and all three land on the same 1,903 with
sizes 16, 32 and 48 short.

The relocation arithmetic is now consistent and worth stating, because it says
exactly what is left: 1,906 currently, minus the 3 surplus `-0.3f` at site C,
plus the 2 relocations January has that we are missing (`_global_up3d` and one
empty string literal, both elsewhere in the function) = **1,905**, January's
count. So site C's rewrite is correct on constants and the two missing
relocations are a separate, independent defect.

## State

`_ai_debug_render_actor`: frame `0x810` exact, size 24,976 exact, relocations
1,906 against 1,905, three surplus constants at site C and two missing
relocations elsewhere. Not credited; the object stays at 11/60 sections and
1,520/42,992 bytes.

## Both missing relocations located

### 1. The empty string: a transcription bug at line 2895

January emits two structurally identical `sprintf` blocks, each selecting between
`""` and `"not-"`:

```
04af1  mov eax, 0    ; ""        04b2d  mov eax, 0    ; ""
04af8  mov eax, 0    ; "not-"    04b34  mov eax, 0    ; "not-"
04afe  push 0        ; "melee-noanimation (%sairborne)"
04b3a  push 0        ; "melee-success (%sairborne)"
```

Ours had only one. The second was written

```c
sprintf(temporary, "melee-success (%sairborne)", actor_debug_info->field_198 ? "not-" : "not-" );
```

-- both arms of the ternary identical, which is meaningless as written and is
plainly a transcription slip from the neighbouring line 2889. Corrected to
`? "" : "not-"`.

**This fix makes the size worse, and that is the useful part.** With the bug the
function was 24,976 -- exactly January's. Corrected it is 24,992. So the dead
ternary was compiling to 16 bytes less than January's real code and was masking a
**16-byte surplus somewhere else** in the function. The size match was a
coincidence of two defects cancelling. Keeping the correct source and carrying a
visible +16 is the better state to work from.

### 2. `_global_up3d`: the third marker branch

January references `_global_up3d` 49 times, we reference it 48. Aligning the two
lists, the missing site is January's `0x1285` -- the *third* marker branch, where
it loads the global before the 0.1f work:

```
01281  fld dword ptr [ebp - 0xc]
01284  mov eax, dword ptr [_global_up3d]
01289  fmul dword ptr [__real@3dcccccd]
```

Reverting the raw-expression rewrite at that branch (keeping it only at the
first) restores the reference -- `missing` drops to 1 -- but costs three surplus
constants and takes relocations from +1 to +5. Measured alternatives for that
branch:

| third branch as | size | relocations | surplus | missing |
|---|--:|--:|--:|--:|
| raw expressions (current) | +0 | +1 | 3 | 2 |
| `point_from_line3d` calls | +0 | +5 | 6 | 1 |
| scale/add/subtract helpers | -48 | +1 | 3 | 2 |

No form yet gets the reference back without paying for it elsewhere, so the
current state is retained as the closest overall.

## State

`_ai_debug_render_actor`: frame `0x810` exact, size 24,992 against 24,976,
relocations 1,907 against 1,905, three surplus constants at site C and one
missing `_global_up3d` at the third marker branch. Not credited; the object
stays at 11/60 sections and 1,520/42,992 bytes.

## The "16-byte surplus" is not one defect

Chasing it produced a more useful answer than a fix.

First, the accounting. The section is 24,992 against January's 24,976, but the
section is body + jump table + padding. Locating the table by its run of
4-byte-spaced relocations:

| | body | table | section |
|---|--:|--:|--:|
| January | 24,576 (`0x6000`) | 388 (97 entries) | 24,976 |
| ours | 24,604 (`0x601c`) | 388 (97 entries) | 24,992 |

The table is already exactly right -- 97 entries, same size -- so the switch is
correct. The **body is 28 bytes too long**, and padding absorbs 12 of that, which
is why the section only shows +16.

Second, where those 28 bytes are: **nowhere in particular**. Taking every
relocation target that occurs exactly once in both objects as a landmark and
plotting the drift between January's address and ours:

```
J 0x00eef  drift  -12      J 0x03220  drift   +9
J 0x013fb  drift  -50      J 0x0379a  drift  +12
J 0x01c34  drift  -41      J 0x03fd0  drift  +16
J 0x02056  drift  -27      J 0x044d3  drift  +22
J 0x02d91  drift   -6      J 0x04a77  drift  +27
J 0x02eb4  drift   -1      J 0x055d2  drift  -31
J 0x0308e  drift   +5      J 0x05bf8  drift  +31
                           J 0x05c7b  drift  +28
```

The drift swings from -50 to +31 and crosses zero repeatedly before settling at
+28. Around 240 blocks differ in size, most of them cancelling. There is no
single 28-byte mistake to find -- the body is a long tail of small deviations
that happen to net out near zero.

**This reframes the remaining work on this function.** Frame, jump table and
relocation count are now essentially right, and the earlier fixes were real
because each targeted a specific, identifiable construct. What is left is not
one more such fix; it is a few dozen small shape differences spread through
2,900 lines, each worth a handful of bytes. Closing it means walking the function
region by region against the disassembly, not hunting a single defect.

The landmark-drift table above is the tool for that: it localises which regions
carry surplus and which carry deficit, so the walk can be ordered by size of
error rather than done blindly.

## Region map for `_ai_debug_render_actor`

Built by taking every relocation target occurring exactly once in both objects
and plotting where our address drifts from January's. Each row is a place where
the drift *steps*, so it localises which region of the function carries the
error. This is the work plan for finishing the function.

| January addr | drift before -> after | delta | landmark |
|---|---|--:|---|
| 0x00ac8 | 0 -> -11 | **-11** | `"turn angvel..."` |
| 0x00eef | -8 -> -12 | -4 | `_unit_get_head_position` |
| 0x00ff0 -> 0x013fb | -12 -> -50 | **-38** | the three marker branches |
| 0x01c34 | -50 -> -41 | +9 | `_ai_communication_get_player_rating` |
| 0x01e1a | -41 -> -32 | +9 | `"prop->enemy"` |
| 0x02d91 | -21 -> -6 | **+15** | `_unit_estimate_position` |
| 0x0308e | -1 -> +5 | +6 | `_encounter_get_squad` |
| 0x044d3 | +16 -> +22 | +6 | `"flee"` |
| 0x0452d | +22 -> +16 | -6 | `"m-%s a-%s"` |
| 0x04a77 | +19 -> +27 | +8 | `"vehicle-success"` |
| 0x055d2 / 0x056c2 | +26 -> -31 -> +18 | -57/+49 | `_global_zero_vector3d` -- almost certainly a landmark mis-pairing, net -8 |
| 0x05a5d | +16 -> +29 | +13 | `_tag_get_name` |

Two things this settles.

**The marker branches are not the problem.** Every form tried there -- raw
expressions, original `point_from_line3d` calls, and the scale/add/subtract
helpers, in each combination across the first and third branch -- produces the
same total body of 24,604. They only move drift around locally (the `-50` at
`0x13fb` becomes `-23` with the original calls) without changing the total. The
region is internally wrong but self-cancelling.

**The +28 is distributed.** Positive steps at `0x2d91` (+15), `0x5a5d` (+13),
`0x1c34` and `0x1e1a` (+9 each), `0x308e`, `0x44d3`, `0x4a77` (+6 to +8) are
where our code is longer than January's, and negative steps elsewhere partly
offset them. No single region accounts for the residual.

Ordering the walk by |delta| gives the sequence to work through: the marker
region (-38, though self-cancelling), `_unit_estimate_position` (+15),
`_tag_get_name` (+13), the two +9s, then the +6s and +8s.

## Working the region map: chunk 0x1c00 -- a wrong prototype

Bucketing the body into `0x400` chunks and taking the net drift change per chunk
gives a clean ranking of where our code runs long:

| delta | January range |
|--:|---|
| **+23** | 0x01c00-0x01fff |
| +18 | 0x04800-0x04bff |
| +15 | 0x05800-0x05bff |
| +14 | 0x02c00-0x02fff |
| +12 | 0x02800-0x02bff |
| -38 | 0x01000-0x013ff (the marker branches, self-cancelling) |

Resolving the largest, the biggest single step inside it is +9 at `0x1c34`, and
the two sides differ like this:

```
January   cmp eax,-1 / je / push 0,0,1 / push eax / call
ours      cmp eax,-1 / mov [ebp-0x48],eax / je / fild [ebp-0x48]
                     / push 0,0,1 / push ecx / fstp [esp]
```

January passes the unit index as an **integer**; we spill it, convert with
`fild`, and push a **float**. The cause is in `source/ai/ai_communication.h`:

```c
real ai_communication_get_player_rating(real unit_index, ...);
```

`unit_index` is declared `real`. It is an index. Corrected to `long`.

| | before | after |
|---|--:|--:|
| body | 24,604 (+28) | **24,588 (+12)** |
| section size | 24,992 (+16) | **24,976 (+0)** |
| relocations | 1,907 | 1,907 (+2) |

The section size is exact again -- and this time it is exact with *both* the
ternary and the prototype correct, rather than two defects cancelling as before.
Checked for fallout: `ai_communication` itself is unchanged at 2/48, and
`objects`, `matrix_math` and `vehicles` are all unmoved.

Remaining body surplus is +12, spread over the chunks listed above.

## Working the region map: a second type defect

Rather than walk every chunk, searching for the *signature* of the prototype bug
-- conversion instructions we emit that January does not -- found the next one
directly:

| | January | ours |
|---|--:|--:|
| `fild` | 2 | 2 |
| `fisub` | 1 | 1 |
| **`fistp`** | **0** | **1** |

The single surplus `fistp` is at our `0x39ad`, in the `"newtarget %d"` call:

```
January   fld [ecx+0x88] / fmul 30.0f / fisub [esi+0x61c]
          sub esp,8 / fstp qword ptr [esp]      <- pushes a double
ours      fld [ecx+0x88] / fmul 30.0f / fisub [esi+0x61c]
          fistp qword ptr [ebp-0x14] / mov edx,[ebp-0x14] / push edx   <- pushes an int
```

Our source carried two casts January does not have:

```c
"newtarget %d",
(unsigned long)(
    ...new_target_pattern_time*TICKS_PER_SECOND -
    (real)actor->control.current_fire_target_timer
)
```

The `(unsigned long)` produces the `fistp`. The `(real)` is also wrong: January
emits `fisub`, an *integer* subtract from a float, which is what C does for
`float - long` with no cast. Both removed.

Note what this means. January passes a floating-point value to `%d` -- a genuine
type bug in the original that the reconstruction had silently "fixed" with a
cast. Reproducing the original faithfully means reproducing the bug. Only 1 byte
of size, but every conversion instruction now matches exactly.

## State

`_ai_debug_render_actor`: frame `0x810` exact, section size 24,976 **exact**,
body +12, relocations 1,907 against 1,905, three surplus constants, one missing
`_global_up3d`. Not credited; object 11/60 sections, 1,520/42,992 bytes.

Two real defects found by this walk so far -- a `real` parameter that should be
`long`, and two spurious casts -- both of them type errors rather than shape
errors. Remaining chunk ranking: 0x04800 (+21), 0x05800 (+15), 0x02c00 (+14),
0x02800 (+12); spot checks in the first of those show scheduling and
register-allocation differences rather than further bugs.

### The unsigned-compare tell, and where the walk stops

`jbe` appears once in ours and never in January. At our `0x310b`:

```
mov al, byte ptr [esi + 0xbc]
test al, al
jbe  0x38eb            <- unsigned; January uses je for a truthiness test
```

The comparable January code elsewhere reads `test cx,cx / jle`, a *signed* test.
So somewhere a `> 0` is written against a field where January tests truth
directly, or against an unsigned type where January's is signed. Two candidates
were tried -- `flee.has_approach_point > 0` at line 1899 and
`alert.move_position_order > 0` at 1905 -- and neither changes a byte, so the
site is elsewhere in the surrounding switch. Left open.

## Where this walk ends

Two genuine defects came out of it, both **type** errors:

1. `ai_communication_get_player_rating` declared `real unit_index` for an index.
   Worth 16 bytes and restored the exact section size honestly.
2. Two spurious casts at the `"newtarget %d"` call -- an `(unsigned long)` around
   the expression and a `(real)` on the timer. Worth 1 byte, but it made every
   conversion instruction match: `fild` 2/2, `fisub` 1/1, `fistp` 0/0.

The productive technique was **not** walking the drift map region by region. It
was diffing the *instruction mix* and chasing mnemonics that betray a type
mismatch -- `fistp`, `fild`, `jbe`, `movsx`. Those find real bugs cheaply. The
drift map finds shape differences, and every shape difference examined in the top
chunks (`0x4800`, `0x2c00`) turned out to be scheduling or register allocation:
the same class as the two resistant negation sites, not source-controllable.

Current: frame `0x810` exact, section size **24,976 exact**, body +12,
relocations 1,907 against 1,905, three surplus constants, one missing
`_global_up3d`, one surplus `jbe`. Object unchanged at 11/60, 3.5%.

For anyone continuing: run the instruction-mix diff first and exhaust the type
tells, then accept that the remaining size delta is distributed scheduling noise
rather than hunting it.

## Correction: the residual was not scheduling noise

The previous section concluded the remaining delta was distributed scheduling
noise and recommended against hunting it. That was wrong, and the way it was
wrong is worth recording: the drift map was being read as the primary signal,
and it is too noisy to separate a real defect from allocation churn. Ranking the
*relocation multiset* instead put two genuine defects on the table immediately.

### The fifth point is real (January 0x1140)

January's PVS branch opens by copying `base_point` into a distinct local:

```
01140  mov eax,[ebp-0x14]    mov edx,[ebp-0x18]    mov ecx,[ebp-0x10]
01149  mov [ebp-0x110],eax   mov [ebp-0x114],edx   mov [ebp-0x10c],ecx
01160  fld dword ptr [eax]        <- only then the 0.2f scale
```

`[ebp-0x114]` is later passed to `_render_debug_line` twice. The four calls form
a closed quad P->Q, Q->R, R->S, S->P, and P is that copy, not `base_point`
itself. So the branch declares five points and opens with `p4 = base_point;`,
passing `&p4` in the first and last calls. This is the local deleted earlier on
the grounds that removing it made the frame come out at 0x810; that agreement was
a coincidence between two errors, and restoring the copy is what closed the one
missing `_global_up3d` relocation.

### point_from_line3d is inlined, so a negative scalar costs three constants

Line 2937 read `point_from_line3d(&p1, &v, -0.3f, &p2);`. Because the callee
inlines component-wise, that spends three references to `__real@be99999a`, and
January references -0.3 nowhere in the function. January instead scales once and
then adds and subtracts, spilling only the z component:

```
04d48  fld [ebp-0x4c]   fmul 0.3   fst [ebp-0x20]   fadd [ebp-4]   fstp [ebp-0x10]
04d5a  fld [ebp-0xc]    fsub st(2)                                 fstp [ebp-0x84]
```

That is the same scale/add/subtract idiom already recovered at line 1011, with
`p2` reused as the scratch vector. Applying it removed all three surplus
constants without disturbing the `+0.3f` count.

**Relocations now match exactly: 1905 against 1905, with the surplus and missing
sets both empty.** Remaining: frame 0x81c against 0x810, so exactly one surplus
12-byte point elsewhere in the function, and size 25008 against 24976.

Method note for whoever continues: rank by relocation multiset first, not by the
drift map. A surplus or missing relocation is always a real source-level
difference; a drift step very often is not.

## Third defect: horizontal_angle is a minimum

The vision-cone setup at line 3188 read the peripheral vision angle against the
iterated angle as a maximum. January loads the local first and branches to the
arm that yields it:

```
05580  fld [ebp-0x1c]   fcomp [ecx+0x28]   test ah,0x41   jne 0x5595
0558d  mov ecx,[ecx+0x28]   mov [ebp-0x2c],ecx      <- A > P, take P
05595  mov edx,[ebp-0x1c]   mov [ebp-0x2c],edx      <- A <= P, take A
```

`test ah,0x41` is zero only when ST(0) > operand, so `jne` is taken on `A <= P`.
That is `A > P ? P : A`, a minimum. Ours loaded the definition field first, which
is `P > A ? P : A`. After the swap the compare, the branch and both arms match
instruction for instruction. The two forms are the same length, so this never
showed up as a size change -- it was found by reading branch polarity.

## Where it stands

Relocations match exactly, 1905 against 1905, surplus and missing both empty.
Outstanding: frame 0x81c against 0x810, and size 25008 against 24976.

The frame delta is exactly one 12-byte point. Two localisation methods were
tried and both failed, which is worth recording so they are not repeated:

* **Offset-boundary.** If one local were surplus, slots shallower than it would
  keep their offsets and deeper ones would shift by 12. Pairing offsets through
  matched landmarks shows the big buffers (0x2b0, 0x410, 0x510, 0x810) do shift
  by exactly 12, but the small locals are *permuted*, not shifted -- 0x1b0 maps
  to 0x1a4, 0x184 to 0x1b4. Adding a local made the allocator reorder, so there
  is no boundary to read.
* **Slot counting per aligned region.** Counting distinct slots between matched
  landmarks flagged January 0x5069-0x526d, 5 slots against 11. Inspection shows
  the block itself is faithful: it is the `field_F4` branch at line 3007, and
  both builds copy the source struct twice rather than copying `p4` into `p3`.
  The extra live slots are the enclosing block's `p0`/`p1`, which January retires
  before the inner block so that `p3`/`p4` reuse their slots, while ours keeps
  them live and allocates fresh ones. That is a *consequence* of the surplus
  local, not its location.

Caution for the next attempt: capstone prints small displacements without the
`0x` prefix, so a regex of `ebp - (0x[0-9a-f]+)` silently drops `[ebp-4]` and
`[ebp-8]`. Both failed methods were skewed by this before it was caught.

## Lifetime analysis: what it settled, and what it did not

### VC7 packs sibling scopes for points, but never for arrays

January's slot `ebp-0xc` holds `offset_vector.i` near 0x1281 and an unrelated
crosshair point near 0x5087; `ebp-0x18` likewise holds `base_point.x` and then a
different point. Those are different variables in disjoint sibling scopes sharing
one slot, so the allocator does pack.

The five big buffers behave the opposite way. They are referenced from disjoint
regions -- 0x2b0 near 0xd9e, 0x510 near 0x1cf0, 0x410 near 0x2317, 0x710 near
0x33c8, 0x310 near 0x5781 -- and yet their sizes *sum*: 0x60 + 0x100 + 0x100 +
0x200 + 0x100 = 0x560, and 0x560 plus the 0x2b0 of small locals is exactly the
0x810 frame. Arrays whose address escapes to `sprintf`/`csstrlen` are never
packed.

So the frame is `sum(arrays) + max over scope chains(points)`, and the 12-byte
delta lives entirely in the small-locals term: 0x2b0 in January against 0x2bc in
ours. The surplus point is on our deepest scope chain.

### The inner crosshair reuses the outer points

January's outer crosshair (lines 2991-3005) uses `ebp-0x18/-0x14/-0x10` and
`ebp-0xc/-8/-4`. Its inner crosshair at 0x5087, inside `if (field_F4)`, uses
*those same six slots*. The inner block is lexically nested inside the outer one,
so packing cannot apply -- same slots therefore means the same variables.
January's inner block has no `p3`/`p4`; it reuses `p0` and `p1`.

Rewriting it that way is size-neutral, because the allocator had already
coalesced them, but it does change our slot assignment to match January's and it
removes a +6 dword excess that the pressure profile had flagged at 0x50cb. Kept
on fidelity grounds, not on size.

### Methods that produced artifacts, with the reason

Five statistical localisations were tried; all four of the failures produced
plausible-looking but false candidates, so they are recorded here rather than
left for someone to rediscover.

* **Offset boundary.** Fails because adding a local permutes the small locals
  rather than shifting them.
* **Distinct slots per aligned region.** Flagged 0x5069-0x526d at 5 slots against
  11. Real, but it is the *consequence* of the inner-crosshair aliasing above,
  not an independent defect.
* **Slots with no preimage under vote-mapping.** Named `0xb8`, `0xc0`, `0x31c`,
  `0x71c`. All four are false: `0x31c`/`0x71c` are January's `0x310`/`0x710`
  losing a vote, and `0xc0` is one of branch 1's four legitimate points --
  branch 1 addresses exactly four locals in both builds.
* **Address-taken locals aligned by first use.** Reports several extras, but the
  alignment is exact-address matching on drift-normalised positions, which
  desynchronises after 0x4278. Its one robust output is a count: January takes
  the address of 30 locals, ours 32.

### The experiment that would settle it

Because arrays sum and points are max-over-chains, the surplus is on the deepest
point chain. Add an address-taken 12-byte dummy to one scope at a time and
rebuild: the frame grows to 0x828 only when that scope lies on the critical
chain. Probing the outermost scope is the control -- it must always grow. That
identifies the chain in a handful of builds, after which the declarations in
those scopes can be compared against January's addressed locals directly, which
is tractable once the candidate set is a few scopes rather than 96 declarations.

## The probe experiment: the surplus is in the cluster_index region

Inserting a self-contained address-taken dummy block as a statement,

```c
{ real_point3d zzprobe; zzprobe.x = 0.f; zzprobe.y = 0.f; zzprobe.z = 0.f;
  render_debug_point(TRUE, &zzprobe, 0.f, global_real_argb_red); }
```

grows the frame only when its ancestor path is the critical chain. Results:

| insertion point                          | frame  |
|------------------------------------------|--------|
| line 641, prop rendering                  | 0x81c  unchanged |
| line 2927, vehicle avoidance              | 0x81c  unchanged |
| line 3216, vision cone                    | 0x81c  unchanged |
| before `if (cluster_index==NONE)`          | **0x828** |
| branch 1, `cluster_index==NONE`            | **0x828** |
| branch 2, PVS set                          | **0x828** |
| branch 3, else                             | **0x828** |

So the chain that sizes the frame is the `cluster_index` if-chain, roughly lines
952-1040, and nothing outside it participates. That narrows the surplus from 96
declarations across the function to about fifteen in one region.

Within the region the per-branch counts already agree with January: branch 1
addresses four locals in both (`0x40 0x70 0x18c 0x1b0` in January), branch 2
four, branch 3 three. The enclosing scope declares only `offset_vector`,
`base_point` and a `short`, which is minimal.

Note the probe also settles that `p4` in branch 2 is a real source variable
rather than a compiler temp. `base_point` is address-taken at line 960 by
`unit_get_head_position`, so it already has a stack home and MSVC would pass its
address directly; the copy into `ebp-0x114` can only be a second, distinct
variable.

The remaining question is which member of that region January does not have, or
which pair of them it aliases. Every count checked so far agrees, so the next
step is to decode January's addressed locals in 0x100e-0x1382 to specific source
variables -- branch 1 is already done, `p0=0x40 p1=0x70 p2=0x18c p3=0x1b0` -- and
do the same for branch 3, where January addresses three locals for four declared
points.

## The -53/+39 pair at global_zero_vector3d is a landmark artifact

It is not a missing block. January's inner loop back-edge is `jne 0x55d0` at
0x568c, and 0x55d0 is `mov edx,[_global_zero_vector3d]` -- the load is the first
instruction *of the loop body*, re-executed every iteration. Our back-edge is
`jne 0x55d1`, and 0x55d1 is `mov ebx,[edx]`: our pointer is already in `edx` and
stays live across iterations, so our load sits above the loop.

The same relocation therefore lands ~50 bytes further along in ours than in
January purely because of which side of the loop head it falls on. The apparent
-53 and the +39 that follows are two halves of that one displacement, not two
defects.

### Why January reloads

January destroys the pointer reading the third component:

```
055dc  mov ebx,[edx]      055ff  mov edx,[edx+8]     <- edx gone
05602  mov [edi+4],ebx    05605  mov [edi+8],edx
```

Ours uses `ebx` for all three and keeps `edx` intact. January needs the extra
register because it is under more pressure: it spills *both* induction variables
to memory (`[ebp-0x58]` the byte index, `[ebp-0x30]` the countdown) and
materialises a separate element pointer (`lea ecx,[eax-8]` / `mov edi,ecx`),
where we hold the index and counter in `ecx` and `edi`.

That accounts for the residual size too. January's inner loop body is 188 bytes
against our 177: the memory-resident induction variables cost it about 13 bytes
and the element pointer about 5, offset by about 4 where `[ecx]` addressing beats
our `[eax-0x14]`.

### Rejected: the subobject pointer

The dedicated element pointer looks like the subobject-pointer idiom, so it was
tried -- hoisting `real_point3d *direction = &direction_vector[side][ring]` and
using it for the three stores and the three `point_from_line3d` calls. It moves
size the wrong way, from -16 to **-80**: MSVC then optimises the whole loop
harder, and January is *longer* than us here, not shorter. Reverted.

So the pair is explained but not closed. What remains is a register-allocation
difference in one loop, and the direction of the residual (-16, we are shorter)
says the missing bytes are spills January makes and we do not.

## The frame surplus is not a declared local

Attempting to close the frame and the -16 together. The probe identified the
critical chain, but a probe cannot say *where* along a chain the surplus sits,
only which chain it is on. So the chain was enumerated directly.

Addressed locals, January against ours, over the whole chain:

| January range | JAN | OUR |
|---------------|-----|-----|
| 0x0600-0x0a00 | 5  | 5  |
| 0x0a00-0x0d00 | 2  | 2  |
| 0x0d00-0x0f40 | 2  | 2  |
| 0x0f40-0x1382 (the if-chain) | 12 | 12 |
| **0x0600-0x1382 total** | **15** | **15** |

Individually too: branch 1 addresses four in both, branch 3 four in both, branch 2
four plus `p1`, and the enclosing scope holds `offset_vector` at `ebp-0xc` and
`base_point` at `ebp-0x18` in both. Every source-declared point on the chain is
accounted for on both sides.

So the 12 bytes are **not a declared local**. They are spill or temporary slots,
which means the frame delta is a register-allocation outcome rather than
something a declaration can be added or removed to fix.

### Three rewrites tried and rejected

All three were aimed at removing a 12-byte point from branch 2; each made things
worse and was reverted.

* `p1` as three `real` scalars -- frame **0x820**, size **+128**. The scalars
  take a slot each plus alignment, costing 16 where the point cost 12.
* `p1` eliminated, relying on CSE of `base_point + up*0.1` -- frame **0x820**,
  size **+128**, and three *surplus* `0.1` constants: MSVC recomputes the
  subexpression rather than CSE-ing it.
* subobject pointer for `direction_vector[side][ring]` -- size **-80**, the wrong
  direction, since January is longer than us there.

### What this means for the -16

The -16 has the same character. January's vision-cone inner loop spills both
induction variables to memory where we keep them in registers, which is most of
the eleven bytes by which its loop body exceeds ours. So both residuals are the
same phenomenon -- our allocator is under less pressure than January's on this
chain -- and they will close together when the remaining expression shapes are
exact, not by a targeted edit to either one.

That is a real constraint, not a hedge: the frame is set by slots the allocator
chose, so there is no declaration to delete. The productive direction is to keep
correcting expression shapes, as the `headspace_vector.j` hoist and the branch 2
`p1` change both did, and let the allocation follow.

## Writing the absent functions

The correct inventory is **46 absent sections, 15,584 bytes** (an earlier note
said 49; that was wrong). Progress this pass: 11/60 sections and 1520 bytes to
**18/60 and 1968 bytes**.

Written and byte-exact: `ai_debug_sound_point_set`, `ai_debug_lineoffire_new`,
`ai_debug_lineoffire_success`, `ai_debug_lineofsight_reset`,
`ai_debug_idle_look_clear`, `ai_debug_lineoffire_addpill`,
`ai_debug_idle_look_addprop`.

### Field names come from the object, not from guesswork

`ai_debug_lineoffire_addpill` asserts on its validity flag, and the assert string
survives in the object as `ai_debug.lineoffire_valid`, which fixes 0xF8 exactly.
`ai_debug_idle_look_addprop` gives `ai_debug.idle_look_valid` at 0x85A14 the same
way. Everything else in those two blocks follows from the code: each count is
compared against its bound before use (16 and 32), and each indexes parallel
arrays whose base offsets differ by exactly bound x element size. Where no such
evidence exists the file's existing `field_XXX` convention is used --
`field_2EC` and `field_402F0` are simply the two longs `lineofsight_reset`
clears.

`hs_globals_external.c` names `ai_debug` fields up to 0xF3 via its script-global
table, and `hs.c` carries real prototypes -- `ai_debug_sound_point_set` is
declared there as taking `void`, which is why an empty body is right.

### Asserts pin January's line numbers

An assert compiles its line number in as an immediate: 0xfc4 is line 4036 and
0x13c6 is line 5062. Our file is ~3600 lines, so these can never agree by
placement. The project already solves this -- `match_assert(file, line, expr)`
in cseries.h takes the file and line explicitly, and `MATCH_LINE` uses them
unless NON_MATCHING is defined. So any absent function containing an assert is
still writable today; it just has to carry January's literal line number.

### Environment note

The Bash tool here collapses `\` to `\` inside heredocs, including quoted ones.
That silently turned `"c:\halo\SOURCE\ai\ai_debug.c"` into a path containing
a bell character, and the only symptom was one mismatched relocation on the
file-name string while every byte of code was already identical. Write C string
literals containing backslashes with the Write/Edit tools, or build them in
Python from `chr(92)`.

## The last two lineofsight functions: what the residual actually is

`code_00037dd0` is byte-exact. `code_00037ee0` and `ai_debug_lineofsight` are
both at exactly January's size with one difference each, and both differences
are the *same* difference seen from two ends.

In `ai_debug_lineofsight` the only differing bytes are:

```
JANUARY  33f6      xor esi, esi        OURS  8bf0  mov esi, eax
         668bf0    mov si, ax
```

January zero-extends the 16-bit result into `esi`; we move the full 32 bits.
Everything after is identical, shifted by three. The same three bytes account for
`code_00037ee0`, whose entry is `push esi / mov si, ax` against our reading `si`
directly.

### Two rejected attempts, and what they establish

* **Caller local as `unsigned short`.** No effect at all. The conversion does not
  drive the extension, because the value is only ever consumed as a 16-bit
  argument.
* **Callee parameters as `unsigned short`.** Actively worse: MSVC *abandons the
  register convention entirely* and falls back to a stack frame --
  `push ebp / mov ebp,esp / movzx esi,[ebp+8]` -- taking the function from 192 to
  208 bytes.

That second result is the useful one. The custom register convention VC7 invents
for a static is **sensitive to the exact parameter type**: `short` earns it,
`unsigned short` loses it. So the convention, and with it whether the caller
zero-extends, is chosen by the compiler from the callee signature, and the
signature that produces the convention at all is the one we already have.

This is therefore not a spelling that can be reached by changing types. It is the
same class as the earlier allocation ties: correct code, one encoding choice
apart. Both functions stay NonMatching.

### Correction: they were not ties

The section above concluded both functions were an unreachable encoding choice.
That was wrong, and wrong in an instructive way: the reasoning stopped at "the
compiler picks this, not the source" without asking what *source* would make the
compiler pick it. Two ordinary spellings did.

**`ai_debug_lineofsight` has no named locals.** The call is a single nested
expression:

```c
code_00037ee0(code_00037dd0(start, start_key), code_00037dd0(end, end_key));
```

With named `short` locals MSVC keeps the full 32-bit return in `esi`, because it
can see only 16 bits are consumed. With a bare rvalue it materialises the short
properly, `xor esi,esi` then `mov si,ax` -- exactly January. That was the whole
three-byte difference, and the function went byte-exact immediately.

**`code_00037ee0` returns `long`, not `short`.** Its result is discarded at the
only call site, so nothing constrains it, and January gives it away in one byte:
the shared exit is `mov eax,esi`, a 32-bit move, where a `short` return compiles
to the 3-byte `mov ax,si`. Its sibling `code_00037dd0` really does return
`short`, and really does emit `mov ax,si` -- the two differ in January itself,
which is the tell that should have been read the first time.

All three lineofsight functions are byte-exact. Object at 25/60 sections,
3120/42992 bytes.

The general lesson, since this is the second time in this object: "the compiler
chose it" is a hypothesis about the source, not a verdict. A difference is only a
tie once a specific source question has been asked and answered.

## ai_debug_update: analysis of the first 0xd0 bytes

Not written. Recording what the head establishes so the next attempt starts here
rather than re-deriving it.

`ai_debug_update` is 1168 bytes, frame 0x6c, and saves ebx/esi/edi. It opens with
three independent guarded blocks:

1. **`ai_debug.render_lineofsight` (0xAA, already named)** gates two stores that
   zero `lineofsight_point_count` and `lineofsight_pair_count` -- byte for byte
   what `ai_debug_lineofsight_reset` does. Whether the source calls that function
   and MSVC inlines it, or writes the two stores directly, is an open question
   and worth settling by trying the call first.

2. **Three new booleans at 0x3C, 0x3D and 0x3E**, currently inside
   `__unknown3C[24]`. 0x3C gates the entire rest of the function -- `je` jumps
   past everything to 0x2d7. 0x3D and 0x3E each gate one of the blocks below.

3. **A player-surface record at 0x4C7E8**, inside `__unknown4C2F4`. When 0x3D is
   clear, the function takes the local player's unit
   (`player_control_get_unit_index(0)`), verifies it with
   `object_try_and_get_and_verify_type`, calls
   `biped_find_pathfinding_surface_index`, and on success fills:

   | offset | contents |
   |--------|----------|
   | 0x4C7E8 | `boolean` valid, set last |
   | 0x4C7EC | `real_point3d`, the out-parameter at `ebp-0x1c` |
   | 0x4C7F8 | `long` surface index, the return value |
   | 0x4C7FC | `long` unit index |

   The point is written from three dwords read out of `ebp-0x1c`, so
   `biped_find_pathfinding_surface_index(unit_index, &point)` returns the surface
   index and fills a point.

4. When 0x3E is clear, it calls `observer_get_camera(0)`, and on a non-null
   result increments a counter in `ai_profile` at 0xb34 and begins scaling
   `global_down3d` by `__real@447a0000`, which is 1000.0f -- a downward ray a
   kilometre long, so this is a screen-pick or ground-probe.

The function is written against `ai_profile` as well as `ai_debug`, so it will
need that structure available too.

### ai_debug_update, 0xd0 to 0x1f0

The camera block ends by casting a ray: `collision_test_vector(0x21, origin,
&scaled_down_vector, -1, &result)` with the result buffer at `ebp-0x6c`. On a hit
it fills a second record next to the first:

| offset | contents |
|--------|----------|
| 0x4C800 | `boolean` valid |
| 0x4C804 | `real_point3d`, read from the result at `ebp-0x54` |
| 0x4C810 | `long`, read from `ebp-0x28` of the result |
| 0x4C814 | `long`, stored as 0 |

Then, gated on the *first* record's validity at 0x4C7E8, it builds a path query:

* `path_input_new(&input, 0, unit_index, 0x3e4ccccd)` -- the constant is 0.2f
* `path_input_set_start(&input, &ai_debug.<point at 0x4C7EC>, surface_index)`
* if `ai_debug.<real at 0x40>` is non-zero, `path_input_set_search_bounds(&input, that)`

Four more scalars appear, all in `__unknown3C`: a `real` at 0x40, a `boolean` at
0x44, and `real`s at 0x48 and 0x4C. The 0x48 and 0x4C ones are each compared
against zero and defaulted when unset -- 0x4C defaults to `__real@41a00000`,
which is 20.0f. `test ah,0x44` with `jp` is the unordered/equal test, so these
read as `x!=0.f ? x : default`.

So `__unknown3C` is not padding, it is the debug pathfinding query parameters:
0x3C..0x3E gating booleans, 0x40 search bounds, 0x44 a "use player as
destination" flag, 0x48 and 0x4C two defaulted reals.

Still unwritten. The remaining ~0xe7 to 0x490 has not been read.

## The 64-byte record array: base and extent proved

Scanning every January section for `_ai_debug` relocations and reading the
displacement out of the instruction gives the owners of that region:

| function | distinct offsets |
|----------|------------------|
| `code_00041220` (2336 b) | 14, incl. 7d380 7d3c7 7d980 7d988 7d98c 7d990 7d9f4 7d9f5 7da18 7da1c 7da20 7da28 |
| `code_000383d0` (512 b) | 859f4 859f8 859f9 859fc 85a00 85a04 85a08 85a0c 85a10 |
| `ai_debug_select_encounter` (80 b) | 7d380 7d384 7d9f4 |

`code_00041220` is the writer. Its offsets settle the layout without needing the
function read: taking the record base as **0x7D9F4** with the known 0x40 stride,
its references land at +0, +1, +0x24, +0x28, +0x2c and +0x34 -- all inside one
record, and nothing outside it.

The extent then proves itself. 512 records of 0x40 from 0x7D9F4 ends at
**0x859F4**, and `code_000383d0`'s lowest reference is 0x859F4 exactly. Two
functions that never touch each other agree on the boundary, so the base and the
count are both right, and the byte the clearing loop writes is field +1 of each
record.

Everything above 0x859F4 up to the idle-look block at 0x85A14 belongs to
`code_000383d0`, at 4-byte spacing, so that is a small scalar block rather than
more array.

### Order to attack

`ai_debug_select_encounter` is 80 bytes and touches only 7d380, 7d384 and the
array base 7d9f4. It is the cheapest way to fix the first two fields and confirm
the base, and it unblocks `ai_debug_select_actor` and
`ai_debug_initialize_for_new_map`, which are otherwise complete -- both already
end by inlining the recovered `ai_debug_idle_look_clear`.

### select_actor: the inline hypothesis was wrong

The previous entry guessed the sixteen missing bytes were `ai_debug_idle_look_clear`
emitted as a call rather than inlined. It is inlined -- the five stores are
present at 0x42 through 0x53, and the guess was arithmetic-driven rather than
read off the build.

Two corrections. First, the real difference is **six bytes, not sixteen**: January
ends at 0x63 and ours at 0x5d, and 112 against 96 is only section alignment
rounding those up. Reading a size delta without checking where the code actually
ends invents ten bytes that were never there.

Second, all six are the clearing loop:

```
JANUARY  mov ecx,0x200 / jmp / lea ecx,[ecx] / mov [eax],dl / add eax,0x40 / dec ecx / jne
OURS                                          mov [eax],cl / add eax,0x40 / cmp eax,0x859f5 / jl
```

January keeps a separate downcounting counter and pays for a rotated,
0x40-aligned loop head; MSVC strength-reduced ours into a pointer compared
against the end address. So the question is what source form keeps the count
live, which is a source question and not yet answered -- the plain
`for (index = 0; index < NUMBER_OF_AI_DEBUG_ACTOR_RECORDS; index++)` we have does
not.

## ai_debug_render is the dispatch table, and it names thirteen statics

`ai_debug_render` (336 b) cannot be written as a pair with anything: it calls
thirteen absent statics, roughly 9,000 bytes of the 13,552 still missing. But
every call is guarded by an already-named `ai_debug` flag, so reading it assigns
a purpose to each anonymous function:

| gate | offset | static |
|------|--------|--------|
| `select_this_actor` | 0x01 | `code_00039e10` (112) |
| `render_lineoffire` | 0xA9 | `code_00037af0` (208) |
| `render_lineofsight` | 0xAA | `code_00037fa0` (304) |
| `render_ballistic_lineoffire` | 0xAB | `code_00037bc0` (272) |
| `selected_squad_index != NONE` | 0x34 | `code_00041220` (2336) |
| debug path query enable | 0x3C | `code_0003af00` (144) |
| `render_paths_failed` | 0xDB | `code_00041120` (176) |
| `render_aiming_validity` | 0xC8 | `code_000383d0` (512) |
| `render_all_actors` (passing `render_inactive_actors` in dl) | 0xA6/0xA7 | `code_000411d0` (80) |
| `render_speech \|\| 0x93 \|\| render_dialogue_variants` | 0xC9/0x93/0xD3 | `code_00039e80` (1120) |
| `render_idle_look` | 0xB6 | `code_00038de0` (336) |
| `render_spatial_effects` | 0xCC | `code_00038f30` (304) |
| `render_vehicles_enterable` | 0xD1 | `code_0003a2e0` (1152), tail call |

It also opens with three things worth keeping: it early-outs on `ai_globals+1`;
it advances a 1000-tick counter at `ai_debug.last_render_id` by `inc` then
`idiv 0x3e8`, storing the remainder; and it refreshes `selected_squad_index`
from the selected actor's `+0x34` through `datum_get(actor_data, ...)`, which is
the same field `ai_debug_teleport_to` reads off a player datum.

### What this means for sequencing

The earlier suggestion to write `ai_debug_render` together with `code_000411d0`
was wrong -- it is a thirteen-way dependency, not a pair. Two orders work:

1. Write the statics bottom-up and `ai_debug_render` last. Nothing is wasted and
   every function is verified as it lands, but no section scores until its
   callees exist.
2. Write `ai_debug_render` first against stub statics. The dispatcher's own bytes
   can then be checked immediately, at the cost of thirteen NonMatching sections
   standing in the tree until they are filled.

Order 1 is consistent with how this object has been worked so far. The cheapest
real entry is `code_000411d0` at 80 bytes, whose body is already decoded: it
iterates actors with `actor_iterator_new(&iterator, !render_inactive)` and calls
`ai_debug_render_actor(index, index == selected_actor_index, NULL)` per actor.

## Unreferenced statics are not emitted (measured)

Adding `static void code_000411d0(boolean)` with a trivial body and no caller
produces **no section at all** -- checked by building and looking for the owner
symbol, not assumed. So none of the thirteen render statics can be scored before
`ai_debug_render` exists to call them, which is the same coupling that forced the
lineofsight trio to be written as one unit.

### Prerequisites for the dispatcher-first order

`ai_debug_render` needs three externals that this tree does not yet declare, all
of them visible in its first 0x40 bytes:

* **`ai_globals`** -- a pointer; the function early-outs on a boolean at `+1`.
* **`global_ai_debug_string_position`** -- a `short` global, written from
  `rasterizer_globals->+0x10` minus 0x14.
* the **`rasterizer_globals` instance** -- `rasterizer.h` defines
  `struct rasterizer_globals_definition` but the pointer itself is undeclared here.

It also needs two more `ai_debug` fields named -- the gate at 0x3C, which is the
same one `ai_debug_update` uses, and the third speech-related flag at 0x93 -- and
the `actor_datum` field at `+0x34`, which is where `selected_squad_index` is
refreshed from and is the same offset `ai_debug_teleport_to` reads off a player
datum.

So the dispatcher-first order is: declare those three externals, name the two
flags and the actor field, add thirteen stub statics, then write
`ai_debug_render` against them. The dispatcher's own 336 bytes become checkable
immediately; each stub becomes checkable as it is filled.

## The two change_selected functions, and where their types came from

Both byte-exact on the first compiled attempt. Object **31/60, 4000 bytes ->
33/60, 4640 bytes**. Neither needed a rewrite of shape; both needed types that
were already provable from objects in the tree.

### `ai_debug_change_selected_encounter` (336 b)

It does **not** use an actor iterator at all -- it steps the encounter data array
with `data_next_index` / `data_prev_index`, which already exist. The previous
plan had it blocked behind the iterator work; that was wrong.

Everything it needs is confirmed by two independent offsets landing on named
fields of `struct encounter_datum` as the tree already declares it:

* `[edi+0x0d]` selects `"active"` / `"inactive"` -- that is `active`, at 0x0d.
* `[edi+0x2a]` is printed as `(%d actors)` -- that is `current_count`, at 0x2a.

The scenario side is equally pinned: element size `0xb0` matches
`sizeof(struct encounter_definition)` exactly (0x80 of scalars plus four
12-byte `tag_block`s), the block is at scenario `+0x42c` which is
`ai_encounters`, `[esi+0x7e]` is `runtime_structure_bsp_reference_index`, and
`[esi+0x20]` is `flags`.

Two flag bits are named, and only two. Bit 5 gates a literal `"3d-positions"`
and bit 6 selects `"manual"` over `"auto"` for a bsp index, so
`_encounter_3d_firing_positions_bit` and
`_encounter_manual_structure_bsp_index_bit` are read directly off the strings.
The other six bits of that word are not evidenced by this object and are left
unnamed rather than invented.

**The only thing that took a second attempt was branch polarity.** Written as
`if (encounter) { ... } else { "no more encounters" }` the body was byte-identical
from offset 0 to 0x51 and then inverted, 352 bytes against 336. January puts the
failure case in the fallthrough, so the source is
`if (!encounter) { fail } else { report }`. Flipping it closed the function
exactly -- the 16 bytes were the duplicated epilogue on the longer arm.

### The iterator struct came out of `encounters.obj`, not out of guesswork

`encounter_actor_iterator_new`, `_next` and `_prev` are undefined externals in
`ai_debug.obj`, but they are **defined** in `build/split/source/ai/encounters.obj`
(80, 64 and 128 bytes). Dumping them settles the whole interface with no
inference at all:

```c
struct encounter_actor_iterator
{
	long encounter_index;		// +0
	long actor_index;			// +4
	long next_actor_index;		// +8
};
```

* `new` stores the index at +0, writes `NONE` to +4, then seeds +8 from
  `encounter_get(index)->first_actor_index` -- offset `0x14`, which is exactly
  where `first_actor_index` sits in the tree's `struct encounter_datum`. When the
  index is `NONE` it seeds +8 from `ai_globals+8` instead.
* `next` copies +8 into +4, returns `NULL` if that is `NONE`, otherwise returns
  `datum_get(actor_data, index)` and refreshes +8 from the actor's `+0x2c`.
* Both `next` and `prev` return the **actor pointer**, not a boolean: the failure
  paths `xor eax,eax` and the success path leaves the `datum_get` result in `eax`.

So all three are declared returning `struct actor *`. Worth stating as method:
when a callee is missing, check whether some other split object defines it before
reasoning about its signature from call sites. Here it converted three open
questions into three readings.

Their bodies are now fully decoded and `encounters.obj` stands at 1.4%, so
writing the three of them there is 272 bytes of easy progress -- left alone
because it is a different object.

### `ai_debug_change_selected_actor` (304 b)

With the iterator declared this is a direct transcription, and it was exact
first try. Two details are worth keeping:

* The counter is a `short`. It is held in `esi` and used as `movsx edx, si` /
  `inc edx`, so `console_printf(..., actor_number+1, ...)`. A `long` would not
  sign-extend.
* The search loop is a plain rotated `while`: January calls `next` once before
  the loop head and again at the bottom, with `inc esi` between the
  `actor_index` test and the second call. That is what
  `while (next(&iterator)) { if (match) break; n++; }` compiles to -- no special
  form needed.

Both `console_printf` calls in the failure arms push a register that MSVC knows
holds zero rather than an immediate `0`; that is a codegen consequence of
`FALSE`, not a source difference.

## The render dispatcher: prerequisites all confirmed, stubs do not work

`ai_debug_render` was written dispatcher-first against thirteen empty statics.
The result settles two things at once.

### The three externals and both flag blocks are right

The first `0x4b` bytes of `ai_debug_render` are **byte-identical to January**,
which validates every prerequisite in one build:

* `ai_globals->initialized_for_new_map` -- `ai_globals` is a pointer to a
  `0x8DC` game-state block (`ai_initialize` mallocs exactly that). `+0` is
  `ai_active`, named by `ai_globals_ai_active` in `ai.obj`; `+1` is written only
  by `ai.c` and cleared by `ai_dispose_from_old_map` after everything is
  disposed, which is what names it.
* `global_ai_debug_string_position = rasterizer_globals.frame_bounds.y1 - 20`.
  The field at `rasterizer_globals+0x0C` is a `rectangle2d`, proved by `main.obj`
  computing `[0x10]-[0x0c]` and `[0x12]-[0x0e]` and using those to subdivide a
  screen -- `y1-y0` and `x1-x0` on the tree's `{y0,x0,y1,x1}` layout. The
  `mov ecx, dword ptr [...+0x10]` for a `short` field is MSVC widening a 16-bit
  load whose result is truncated back to 16 bits anyway; it is not evidence of a
  `long`.
* `global_ai_debug_string_position` is an **undefined external** in
  `ai_debug.obj`, as are `ai_globals` and `rasterizer_globals`, so only the
  declaration belongs here.
* `ai_debug.selected_squad_index = actor_get(...)->meta.encounter_index`. The
  actor field at `+0x34` needed no new name: `struct actor_datum` puts `meta` at
  `0x04` (the `short identifier` is followed by 2 bytes of padding because
  `actor_meta_data` is 4-aligned), and `meta.encounter_index` is at `0x30`
  within it. The same shift independently explains the iterators' `actor+0x2c`
  as `meta.next_actor_index`.

### The `ai_debug` flag names came from the script global table

`hs_globals_external.c` names every byte of both unknown blocks, so no guessing
was needed and none was done:

* `__unknown3C[24]` is the debug pathfinding query: `path`, `path_start_freeze`,
  `path_end_freeze`, `path_flood`, `path_maximum_radius`, `path_attractor`,
  `path_attractor_radius`, `path_attractor_weight`, `path_accept_radius`. The
  declared fields sum to exactly 24 bytes with natural alignment.
* `__unknown85[32]` is the print-flag block, `communication_random_disabled`
  through `print_uncovering`, exactly 32 bytes. The dispatcher's third
  speech-related gate at `0x93` is `print_speech`.
* The gate at `0xA5` that the earlier dispatch table missed is just
  `ai_debug.render`, already named.

Four `offsetof` asserts now pin `path` at `0x3C`, `print_speech` at `0x93`,
`render` at `0xA5` and `last_render_id` at `0xF4`.

### Empty stubs are inlined away -- measured

The earlier note said unreferenced statics are not emitted. The stronger fact is
that a **referenced empty static is inlined and eliminated**: with thirteen
`{ return; }` bodies the dispatcher came out 128 bytes against 336 and *every*
`call` relocation was gone, including the `code_00041220` gate whose condition
was folded away with it.

So the dispatcher-first order does not actually work under `/O2`. Each static
has to carry a real body before `ai_debug_render` can be scored, which is the
log's "order 1" after all. The dispatcher is written and its head is verified;
it will close as the statics land.

## `code_000411d0` and `code_00041120`, and the actor iterator

Both byte-exact. Object **33/60, 4640 bytes -> 35/60, 4896 bytes**.

`struct actor_iterator` came out of `encounters.obj` the same way the encounter
iterator did -- both `actor_iterator_new` and `actor_iterator_next` are defined
there:

```c
struct actor_iterator
{
	struct data_iterator encounter_iterator;	// +0x00, data_iterator_new(.., encounter_data)
	boolean encounterless_actors_done;			// +0x10
	boolean active_only;						// +0x11, the parameter
	word pad;
	long actor_index;							// +0x14
	long next_actor_index;						// +0x18
};
```

`0x1C` total, matching `code_000411d0`'s `sub esp, 0x1c`, and `+0x14` is the
`[ebp-8]` the caller reads. `next` walks encounters through the data iterator,
skipping inactive ones when `active_only`, then falls through to the
encounterless list at `ai_globals+8` once.

### Two source questions answered, both previously parked

`code_00041120` reached January's exact size immediately but differed in two
places, and both had been recorded elsewhere in this log as compiler choices.
Neither was.

**The loop counter is `short`, not `long`.** With `long index` MSVC strength-
reduces the whole loop to a byte offset compared against the end constant
(`cmp edi, 0x394f80 / jl`). With `short index` it keeps a separate downcounter
in `ebx` exactly as January does (`mov ebx, 0x20 ... dec ebx / jne`), and the
entire rest of the function fell into place with that one change. This is the
same shape flagged as unanswered for `ai_debug_select_actor`'s clearing loop --
"what source form keeps the count live" -- and the answer is the width of the
index variable.

**The stack base is `point_from_line3d(..., 1.f, ...)`, not `add_vectors3d`.**
January loads the vector component first:

```
mov  eax, [_global_up3d]
fld  dword ptr [eax]            ; up.i
fadd dword ptr [esi + 0x28]     ; + start_point.x
```

Every spelling of an addition produces the reverse for the *first* component
only -- `add_vectors3d(global_up3d, ..)`, explicit per-component assignments,
and the nested-call form all give `fld [esi+0x28] / fadd [eax]`, because MSVC
hides the latency of the freshly loaded pointer. Three attempts at rewording the
addition changed nothing, which is the tell that the operation itself is wrong.
`point_from_line3d` computes `(v->i*t) + p->x`, so the *vector* term is loaded
first, and MSVC folds the `*1.0f` away entirely. Same instruction count, correct
operand order, function exact.

Worth generalising: when several spellings of an expression all produce the same
wrong code, stop rewording it and ask whether a different helper is being
called.

## `code_00037af0`: the operand order of a two-global add

Byte-exact. Object **35/60, 4896 -> 36/60, 5104 bytes**.

Every field it touches was already named: `lineoffire_valid` (`0xF8`),
`lineoffire_success` (`0xF9`), `lineoffire_start` (`0xFC`), `lineoffire_end`
(`0x108`), `lineoffire_pill_count` (`0x114`) and the three parallel pill arrays
at `0x118`, `0x128`, `0x1E8`, `0x2A8`. The `lea edx, [edi + 0xc0]` inside the
pill loop is `pill_end[index]` reached from `pill_start[index]`, which confirms
both array bases independently: `0x128 + 16*12 = 0x1E8`.

The only thing that took a second attempt was the sum
`lineoffire_start + lineoffire_end`. January emits
`fld [lineoffire_start] / fadd [lineoffire_end]`;
`add_vectors3d(&lineoffire_start, &lineoffire_end, &out)` emits the reverse for
**all three** components. Swapping the two source arguments gives January
exactly.

So MSVC's operand order for `a->i + b->i` where both are absolute globals is
`fld b; fadd a` -- the second argument is loaded first. That is the opposite of
the `code_00041120` case, where the deciding factor was the multiply inside
`point_from_line3d` rather than the argument order. Both are now measured; the
rule is that the emitted `fld` operand has to be read off the target and matched,
not reasoned about from the source order.

Note also that `lineoffire_end` and `lineoffire_pill_end` are **vectors**, not
points: `render_debug_pill(immediate, base, height, width, color)` takes a
`real_vector3d const *height`, and the line is drawn from `lineoffire_start` to
`start+end`. The names are the tree's existing ones and were left alone.

## `code_00037fa0`: exact size, one strength-reduction anchor apart

304 bytes against January's 304, with **21 differing bytes in one 43-byte
window** (`0xDD`-`0x107`). Everything else -- the thirteen-entry local colour
table, both loops, both `MIN`s, every relocation -- is byte-identical.

### What it is

A local `real_argb_color const **colors[13]`, thirteen stores of the *addresses
of the colour pointer globals* (`&global_real_argb_black` ... `&global_real_argb_white`),
which is why each use is a double indirection: `mov ecx,[ebp+eax*4-0x34]` then
`mov edx,[ecx]`. Thirteen pointers is exactly the `0x34` frame. Then a pass over
`lineofsight_point` labelling each point with its reference count, and a pass
over `lineofsight_pair` drawing a line per pair, both colouring by
`MIN(reference_count, 12)`. `MIN` in `cseries.h` is `((a)>(b)?(b):(a))`, which
is precisely January's `cmp / jle / mov 12`.

The pair stride confirms the tree's `struct ai_debug_lineofsight_pair` at 6
bytes, and `lineofsight_point` at `0x2F0` with 16384 elements places
`lineofsight_point_reference_count` at `0x302F0` and `lineofsight_pair_count` at
`0x402F0`, all of which the code addresses directly.

### The two forms pull against each other

The residual is a single question with two half-answers, and it is worth stating
precisely rather than calling it a tie:

* Written as `MIN(pair->reference_count, 12)` **inside the call**, MSVC anchors
  the strength-reduced pair pointer at `&pair->reference_count` (`0x402F8`),
  exactly like January -- but compares in **16 bits** (`mov ax` / `cmp ax`),
  which costs a second arm and a `jmp`, +16 bytes.
* Assigned to a **`long` local** first, the compare is 32-bit (`movsx eax` /
  `cmp eax`) and the size is exact -- but the anchor moves to
  `&pair->end_index` (`0x402F6`), so all three displacements shift.

`12L` was tried as a diagnostic and changes nothing: the narrowing is not driven
by the literal's type, it is driven by whether the value passes through a 32-bit
store. So January's source has the value in something 32-bit *and* keeps the
pointer anchored on `reference_count`, and no arrangement of a `long` local
found so far does both. Five were measured: local initialised in the declaration,
local assigned as a statement, local hoisted to function scope, no `pair` local
with the array subscripted three times, and the direct expression.

The same 32-bit-store requirement is what fixed the *first* loop, where the
value additionally has to be reloaded rather than held across the `sprintf`
call -- declaring `long reference_count;` uninitialised and assigning it after
the `sprintf` gives January's two independent `movsx` loads and made that whole
loop exact.

Left NonMatching. The specific open question is: what source form gives a
32-bit `reference_count` while leaving the pair pointer anchored on the
`reference_count` member?

## The 0x4C2F4 block, decoded

`__unknown4C2F4[200844]` is now structure rather than filler. Object
**36/60, 5104 bytes -> 38/60, 5520**; `code_00037bc0` (272) and `code_0003af00`
(144) are both byte-exact, each on the first compile.

Only **three** functions in the whole object reference the block --
`ai_debug_update`, `code_00037bc0` and `code_0003af00` -- so it is derivable
from them alone. Scanning every `.text` section's `_ai_debug` relocations and
decoding the displacement gives 18, 11 and 10 distinct offsets respectively, and
those 39 offsets fix the whole layout.

### The boundary that proves itself

`0x60904 + sizeof(struct path_debug_storage)` = `0x60904 + 0x1CA7C` = **`0x7D380`**,
which is `field_7D380`, the next already-named field. So the block *ends* with
one whole `path_debug_storage`. Two independent things confirm the base:

* `code_0003af00` passes `0x6092C` to `ai_debug_drawstack_setup` and `0x60904` to
  `ai_debug_render_path_storage` -- and `0x60904 + 0x28` is
  `path_state.input.start_point`, exactly the pair `code_00041120` uses on the
  same type.
* `ai_debug_update` writes `0x60904 = NONE`, `0x60908 = game_time_get()` and
  `0x60910 = TRUE`, which are `actor_index`, `path_time` and `valid` at +0, +4
  and +0xC.

### The ballistic block mirrors the lineoffire block

`0x4C2F4` is the ballistic line-of-fire record, field for field the same shape as
`lineoffire_*` at `0xF8`, minus the `pill_hit` array:

| offset | field |
|---|---|
| 0x4C2F4 | `ballistic_lineoffire_valid` |
| 0x4C2F5 | `ballistic_lineoffire_success` |
| 0x4C2F8 | `ballistic_lineoffire_start` (`render_debug_point`, 0.1f) |
| 0x4C304 | `ballistic_lineoffire_vector` (`render_debug_vector`, 1.0f) |
| 0x4C310 | `ballistic_lineoffire_pill_count` |
| 0x4C314 | `pill_start[16]` |
| 0x4C3D4 | `pill_end[16]` |
| 0x4C494 | `pill_radius[16]` |
| 0x4C4D4 | `ballistic_lineoffire_point_count` |
| 0x4C4D8 | `ballistic_lineoffire_point[]` |

The three pill spacings check independently -- `0xC0`, `0xC0`, `0x40` for 16
elements of 12, 12 and 4 bytes -- and reuse `MAXIMUM_AI_DEBUG_LINEOFFIRE_PILLS`.

The trailing array is a **polyline**: the second loop runs to `count-1` and calls
`render_debug_line(&point[index], &point[index+1], ...)`, which is why the two
relocations are 12 bytes apart. It colours the final segment orange
(`index == count-2`) unless the trace succeeded, in which case every segment is
green. Its bound is **not** provable -- nothing references the end of the array --
so it is declared `[64]` with the leftover 16 bytes left explicitly unknown and
a comment saying so.

Both loops use a `short` index (`movsx eax, si`), consistent with the rule
recovered in `code_00041120`.

### `0x4C81C` is a whole `struct path_state`

The single most useful line in `ai_debug_update` is

```
0220  push 0x60904 / push 0x4c81c / push &input / call _path_state_new
```

which types `0x4C81C` outright. Everything the earlier pass had recorded as
anonymous scalars then falls out of `path.h`:

* `0x4C868` = `+0x4C` = `path_state.destination_valid`
* `0x4C89C` = `+0x80` = `path_state.node_count`, and the bare `cmp ax, 0x400`
  in `code_0003af00` is `>= PATH_NODE_LIST_SIZE`, which `path.h` defines as
  `0x400`.

So the colour ladder in `code_0003af00` is not arbitrary: blue when no
destination is set, green when the search found nothing, yellow when the node
list is **full**, pink otherwise.

`path_state_destination(&path_state, &path_end_point, 0x4C810, ai_debug.path_accept_radius)`
then names `0x4C810` as `path_end_surface_index`, and confirms `0x50` as
`path_accept_radius` from the hs table.

### What is named and what is not

Named only where a *second* use proves it. `path_start_surface_index` and
`path_start_unit_index` are read back by `path_input_set_start` and
`path_input_new`; the `0x3D`/`0x3E` freeze flags gate the two records, which is
what makes them start and end rather than a guess. Fields written but never read
in this object keep `field_XXXXX`: `field_4C814` (stored 0), `field_4C818` (set
TRUE at the end of the update), and `field_608A8`, which is the boolean
out-parameter of `path_state_build_path` and which `code_0003af00` uses to
suppress the start-to-end line.

### Caveat on the 16,384-byte gap

Declaring `struct path_state` at `0x4C81C` leaves exactly `0x4000` bytes before
`field_608A8`. The `offsetof` asserts pin the block's *total*, and the bytes of
both functions match, so nothing here is wrong -- but `path.h` models
`path_debug_storage` with a trailing `__unknown[51676]`, so the split between
`sizeof(path_state)` and that filler is approximate and the `0x4000` may belong
inside `path_state` rather than after it. Fields before `node_count` at `+0x80`
are unaffected, which is why the code still matches.

### Verification

Five `offsetof` asserts now pin the block -- `ballistic_lineoffire_valid` at
`0x4C2F4`, `ballistic_lineoffire_point_count` at `0x4C4D4`, `path_state` at
`0x4C81C`, `path_storage` at `0x60904` and `field_7D380` at `0x7D380` -- so any
future edit that moves the layout fails to compile rather than silently
mismatching.

## `ai_debug_update`: written, path half exact, blocked on two statics

1168 bytes against January's 1168, **239 differing bytes all past `0x2F6`**. The
whole path-query half -- `0x000`-`0x2F5`, 758 bytes, 65% of the function -- is
byte-identical. **Not credited**, and it cannot be until two callees exist.

### The blocker, stated up front

The function ends `call code_000386a0; call code_00039060`. Both are
**file-local statics** -- neither symbol appears in any other split object -- so
they must be defined in this file, and an empty definition is inlined away under
`/O2`. The relocation multiset says exactly this and nothing else:

```
MISSING: _code_000386a0 x1, _code_00039060 x1
SURPLUS: (none)
```

Every other relocation of the 95 matches -- all eight path calls, both console
strings, and every `_ai_debug` displacement. So the remaining work on this
function is `code_000386a0` (496 bytes) and `code_00039060` (288), plus the
register allocation in the two editor fix-up loops.

### What the function does

Three independent sections:

1. `render_lineofsight` zeroes `lineofsight_point_count` and
   `lineofsight_pair_count` -- the two stores `ai_debug_lineofsight_reset` makes,
   written out rather than called.
2. `ai_debug.path` gates the whole debug pathfinding query: fill the start record
   from the local player's biped unless `path_start_freeze`, fill the end record
   by casting a 1000-unit `global_down3d` ray from the camera through
   `collision_test_vector` unless `path_end_freeze`, then build a `path_input`,
   run `path_state_new` / `path_state_find` / `path_state_build_path`, and stamp
   the debug storage with `NONE`, `game_time_get()` and `valid`.
3. Two one-shot editor migrations, each gated on a flag it clears when done.

The `path_flood` flag decides whether the destination is set **before or after**
`path_state_find` -- the same call appears twice with opposite guards, which is
what a flood-fill visualisation needs.

`ai_fix_defending_guard_firing_positions` shifts every squad's
`firing_position_groups` up by one from index 2 and copies `attacking` into
`attacking_guard` -- a migration for an enum that gained a member. That is
readable straight off the four-iteration copy loop and the offsets `0x54`-`0x6C`.
`ai_fix_actor_variants` walks every squad starting location and sets a `short` at
`+0x18` to `NONE`; the string "reset the actor variant in all %d starting
locations" names that field, so `struct actor_starting_location` is declared with
`actor_variant_index` and the rest unknown.

### Four source facts recovered, each worth one measurement

* **The frame is 0x6C, not 0xC0, and scoping is what does it.** January overlaps
  the `collision_result` (0x50), the `path_input` (0x48) and one 12-byte point
  slot across three disjoint scopes. Declaring `input` at the top of the
  `if (ai_debug.path)` block keeps it live across the camera ray and costs 84
  bytes of frame. Moving it into `if (path_start_valid)`, and giving the
  path-start block, the camera block and the attractor block each their own
  12-byte local, reproduces January's frame exactly. This is the *scope a dying
  local* rule paying off on a real function rather than a toy.
* **`field_4C814` is a `real`, not a `long`.** January emits the 10-byte
  `mov dword ptr [0x4c814], 0`; ours emitted the 6-byte `mov [0x4c814], edi`,
  because MSVC knew `edi` held the integer 0 and reused it. It will not do that
  for a float store. Four bytes, and the only clue was the instruction length.
* **Assignment order decides which store gets the 5-byte accumulator form.**
  In the start record January stores `path_start_surface_index` out of `eax`
  *first*, which frees `eax` for the third point component and lets that store use
  the short `A3` encoding. Writing the point first cost one byte. The end record
  is the same trick with `path_end_valid` assigned before the point.
* **The default-value ternary needs the default in the fallthrough.** Written
  `x!=0.f ? x : default` MSVC puts the value in the fallthrough and jumps to the
  default; January is the other way round. `x==0.f ? default : x` matches. Both
  arms are the same total size, so this never showed as a size delta -- it was
  found by reading branch polarity, the same way the `horizontal_angle` minimum
  was.

### What is still wrong, precisely

Everything from `0x2F6` -- the two fix-up loops. The signature difference is that
January materialises the subobject pointer once, `lea ebx, [eax+0x42c]`, and then
reads the count through it (`cmp [ebx], edi`), spilling it to `[ebp-0xC]` because
the inner copy loop clobbers `ebx`. Ours keeps the scenario pointer and computes
`[ebx+0x42c]` lazily, so it never needs the spill and allocates differently. The
loop counter widths are already right -- `short` for all three indices, `long` for
the count, confirmed by the `movsx` on each back-edge.

### Supporting declarations added

`ai_profile_globals` moved from `ai_profile.c` to `ai_profile.h` so `ai_debug.c`
can reach the counter at `0xB34`. `map_data` became a named sub-struct rather than
a flat `byte[0xEE0]` so the counter is a real `short` field instead of a punned
offset -- `sizeof` is unchanged, asserted, and `ai_profile.obj` is unmoved at
9/23. The per-map block is an array of `0x88`-byte records after a `0x110`
header, which is where `0xB34` falls, but only the one counter is named.

Eight `path_*` prototypes were added to `path.h` with argument order read off the
call sites. Every parameter is dword-sized, so the middle parameter *names* of
`path_input_new` are not pinned by the object; that is flagged in the header.

`__unknown02[16]` in `ai_debug_state` is now fully named -- the hs script global
table covers exactly those sixteen bytes, `ai_fix_defending_guard_firing_positions`
through `ai_debug_oversteer_disable`.

## `code_00039060` and `ai_debug_get_last_path`

Object **38/60, 5520 bytes -> 40/60, 5904**.

### `code_00039060` (288 b), exact on the first compile

Duplicate-path culling over `actor_path_debug_array`. For each valid *failed*
path it scans the later entries for one belonging to the same actor whose start
point is within 0.25 and whose destination agrees, and drops whichever of the two
is older. Every offset came free from the `path_state` typing done earlier:
`+0x28` is `path_state.input.start_point`, `+0x60` is
`path_state.destination_valid`, `+0x64` is `path_state.destination.point`.

One thing worth recording because it looks like evidence and is not. January
**inlines** `distance_squared3d` at the start-point test -- a run of
`fld/fsub` leaving three values on the x87 stack, cleaned by three `fstp st(0)`
-- and emits a real `call _distance_squared3d` at the destination test. That is
tempting to read as two different helpers. It is not: writing both as ordinary
calls to the one `real_math.h` `__inline` reproduces the split exactly. MSVC
declines to inline the second under the register pressure of the inner loop.
Frame `0xC` with no vector slot confirms the first one really is register-only.

Loop shape came free too: `cmp bx, 0x20` and `movsx esi, bx` on both counters is
the `short` index rule, and the outer loop carries the separate downcounter that
rule predicts.

### `ai_debug_get_last_path` (96 b): one character

This function was already in the file, already logically correct, and already the
right size -- 96 against 96, differing only from offset `0x40`. The whole
difference was the final ternary:

```c
return found_path_index!=NONE ? &actor_path_debug_array[found_path_index] : NULL;   // 96 bytes, wrong
return found_path_index==NONE ? NULL : &actor_path_debug_array[found_path_index];   // exact
```

### The rule, now measured twice

**MSVC puts a ternary's true arm in the fallthrough.** So read which arm January
falls through to and make that the true arm. `a!=b ? x : y` and `a==b ? y : x`
are the same program and different code.

Both of today's instances were the same rule seen from different directions: the
`path_attractor_weight` default in `ai_debug_update`, where January falls through
to the *default*, and this one, where it falls through to *NULL*. Neither showed
up as a size delta -- both arms are the same length in each case -- so neither was
findable by any size or relocation diagnostic. Only branch polarity finds it.

Added to `docs/house_rules.md` under Control flow.

## `code_000386a0` (496 b): decoded, not written

Recording the analysis so the next attempt starts here. This is the last blocker
on `ai_debug_update`, together with nothing else -- `code_00039060` is now done.

It is the debug speech driver, gated on `speak_valid` (`0x85B20`) with the unit
in `speak_unit_index` (`0x85B24`), all already named in the tree.

```
0x00  speak_valid and speak_unit_index != NONE
0x21  unit_get(unit_index)  (mask 3)
0x34  bail if unit->+0xB6 has bit 2 set
0x41  bail if unit->+0x338 (short) is non-zero
0x4f  if field_85B28 > 0, decrement it; bail while it is still non-zero
0x6b  range-check speak_vocalization_type against 0 and 0xD1
0x84  unit_test_speech(unit_index, 3, 0, 0, 0, &vocalization, &tag_index)
0xb1  if the result >= 2:
        csmemset(&packet, 0, 0x30)
        packet: +0 = 4 (short), +2 = vocalization, +4 = tag_index, +0xC = 0xF
        ai_communication_packet_new(&packet + 0x10)
        unit_speak(unit_index, result, &packet)
0xf8  if tag_index != NONE, tag_get_name, then strstr(name, "conditional")
        and strchr(that, '\\')+1 to trim to the leaf name; else "<none>"
0x138 console_printf(FALSE, "%s: %s", dialogue_get_vocalization_name(type, 0), name)
0x159 if field_85B21, advance to the next vocalization whose name is not
        "unused", resetting field_85B28 to 0xF, stopping at 0xD1
0x1cf "speech done", then speak_valid = FALSE
```

What it needs that the tree does not have: prototypes for `unit_test_speech`,
`unit_speak`, `ai_communication_packet_new`, `dialogue_get_vocalization_name`
and `tag_get_name`; the `unit_datum` fields at `0xB6` and `0x338`; and a 0x30-byte
communication packet whose first four fields are visible above but whose tail is
not. `ai.h`'s `struct ai_information_packet` is about 0x1C and does **not** fit,
so the packet here is a different, larger type. That is the piece to establish
first, and it should be established from another object that builds one rather
than guessed from this call site alone.

## Sweeping for flipped arms, and what it found in `render_actor`

The ternary rule recovered from `ai_debug_get_last_path` is mechanically
detectable, so rather than eyeball for more instances it was turned into a
scanner: align the two instruction streams with branch and call targets erased
so accumulated drift does not desynchronize them, then report any 1:1
substitution of opposite-polarity conditional branches.

Validated both ways before use, per §7 -- silent on three byte-exact functions,
and fires on exactly one site when `ai_debug_get_last_path`'s ternary is flipped
back. A clean object and a broken scanner look the same otherwise.

It now lives at `tools/inverted_branch_scan.py`, takes two object files rather
than needing the build harness, and is object-agnostic.

### Two hits, both real source errors

**`0x13D` -- a genuine logic inversion, not a spelling.** The unit-highlighting
block read

```c
if (!actor->meta.swarm) { ...iterate swarm->unit_indices... }
else                    { ai_debug_highlight_unit(actor->meta.unit_index, ...); }
```

January's `je 0x1a4` puts the **swarm-set** case in the fallthrough, and `0x1a4`
-- the swarm-clear arm -- highlights the single `meta.unit_index`. So ours had
the condition negated: it iterated a swarm's units for non-swarm actors and
highlighted a swarm actor's `unit_index` as if it were a lone biped. That is a
behavioural bug that happened to survive because the function is not credited.

**`0x1B8E` -- the team name ternary.** `actor->meta.team_index!=NONE ?
teams[...] : "none"` against January falling through to `"none"`. Corrected to
`==NONE ? "none" : teams[...]`, the same one-character shape as
`ai_debug_get_last_path`.

Neither changed the section size, the frame or the relocation count -- both arms
are the same length in each case -- which is exactly why nothing else had caught
them. `render_actor` stays 24,960 against 24,976 with the frame at `0x81C`.

### The sweep is exhausted

After both fixes the scanner reports **0 hits across all 53 functions of
`ai_debug.obj` that exist in our build**, and 0 across 779 shared functions in
ten other objects -- `hs` (441), `objects` (121), `players` (68), `main` (65),
`vehicles` (39), `actor_perception`, `actors`, `encounters`,
`ai_communication`, `ui_widget`. `ui_widget_event_handler_functions` at 100/100
reports 0 as the control it should.

So this residual class is closed for the reachable corpus. It is worth re-running
whenever a new function is written, which is cheap, rather than when a function
is already stuck.

## `code_000386a0` byte-exact: the packet type was already in the tree

Object **40/60, 5904 bytes -> 41/60, 6400**.

### The packet was `struct unit_speech_item`, which already existed

The previous entry said the 0x30-byte record it builds does not fit `ai.h`'s
`ai_information_packet` and should be established from an object that constructs
one rather than guessed. Doing that found it immediately, and found that nothing
needed inventing.

`action_obey.obj` builds the same record for the same three calls:

```
csmemset(&item, 0, 0x30)
[item + 0x00] = 6          (ai_debug writes 4)
[item + 0x02] = vocalization
[item + 0x04] = tag index out of unit_test_speech
ai_communication_packet_new(&item + 0x10)
unit_speak(unit_index, result, &item)
```

Two independent constructors agreeing on `0x30` and on the `+0x10` sub-object is
the boundary-trust signal. And `units.h` already declares exactly that shape:

```c
struct unit_speech_item
{
	short priority;                  // +0x00 -- 4 here, 6 in action_obey
	short vocalization_type;         // +0x02
	long sound_definition_index;     // +0x04
	short delay_time;
	short ai_notification_delay;
	short pause_time;                // +0x0C -- set to 15
	word pad;
	struct ai_information_packet ai; // +0x10, and sizeof is 0x20
};
```

`0x10 + 0x20 = 0x30`. So the field at `+0x00` is a **priority**, not a type
selector, which is why the two call sites differ.

### The compiler as an offset oracle

Two unit fields were unnamed: a byte at `+0xB6` tested against bit 2, and a short
at `+0x338`. Rather than counting struct members by hand, generate one
`char probe_N[offsetof(struct unit_datum, <member>)+1];` per member of
`_object_datum` and `_unit_datum`, compile once with the unit's real flags, and
read the symbol sizes back out of the COFF. That named both in a single build:

* `+0xB6` is `object.damage_flags`, and bit 2 is `_object_dead_bit`
* `+0x338` is `unit.speech`, whose first member is `current.priority`

So the two guards are "the unit is dead" and "the unit is already speaking". This
technique generalises to any unnamed offset in an already-declared struct and is
much faster than reading headers.

### Three residuals, three source facts

**Nothing was declared.** `unit_test_speech`, `unit_speak` and
`ai_communication_packet_new` had no prototypes anywhere, and MSVC compiled the
calls cleanly through implicit declaration -- exactly the trap §6 warns about.
The widths are readable off the target: the result is compared as `cmp si, 2`, so
it returns `short`, and `unit_speak`'s second argument is pushed straight from
`esi` with no sign extension, which only happens when that parameter is declared
`short`. With a `long` parameter MSVC emits `movsx edx, si` first. Declared in a
new `units/unit_dialogue.h` and in `ai_communication.h`.

**`name` must not be initialised at its declaration.** January coalesces
`speech_type` and `name` into `esi`, because the first is dead before the second
is born. Writing `char const *name = "<none>";` at the top overlaps their live
ranges, forces a second callee-saved register, and costs a `push edi` / `pop edi`
/ `mov edi, 0`. Assigning `"<none>"` in genuine `else` arms instead lets MSVC
merge the two stores into one block *and* reuse the register -- the dying-local
rule again, this time about an initialiser rather than a brace.

**`++separator`, not `separator+1`.** January reads the flags `inc` already set:
`inc eax / je`. Writing the condition as `separator && separator+1` produces
`inc eax / test eax,eax / je` -- MSVC will not reuse the flags of an expression
it treats as a value rather than a side effect. `separator && ++separator` does.
One byte, and it was the last one.

## `ai_debug_update` is now unblocked and clean everywhere but one region

With both callees written, `ai_debug_update` is **1168 bytes against 1168 with
all 95 relocations matching and no surplus** -- the missing-call pair is gone.
The first `0x2F6` bytes, 758 of them, are byte-exact.

The remaining 242 differing bytes are entirely the two editor fix-up loops, and
the signature is one scheduling decision at each loop's entry test:

```
January   lea ebx,[eax+0x42c]      ; &scenario->ai_encounters
          cmp dword ptr [ebx], edi ; compare the count in memory
ours      mov ebx, eax
          mov eax,[ebx+0x42c]      ; load the count into a register first
          add ebx, 0x42c
          cmp eax, edi
```

Both spill `encounters` to `[ebp-0xC]`, both re-read the count from memory on the
back edge, and the inner loop body matches. Only the order of "adjust the pointer"
against "read the count" differs at entry. Not yet resolved.

## `ai_debug_describe_actor` byte-exact

Object **41/60, 6400 bytes -> 42/60, 6832**. Size was exact at 432 on the first
compile; the only residual was the platoon `if`/`else` arm order, the same rule
as `ai_debug_get_last_path`. January falls through to the two-argument
`sprintf`, so `if (!platoon)` is the true arm.

It was worth doing early for a reason that is easy to miss: it is **not static**,
and two already-exact functions -- `ai_debug_change_selected_actor` and
`code_00041120` -- already referenced it as an undefined external and matched
anyway, because the comparator resolves relocations by symbol name. So it could
be written and credited standalone, unlike the render statics.

Four offsets were named with the probe technique rather than by reading headers:

| reference | field |
|---|---|
| `scenario + 0x204` | `object_names`, element `0x24` = `scenario_object_name` |
| `object + 0x6A` | `name_index` |
| `object_definition + 0x2C` | `model.name` -- `model` is a `tag_reference` at `0x28` |
| `encounter + 0x8C` | `platoons`, element `0xAC` = `sizeof(platoon_definition)` |

## Status after this pass

**42 of 60 functions exact, 6832 of 42992 bytes (15.9%), 346 of 398 sections.**

Twelve functions are written but not exact, and seven are absent:

| state | count | bytes |
|---|--:|--:|
| exact | 42 | 6,832 |
| written, not exact | 11 | 33,040 |
| absent | 7 | 3,120 |

Of the eleven written-but-not-exact, **seven are deliberate empty stubs** that
exist only so `ai_debug_render` has something to call -- `code_00041220` (2336),
`code_0003a2e0` (1152), `code_00039e80` (1120), `code_000383d0` (512),
`code_00038de0` (336), `code_00038f30` (304), `code_00039e10` (112). Those
5,872 bytes are unwritten work, not near-misses.

The genuine near-misses are four:

* `ai_debug_update` -- 1168/1168, all 95 relocations, first 758 bytes exact;
  residual is one scheduling decision at each fix-up loop's entry test.
* `ai_debug_render_actor` -- 24,960 against 24,976, frame `0x81C` against
  `0x810`.
* `ai_debug_render_path_storage` -- 512 against 816; needs `code_00037890`,
  which it calls three times.
* `code_00037fa0` -- 304/304, 21 bytes, the pair-pointer anchor.
* `ai_debug_render` -- 224 against 336, blocked on the seven stubs.

### Note on `code_00037890`

Decoded but not written, because it is a **static called only from
`ai_debug_render_path_storage`**, so it cannot be scored until that function
calls it. It takes two register parameters -- `ecx` a `real_point3d const *`
start and `ebx` a `real_argb_color const *` -- plus a `short` count and a
`struct path_step *` on the stack, and draws a path as a polyline of
`render_debug_line_offset` segments with a `render_debug_tick` per node. The
`0x10` stride and the `+4` offset are `struct path_step`, which `path.h` already
declares. It should be written together with `ai_debug_render_path_storage`.

## Flag provenance addendum (2026-08-07)

The unit's configured `/QIfist` is **placebo and a hazard**. Census
(`tools/flag_provenance_census.py`): the January target contains no
float→int conversion evidence at all — no `__ftol2` reference and no
inline fist site in any of the 60 functions — so the flag is unfalsifiable
here and does nothing today: under full default flags the unit measures an
identical 42 exact of 54 shared. The hazard is prospective: the census
proves the January build used no `/QIfist` anywhere (zero qword `fistpll`
sites in 467 targets), and neighbouring ai units (`actor_combat`,
`actor_perception`, `actor_looking`) carry inline-helper conversion sites
in unwritten functions. Any conversion written into this TU under the
current config would emit unmatchable `fistpll`. Recommendation: flip the
unit's cflags to default at this lane's next session (measured cost-free)
before the render functions are attempted.

## `code_00037890` byte-exact, and the path step arrays named

Object **42/60, 6832 bytes -> 43/60, 6960**. Exact on the first compile, custom
register convention included.

### The convention came free from declaration order

This static takes **two register parameters and two stack parameters**: `ecx` a
`real_point3d const *` start, `ebx` a `real_argb_color const *`, then a `short`
count at `[ebp+8]` and a `struct path_step *` at `[ebp+0xC]`. Declaring them in
that order and letting MSVC choose reproduced `ecx`/`ebx` exactly -- the same
lesson as the lineofsight trio, where the invented convention follows the
signature and cannot be dictated.

Both loop counters are `short`, read off `test si,si` and `cmp di,[ebp+8]`.

### Three call sites fix three arrays

`ai_debug_render_path_storage` calls it three times, gated on
`render_paths_raw` / `_smoothed` / `_avoided` (`0xDD`/`0xDE`/`0xDF`), which
places three `{short count; word pad; struct path_step steps[N];}` blocks inside
`path_debug_storage`'s 51,676-byte tail:

| offset | field | extent |
|---|---|---|
| 0x140FC | `raw_step_count` | steps at 0x14100, 64 entries |
| 0x14500 | `smoothed_step_count` | steps at 0x14504, 4 entries |
| 0x14544 | `avoided_step_count` | steps at 0x14548, 4 entries |

The 64/4/4 split is asymmetric and looks wrong at first, but each extent is
pinned by the *next* referenced offset, and the run ends two bytes before
`0x1458A`, which the avoidance-segment code reads as a `short`. Three
`offsetof`/`sizeof` asserts hold the layout, and the `0x1CA7C` total is
unchanged.

`struct path_result`'s `steps[4]` is **not** these blocks -- its header is
`{boolean valid; path_destination endpoint; ...}`, so `count` would be at `+0x19`,
not `+0x00`. Different type, same element.

## `ai_debug_render_path_storage`: decoded, deliberately not finished

640 bytes against 816. The remainder is a genuine data-model job rather than a
shape problem, and it is recorded here rather than half-attempted.

What is already read off the target:

* Two adjacent **6-entry string tables** in the frame at `[ebp-0x34]` and
  `[ebp-0x1C]` -- the `path_traverse_result` and `path_build_result` names, whose
  enumerations `path.h` already declares. The two `match_assert`s at January
  lines **3944** and **3945** carry their bounds and name both constants outright:
  `NUMBER_OF_PATH_TRAVERSE_RESULTS`, `NUMBER_OF_PATH_BUILD_RESULTS`.
* The header colour is green only when *both* results are `5` (`_success`), else
  red, and the label is `csprintf(temporary, "%s / %s (%d)", traverse_name,
  build_name, game_time_get()-path->path_time)`.
* `[ebp-4]` is a `boolean` holding
  `path->structure_bsp_index == global_structure_bsp_index_get()`, reused much
  later to gate `render_debug_path`.

What it still needs, all inside the same 51 KB tail:

* `0x1458A` a `short` avoidance-segment count, `0x1458C` an array of stride
  **0xC08**, and `0x175AC` an array of stride **0x1534**, indexed by
  `ai_debug.render_paths_avoidance_segment` (`0xE0`).
* Prototypes for `render_debug_obstacles` and `render_debug_path`, neither
  declared in the tree.

Those three strides are the work: each is a substantial structure, and guessing
one from a single indexed reference is exactly the failure this log has recorded
before. Establish them from an object that *builds* one, as the speech packet was
established from `action_obey.obj`.

## `code_00038f30` byte-exact: the spatial-effect ring, and the subobject rule both ways

Object **43/60, 6960 bytes -> 44/60, 7264**.

### The ring proves its own extent, twice

`ai_globals` carries a debug spatial-effect ring: a `short` head at `0x130`, a
`short` tail at `0x132`, and 20-byte records from `0x134`. The record is
`{short type; short field_02; real_point3d point; long time;}` -- `0x14`, which
is the `lea eax,[eax+eax*4]` then `*4` scale at the indexing site.

Two independent facts fix the count at 32, so it is not a guess:

* the index wraps with `inc cl / and ecx, 0x1f`;
* `0x134 + 32*0x14 = 0x3B4`, and `0x3B4` is the next `ai_globals` offset used
  anywhere -- `ai.c` writes a byte there and `actions.c` reads it.

`field_02` is left unnamed. It is printed as `"c%d"`, which suggests a count, but
one format letter is not evidence.

### The named subobject pointer, applied in both directions

This function is a clean demonstration of `house_rules` §6's symmetric rule, and
it took both halves to close.

At exact size (304/304) the residual was purely register assignment: January
keeps the *record* in `edi` and materialises `lea esi,[edi+4]` for the point,
reading `[esi]`, `[esi+4]`, `[esi+8]` through it while `edi` stays live for
`[edi]`, `[edi+2]` and `[edi+0x10]`. Ours held the record in `esi` and addressed
the point off it.

1. **Naming the pointer** -- `real_point3d *effect_point = &effect->point;` --
   produced January's `edi`/`esi` split immediately.
2. **But it then emitted the `lea` too early**, before the colour selection,
   where January emits it after. Declaring the pointer without an initialiser and
   assigning it just before its first use moved the `lea` to January's position
   and closed the function.

So the rule is genuinely two-sided: *whether* the subobject is named decides the
register split, and *where the name is born* decides the `lea` placement. This is
the same lesson as `code_000386a0`'s `name` initialiser -- an initialiser at the
declaration is a lifetime -- reaching a different instruction.

## `code_00038280` and `code_00039e10` byte-exact

Object **45/60, 7600 bytes -> 47/60, 8048**.

`code_00038280` is the screen-pick: find the first existing local player, cast a
50-unit ray along `camera->forward` with `collision_test_vector(0x81, ...)`,
ignoring the player's own unit unless the director perspective is first-person,
and resolve the hit object to an actor. `code_00039e10` is its caller -- it
describes the picked actor, prints `"selected %s"`, selects it, and clears
`select_this_actor`.

Four field names came from the offset oracle: `unit.actor_index` at `0x1A4`,
`unit.swarm_actor_index` at `0x1A8`, `unit.driver_object_index` at `0x2D4`, and
`player_datum.unit_index` at `0x34`. `observer_result.forward` at `0x20` and
`collision_result.object_index` at `0x38` fall out of the declared structures.

### Three defects, three different classes

**A missing include is a wrong prototype.** `director_get_perspective` was not
declared, so MSVC compiled an implicit `int`-returning call: `movsx ecx, si` on
the argument and `test eax, eax` on the result where January passes `si`
directly and tests `ax`. `director.h` declares it `director_perspective` --
a `short` -- taking a `short`. Adding the include fixed both instructions and
16 bytes. This is the same trap as `unit_speak`: **MSVC compiles an undeclared
call silently, and the width shows up several instructions away.**

**The two `swarm_actor_index` ternaries are spelled differently.** The first
falls through to the `actor_index` fallback, the second falls through to the
swarm value:

```c
actor_index = unit->unit.swarm_actor_index==NONE ?
	unit->unit.actor_index : unit->unit.swarm_actor_index;      // first
...
actor_index = driver->unit.swarm_actor_index!=NONE ?
	driver->unit.swarm_actor_index : driver->unit.actor_index;  // second
```

Flipping *both* to the same spelling was wrong. The fallthrough rule is per
site, and two structurally identical expressions in one function can genuinely
differ in the original source.

**`match_assert` stringizes source spacing.** With the bytes byte-for-byte
identical, the comparator still refused on one relocation: January's assert text
is `"camera != NULL"` and ours was `"camera!=NULL"`. `#expr` preserves whatever
the source wrote, so the operator spacing inside an assert is load-bearing even
in a file whose prevailing style is unspaced. `house_rules` §3 says spacing is
per-file; this shows it can be **per-expression** when an assert captures it.

## `ai_debug_speak_list` byte-exact

Object **47/60, 8048 bytes -> 48/60, 8512**.

A local, **null-terminated** table of fifteen 8-byte records --
`{char const *name; short vocalization_type; boolean all;}` -- naming the
vocalization ranges: `all` at 0, `idle` 0, `involuntary` 6, `hurting people` 21,
`being hurt` 29, `killing people` 49, `player kill comments` 80,
`friends dying` 96, `shouting` 108, `group communication` 123, `actions` 148,
`exclamations` 177, `post-combat actions` 188, `post-combat chatter` 197, then
`{NULL, NONE, FALSE}`. Fifteen times eight is `0x78`, the exact frame, which is
what confirms the terminator is a real record rather than padding.

`all` is `TRUE` only on the first entry, and it lands in `field_85B22` -- the
same flag `code_000386a0` reads to decide whether to keep advancing past
vocalizations named `"unused"`. Two functions agreeing on its meaning is better
evidence than either alone, though the field keeps its placeholder name because
`ai_debug_speak` also writes it and renaming would reach an already-exact
function.

The only residual at exact size was **block order**: January's `jne 0x180`
sends the *found* case forward and falls through to the not-found message, so
the source tests `if (!entry->name)` first. Written the natural way round --
`if (entry->name) { work } else { report }` -- everything matches except which
half sits where. This is the fallthrough rule reaching a whole basic block
rather than a ternary arm, and it is now the fourth instance in this object.

## `code_000383d0`: 512/512, five bytes short of exact

Written, **not** credited. 512 against 512 with **five differing bytes** in one
place, and the `__unknown859F4` block fully named on the way.

### The aiming-validity block names itself

`0x859F4` is a cached copy of `selected_actor_index`; when it changes, two
booleans are cleared. The block then runs to `0x85A14`, which is
`idle_look_valid` -- so the extent is fixed by the next named field:

```
0x859F4  long          field_859F4     cached selected actor
0x859F8  boolean       field_859F8
0x859F9  boolean       field_859F9
0x859FC  real_vector3d field_859FC
0x85A08  real_vector3d field_85A08
```

`4+1+1+2+12+12 = 32`, exactly the old `__unknown859F4[32]`. The two booleans
and two vectors are the out-parameters of
`actor_looking_test_validity(actor_index, &vector, &a, &b)`, which is otherwise
undeclared in the tree; the offset oracle also gave `actor.input.facing_vector`
at `0x174`.

### The subobject pointer has a scope, not just a name

Two rounds of the §6 rule again, and this one added a third dimension. Reading
`[esi+0x124]`/`[esi+0x128]` instead of January's `[edi+4]`/`[edi+8]` cost 6
bytes, so the head position wants a named pointer -- but naming it at the top of
the enclosing block made the function **32 bytes short**, because MSVC then also
used it for the two tail blocks. January pops `edi` *before* those, and they use
the long `[esi+0x120]` form.

Scoping the pointer to the `if (camera)` block gave the exact 512. So the rule
now reads: whether the subobject is named decides the register split, where the
name is born decides the `lea` placement, and **where the name dies decides how
far the addressing mode reaches**.

### The residual

Five bytes at `0x15D`: January emits `push 1` then `fstp [ebp-0x10]`, ours the
reverse -- the `TRUE` argument pushed before the point's first component is
stored rather than after. Only the *first* of the two structurally identical
tail blocks differs; the second matches. Both the named-local and the
nested-`point_from_line3d`-as-argument spellings produce the same five bytes, so
the call shape is not the lever. Left as the simpler named-local form.

## `code_00039e80`: decode started, not written

1120 bytes, frame `0x230`, still a stub. Recording what is established so the
next attempt does not re-read it.

**It iterates every unit in the world**, not the selected actor:

```c
object_iterator_new(&iterator, _object_mask_unit, 0);
while ((unit = object_iterator_next(&iterator))!=NULL) { ... }
```

`struct object_iterator` is already declared (16 bytes; the object index at `+8`
is the `[ebp-0x28]` the body reads). Per unit it takes
`unit_get_head_position(object_index, &head)`, offsets by `global_up3d*0.1f`,
and calls `ai_debug_drawstack_setup`.

**Three independent gates follow**, matching the dispatcher's condition
`render_speech || print_speech || render_dialogue_variants`. The first, at
`0xD3` (`render_dialogue_variants`), is decoded:

* `tag_get('unit', unit->definition_index)` gives the unit definition, whose
  `+0x2B4` is a `tag_block` of `0x18`-byte elements.
* It scans that block for the element whose `+0x14` equals the unit's `+0x334`,
  and takes that element's leading `short`.
* The label is `tag_name_strip_path(tag_get_name(unit-><0x334>))`, defaulting to
  the literal `"<none>"`.

Two things to know before starting:

* **`ai_debug_drawstack` is inlined here.** The next/last position swap and the
  `up * drawstack_height` add appear as straight-line code at `0x109`-`0x147`,
  where every other function in this object emits a `call`. Writing it as a call
  will not match, and the file-local-static-versus-inline distinction in §6 is
  the relevant rule.
* Two fields still need names: unit definition `+0x2B4` (the dialogue-variant
  block) and unit `+0x334`. `unit.speech` is at `0x338`, so `+0x334` is the
  field immediately before it -- run the offset oracle on `_unit_datum` rather
  than guessing.

### `code_000383d0` closed: the two tail blocks are written differently

Object **48/60, 8512 bytes -> 49/60, 9024**.

The five-byte residual was not scheduling. The two tail blocks are structurally
identical -- offset the head position by `up*0.05f`, draw a vector -- but the
original writes them **differently**:

```c
/* first block, the aiming vector */
point.x = global_up3d->i*0.05f + actor->input.position.head_position.x;
point.y = global_up3d->j*0.05f + actor->input.position.head_position.y;
point.z = global_up3d->k*0.05f + actor->input.position.head_position.z;

/* second block, the looking vector */
point_from_line3d(&actor->input.position.head_position, global_up3d, 0.05f, &point);
```

The explicit form finishes the argument list before storing `point.x`
(`push edx / push 1 / fstp`); the helper form stores first. Both spellings were
tried on *both* blocks and neither uniform choice matched -- only the mixed one
does.

This is the second time in two functions that structurally identical code turned
out to be spelled two different ways in the same function: the pair of
`swarm_actor_index` ternaries in `code_00038280` had opposite arm order, and
these two blocks use different helpers. **Normalising repeated code to one form
is an active mistake**, and a residual that survives applying the same fix to
every site is the signal for it.

### `code_00037fa0`: the residual is one constant, and ten forms do not move it

Still 304/304 with 21 differing bytes, but the difference is now fully
characterised, which it was not before. It is **a single immediate**:

```
January   mov esi, 0x402f8      ; &pair[0].reference_count  (element + 4)
ours      mov esi, 0x402f6      ; &pair[0].end_index        (element + 2)
```

Every instruction after that is identical, displaced by two and shifted one byte
because `[esi]` encodes shorter than `[esi+2]`. The three field reads are
`reference_count`, `end_index`, `start_index` in both builds, in that order.
So MSVC anchors the strength-reduced induction pointer on the **highest**
accessed field and we anchor on the middle one, and nothing about the access
order explains it.

Forms measured and rejected, all 304/304 unless noted:

| form | result |
|---|--:|
| `long` local assigned from `pair->reference_count` | 21 |
| direct `MIN(pair->reference_count, 12)` | 16-bit compare, +16 bytes |
| `long` local, no `pair` pointer, array subscripts throughout | 21 |
| `long` local hoisted to function scope | 21 |
| mixed: `pair->` for the count, subscripts for the points | 21 (CSE'd back) |
| explicit pointer induction (`pair++` in the for-clause) | 21 |
| `start_point`/`end_point` named before the count | 41 |
| `reference_count = MIN(pair->reference_count, 12)` | 320 bytes, 73 |
| `12L` bound | no effect |

The two halves still pull against each other exactly as recorded earlier: any
form that puts the value through a 32-bit store gets the compare width right and
the anchor wrong; the direct expression gets the anchor right and the width
wrong. Nothing tried gets both.

**What has not been tried** and is the obvious next move: the sibling loop.
Loop 1 is byte-exact and reads `lineofsight_point_reference_count[index]`, a
*separate array*, so it never exercises the anchor question. If the anchor is
chosen by something in the enclosing function rather than the loop -- register
pressure from the 13-entry colour table, say -- then loop 2 cannot be fixed in
isolation and the lever is elsewhere in the function.

## `ai_debug_render_actor`: three negatives worth having

No score change. The function stays **24,960 against 24,976 with the frame at
`0x81C` against `0x810`** -- 16 bytes short and 12 bytes of frame over.

### The relocation diff is entirely a csplit artifact -- do not chase it

A naive relocation-name comparison reports 109 differences: January has
`_ai_debug_render_actor_jmptable` x12 and `_ai_debug_render_actor` x97, ours has
about a hundred `$L…` internal labels. **This is the switch jump table and its
self-references, and it does not block the comparator** -- the csplit research
established that by blinding internal relocations across all 18 jump-table
residuals, and 93 exact functions in the tree contain one. Any future session
should filter internals out before reading a relocation diff on this function,
or the real signal is buried.

### All three mechanical detectors are clean on the whole object

* `inverted_branch_scan` -- 0 hits (it found and closed two real bugs here
  earlier, including the inverted `actor->meta.swarm` guard).
* `loop_index_width_scan` -- 0 downcounter differences across 58 shared functions.
* `access_width_scan` -- 0 aligned width mismatches.

So the residual is **not** a branch-polarity, index-width or access-width defect.
That is a real narrowing: those three classes account for most of what has closed
in this object, and none of them applies here.

### Rejected: reusing `p2` as the PVS branch's scratch

The frame is over by exactly one 12-byte point, and the PVS branch declares five
points where the other two declare four -- `p1` exists only to build `p3` and
`p2`. Reusing `p2` as that scratch is the idiom §5 of this log records working at
another site in the same function, so it was the obvious candidate.

It is wrong. Frame went **up** to `0x820` and size to **25,088** (+112), because
making `p2` both source and destination of the subtraction forces MSVC to
materialise a temporary of its own.

Together with the three forms the earlier session measured -- `p1` as three
`real` scalars (0x820, +128), `p1` eliminated relying on CSE (0x820, +128, and
three surplus constants), and a subobject pointer for the direction vector
(-80) -- **four** ways of removing that fifth point are now measured and all are
worse. The frame surplus is not reachable by deleting a local from this branch.

## `code_0003a2e0`: decode started, `ai_globals` extended

1152 bytes, frame `0x180`, still a stub. About 400 bytes read; recording the
structure so the next attempt starts from evidence.

### The enterable-vehicle array, extent proved

`ai_globals` carries a second array, and it proves its own extent exactly the way
the spatial-effect ring did:

```
0x3B4  boolean field_3B4                 (written by ai.c, read by actions.c)
0x3B6  short   enterable_vehicle_count
0x3B8  struct ai_enterable_vehicle[32]   stride 0x28
0x8B8  short   field_8B8                 (ai.c increments it)
```

`0x3B8 + 32*0x28 = 0x8B8`, and `0x8B8` is the next `ai_globals` offset used
anywhere -- `ai.c` does `inc word ptr [eax+0x8b8]`. Two files that do not know
about each other agreeing on the boundary is the same signal that fixed the
effect ring at 32. Asserts added for both offsets and the `0x28` record size.

Three record fields are read by this function: `object_index` at `+0`,
`distance` at `+4` (printed as `"enterable: dist %.1f"`), and a `short` team
bitmask at `+8`. The remaining 30 bytes stay unknown.

### What the function does so far

Iterates `enterable_vehicle[0 .. enterable_vehicle_count)`. Per entry it verifies
the object as a unit, takes `object_get_origin`, offsets the drawstack by
`global_up3d*0.5f`, and prints the distance. When the team bitmask is non-zero it
builds a `"teams: "` string by walking bits 0..9 against a **ten-entry local
table** -- `default, player, human, covenant, flood, sentinel, unused6..unused9`
-- appending `" "` and the name with `csstrcat` for each set bit. That is the
same team-name table `ai_debug_render_actor` carries at `[ebp-0xb0]`, which is
worth knowing: it is a local in both, not a shared static.

Roughly 750 bytes past `0x180` are unread.

## `code_0003a2e0` written: 1136 against 1152, allocation only

No longer a stub. Every structure, string, constant and call is in place; the
residual is **16 bytes and 4 bytes of frame**, entirely register allocation.

### Both name tables prove their own size

The function prints three things per enterable vehicle, and two of them are
bitmask-to-name expansions over **local** tables whose sizes are confirmed by
existing enumerations rather than counted off the disassembly:

* teams -- 10 entries, `default player human covenant flood sentinel unused6..9`,
  exactly `NUMBER_OF_SOLO_CAMPAIGN_TEAMS` in `game.h`, names matching the enum
  member for member;
* actor types -- 16 entries, `elite`..`mounted weapon`, exactly
  `NUMBER_OF_ACTOR_TYPES` in `actor_types.h`, again matching member for member.

That completes the record: `object_index` `+0`, `distance` `+4`, `team_flags`
`+8`, `actor_type_flags` `+0xA`, `actor_count` `+0xC`, `actor_index[6]` `+0x10`
-- `0x28`, and the array's 32 entries land on `0x8B8` as recorded above.

The third block calls `ai_index_to_string(ai_index, global_scenario_get(),
string, 256)`, declared here for the first time.

### The residual is the render_actor phenomenon again

January is under **more** register pressure than we are and spills accordingly:

* it keeps `ai_globals` and the vehicle pointer in the *same* register, reloading
  `[_ai_globals]` at the bottom of every outer iteration, and spills the vehicle
  pointer to `[ebp-0xC]` across the first call;
* it walks each name table through a **memory** pointer at `[ebp-8]` with the
  countdown at `[ebp-4]`, where we hold the table pointer in a register.

That is four bytes of frame and sixteen bytes of code, all in our favour, and it
is the same signature as `ai_debug_render_actor` -- *our allocator is under less
pressure than January's.* Writing the table walk as an explicit
`char const **name = table; … name++` pointer induction does **not** reproduce
it: MSVC folds it straight back to indexed addressing, same 1136 bytes.

Two functions now share this exact signature. It is worth treating as a single
open question about what raises VC7's pressure in this translation unit, rather
than as two independent scheduling puzzles.

## `code_00041220`: head decoded, not written

2336 bytes, frame `0x8EC`, still a stub and the largest unwritten function in the
object. Only the first `0x90` is read. Recording it so the next attempt starts
from evidence rather than a cold dump.

It takes the encounter index as its one stack argument and opens with:

```c
encounter_get(encounter_index);            /* result discarded -- the assert */
definition = TAG_BLOCK_GET_ELEMENT(&global_scenario_get()->ai_encounters,
    encounter_index&0xffff, struct encounter_definition);   /* kept in ebx */

encounter_actor_iterator_new(&iterator, encounter_index);
while (encounter_actor_iterator_next(&iterator))
{
    boolean selected = (ai_debug.selected_actor_index==iterator.actor_index);
    ...
    if (!selected && ai_debug.selected_actor_index!=NONE &&
        !ai_debug.render_all_actors)
        continue;
    ...
}
```

The `datum_get` at `0x16` has its result overwritten immediately, so it is there
for its own assert -- the same shape as the discarded `unit_get` in
`code_00038280`. `[ebp-0x38]` is the `encounter_actor_iterator` (its
`actor_index` read at `[ebp-0x34]`, `+4`, confirming the struct again), and
`[ebp-0x24]` is the `selected` boolean produced by `sete`.

What is already known from the relocation survey, and does not need re-deriving:
this function is the **writer** of the 64-byte `ai_debug_actor_record` array at
`0x7D9F4`, touching `+0`, `+1`, `+0x24`, `+0x28`, `+0x2C` and `+0x34` of one
record, plus `0x7D380`, `0x7D3C7` and the `0x7D980` block. The record stride and
the 512-entry count are already proved by `code_000383d0`'s lowest reference
landing on `0x859F4`.

### `code_0003a2e0`: declaration scope is not the lever either

Hoisting both name tables and the `string[256]` buffer from their guarded blocks
to function scope produces **byte-identical output** -- still 1136 against 1152,
frame still `0x17C` against `0x180`. MSVC lays the frame out the same way
regardless, so the arrays' declaration scope is neutral here and the missing
dword is not one of them.

The frame accounting is exact and worth writing down, because it says precisely
what is missing. January's `0x180`:

```
[ebp-0x180] char string[256]
[ebp-0x080] actor_type_names[16]     0x40
[ebp-0x040] team_names[10]           0x28
[ebp-0x018] real_point3d origin      0x0C
[ebp-0x00C] spilled vehicle pointer  4
[ebp-0x008] table walk pointer       4
[ebp-0x004] countdown                4
```

`256+64+40+12+4+4+4 = 384 = 0x180`. Ours is `0x17C` -- **one dword short, and it
is the spilled vehicle pointer**. January reuses a single register for
`ai_globals` and then for `&vehicle`, destroying the global and reloading
`[_ai_globals]` at the bottom of every outer iteration; we keep `ai_globals`
live in `ebx` for the whole function and never need the spill.

So the question is narrow and shared with `ai_debug_render_actor`: **what makes
VC7 decline to hoist a global pointer across the calls in this loop?** Five
source forms are now measured against it -- named `vehicle` pointer, direct
subscripting, pointer induction over the tables, tables at block scope, tables at
function scope -- and all five hoist. Until that is answered neither function can
close, and answering it once closes 26 KB.

## code_0003a2e0 — matched (1152/1152). The question was never hoisting.

For several sessions this and `ai_debug_render_actor` were parked on "what makes
VC7 decline to hoist `_ai_globals` across the calls in this loop?"  That question
had a false premise.  Both builds do the same thing with the global: load it at
the top, turn it into `&vehicle` in a callee-saved register, and reload it at the
bottom for the loop test.  Nothing declines to hoist.

What differed was which locals won the three callee-saved registers.  Tracing
every write to `esi` in January gives exactly seven:

    0011 xor esi, esi          0290 mov esi, 0x10       045f inc esi
    0129 mov esi, 0xa          037c xor esi, esi        046f pop esi
    03d2 inc esi

`0xa` and `0x10` are `NUMBER_OF_SOLO_CAMPAIGN_TEAMS` and `NUMBER_OF_ACTOR_TYPES`.
They are not scratch — they are the **constant-folded exit values** of the two
fixed-trip inner loops, written straight into the outer loop's counter.  A
compiler only materialises one loop's exit value into another loop's counter if
they are the same variable.  All four loops in the function share one `short`.

The consequences cascade exactly as observed:

- one live range spans the whole body, so it holds `esi` outright;
- the team and actor-type loops get `ebx` and `edi` for their own induction;
- with all three callee-saved registers taken, each inner loop's pointer and
  downcounter are pushed to memory (`[ebp-8]`, `[ebp-4]`);
- `&vehicle` is spilled to `[ebp-0xc]` and reloaded at 0x35c — the dword our
  frame was missing (0x180 vs 0x17c).

Four separate indices, as we had them, give the allocator a short outer range it
is happy to leave in memory, and then `esi`/`edi` are free for the inner loops.
Same instructions either way; entirely different allocation.

**Rule.** Before treating a register assignment as a compiler convention, count
the loop variables.  A constant equal to an inner loop's trip count appearing in
the outer loop's counter register is proof the source shares one variable.

## ai_debug_vocalize — matched (208/208). Declaration site, not register convention.

Parked as an "esi/edi swap".  It was a placement: January stores NONE into
`[ebp-8]` at 0x73, after both guards, immediately before the `unit_test_speech`
call that takes its address.  We stored it at 0x56, beside `vocalization_type`
and before the guards — because `sound_definition_index` was declared beside
them.  Moving the declaration inside the `if (priority>0 && vocalization_type
!=NONE)` block shortens the live range past the guards and all three registers
fall into January's assignment on their own.

Same lever as code_0003a2e0 from the other end: there a live range too short,
here one too long.  Register assignment is not dictatable and was never the thing
to reason about.  The declaration site is.

## code_00037fa0 — still NonMatching (320 vs 304). Now down to one typed expression.

The second loop walks the 6-byte `ai_debug_lineofsight_pair` records with a
strength-reduced pointer.  Two independent properties were wrong; six source
forms were measured.

**The anchor.**  January anchors the induction pointer at record+4
(`mov esi, 0x402f8`), ours at record+2.  Measured:

| form                                                        | anchor |
|-------------------------------------------------------------|--------|
| `reference_count = pair->reference_count;` then call         | +2     |
| same, but direct `lineofsight_pair[i]` subscripts throughout | +2     |
| block-scope `long x = pair->reference_count;` then call      | +2     |
| all three field reads inside the call                        | +4 ok  |
| start/end hoisted to locals, count still a statement         | +4 ok  |
| `color` local initialised from the count, points in the call | +4 ok  |

A `pair->field` read that is its own assignment is excluded from the cluster the
compiler strength-reduces; the pointer then anchors on the highest offset among
the reads that remain inside the call.

**The width.**  January emits `movsx eax, word ptr [esi]` / `cmp eax, 0xc`; every
form of ours that anchors correctly emits `mov ax` / `cmp ax` and widens later,
costing 6 bytes and a `jmp`.  The only forms that widen at the load are the ones
routing the field through a `long` — and those are exactly the forms that lose
the anchor.  The two properties are mutually exclusive across everything tried,
so one premise is wrong, and it is most likely "32-bit width needs a long
intermediate".

Ruled out for the ceiling operand: `NUMBEROF(colors)-1` (cseries.h:140 yields
`size_t`, which would give `jbe`, not January's `jle`), and `PIN` (would emit a
floor test January does not have).  A corpus sweep for exact functions emitting
`movsx r32, word ptr [..]` / `cmp r32, imm8` / `j..` / `mov r32, imm8` returns
**zero** hits across 3,332 exact functions, so there is no worked example to read.

Parked at the form with the correct anchor and the correct evaluation order —
`color` as an initialised local, the two point subscripts inside the call — which
is also the shape code_00038f30 proved for this object.  The single open question
is what gives the clamp expression `long` type without a separate statement.

## ai_debug_update — 1168/1168, net 0 bytes, one construct twice

Both `fix_defending_guard_firing_positions` and `fix_actor_variants` open the
same way and diverge identically (+8 bytes each, at 0x2f6 and 0x3b6):

    January   lea ebx, [eax + 0x42c]            ours   mov ebx, eax
              cmp dword ptr [ebx], edi                 mov eax, dword ptr [ebx + 0x42c]
                                                       add ebx, 0x42c
                                                       cmp eax, edi

January forms `&scenario->ai_encounters` with one `lea` off the
`global_scenario_get()` result and folds the count read into the compare.  We
keep the scenario pointer, read the count through a displacement, and only then
advance to the tag block — so the count needs a register.  Everything else in
the function follows: the remaining hunks are the store order of the two zeroed
locals, and the `jmp` + `lea ecx,[ecx]` pair with which January aligns the loop
head to 0x310, which we do not need because our preamble is 8 bytes longer.

Measured and rejected, all byte-neutral or worse:

- declaring `encounters` after the two counters (moves a zero store *before* the
  call — further from January, not closer);
- declaring `encounters` without an initialiser and assigning it as the first
  statement;
- hoisting the inner `group_index` declaration out of the squad loop.

The scheduler is reading the count before the pointer exists, which is what MSVC
does when the pointer variable's first use comes after the count read.  In our
source the initialiser is textually first, so the ordering is being set by
something else in the block, not by this declaration.  Parked here rather than
guessed at.

## `code_00041220`: two more blocks decoded (0x90–0x230)

Adding to the head already recorded above. Float constants throughout:
`__real@3f800000` = 1.f, `__real@3e800000` = 0.25f, `__real@3d4ccccd` = 0.05f.

### 0xa3–0x119 — the colour table is self-measuring

`global_ai_debug_firing_position_color_count` is computed once, lazily, by
walking `global_ai_debug_firing_position_colors` until a component exceeds 1.f.
The loop is rotated and software-pipelined — `ecx` starts at `colors+8` and each
iteration tests `[ecx-4]`, `[ecx]`, `[ecx+4]`, `[ecx+8]` then `add ecx, 0x10` —
which is the four components of one `real_argb_color` plus the *first* component
of the next, with `colors[0].a` peeled out ahead of the loop at 0xab. `edx`
increments once per 16-byte colour, so it counts colours, not components:

```c
if (global_ai_debug_firing_position_color_count==0)
{
    long color_count;

    for (color_count = 0;
        global_ai_debug_firing_position_colors[color_count].alpha<=1.f &&
        global_ai_debug_firing_position_colors[color_count].red<=1.f &&
        global_ai_debug_firing_position_colors[color_count].green<=1.f &&
        global_ai_debug_firing_position_colors[color_count].blue<=1.f;
        color_count++)
        ;

    global_ai_debug_firing_position_color_count = color_count;
}
```

There is no bound test in the loop — **the table is sentinel-terminated by a
colour with a component above 1.f**, and the array must be declared that way.
The `mov [count], edx` at 0xb9 (storing zero before the peeled test) and again
at 0x113 are the same assignment on two paths, not two statements.

### 0x119–0x230 — the firing-position walk

`encounter_build_firing_position_owner_actor_indices(encounter_index,
&[ebp-0x8ec])` fills the large stack buffer the `0x8EC` frame is mostly there
for. Then the loop walks `ebx+0x98`, the encounter definition's firing-position
tag block, via `tag_block_get_element_with_size` with element size 8.

Each element `edi` is `{real x@0, real y@4, real z@8, short group_index@0xc}`.
The body builds four corners of a square, 0.25 out in x and y and 0.05 up in z,
into `[ebp-0x6c]` upward — twelve floats, four `real_point3d`, in
counter-clockwise order:

```
[-0x6c] x-0.25  [-0x68] y-0.25  [-0x64] z+0.05
[-0x60] x+0.25  [-0x5c] y-0.25  [-0x58] z+0.05
[-0x54] x+0.25  [-0x50] y+0.25  [-0x4c] z+0.05
[-0x48] x-0.25  [-0x44] y+0.25  [-0x40] z+0.05
```

MSVC computes each of the five distinct expressions once and `fst`s it to its
two or four homes, so the source is the plain twelve assignments in corner
order — do not try to reproduce the interleaving.

The colour for the position is `&global_ai_debug_firing_position_colors[
firing_position->group_index % global_ai_debug_firing_position_color_count]`,
emitted as `movsx`/`cdq`/`idiv`/`shl edx,4`/`add edx, offset colors` at
0x1f3–0x206 and parked in `[ebp-0xbc]`. The `shl 4` re-confirms
`sizeof(real_argb_color)==16`.

Still not written. What remains undecoded is 0x230–0x920, which is where the
`ai_debug_actor_record` array at `0x7D9F4` is written.

## `ai_debug_render`: the source is already right, it is gated on two bodies

288 of 336, three hunks, net -48. Both missing blocks are already written in our
`ai_debug_render` exactly as January has them —

```c
if (ai_debug.selected_squad_index!=NONE)
    code_00041220(ai_debug.selected_squad_index);        /* J 0x9b-0xab */
...
if (ai_debug.render_speech || ai_debug.print_speech ||
    ai_debug.render_dialogue_variants)
    code_00039e80();                                     /* J 0x103-0x11e */
```

— and MSVC deletes both because `code_00041220` and `code_00039e80` are empty
statics (house_rules §6: an empty referenced static is inlined away under /O2).
The field offsets confirm out of January: `0x34` `selected_squad_index`, `0xc9`
`render_speech`, `0x93` `print_speech`, `0xd3` `render_dialogue_variants`.

**Nothing about this function needs changing.** It closes the moment both
callees have bodies, and not before. It should not be counted as a residual with
a source question.

## `code_00039e80`: fully skeletonised (1120 bytes)

An `object_iterator` walk, `[ebp-0x30]`, over units; `esi` is the unit datum and
`[ebp-0x28]` its index. The loop back-edge is at 0x434 to 0x30, so everything
below is one loop body.

Preamble, once per unit — note **0.1f**, not the 0.05f used elsewhere in the
object:

```c
unit_get_head_position(unit_index, &head);
point.x = global_up3d->i*0.1f + head.x;     /* __real@3dcccccd */
point.y = global_up3d->j*0.1f + head.y;
point.z = global_up3d->k*0.1f + head.z;
ai_debug_drawstack_setup(&point);
```

Then three independent guarded blocks, in this order:

1. `if (ai_debug.render_dialogue_variants)` (0x79) — `tag_get('unit', unit->tag)`,
   then a linear search over the tag's `tag_block` at `+0x2b4` (element size
   **0x18**) for the element whose tag index at `+0x14` equals `unit->[0x334]`,
   taking that element's `short` at `+0`. Falls back to the literal `"<none>"`
   and `NONE`. Prints `"variant %d dialogue %d %s"` with `unit->[0x6e]`.
2. `if (ai_debug.render_speech)` (0x1a5) — guarded on
   `unit->[0x338] > 0`; names come from `tag_get_name`/`tag_name_strip_path` on
   `unit->[0x33c]` and `dialogue_get_vocalization_name(unit->[0x33a], 0)`, each
   falling back to `"NONE"`.
3. `if (ai_debug.print_speech && !ai_debug.render_speech)` (0x3ae) —
   `unit_describe_speech(unit_index, 0, 0x200, buffer)` into `[ebp-0x230]`,
   coloured by a **switch on `unit->[0x338]`**: `add eax,-2` / `cmp eax,8` /
   `ja default`, so the cases are 2..10, dispatched through a byte index table
   at 0x450 into a 4-entry jump table at 0x444. Three arms only —
   `global_real_argb_red`, `global_real_argb_blue`, and white as the default.

### The trap, now pinned

`ai_debug_drawstack` is **inlined** in blocks 1 and 2 (the
`last_position = next_position; next_position += up3d*height` sequence appears
open-coded at 0x109 and 0x202) but **called** in block 3 (`call
_ai_debug_drawstack` at 0x414). Same function, same translation unit, two
treatments in one function — so this is MSVC's own inlining heuristic firing on
call-site context, not something the source spells differently. Do not try to
force it; write the three call sites identically and check what falls out.

Still needed before it can be written: `object_iterator_next` and
`unit_describe_speech` prototypes, `dialogue_get_vocalization_name`, and names
for `unit_datum` `0x334`, `0x338`, `0x33a`, `0x33c`, `0x36a` and `0x6e`. The
0x230-0x3a0 stretch (block 2's tail) is the only part not yet read.

## `ai_debug_render_actor`: the frame, and an unresolved trade

### What the frame permutation is, and what it is not

Aligning the two instruction streams with displacements blanked and reading off
every `[ebp-X]` pair recovers the frame permutation directly
(`framemap.py`). Two things fall straight out:

* everything at `0x2b0` and deeper sat at exactly **+12** in our build, so the
  excess was one `real_point3d` allocated shallower than the buffers;
* the shallow slots `0xc`–`0x1c` already matched with 86, 38, 50, 47 and 52 uses
  each, so the disagreement was localised, not global.

The slot *assignment* is a different matter and is **not** settable from
declaration order. Our declarations run `actor`, `actor_debug_info`,
`actor_definition`, `actor_variant_definition`, `path`; January's slots for those
same five are `0x104`, —, `0x74`, `0x20`, `0x108`. That is not any permutation of
declaration order — it is spill order, downstream of codegen. Do not try to
reorder declarations to chase it.

### The 12 bytes: `p4`

In the `BIT_VECTOR_TEST_FLAG(pvs, cluster_index)` arm we declared a fifth point
`p4` holding a copy of `base_point`, taken *before*
`point_from_line3d(&base_point, ...)`. MSVC cannot fold that copy away — the call
takes `base_point`'s address, so the old value may differ — and must keep `p4` as
a real 12-byte object. Passing `&base_point` to the two `render_debug_line` calls
directly removes it.

**Result: frame `0x810`, exactly January's.**

### The trade, stated honestly

This is not a clean win and should not be recorded as one.

| | frame | relocations |
|---|---|---|
| with `p4`    | `0x81c` (12 over) | 1905 == 1905 |
| without `p4` | `0x810` **match** | 1904, one short |

The lost relocation is a single `_global_up3d` **reload** at January `0x1285`,
inside the same three-arm block: January reloads the pointer at the head of the
arm, we keep it live. That is a register-pressure/CSE consequence, not a missing
reference — no `global_up3d` *use* disappeared from the source.

So one of two things is true, and the evidence does not yet separate them:

1. `p4` is genuinely ours and the reload difference is an independent residual
   that removing 12 bytes of stack traffic exposed; or
2. January really has five points here, and we carry a *different* redundant
   12-byte object elsewhere, in which case both invariants would come good at
   once by restoring `p4` and finding that other object.

The frame value is a sum over every stack object and landing on `0x810` exactly
is hard to do by accident, which favours (1). But a relocation count is a count
of real symbol references and had been matching, which is the stronger kind of
invariant. **Kept the change, because the frame was the question asked and the
byte arithmetic corroborates it — we were 16 short of January while carrying
`p4`'s 16-byte copy and are a clean 32 short without it — but this is flagged,
not settled.** Anyone resuming should first look for a second redundant
`real_point3d` before trusting the frame match.

### Resolved: there is no second redundant `real_point3d`

The flag raised above asked whether January really has five points in that arm
and we carry a different redundant twelve bytes elsewhere. Searched for, and the
answer is no. Three checks, with `p4` restored so the frame carried the excess:

* **Nothing in our frame is unmapped.** Aligning both instruction streams and
  reading off every `[ebp-X]` pair, every slot of ours corresponds to a January
  slot. A genuinely extra object would have no partner; none exists.
* **The one apparent twelve-byte hole is not an object.** Set membership puts
  January's slots at or below `0x15c` in our frame as-is and everything from
  `0x160` up only at +12, which looks like an extra local occupying our
  `0x160`-`0x16b`. It has **zero** references anywhere in the function — no load,
  no store, not even a `lea`. MSVC does not allocate an unreferenced local, so
  that gap is an artifact of the permutation, not something we declared.
* **Source has no other candidate.** No remaining struct-copy assignment between
  point or vector locals in the function.

So the twelve bytes were `p4` and nothing else, and removing it was right. The
lost `_global_up3d` reload is an independent residual that the reduced stack
traffic exposed, not evidence against the change. **Reading (1) above is the
correct one; the caveat on the frame match can be dropped.**

### What the search did turn up: two adjacent locals in the wrong order

The alignment map shows a clean transposition, the strongest pair at n=3:

    jan 0x190-0x198  ->  ours 0x19c-0x1a4     +12
    jan 0x19c-0x1a4  ->  ours 0x190-0x198     -12

Two adjacent twelve-byte locals with their order reversed — a declaration-order
difference, which unlike spill order *is* settable from source. Ours are the two
points whose `z` components are stored at `0x108f` (`fstp [ebp-0x19c]`, built
with 0.2f off `[ebp-0x3c]`) and `0x10ca` (`fstp [ebp-0x190]`, off `[ebp-0x60]`),
immediately before a `render_debug_line`. Swapping those two declarations is the
next concrete step, and it is independent of the spill-order permutation that
dominates the shallow slots.

### `ai_debug_render_actor`: declaration order does not move the frame either

Following the transposition found above (`jan 0x190-0x198` against
`ours 0x19c-0x1a4`), the two locals were identified: `p2` and `p3` in the
cluster-visibility arms, written by `point_from_line3d` inlined —
`fld [up3d+k]` / `fmul 0.2f` / `fadd [p0+k]` / `fstp [p2+k]`. January puts `p2`
shallower than `p3`; we put `p3` shallower.

Swapping the two declarations at all three sites changes **nothing at all**:
identical size, identical relocation count, identical slot-use agreement. So
declaration order does not drive the layout for block-scope points any more than
it does for the spilled pointers.

**Both halves of the frame question are now closed off.** Neither the spilled
pointers (shown earlier: January's slots for our first five declarations are not
any permutation of their declaration sequence) nor the block-scope points respond
to declaration order. The slot assignment is not a source lever in this function;
it will fall out when the surrounding code matches, not before. Anyone resuming
should spend their time on the 152 code-divergence sites and let the frame follow.

A baseline metric to measure against, from aligning both streams and comparing
the `[ebp-X]` displacement on every instruction that survives alignment:

    slot-uses matching 364 / 915      size 24944 / 24976      relocs 1904 / 1905

### `ai_debug_update`: the loop indices are `short`, confirmed

Testing whether January's `cmp dword ptr [ebx], edi` came from `long` indices:
widening `encounter_index`, `squad_index` and `location_index` to `long` moves
the first differing byte from `0x2f6` all the way back to `0xb`, wrecking the
prologue. `short` is right and the divergence at `0x2f6` is not about the index
type. Four source forms rejected for this function now.

### `code_00039e80`: every speech offset resolved against declared types

Not written. But the data-model blocker is gone: the offsets this function uses
are **already declared** in `units.h`, and the mapping is verified by the
compiler rather than inferred. Appending
`char probe_X[offsetof(struct unit_datum, unit.X)+1];` for all 85 `_unit_datum`
fields to this unit and reading the COMMON symbol sizes back (offset = value-1)
gives `dialogue_index` at **0x334** and `speech` at **0x338** exactly, which
pins the whole run:

| target offset | field | read as |
|---|---|---|
| `0x334` | `unit->dialogue_index` | long, tag index |
| `0x338` / `0x33a` / `0x33c` | `unit->speech.current.priority` / `.vocalization_type` / `.sound_definition_index` | short / short / long |
| `0x368` / `0x36a` / `0x36c` | `unit->speech.queued.` same three | " |
| `0x3aa` / `0x3ae` | `unit->speech.sound_timer` / `.post_delay_timer` | short |

`sizeof(struct unit_speech_item)` is `0x30`, which is what makes `current` and
`queued` land 0x30 apart, and `last_damage_category` probes to `0x3b4`, so
`unit_speech` is `0x7c` — both consistent with the two sub-blocks being the
current and queued speech items. **No new field names are needed.**

Read off the call sites, both still undeclared:

```c
char const *unit_get_speech_priority_name(word priority);   /* xor edx,edx / mov dx */
char const *unit_describe_speech(long unit_index, boolean abbreviated,
    long buffer_size, char *buffer);        /* (unit_index, 0, 0x200, &[ebp-0x230]) */
```

`object_iterator`, `object_iterator_new` and `object_iterator_next` are already
declared in `objects.h`, and `dialogue_get_vocalization_name` in
`dialogue_definitions.h`.

**The one thing still unnamed is `unit_datum + 0x6e`**, the `%d` printed as
"variant" in block 1. The probe run settles where it is not: every `_unit_datum`
field begins after `0x6e`, so it lies inside the preceding `object` member -- an
`_object_datum` field, not a unit one. Probe `struct unit_datum, object.*` the
same way to name it. It is used at exactly one site, so it must be named from
the probe and not guessed.

## `code_00039e80` written: 1120/1120, 107/107 relocations, one instruction out

Size and every relocation match. The sole difference is at `0x349`: January
schedules `xor eax, eax` before the `fstp`, we emit it after. Net zero bytes.

Nothing in it was invented:

* probing all 59 `_object_datum` fields names `unit_datum+0x6e` as
  `object.variant_number` — exactly what the `"variant %d"` format prints;
* every speech offset lands on a field `units.h` already declared, through
  `speech.current`, `speech.queued`, `speech.sound_timer`, `speech.post_delay_timer`;
* the switch index table at `0x450` is reproduced byte for byte — red for
  priorities 2, 7, 10, blue for 6, white otherwise. The priority *names* are not
  recovered, so the cases are written as the integers the target switches on.
  Naming them would have been a guess with a single use site.

### The polarity lesson, applied twice in one function

Block 1's `NONE` check is a genuine `if` over a default (`je`, fallthrough into
the call). Block 2's four `NONE` checks are **ternaries** — January's `jne`
falls through into the `"NONE"` arm, which is the recorded rule that MSVC puts a
ternary's true arm in the fallthrough. Writing block 2 the way block 1 reads took
the function from **34 hunks to 8**. The two sites are spelled differently in the
original and normalising them costs 16 bytes.

### The `0x349` tie

Five forms measured, none moves it: queued declarations swapped (worse — first
difference retreats to `0x2c6`), strings inline in the `csprintf`, the drawstack
hoisted to a local, and the two string locals shared across both sub-blocks
(the `code_0003a2e0` reuse lever). Argument types are confirmed rather than
assumed: the guard is `jle`, so `priority` is signed `short`, and the
`xor eax,eax` / `mov ax` pair is what a `word` parameter takes — a `long`
parameter would emit a single `movsx`.

`ai_debug_render` moved from 288 to **320/336, 39/41 relocations** as a result;
the two missing relocations are exactly the `code_00041220` call and the
`selected_squad_index` read, so it closes when that function gets a body.

## `code_00041220`: decoded to 0x520 of 0x920. Not written.

Roughly 60% read. Recording it so the next attempt starts from evidence. Float
constants: `__real@3ec00000` = 0.375f, `__real@3f000000` = 0.5f,
`__real@3dcccccd` = 0.1f, `__real@3d4ccccd` = 0.05f, `__real@3e800000` = 0.25f.

### 0x00-0xa3 — the actor pass, fully readable

```c
encounter_get(encounter_index);          /* discarded; there for its assert */
definition = TAG_BLOCK_GET_ELEMENT(&global_scenario_get()->ai_encounters,
    encounter_index&0xFFFF, struct encounter_definition);   /* ebx, element 0xb0 */
history_start_time = NONE;               /* [ebp-0x3c] */

encounter_actor_iterator_new(&iterator, encounter_index);   /* [ebp-0x38] */
while (encounter_actor_iterator_next(&iterator))
{
    boolean selected =                                       /* [ebp-0x24] */
        (ai_debug.selected_actor_index==iterator.actor_index);   /* [ebp-0x34] */

    if (selected || ai_debug.selected_actor_index==NONE ||
        ai_debug.render_all_actors)
    {
        ai_debug_render_actor(iterator.actor_index, selected, &history_start_time);
    }
}
```

`&history_start_time` is the third parameter our `ai_debug_render_actor` already
declares, which is independent confirmation of that signature.

### 0x2a5-0x38b — firing-position group selection

Two `neg`/`sbb`/`and` sequences off one boolean at `unit+0x374`, which is the
MSVC idiom for a boolean-selected constant:

    ebx = flag ? 3 : 0          ecx = flag ? 4 : 1          (and, earlier, flag ? 5 : 2)

then `if (esi->[0x98])` swaps which of the two indexes the group is read from.
Each is used as `test dword ptr [eax + index*4 + 0x54], edx` with
`edx = 1<<(firing_position->group_index)` — a bit-vector over firing-position
groups, at `+0x54` of the 0x80-strided element fetched at 0x29a. Matching colours
accumulate into an array at `[ebp-0xbc]` with a count in `[ebp-4]`; green when
the first test passes, aqua from the flags at `+0x6c`, white when the count is
still zero. The count is then asserted below 8:

    display_assert("num_firing_po...", "c:\halo\SOURCE\...", 0x3c4 /* line 964 */, 1);
    system_exit(-1);

### 0x38e-0x442 — the marker polygon

Four vertices built at `[ebp-0xec]`, 0.375f out from the firing position, drawn
with `render_debug_polygon(TRUE, points, 4, actor_action_debug_color(...))`.

### 0x445-0x520+ — the `ai_debug.field_CD` overlay

Guarded on `ai_debug` byte `0xcd`. Colour is red when `firing_position->[0x14]`
is `NONE`, white otherwise. Two points at `[ebp-0x14]` and `[ebp-0x20]` are
copied from the position and then walked through a sequence of ±0.5f and ±0.1f
adjustments with a `render_debug_line` after each pair — a cross or box glyph.
This stretch runs past 0x520 and is where the reading stopped.

### Still unread: 0x520-0x920

About 1,000 bytes, which per the relocation survey is where the
`ai_debug_actor_record` array at `0x7D9F4` is written. **Not written, and no
partial body committed** — `ai_debug_render` closes as soon as this function has
any body at all, so writing a speculative one would credit `ai_debug_render`
against a reconstruction that is not the original. It should be written once,
completely, from a full decode.

## `code_00041220`: 0x520-0x920 decoded. Whole function now read.

### The loop frame, settled

The firing-position loop runs `0x14a`-`0x8fa`. Its bookkeeping:

| slot | meaning |
|---|---|
| `[ebp-0x28]` | `short` firing-position index |
| `[ebp-8]` | the same index sign-extended to `long`, used for all array indexing |
| `[ebp-0x24]` | `struct tag_block *` = `&definition->firing_positions` (`ebx+0x98`) |
| `[ebp-0x8ec]` | the owner-actor-index array filled by `encounter_build_firing_position_owner_actor_indices` |
| `[ebp-0xbc]` | up to 8 accumulated colours, count in `[ebp-4]` |

The tail is `inc`, `movsx eax, ax`, `cmp` against `firing_positions->count`
reloaded through `[ebp-0x24]`, which is why both a `short` and a `long` copy of
the index are live at once.

### 0x540-0x5a9 — draw once per accumulated colour

Loops `esi` from 0 to the colour count. Per colour, either two
`render_debug_line` calls (points `[ebp-0x6c]`/`[ebp-0x54]` and
`[ebp-0x60]`/`[ebp-0x48]`) or a single
`render_debug_polygon_edges(&[ebp-0x6c], 4, colour)`. Both take the colour from
`[ebp+esi*4-0xbc]`, so this is the fan-out of the group colours gathered earlier.

### 0x5ab-0x5de — drawstack anchor

`point = firing_position + global_up3d*0.2f`, then `ai_debug_drawstack_setup`.

### The actor-record array, fully mapped

`shl esi, 6` — **stride 64** — with base `ai_debug + 0x7d9f4`. Fields touched:

| offset in record | absolute | read as |
|---|---|---|
| `+0x00` | `0x7d9f4` | boolean |
| `+0x01` | `0x7d9f5` | boolean |
| `+0x24`/`+0x28`/`+0x2c` | `0x7da18/1c/20` | a `real_point3d` |
| `+0x34` | `0x7da28` | boolean |
| `+0x38` | `0x7da2c` | `real`, compared against 0.0f |
| `+0x3c` | `0x7da30` | `real`, printed `"%3.2f"` (pushed as a qword — vararg double promotion) |

Companion globals: `ai_debug + 0x7d980` boolean, `0x7d3c7` boolean, and a
`real_point3d` at `0x7d988/8c/90`.

### 0x5e3-0x800 — the `ai_debug` byte-`0xc4` branch

Entered when `ai_debug[0xc4]` and `record->[0]` are both set.

* `actor_get_pursuit_location(...)` returns a pointer; the test is
  `!= NULL && location->[0]==1 && location->[2]==firing_position_index`,
  producing a flag in `bl`.
* `encounter_pursuit_position_already_examined(encounter_index,
  ai_debug.selected_actor_index, history_start_time, index, &[ebp-0x2c], 0)` —
  note `sete cl` / `dec ecx` / `and ecx, eax`, which is the idiom for
  `history_start_time==NONE ? 0 : history_start_time`.
* Colour: yellow when `bl`, else blue when the call returned true, else white.
  The drawstack is **inlined** here, then `csprintf(temporary, "%d", [ebp-0x2c])`.
* `0x771-0x800`: gated on `ai_debug[0x7d980] && ai_debug[0x7d3c7] &&
  record->[1] && record->[0x34]`, draws two lines — yellow from the global point
  at `0x7d988` plus `record->[0x24]`, then green back to the firing position.

### 0x805-0x8e3 — the else branch, byte `0xc3`

Gated on `ai_debug[0xc3] && record->[1] && !record->[0] &&
ai_debug.selected_actor_index!=NONE`. Colour ladder: red when `record->[0x34]`
is clear; otherwise if `record->[0x38] > 0.f` white, and within that yellow when
`*(long *)[ebp-4] == ai_debug.selected_actor_index` else blue; white when the
compare fails. Two `csprintf`/`ai_debug_drawstack`/`render_debug_string_at_point`
groups, the second printing `record->[0x3c]` as `"%3.2f"`. Here the drawstack is
a **real call**, not inlined — the same split this object showed in
`code_00039e80`.

### Symbols this function needs that the tree does not declare

`encounter_build_firing_position_owner_actor_indices`,
`encounter_pursuit_position_already_examined`, `actor_get_pursuit_location`,
`actor_action_debug_color`, `render_debug_polygon`, `render_debug_polygon_edges`.
Everything else it references is already declared.

The decode is complete; what remains before writing is naming the six prototypes
above and the `ai_debug` fields at `0xc3`, `0xc4`, `0xcd`, `0x7d380`, `0x7d3c7`,
`0x7d980`, `0x7d988` and the 64-byte record's members.

### `code_00041220`'s prototypes: four, not six

Correcting the list above. `actor_get_pursuit_location` and
`actor_action_debug_color` were **already declared** in `actions.h` (lines 444
and 446), along with `struct pursuit_location` at line 164 — and `ai_debug.c`
already calls the first at two other sites. Adding them to `actors.h` produced a
C2371 against the existing declarations, which is how the duplication surfaced.
Reverted; the existing ones are correct and the return type is
`struct pursuit_location *`, not a new type.

Four were genuinely missing, now declared, with the argument counts read off the
merged stack cleanups rather than assumed:

```c
void encounter_build_firing_position_owner_actor_indices(long encounter_index,
	long *owner_actor_indices);                        /* push array, push index, add esp,8 */
boolean encounter_pursuit_position_already_examined(long encounter_index, long actor_index,
	short firing_position_index, long start_time, short *examined_count, boolean flag);
void render_debug_polygon(real_point3d const *points, long point_count,
	real_argb_color const *color);
void render_debug_polygon_edges(real_point3d const *points, long point_count,
	real_argb_color const *color);
```

Two things worth keeping:

* **Neither polygon call takes a leading `immediate` flag**, unlike every
  `render_debug_line`/`render_debug_string_at_point` in this object. The count
  comes from `add esp, 0xc` on the `_edges` call, which has no other candidate.
* The `0x442` cleanup is `add esp, 0x18` covering **three merged calls** —
  `datum_get` (2 args), `actor_action_debug_color` (1), `render_debug_polygon`
  (3) — 8+4+12 = 24. Reading that as one call's argument list is how a wrong
  arity gets baked in; the `actor_get(...)` result at `0x3b3` is discarded, the
  same assert-only shape as `unit_get` in `code_00038280`.

The firing-position element is **0x18** bytes (`push 0x18` at `0x153`), with
`x`/`y`/`z` at 0, 4, 8, a `short` group index at `0xc`, and a `long` at `0x14`
tested against `NONE`.

### `code_00041220`: the types are almost all already in the tree

Checked before writing, and most of what I expected to have to name is declared:

* `encounter_definition.firing_positions` is at **0x98** and `.squads` at
  **0x80** — the `ebx+0x98` and `eax+0x80` in the target — given a 12-byte
  `tag_block` and the `unused[18]` run. Both already declared.
* `squad_definition.firing_position_groups` is at **0x54**, matching
  `test dword ptr [eax+ebx*4+0x54], edx`. The fixed `[eax+0x6c]` test is
  group index 6 of that same array.
* `struct ai_debug_actor_record actor_record[]` already sits at exactly
  **0x7D9F4** (`field_7D384[1648]` ends there), with the 64-byte stride the
  `shl esi, 6` requires. `field_01` is already named.
* Probing `struct ai_debug_state` names the three gates outright:
  **0xc3 `render_evaluations`**, **0xc4 `render_pursuit`**,
  **0xcd `render_firing_positions`**.

What is left before the body can be written, all mechanical:

1. split `ai_debug_actor_record.__unknown00`/`__unknown02` to name `+0x00`,
   `+0x24`-`+0x2c` (a `real_point3d`), `+0x34`, `+0x38` and `+0x3c`;
2. split `field_7D384[1648]` to name `0x7D980` (boolean) and the
   `real_point3d` at `0x7D988`;
3. locate `0x7D3C7`, which falls below `field_7D384` and so is inside the
   `path_storage`/`__unknown608A9` run.

None of these needs a guess — each is a probe or an offsetof assert away, and
the `field_XXXX` convention this header already uses covers the ones whose
meaning is not recovered.

### `code_00041220` 0x217-0x2a5: the group colour table

The one stretch I had skipped, and it holds the block's shape. Seven colour
pointers are written to consecutive stack slots `[ebp-0x9c]` upward, which is the
array the later `mov ecx, [ebp+esi*4-0x9c]` indexes:

| index | slot | colour |
|---|---|---|
| 0 | `-0x9c` | `global_real_argb_red` |
| 1 | `-0x98` | `global_real_argb_orange` |
| 2 | `-0x94` | `global_real_argb_green` |
| 3 | `-0x90` | `global_real_argb_blue` |
| 4 | `-0x8c` | `global_real_argb_lightblue` |
| 5 | `-0x88` | `global_real_argb_green` |
| 6 | `-0x84` | `global_real_argb_aqua` |

Green appears twice, at 2 and 5 — written first, out of order, at `0x21d`-`0x228`
because MSVC shares the one relocation between both slots. Anyone reconstructing
the initialiser from emission order would get the entries wrong.

The rest of the block resolves the selected actor's own squad:

```c
struct actor_datum *actor = actor_get(ai_debug.selected_actor_index);
struct encounter_definition *actor_encounter = TAG_BLOCK_GET_ELEMENT(
    &global_scenario_get()->ai_encounters,
    DATUM_INDEX_TO_ABSOLUTE_INDEX(actor->meta.encounter_index), ...);   /* 0xb0 */
struct squad_definition *squad = TAG_BLOCK_GET_ELEMENT(
    &actor_encounter->squads, actor->[0x3a], struct squad_definition);  /* 0xe8 */
```

`squad_definition` elements are **0xe8** bytes, and the squad index is the
`short` at `actor+0x3a`. The `neg`/`sbb`/`and` pairs then pick which of the
seven colours applies from the squad's `firing_position_groups` bit vector.

The decode is now genuinely complete — no unread stretches remain.

### `code_00041220`: first write attempt — compiles down to two unknown actor fields

Written in full and put through the compiler. Everything resolved except two
booleans read off the **actor datum**, which I had guessed as `meta.defending`
and `meta.berserk` and which do not exist:

* `actor + 0x374` — selects between firing-position groups `5` and `2` for the
  first test, and between the `{3,4}` and `{0,1}` pairs for the second;
* `actor + 0x98` — swaps the attacking/defending pair when set.

Both are single-site reads in this object, so they need probing against
`struct actor_datum` the same way `unit_datum+0x6e` was resolved, not naming from
the surrounding logic. That is the whole remaining blocker: the body is written
and everything else in it type-checks.

Reverted rather than committed, so the tree stays green. Everything else from the
attempt is kept: `struct firing_position` is declared (0x18, `position` at 0,
`group_index` at 0xc, `field_14` at 0x14) and the seven-entry group colour table,
the squad lookup, the polygon draw and the colour-count assert at line 964 are
all written and compile.

## `ai_debug_update` — matched (1168/1168). The pointer local was the defect.

The construct at 0x2f6/0x3b6 — `lea` off the call result with the count folded
into `cmp [ebx], edi` — is not reachable while the source declares
`struct tag_block *encounters = &global_scenario_get()->ai_encounters;`. A
source-level pointer local is forward-substitution fodder: MSVC rewrites the
count read as `[scenario + 0x42c]`, hoists that load above the pointer
materialisation to hide latency, and pays `mov ebx,eax / mov eax,[ebx+0x42c] /
add ebx,0x42c / cmp eax,edi` — 8 bytes that then eat both of January's
loop-head alignment pads.

The worked example was in the exact corpus: scanning all 3,332 exact functions
for `lea r,[eax+imm]; cmp [r], r2` returns exactly one hit,
`find_netgame_flags` (game_engine.c), and its proven source keeps a
**`struct scenario *scenario` local and spells the tag block
`scenario->netgame_flags` at every use** — loop condition and
`TAG_BLOCK_GET_ELEMENT` alike. The `lea` is the compiler's own CSE of the
repeated address expression, and that CSE temp — unlike a named pointer —
is the canonical base for the count read, so the load folds into the compare.

Both fix blocks rewritten that way; both sites fell at once, and the zero-store
order and the `jmp / lea ecx,[ecx]` / `nop` pads followed on their own.

**Rule (pairs with the subobject-pointer rule):** when January reads a tag
block's count through a bare `[reg]` freshly `lea`'d from a call result, the
original has no pointer local for the block — it names the *parent* and spells
the member path everywhere. Scan the exact corpus for the emission shape before
theorising: one worked example outweighs any number of rejected guesses.

## `code_00037fa0` — matched (304/304). The anchor followed statement order, not spelling.

The parked question was "what gives the clamp expression `long` type without a
separate statement". Wrong question — the separate statement was fine; what
mattered was **when the record's address is first demanded relative to the
`pair` binding**. The exact form:

```c
struct ai_debug_lineofsight_pair *pair;

reference_count = ai_debug.lineofsight_pair[pair_index].reference_count;
pair = &ai_debug.lineofsight_pair[pair_index];

render_debug_line(TRUE, &point[pair->start_index], &point[pair->end_index],
    *colors[MIN(reference_count, 12)]);
```

Same shared `long reference_count`, same subscript spelling of the hoist, and
the same `MIN` in the call as loop 1 — the author's one consistent voice — with
`pair` bound *after* the hoist, which is exactly what a programmer does when
they add the pointer later for the two long point expressions.

Eleven further forms measured this session (six were already logged). The two
properties resolve independently:

**Width.** The compare is 32-bit iff the compared lvalue is the `long` local
and the field read feeds *only* that store. Any direct consumer of the field —
condition, `MIN`, subscript, ternary arm, either arm order — lets the
narrowing pass canonicalise the whole cluster to 16-bit (`mov ax/cx`,
`cmp ax/cx`, per-arm `movsx`, +16 bytes). With mixed narrow/wide consumers VC7
loads narrow and widens register-to-register (`mov cx,[esi] / movsx eax,cx`);
only a wide-only consumer set fuses to `movsx eax,[mem]`. A guarded-overwrite
pair of `*colors[...]` expressions does not tail-merge, so every two-subscript
form pays a second table load. Corpus corroboration: across 3,332 exact
functions, every `movsx r32, word` + `cmp r32, imm` is a switch head (39),
an equality test (10), or a parameter/global (5 relational) — **no short
field is compared wide-relationally anywhere**, so the value must reach the
compare as a long.

**Anchor.** The induction pointer anchors at the canonical address of the
first-demanded read: `pair`-spelled reads contribute their field offset but a
store-feeding read is skipped (+2, five forms); a subscript-spelled hoist
*after* `pair` is bound re-roots the walk at the record base (+0); a
subscript-spelled hoist *before* `pair` is bound anchors at the hoisted
field itself (+4 — the only +4 that doesn't sacrifice the width). J and V1
differ only in the order of the copy and the binding: +0 against +4.

With both right, the `jle`-fallthrough `mov eax, 0xc`, the fold of the clamp
into the table index, and the 3/4-byte displacement split all follow. The
`if`-clamp spelling (`if (reference_count>12) reference_count = 12;` with a
plain `colors[reference_count]`) is byte-identical to the `MIN` form over the
long local — the macro was kept for loop-1 consistency.

## `code_00039e80` — matched (1120/1120, relocations identical). Two defects, neither where the diff pointed.

The `0x349` tie — January schedules the word-argument `xor eax,eax` above the
drawstack inline's first `fstp`, we below — was never movable from inside the
queued sub-block. Ten in-block forms are now measured and rejected: the seven
already logged, the csprintf result hoisted to a `string` local (restructures
the merged argument frames, `add esp,0x24`, -3 bytes, first diff retreats to
0x29), the colour routed through a local (0x314), and a block-3-style hoisted
`short priority` (0x29).

**What moved it was the body of `ai_debug_drawstack` itself.** Respelling the
callee from `point_from_line3d(...)` to the three component assignments leaves
the standalone function byte-identical — so the change is invisible to its own
section — but renumbers the IR that the inliner drops into every call site,
and VC7's scheduler breaks the `xor`/`fstp` tie by that order. The preamble's
spelling (longhand against `point_from_line3d`) changes nothing, measured both
ways. **Rule: when a scheduling tie sits at an inlined callee's boundary, the
callee's internal spelling is a live lever even though its own section already
matches. Sweep the callee, not just the call site.**

With the tie gone the byte stream is exact, and `section_infos_equal` exposed
the second defect, invisible to every raw-byte diff because relocation counts
matched at 107/107: the current-speech format string is
`"%s %s %s (%d %d)"` — January parenthesises the two timers — where the
reconstruction had `"%s %s %s %d %d"`. Wrong-string defects live in `.rdata`
COMDATs the gate does not count, so the only tell is the relocation *identity*
at the csprintf site. Fixed; the function passes strict comparison.

`ai_debug_render` (already exact) and `_ai_debug_drawstack` (standalone) are
both unchanged, and the object stands at **55/60 sections, 13,312/42,992
bytes**, with `ai_debug_update` and `code_00037fa0` also closed this session.

## `code_00041220`: seven recoveries, still NonMatching (2560 vs 2336)

Ranking the relocation multiset (not the byte diff) drove this whole pass, and
every recovery below was pinned by an identity difference before any source
was written.

1. **The assert carried bell characters.** The environment note about heredocs
   collapsing backslashes struck again: line 964's `match_assert` file string
   had single backslashes, so `\a` compiled to BEL — the mangled name showed
   `?$AH` where January has `?2`. The expression string also needed January's
   spaces (`a < b`, not `a<b`); stringize preserves source spacing.
2. **`record`/`owner`… the pointer question is per-object.** January spells
   `ai_debug.actor_record[index].field` at every one of the fifteen uses
   (J=34 `_ai_debug` references against our 19 — the delta equalled the use
   count exactly), but it *does* hold `long *owner_actor_index` — the
   evaluations compare goes `mov edx,[ebp-4] / cmp [edx],ecx` through the
   spilled pointer. One derived pointer kept, one dissolved, in the same loop.
3. **The owner polygon is a second quad**, 0.375f out (`__real@3ec00000` J=4
   O=0), built at `[ebp-0xec]` and passed to `render_debug_polygon`; the
   0.25f quad only feeds the edge/line fan-out.
4. **Both quads are chained assignments, not twelve statements.** The
   `fst/fst/fstp` runs with store order c3→c0 decode as
   `corner[0].z = corner[1].z = corner[2].z = corner[3].z = pos->z+0.05f;`
   and four two-home chains for x±/y±. The earlier decode's "plain twelve
   assignments in corner order" was wrong — spelled that way the expressions
   refuse to CSE (0.25f×8) because the corner stores sit between the reads.
5. **The colour-count global is its own loop variable.** January stores the
   count on *two* paths (`0xb9` in the peeled test's flag shadow, `0x113` at
   the loop exit) — the signature of
   `for (global_..._count = 0; …; global_..._count++);` with the global
   registerised, not a `color_count` local assigned afterwards.
6. **The fan-out loop raises the quad 0.05f per colour** — another chain,
   `corner[0].z = … = corner[0].z+0.05f;`, missing from the reconstruction.
7. **The branch shapes.** The pursuit arm is two *sequential*
   `if (selected!=NONE)` blocks (VC7 folds the second test on the failed
   path — no call intervenes — and re-tests after the render's calls); the
   `%d` render is ungated with a three-arm ladder
   `pursued ? yellow : examined ? blue : white`; a fifth gate
   `ai_debug.field_7D380` fronts the field_604 block; and the evaluations
   arm needs `string_color` — NULL, assigned `global_real_argb_white` inside
   the `field_38>0` arm, gating the inner render. The compiler cannot fold
   `string_color != NULL` because the global's *value* is unknown, which is
   exactly the dead-looking `mov eax,[white] / test eax,eax` January emits.

**What remains is only the drawstack split.** Relocation identity is now
clean except: January calls `_ai_debug_drawstack` at sites 1/3/4 and inlines
site 2; we inline 1/2/4 and call 3 (+224 bytes, the whole residual). The same
all-calls spelling fell out correctly in `code_00039e80` (3 inline + 1 call,
matched), so the inliner's per-site choice is context-driven and our remaining
context difference is presumably still upstream. The component-wise copy
respelling of the callee was measured and rejected — it wrecks the standalone
emission (48/60) — and January's function definition order matches ours.

**Process warning, earned the hard way:** a `replace('record->field_01', …)`
without function scoping rewrote `ai_debug_select_actor`'s clearing loop and
silently regressed four matched sections; the object dropped to 51/60 until
the line was found and restored. Scope mechanical renames to the function.

## The last five: a dependency chain, and two residuals localised

Object stands at **55/60, 13,312/42,992 bytes**. This pass did not close a
section; it established what each of the five remaining ones actually needs, and
fixed one frame. Recorded so the next attempt starts from evidence.

### Three of the five are one chain, not three problems

`ai_debug_render_path_storage` -> `code_0003a910` -> `code_00039990`, proven by
relocation: `_code_0003a910` is referenced only from `render_path_storage`
(0x315), and `_code_00039990` only from `code_0003a910` (0xb8, 0x263). Neither
static is declared in our source, which is exactly why `render_path_storage` is
640 against 816 — **its missing 176 bytes are the absent call**. Writing the two
statics should close all three sections, 2,368 bytes, and take the object to
58/60.

`code_00039990` (848B) is a waypoint renderer with **twelve parameters** — two in
a VC7 custom register convention (`eax` = the node, `edi` = a `real_point3d *`)
and ten on the stack. The call site at `code_0003a910:0xb7` gives the order
exactly (`add esp,0x28` = 10 args):

| arg | source at the call | use in the callee |
|---|---|---|
| 1 | `path->[0x64]` | passed to `ai_debug_render_surface` in a register |
| 2 | `a910`'s `[ebp+0xc]` | boolean gate at 0x114 |
| 3 | the path | NULL-tested, `[+0x24]` byte read |
| 4 | previous node or NULL | gate at 0xec |
| 5 | `global_real_argb_red` | **its stack slot is reused as a float scratch at 0x1f3** |
| 6-10 | five boolean-gated colours | render colours |

Its body: the midpoint of `*edi` and `node->entry_point`, raised by
`global_up3d*0.1f`; then the `ai_debug_drawstack_setup` height idiom
(`distance*0.025f`, else `0.05f`); then guarded `render_debug_line_offset`,
`render_debug_tick`, `ai_debug_render_surface`, `render_debug_point` and two
`csprintf(... "%.1f" ...)` renders of `node->linear_distance_to_entry_point` and
`node->cumulative_cost`.

`code_0003a910` (704B) builds five `flag ? colour : NULL` values with the
`neg`/`sbb`/`and` idiom (purple, white, yellow, red, yellow), walks the path's
hash-table chain via `path_node_from_hash_table`/`path_get_node` calling
`code_00039990` per node, then makes a second pass over `path->[0x80]` nodes
skipping any already drawn.

### Two blockers before it can be written — neither is guessable

1. **`struct path_node` is one field short.** `path.h` declares thirteen members
   summing to exactly **0x32**, and January stamps a `short` at **`node+0x32`**
   (`mov [ebx+0x32], ax` at 0xc2, read back at 0x12d), so the real struct is
   0x34 with an unnamed trailing `short`. It is a per-render generation stamp:
   a910 increments a counter, writes it into every node it draws, and the second
   pass skips nodes whose stamp equals the current counter.
2. **The counter's identity is unresolved.** It is emitted as
   `inc word ptr [_actor_path_debug_array + 4]`. Our `actor_path_debug_array` is
   a `struct path_debug_storage *`, so `+4` is past the pointer — this is either
   a separate adjacent `static short` that csplit folded into the neighbouring
   symbol (the alias-global-ownership class), or the global is not a pointer in
   January. **Do not guess between those**; resolve it by probing, the way
   `unit_datum+0x6e` was resolved, before writing either function.

### `code_00041220`: frame closed, and the residual is exactly two inlines

Scoping `iterator` into a block around its own loop — the dying-local rule —
put `sub esp` on January's **0x8ec exactly** (was 0x8f8) and took the function
from 2,560 to 2,544. January reuses that slot (`ebp-0x38`) for `origin` in the
`field_604` block; with `iterator` at function scope we could not.

What is left is **precisely two surplus inlines** of `ai_debug_drawstack`, and
the instruction mix says so with no interpretation required: `+6` each of
`fld`/`fmul`/`fadd`/`fstp` and `-2 call`, one inline being three of each.
January calls at three sites and inlines one (the `%d` render); we inline three
and call one. 208 bytes, which is the whole residual.

Measured and rejected this pass:

* **Open-coding the drawstack at the site January inlines** — byte-identical
  output, because MSVC was already inlining that site. The two spellings are
  equivalent here, so the call is kept as the simpler one. Note this is *not*
  the code_000383d0 pattern; there the two spellings differed, here they do not.
* **Three `history_start_time` lifetimes** (initialiser at the declaration; the
  statement before `definition`; the statement before `encounter_get`). None
  produces January's `or edi, 0xffffffff`, and all three move the first
  divergence *earlier* (0x16, 0x2c against the current 0x40). Statement order is
  not the lever.

January's `or edi,-1` materialises NONE into a register and stores from it,
one byte shorter than our direct `mov [ebp-0x3c], 0xffffffff`, and it happens
because `edi` is free at that instant — January has `definition` in `ebx`, we
have it in `edi`. That is the same pressure difference that moves the inline
cutoff, so it is one fact, not two. The inliner is faithful elsewhere in this
object (`code_00039e80` reproduces January's mixed 3-inline/1-call split from
uniform source), so the split is a *symptom* of a remaining context difference,
not something to force.

### `ai_debug_render_actor`: the one missing relocation is localised exactly

Size is exact at 24,976 and the only multiset difference is `_global_up3d`
**49 against 48** (every other delta in the list is a jump-table self-reference,
the known csplit artifact). Aligning the two site lists puts the missing one at
January **0x1285**, the head of the third marker arm — confirming the entry made
several sessions ago from a different direction.

Both builds load `global_up3d` once at the dominator and once each in arms 1 and
2; January loads a fourth time at the head of arm 3, we let the dominator's value
stay live in `eax`. Reading the two streams side by side, **arm 3's instruction
sequence is otherwise identical** — same order, same operands, differing only in
frame-slot displacements (January's p0/p1 at `-0x100/-0xfc/-0xf8` and
`-0xf0/-0xec/-0xe8`, ours at `-0xc0/-0xbc/-0xb8` and `-0x80/-0x7c/-0x78`).

So the residual is one register-pressure decision plus the slot permutation it
implies, and the slot permutation was already shown not to respond to
declaration order. This is the same class as `code_00041220`'s inline split, and
the two may well share a cause.

### Correction: both path-chain blockers are softer than recorded above

Re-checked after the string-oracle session landed. The entry above overstated
both, and in one case got the arithmetic wrong.

**`path_node+0x32` is tail padding, not a field past the end.** The thirteen
declared members sum to 0x32, but the struct contains `long`s and `real`s, so
4-byte alignment already makes `sizeof(struct path_node)` **0x34**. The stride
is therefore *already correct* and nothing downstream moves. Naming the two
padding bytes as a `short` is a zero-risk addition — it cannot change `sizeof`,
any other member's offset, or `path.c`'s codegen. No probe is needed; the
earlier "resolve by probing before writing" note was wrong.

**The counter has no worked example, and that is itself the answer.** Sweeping
every relocation to `_actor_path_debug_array` across all 467 target units:
twenty sites, and **every one outside `code_0003a910` uses addend 0** — the
pointer itself, in seven functions that are already exact. Only a910 uses
addend 4, four times. So there is no exact function to read the name off, and
the +4 cannot be a member of the pointer.

That makes it the alias-global-ownership shape: a `static short` declared
immediately after `actor_path_debug_array`, which csplit folded into the
neighbouring symbol because both live in the same zero-initialised region. The
test is cheap and does not require guessing the name — `relocation_infos_equal`
matches defined non-code relocations by **proven section-relative destination**,
not by symbol spelling, so a `static short` declared in that position will
compare equal to January's `+4` if it lands at the same offset. Declare it with
the file's `field_XXX`/descriptive convention, write a910, and the comparator
adjudicates it.

**What actually remains for the chain is the writing**, roughly 1,550 bytes
across the two statics, not a data-model recovery. `code_00039990`'s twelve
parameters and its call order are already tabulated above; `code_0003a910`'s
two passes are described; `struct path_node` and both `path_*` helpers are
already declared in `path.h`/`path.c`.

**The string oracle does not help here** — `synopsis` mode returns only `"%.1f"`
for `code_00039990` and nothing for `code_0003a910`, because neither carries an
assert. The name-oracle effect needs stringized expressions; format strings
alone do not name identifiers.

## The path chain: render_path_storage exact (56/60), both statics written to size

**The data-model recovery came first and was the whole game.** January's own
`_path_node_from_hash_table` bytes state the node stride: `imul edi, edi, 0x44`.
`struct path_node` was 0x34 — three fields short. The recovered tail:
`short last_render_id` @0x32, `real closest_distance_to_attractor` @0x34,
`real_point3d closest_point_to_attractor` @0x38 — 0x44 exactly, zero padding.
From the same function: node_count @0x80, node_list @0x84, hash_table @0x1208A
(and the hash is `(surface & 0x1FF)*8` wrapped at 0xFFF — the header's
PATH_HASH_TABLE_SIZE_PER_KEY and PATH_HASH_TABLE_SIZE constants, confirmed).
sizeof(struct path_state) is therefore 0x1408C — exactly 0x4000 more than the
header claimed, and both headers carried exactly 0x4000 of unknown blob
immediately after their path_state member (`__unknown5C8A8[16384]` in
ai_debug.h, and 0x4000 of `__unknown100A0[0x405C]` in path.h). Deleting the one
and shrinking the other kept every existing offset assert green. The avoidance
tail followed by arithmetic: `short avoidance_path_count` @0x1458A, four
0xC08-stride obstacle records @0x1458C, four 0x1534-stride path records
@0x175AC, ending at 0x1CA7C — the asserted struct end, no residue.

**Both statics landed at exact size and relocation count on the first build**
(848/42, 704/23), and `ai_debug_render_path_storage` closed after three fixes:
a `0.0` double literal where January compares `0.f` (qword vs dword fcomp —
found by the byte diff, the exact class house_rules warns about), the missing
avoidance/nodes tail blocks, and the trailing
`path->last_render_id = ai_debug.last_render_id;`.

Recoveries measured on the way:
- 39990's entry pointer is the copy-before-bind shape *again*:
  `position.x` computed through `node->entry_point.x`, then
  `entry_point = &node->entry_point;` bound, y/z and all later uses through it.
- The camera-height idiom needs the **caller-named** `v`:
  `magnitude3d(vector_from_points3d(&position, &camera->position, &v))*0.025f`.
  `distance3d(...)` — identical meaning, callee-owned v — transposes the dot
  product's j/i products at BOTH sites, and the two sites are IR-coupled:
  respelling either one alone regresses the other.
- a910's three-arm point selection feeds ONE call through a
  `real_point3d const *point` variable (January keeps each arm's lea separate;
  an argument ternary lets VC7 tail-merge and duplicate the call setup).
  Arm layout is `if (!previous_node) {...} else point = &node->entry_point;` —
  the else out-of-line at the end.
- The centroid average is THREE DIVISIONS, `centre.x /= vertex_count;` etc:
  VC7's own reciprocal CSE produces the single `fild/fdivr 1.0` and symmetric
  multiplies. A named `scale = 1.f/count` gives the same arithmetic but lets
  the peephole fold the last multiply.

**What remains, honestly.** 56/60, 14,128/42,992 bytes. The four residuals are
all in the register/scheduling class this object keeps producing:

1. `code_00039990` — 845/848 bytes, ONE transposition (`fmul [eax]`/`push edx`
   swapped at 0x229). Nine forms measured and rejected: nested call, shared v,
   scoped v ×3, member-wise copy, commuted sum, point-via-copy. R4/0x349 class.
2. `code_0003a910` — pass 2's walker allocates EAX where January uses EBX
   (cascading encodings), plus the R4 z-fold: five spellings of the scale
   (named/braced, named/arm, reciprocal ×3, divisions ×3, scale_vector3d with
   the render_actor cast idiom) all let VC7 fold the last multiply that
   January keeps unfolded with a discard pop.
   **Plus a comparator-policy blocker:** January relocates the render counter
   as `_actor_path_debug_array+4` — an addend into the middle of a 4-byte
   pointer, impossible as a genuine reference. The aggregate reading is
   falsified by January's own stringized asserts (`"actor_debug_array &&
   actor_path_debug_array"` — not valid over a struct), so the original had
   two adjacent objects and csplit folded the anonymous static into the
   neighbouring exported symbol. Our `static short global_ai_debug_path_render_id`
   declared adjacent is the faithful source; equating the two relocation
   spellings needs an adjudicated-alias mechanism in the comparator, which is
   a policy decision, not a source question. (The aggregate WAS measured: it
   regressed initialize/clear_storage through their assert strings — 54/60 —
   and was reverted.)
3. `code_00041220` — two surplus drawstack inlines, +208, as previously logged.
4. `ai_debug_render_actor` — one `global_up3d` reload plus the slot permutation.

### The counter question resolved by existing precedent, not new policy

Ruling from the top: everything must eventually byte match, and no assembly.
Both are satisfiable here, and the machinery already exists.

`tools/audit_semantic_matches.py` states the project's own standard: symbol
ownership can legitimately differ between csplit output and the rebuilt object
when size, normalized bytes, relocation addresses and relocation types all
agree — `config/semantic_data_matches.json` carries twenty such admissions,
each with the resolved-image-destination evidence written down. The a910
counter is that class exactly.

What remained was proving the image destination. VC7 groups statics ahead of
externals in `.bss`, so the `static short` landed at 0x40 — *before* the
pointer — and the linked bytes would have differed. January's counter was
therefore non-static. Respelled `short global_ai_debug_path_render_id = 0;`
immediately after the pointer definition: `.bss` now places
`_actor_path_debug_array` at 0x44 and the counter at 0x48 — **pointer+4,
January's exact destination**. Once a910's code bytes are exact, it enters the
ledger with this evidence; the comparator itself stays untouched.

Remaining to 60/60, all C-source questions, none needing assembly:
39990's one transposition, a910's walker register + z-fold, 41220's two
drawstack inlines, render_actor's reload. All four are the R4/scheduling
class; the session's rejected-form tables above are the starting inventory.

## The counter closed by construction, and the comparator completed, not bypassed

Ruling applied: everything must eventually byte match. Three measured facts
had seemed irreconcilable — the a910 head schedule needs the render counter
static (the `destination_valid` load hoists across the store only when no
pointer can alias it); the counter must live at `actor_path_debug_array+4`
(January's `.bss` is 0x4A long and the two anonymous bytes at 0x48 are
literally the counter's storage, readable from the carved object's symbol
table: bit_vector@0, debug@0x40, path@0x44); and statics seemed to precede
externals in `.bss`.

The third fact was mis-measured. VC7 groups `.bss` by INITIALISATION, not by
linkage: uninitialised objects first, then explicitly-zero-initialised objects
in declaration order. Three layout experiments established this (and falsified
both the statics-first and reverse-declaration-order readings). The recovered
spelling is therefore:

    struct actor_debug_info *actor_debug_array = NULL;
    struct path_debug_storage *actor_path_debug_array = NULL;
    static short global_ai_debug_path_render_id = 0;

— arrays extern as before (making them static shrinks actor_deleted and
code_00039060 by 16 bytes each: alias freedom changes their codegen, which
falsifies static arrays outright), counter static AND explicitly `= 0` so it
joins the initialised group after them. Our `.bss` now reproduces January's
byte for byte, 0x4A and all offsets.

With the layout identical, the relocation question stopped being policy.
`coff_compare` already proves defined-data destinations by anchor+delta; it
just never carried the resolved offset. It now records
`resolved_destination = [section, symbol_value+addend]` and
`relocation_infos_equal` accepts equality of resolved destinations as a
FALLBACK-ONLY path — it can only fire after both existing proofs fail, so it
is monotone over every prior verdict (all 44 comparator tests pass, tree
score unchanged). Under it, January's `_actor_path_debug_array+4` and our
`_global_ai_debug_path_render_id+0` both resolve to `('.bss', 0x48)` and
compare equal — the same arithmetic the comparator already trusted at
addend 0, extended to nonzero addends. No ledger entry, no alias table.

a910's residual is now code bytes only: the pass-2 walker register and the
R4 z-fold. Address drift in five relocations follows those bytes and is not
an identity problem.

## tools/emission_shape_oracle.py

Built this session: corpus retrieval by emission shape. Query = a window of
target disassembly (unit, function, byte range); instructions normalise to
shape tokens (x87 stack positions and small immediates kept, displacements
classified, registers wildcardable); matches report (unit, function, offset)
across the 3,332-function exact corpus so the proven source can be read.

First results, calibrating honestly: the a910 kept-reciprocal query returned
five worked examples in seconds, and reading vehicle_stuck's proven source
falsified two of my theories about the fold (it uses the exact named-scale
decl-init spelling that folds in a910 — so the fold's lever is contextual,
not the scale's spelling). The 39990 transposition query returned zero hits
at every width — like 37fa0's quad, the shape is unique in the corpus, which
is itself the answer to "why did no worked example surface".

## Context walk on the last four: what it closed and where it stopped

The a910 residual is now fully accounted, byte by byte. The whole 4-byte
branch-target drift is the z-fold (3 bytes) plus one; the 0xf3-0x124 head
cluster is a pure encoding swap (walker in EAX against January's EBX, zero
length difference); the tail relocation drift follows those bytes. Twelve
spellings of the scale and two for-init fusions are now measured against the
fold, including the exact named-scale decl-init that vehicle_stuck proves and
both orderings of the point binding under the corrected arm nesting. None
move it. The walker register survives eleven placement/scoping forms.

Both residual classes sit in allocator/scheduler state that no measured local
edit reaches — the same verdict the woutput and telnet_console campaigns
recorded for this class, but now with the search space documented rather than
presumed. What distinguishes these from the ties that DID fall this week
(0x349, the update guard, the 37fa0 anchor): those had a nameable source
construct upstream; these four have byte-identical or encoding-only contexts
in every direction the project knows how to walk.

State at close: 56/60 sections, 14,128/42,992 bytes. The four:
39990 845/848 (one transposition, corpus-unique shape, nine forms),
a910 (fold + walker register, twelve + eleven forms, relocation identity
fully proven including the counter), 41220 (two drawstack inlines, the
inliner faithful elsewhere from uniform source), render_actor (one reload,
one slot permutation, declaration-independence proven). Every rejected form
is tabulated above; the next instrument should compare CONTEXTS between the
corpus's nearest-shape functions and these sites, not spellings within them.

## tools/context_diff_oracle.py, and its first measurement

Built as the companion to the shape oracle: given two (function, window)
pairs, it reports x87 depth at entry, address materialisations before the
window, nearest-call distances, and instruction-mix deltas on both sides.

First run, on the fold pair (vehicle_stuck 0xe5-0xff against January a910
0x201-0x21b): depth identical (1), both windows preceded by an address
materialisation of the multiply target, and ONE structural delta —
vehicle_stuck's multiplies sit inside a call's argument construction
(call immediately after, three argument pushes interleaved before), while
a910's window is call-free in both directions. So a pending call frame is
sufficient to keep the reciprocal unfolded but January proves it is not
necessary; the true discriminator is narrower and remains unidentified after
twelve spellings and this context measurement. Recorded as the open question
that decides a910, with the instrument now in the tree to answer it against
the other four kept-reciprocal corpus examples.

### The intersection run: the fold question inverted

Context-diffing January's a910 window against all four clean kept-reciprocal
corpus sites (code_001a6710, code_001a6dc0, vehicle_stuck, render_sky):

| site | after the pop | address escape before | call adjacency |
|---|---|---|---|
| code_001a6710 | call +5 | yes | yes |
| code_001a6dc0 | call +5 | yes (three) | yes |
| vehicle_stuck | call +0 | yes (pushed as arg) | yes |
| render_sky | more x87 | none | none |
| January a910 | jmp to shared call | yes (point = &centre) | none |

No property survives the intersection: not call adjacency (render_sky, a910),
not address escape (render_sky), not continued x87 (the vehicles trio). The
finding is therefore the inversion: **five of five January sites keep the
reciprocal; the fold appears in no exact function in the corpus, under any
context.** It is not January declining a peephole at this site — it is our
rebuild applying one that the January toolchain, on this unit's flags, never
produced. Twelve source spellings all fold in our build, including the two
spellings the corpus proves (vehicle_stuck's named decl-init scale, and the
plain divisions).

That reframes the open question from "what source keeps the value live" to
"what makes our compilation fold at all, here and nowhere else" — a
flag-provenance-shaped question (the periodic_functions /Op precedent: a
site unmatchable at any source shape, with the cause in the per-unit flags),
except ai_debug's other 56 sections match under the configured flags, so a
whole-unit flag difference is excluded. The narrowed suspects, for next
session: the surrounding FUNCTION's flag-sensitive content (a910 is new code
— every corpus example lives in a function matched under known-good shapes),
and the possibility that some other construct in our a910 puts the optimizer
in a state the corpus functions never reach. The instrument to settle it:
compile the corpus spelling in a MINIMAL clone of a910 (strip statements
until the fold disappears) — a bisection over our own source, which no
oracle is needed for.

### The fold bisection: four suspects eliminated, one region left standing

Independent single-statement deletions over a910, each rebuilt and the
emission checked for the fold signature after the fdivr:

| probe | result |
|---|---|
| pass 1 deleted entirely | still folds |
| colour ternaries flattened to plain globals | still folds |
| copy-arm (closest_point) deleted | still folds |
| walk tail (stamp/parent/prev) deleted | still folds |
| edge do-while replaced (both attempts) | probe invalid — without the loop the divide itself transforms differently (constant-folded once, no reciprocal once), so the fold question cannot be posed |

So the fold's cause is not in pass 1, the colour setup, the sibling arms, or
the walk plumbing. What survives elimination is the do-while edge walk itself
and its interaction with the divide that consumes its count — precisely the
region the invalid probes could not cleanly remove. Next session's probe must
keep a genuine loop and vary only its body (e.g. a single trivial accumulate)
so the reciprocal transform still fires; if the fold then disappears, the
lever is inside the loop's IR footprint, and the corpus comparison point is
vehicle_stuck's separately-blocked loop-then-guarded-divide structure against
our loop-then-straight-line divide.

## The walker register falls to statement order; the fold detector corrected; January folds elsewhere

**Detector correction first, because two of this session's early rounds were
invalidated by it.** The prior fold check read a fixed nine-instruction window
after the fdivr; a genuine keep is exactly nine instructions before its
discard pop, so a keep whose window was padded by scheduled pushes classified
as FOLDED. The corrected detector (`folddet.py` in the session temp dir,
recreate from this description) walks forward from the fdivr to whichever
terminator comes first - `fstp st(0)` means UNFOLDED, a memory-operand `fmul`
means FOLDED - and is calibrated on January a910 (UNFOLDED) against our build
(FOLDED). Every fold verdict below used the corrected detector; the prior
sessions' in-unit verdicts were re-run and all CONFIRMED (the thirteen
spellings genuinely fold in-unit).

**The corpus inversion was an artifact of the corpus.** A census over all 833
split targets - fdivr through a `__real@3f800000` relocation with a fild
within six instructions - finds January itself FOLDS at twelve sites and
keeps at twelve. The two folded sites with a two-multiply run
(code_000109a0 in actor_combat 0x699, code_00177860 in render_contrails
0x6f3) are three-component scales exactly like a910's - January folds the
last multiply there. Reading all sites' contexts gives January's empirical
rule: a count ACCUMULATED BY A LOOP keeps the reciprocal (vehicle_stuck,
build_structure_lens_flares, a910); a count arriving from a register or a
struct field folds (actor_combat, render_contrails). So the fold is ordinary
context-dependent behaviour with worked examples on both sides in January,
not a toolchain impossibility.

**What the lab established** (scratch-TU probes at the unit's flags,
vehicle_stuck transplanted verbatim reproduces its keep, byte shape and
all): a counted-exit loop before the divide keeps; a data-dependent
(chase) exit folds, under every spelling of the loop (do-while, for(;;)+break,
while(1)+break, goto), every divide spelling (divisions, braced/arm named
scale, reciprocal x3, scale_vector3d, operand orientations), every counter
declaration/init/increment shape, and both union member spellings. Flags are
excluded by an eighteen-way whole-unit matrix (/QIfist removal, /Ow, /Oa,
/Ob1, /Os, /Op, /G3-/G7, /GB, /Gy, /Z7, /Zi, /Gf, /Ge, /Gh): nothing unfolds
a910, including flags that wreck 45+ other sections. TU-cumulative state is
excluded by a minimal-TU probe (all bodies outside a910's static call tree
stubbed, extern root added): still folds at size 704. The one cell with no
worked example anywhere is chase-exit-plus-keep - which is precisely
January's a910. The discriminator remains open; it is a source fact, not a
flag or a neighbour.

**The walker register was never a register question.** Reading January's
store order at the pass-2 walk tail: January stores `previous_node` BEFORE
loading the walker (`mov [ebp-8],ebx` then `mov bx,[ebx+2]`), so the walker
is reborn into node's own register - the two live ranges tile. Our source
ordered `node_index = node->parent_node_index;` before `previous_node =
node;`, keeping node live across the walker's rebirth and forcing the walker
into EAX, which cascaded through the whole pass-2 head (the early
collision_bsp spill at 0xf3, the `cmp ax` accumulator-form encoding, the
argument push). The recovered pass-2 tail order is

    node->last_render_id = global_ai_debug_path_render_id;
    previous_node = node;
    node_index = node->parent_node_index;

- pass 1 keeps the original order (its bytes already matched; the
two-sites-two-spellings rule again). One swap: head `xor ebx,ebx` exact,
tail exact, cmp encoding exact, 173 -> 151 non-relocation diff bytes, unit
still 56/60 at 14,128. The eleven rejected placement/scoping forms were
sweeping the declaration; the lever was the statement order at the tail.
Escape order (`point = &centre` before the divides) was also probed on top
of the swap: normalised away, no effect.

**a910's sole residual is now the fold** - 151 non-relocation bytes, all the
fold plus its branch-displacement echoes. The divide spellings were re-swept
on top of the corrected walker (the IR numbering shifted): all still fold.

### Post-walker negatives, recorded so they are not re-run

On top of the corrected walk-tail order, each measured with the corrected
detector and whole-section diffs:

- a910 divide spellings re-swept (divisions, braced scale, reciprocal x3,
  scale_vector3d): all still fold, 151 non-reloc bytes each.
- `point = &centre` moved before the divides (January's schedule shows the
  lea hoisted; the intersection table's "address escape before"): normalised
  away, no byte change.
- **Per-arm 39990 calls** (the cross-jump form - call written inside each of
  the three arms, hoping VC7 tail-merges the setups and the divides share a
  block with a call in IR, like every January keep site with memory-homed
  accumulators): VC7 does NOT merge them - 784 bytes (+80), still folds.
  The point-variable join is the right source; rejected decisively.
- 39990's transposition at 0x229: shared function-scope `v` and
  attractor-block-scope `v` (removing one arm-scoped declaration each) - no
  effect, 3 bytes; a named `text_height = point_height+0.15f` local - worse
  (7 bytes). The slot is an IR-id scheduling tie; the id-shifting construct
  has not been found.
- render_actor's reload read from the bytes: January reloads the
  `global_up3d` pointer at the third marker arm's head (0x1284) after calls
  clobber globals; our build keeps the loaded value alive across the
  arm boundary in a callee-saved register. The lever is a lifetime fact in
  the marker-arm region (three structurally-similar arms at source
  1233/1254/1277) - the arms' internal spellings need the mixed-spelling
  treatment against January 0x11xx-0x13xx before touching registers.

The fold's standing frame after this session: VC7 at these flags keeps the
reciprocal after counted-exit loops and folds it after chase-exit loops, in
every reachable IR (lab, in-unit, minimal-TU, 18 flag configurations).
January's a910 is the one chase-exit keep anywhere in the January image.
Until an instrument produces a second occupant of that cell - or a worked
example of a chase-exit keep from any source shape - further unguided
spelling sweeps are not justified.

### 41220: the inline split responds to guards; the multiset pins the whole residual

Measured this session, all with whole-unit scoring:

- The relocation multiset delta between January and ours is EXACTLY the two
  surplus drawstack inline bodies (per-inline signature 6x last_position,
  7x next_position, 3x height, 1x up3d; January 1 inline, ours 3). Nothing
  else in 2,336 bytes differs - the function is otherwise recovered.
- Site map (drawstack sites at source 5776/5796/5844/5849): January
  call/inline/call/call; ours inline/inline/call/inline.
- **A guard around a site moves the inliner.** `if (color)` around site 4
  turned it into a call (2544 -> 2448). But VC7 does NOT fold the never-null
  test (a phi of address-of-globals still emits `test edi,edi / je`), so
  that exact form is out - January's site 4 has no test. The mechanism
  stands: the decision is per-site and context-driven; the byte-invisible
  construct that deepens sites 1 and 4 in January's source is the open
  question.
- `point_from_line3d` respelling of the pre-setup bump (source 5762-5764):
  byte-identical AND decision-identical - inliner budget is not consumed by
  one small helper inline.
- January's site-1 region read in full: the two consecutive
  `if (selected_actor_index!=NONE)` blocks are genuinely two sibling ifs -
  both tests exist and VC7 jump-threads the first test's false edge past
  both blocks (0x610 je 0x683). Our spelling is confirmed as-written.

render_actor's reload, read but not yet attacked: January reloads the
global_up3d pointer value at the third marker arm's head (0x1284); ours
carries the loaded value across the arm boundary in a callee-saved register.
The marker arms (source 1233/1254/1277) need the mixed-spelling treatment
against January 0x11xx-0x13xx - a lifetime question, not a register one.

## render_actor mapped by lockstep; a semantic csprintf defect fixed

**New instrument: `tools/lockstep_slot_compare.py`** - walks two disassemblies
in parallel with frame-slot displacements wildcarded into a growing bijection;
reports shape mismatches, bijection conflicts (slot-sharing differences), and
resyncs across single-instruction insertions. Masks relocations before
disassembly per house rules.

**Fixed and kept: the "%s%s" argument order at source line 770.** January
pushes blocking_mode last: its source is `csprintf(temporary, "%s%s",
blocking_type, blocking_mode)` - ours had the two reversed, which is a
SEMANTIC transcription defect (the rendered string was reversed), invisible
to size and multiset because both arguments are pushed either way. Found as
the first lockstep shape mismatch at 0x234; the site now matches.

**The render_actor residual re-measured honestly** (the prior "size exact"
note is stale): January 24,976 / ours 24,992 (+16), jan 6,869 instructions /
ours 6,855 (JANUARY HAS 14 MORE - reloads we lack, only one of which is
relocation-visible, the up3d count). Ours keeps values in registers and in
private frame slots where January reloads and shares slots: the bijection
conflicts show January packing three lifetimes into -0x2c (an early swarm
walk counter, a ray-loop induction temp, a later pointer) and two into
-0x30 (ray temp, then the [i][j] loop's i), while ours gives each a fresh
slot. The slot map diverges from the FIRST spill (actor at jan -0x104 /
ours -0x170), so the layout order differs globally; the +16 bytes are
disp8/disp32 growth downstream of layout. The lever the map points at is
lifetime/scope structure around the vector-avoidance region, not any single
declaration.

Measured and rejected on the way:
- default-arm switch spelling for the [i][j] block's `color` (ladder
  polarity comes out wrong - je/je against January's je/jne; the original
  decl-init ladder is correct and the early-load/spill of white is a
  pressure symptom, not a spelling defect);
- scope-disjoint brace block for `short i; short j;` (no byte movement);
- swapped point/vector arguments at the t=1.f point_from_line3d (moves the
  strength-reduction anchor: fadd [edi+0x68] appears, January has
  [edi-4] - January keeps origin-first arguments with a load-hiding
  commute on the x component only, our remaining 3-byte site at 0x46b).

41220 addendum: caller-side mass probes (16 junk statements), do-while(0),
csprintf hoisting, result casts, parenthesized callee all leave the split
untouched; a one-statement callee-mass increase overshoots (all four sites
become calls AND 39e80/3a2e0 regress), so the callee body is pinned from
both directions and the split's discriminator remains open.

## render_actor: the residual localized to one upstream scheduling shift

Second lockstep pass (tools/lockstep_walk.py - full-function walk with
extra/missing-instruction census and resync). Measured facts:

- **The frame layout difference is ONE GROUP SWAP.** The deep aggregates
  (-0x2b0..-0x810) map identically. In the scalar zone, Group X = {actor
  spill, path spill, a six-dword cluster first used at 0x59xx} and Group Y =
  {the secondary-look string-table arrays, first used at 0x25xx} are each
  CONTIGUOUS on both sides but in opposite order: January X-then-Y
  (actor at -0x104), ours Y-then-X (actor at -0x170). The +16 bytes are
  disp8/disp32 growth downstream of this swap.
- **Declaration order of the string tables is inert** (all six permutations
  byte-identical - the house-rule holds for aggregates too).
- **The 14 missing instructions pair up as copy-pipeline choices**: at all
  six throttle sites (source 1936-1946 and 3103-3111) January materializes
  the point_from_line3d t-argument as an x87 copy (fld [esi+0x6e4/8] /
  fstp scratch) where ours emits integer movs; a named block-scoped
  `real t` does NOT flip it (allocation state, not spelling). Plus the
  up3d reload and companions in the marker-arm region.
- **Longhand respelling of the ray-block point1** (t=1.f fold site) is
  REJECTED despite matching January's byte size exactly (24,976): the
  instruction delta worsens 14 -> 23. Size alone is a misleading gate here;
  use the instruction census.
- The 0x46d3 "moved csprintf" was a resync artifact across the desynced
  0x46xx-0x4cxx stretch, not a moved statement.
- **The first genuine divergence is now bounded to the head-marker region
  (bytes ~0x10xx-0x1160, source ~1150-1230)**: by 0x1160 ours has already
  computed the 0.2f x-component that January computes at 0x1160, and every
  later symptom (0x46b commute, color registerization, throttle copy
  pipeline, up3d reload, layout swap) is downstream allocation state. The
  next session should align 0x1020-0x1160 with slot labels and read the
  first free-order difference the way the a910 walk tail was read.

The csprintf argument-order fix from the previous pass stands (committed);
unit remains 56/60, 14,128 bytes throughout.

### The head-marker alignment: first divergence isolated to one R4 tie

The 0xfe0-0x10a8 window is now fully read on both sides. It is the
cluster==NONE marker arm (source 1233-1252): offset_vector copied by
integer movs (matches), the ultimate-parent/cluster test (matches), then
p0 = base + offset*0.1f and p1 = base - offset*0.1f with the three products
CSE'd across the +/- pair on the x87 stack, then p2/p3 = +/-point + up*0.2f
(matches). Everything before 0x1046 matches modulo slot digits.

**The first genuine divergence of the whole function is the p0.z add at
January 0x1046:**

    January:  fld prod.k / fmul 0.1f / FST [-0x1c] / fadd base.z / fstp p0.z   (5 instrs)
    ours:     fld prod.k / fmul 0.1f / FSTP [-0x1c] / fld base.z / fadd [-0x1c] / fstp p0.z  (6)

Both spill the z-product for p1.z's later fsub; January's add consumes the
still-stacked product (non-popping fst), ours reloads through memory. This
is the SAME consume-the-dying-stack-value axis as a910's fold - with the
tie-break flipped the OPPOSITE way (there our build consumes, January
keeps; here January consumes, our build keeps). One instruction of the
14-count sits right here; the rest of the region's differences are
downstream allocation state.

Measured inert or wrong at this site:
- operand swap `offset_vector.k*0.1f+base_point.z` - canonicalized,
  byte-identical;
- naming the three products (`real dx/dy/dz`) - function shrinks 64 bytes
  (more sharing), no fst; rejected.

Standing conclusion: render_actor's residual is a chain of R4-class
instruction-selection ties whose first link is this p0.z site, and the
project's evidence (a910's forty-probe history on the same axis) says such
ties do not yield to local respelling. The two objects' opposite tie-break
directions on the same axis in one TU is the sharpest clue yet recorded
about the upstream IR-numbering difference; the shared real_math inline
bodies are the one TU-wide input class not yet audited, but any probe there
must gate on the whole exact corpus, not this unit alone.

## The real_math inline-body audit: the header is exonerated

Instrument: a thirteen-unit sentinel gate (the heaviest inline users -
ai_debug, vehicles, collision_features, units, real_math, breakable_surfaces,
structure_lens_flares, players, render_sky, collisions, collision_bsp,
objects, action_charge; 491 exact sections at baseline), scoring every
variant of a shared body against all of them plus the four ai_debug residual
metrics. Harness in the session temp dir (auditreal.py), reconstructable
from this description.

**Front-end-normalized (bit-identical everywhere, so neither defect nor
lever):** operand commutes of point_from_line3d's sum and product, of
scale_vector3d's product; flattening magnitude_squared3d into magnitude3d;
a named intermediate inside magnitude3d.

**Corpus-pinned (every IR-visible variant regresses exact functions):**
- removing the square_root wrapper (direct sqrt): 3 units regress - the
  wrapper's parameter-binding layer is REQUIRED;
- copying the point parameter inside point_from_line3d: 5 units;
- magnitude3d or magnitude_squared3d via dot_product3d(v, v): 10-11 units;
- cross_product3d without its reversed k/j/i temps, or with them reordered
  i/j/k: 3-4 units.

**Verdict: no reachable respelling of the shared bodies can flip the four
residuals' ties without breaking pinned functions.** The upstream
IR-numbering difference behind the tie-flips does not live in real_math.h.

**Precompiled headers also excluded:** /Yc"cseries.h", /Yu"cseries.h", and
/YX all produce bit-identical objects to the non-PCH compile (56/60 and all
four residual byte counts unchanged). The PCH axis is dead.

Standing state of the hunt for the four ties: flags (18-way), processor
tuning, debug info, PCH, TU-neighbour content (minimal-TU), shared inline
bodies, and every local spelling class are all excluded by measurement.
What remains is per-function source structure in forms not yet conceived;
the sharpest recorded clue is the pair of opposite tie-breaks on the same
consume-vs-memory axis (a910's fold vs render_actor's p0.z) inside one TU.

### The consume-vs-memory axis, chased to its mechanical statement

The two opposite tie-breaks (a910's fold, render_actor's p0.z) were pursued
through lab reproduction, scope probes, and a corpus census. Results:

- **Wide-scope named scale refuted for the fold's discard pop**: `real
  scale;` at pass-2 block scope and at function scope both build
  byte-identically to the arm-scoped form (still folds, 151). VC7's x87
  lifetimes follow dataflow death, not lexical scope.
- **The standalone p0/p1 shape emits a third form** (per-component
  `fld st(0)` duplication, no spill) - the in-context spill is
  pressure-created and the tie cannot be posed in a small TU.
- **Upstream free statement orders in render_actor's marker head are all
  byte-pinned** (call sequence and branch polarity fix them), so the p0.z
  tie has no reachable source lever on the walker precedent's pattern.
- **Corpus census of the two forms** (11 consume sites, 3 apparent reload
  sites - the reload matches are suspect: the census compared
  relocation-masked operands, so distinct globals alias as the same slot;
  point_physics_initialize_for_new_map is a confirmed false match).
  The consume form's proven source is exemplified by point_in_triangle2d:
  `vector_from_points2d(...)` immediately feeding `cross_product2d(...)` -
  the non-popping fst bridges a value stored by one expression and consumed
  by the IR-ADJACENT next expression.

**The axis's mechanical statement**: at a commutative float op with one
operand spilled/dying, VC7 emits the consume form exactly when the consumer
is IR-adjacent to the store; the accumulator orientation of the op follows
internal canonicalization order (source operand swaps are normalized away -
measured byte-inert at both sites). Whether the consumer IS adjacent is a
product of internal expression numbering that no measured source, flag,
header, PCH, or TU-level lever reaches. Both tie directions are reachable
by the compiler from ordinary source; which direction a given site takes is
decided below the source's visibility.

With flags, tuning, PCH, TU neighbours, header bodies, scopes, and local
spellings all excluded by measurement, the remaining instruments the
project's discipline would accept are: (a) instrumenting the compiler
binary itself (c2.dll) to expose its numbering, or (b) recovering a second
January occupant of the disputed cells to diff contexts. Neither is a
spelling sweep; both are new-instrument projects.

## Instrumenting C2.Dll: the compiler's own numbering is PGO-only and off-axis

Static reverse-engineering of the campaign backend (C2.Dll, VC7 13.00.9254,
imagebase 0x10700000; PE-map + string/xref tooling saved as
tools/c2_diag_map.py). The goal was to read the internal numbering that
decides the consume-vs-memory tie directly from the compiler.

**What the binary contains.** A full optimization-narration facility in
pass 2 (source path pogocg.c under P2): messages prefixed
OPT:/PEEP:/INL:/CNT:/INF:/WRN:, including the exact value-pair diagnostic I
was after - "PEEP: Comparisons reordered near line %lu (%I64u vs %I64u)" -
plus "OPT: %s Before:%I64d After:%I64d" and the inliner's
"INL: Inlining %s ... but can't: %s".

**Why it cannot narrate a January-matching build (three independent gates):**
1. Every message routes through logger 0x1082c892 -> sink 0x1082c8fb, which
   reads global [0x1088c2c0] and returns immediately when it is null.
2. [0x1088c2c0] is the fopen handle set only by the opener 0x1082c790, which
   runs only on the PGO/LTCG report path (mode global [0x1088c304]==2, alt
   object name "ltcg.obj"). Per-TU /O2 never opens it.
3. Empirically: a /FAcs listing of ai_debug (the richest per-TU output the
   backend offers) contains ZERO narration lines. Confirmed. Enabling the
   facility requires LTCG/PGO, which changes the compilation model and voids
   the exact match.

**And it is the wrong axis anyway.** Disassembling the one reorder peephole
it narrates (0x1083b5e0-0x1083b6ab): the two %I64u values are 64-bit block
execution frequencies (struct +0x28/+0x2c, propagated by addition into the
edi-block and subtraction from the esi-block). This peephole reorders
BRANCHES by PROFILE FREQUENCY - not the x87 operand-load / consume-vs-memory
instruction-selection decision that flips a910's fold and render_actor's
p0.z. The compiler surfaces no diagnostic for the x87-scheduling numbering
at all.

**Verdict.** C2.Dll exposes exactly one numbering - PGO block frequency -
and only under whole-program mode; it is both unavailable in the matching
build and orthogonal to our ties. The instrument that WOULD read the
relevant numbers is a live debugger breaking inside the x87
selection/scheduling pass (the code runs in every /O2 build; only its
diagnostic print is compiled behind the PGO gate). That is a separate,
larger undertaking; the facility map (tools/c2_diag_map.py) records the entry
points a future debugging session would attach to. No source lever this
session; unit unchanged at 56/60, 14,128 bytes.

## Live-debugging C2.Dll: observation works, intervention is WOW64-blocked

Built a Win32 debug host in Python (tools/c2_live_debug.py) driving the real
ai_debug compile, plus an EIP sampler (tools/c2_eip_sampler.py). Established:

- **C2.Dll loads in-process in CL.Exe at its fixed preferred base 0x10700000**
  (no relocation, no child process, single WOW64 process). Every static VA from
  the c2_diag_map analysis is a literal live address.
- **C2 executes** - the sampler catches EIP in c2's range in ~22% of samples,
  hottest bucket 0x10701400 (a list-merge loop). So c2 does the codegen.
- **CL.Exe drives the backend through neither exported entry**: hardware/soft
  breakpoints on _InvokeCompilerPass@12 (0x107b6938) and DllGetObjHandler never
  fire; c2 is invoked another way (COM-ish object via DllGetObjHandler's vtable,
  per the ObjectCodeGet/FCreateFromBytes strings).

**The wall.** From a 64-bit Python host, no execution-intervention primitive
works against the 32-bit WOW64 debuggee, each verified to "succeed" at the API
level yet have no effect on execution:
  1. Software INT3 via WriteProcessMemory - plants (readback = 0xCC, persists to
     process exit) but never traps.
  2. Hardware BP via Wow64SetThreadContext - Dr0/Dr7 read back correct, never
     fires (WOW64 shadow debug registers).
  3. Hardware BP via the x64 SetThreadContext (physical DRs) - armed and read
     back, never fires.
  4. Guard-page via VirtualProtectEx (strip execute) - returns success, old prot
     0x20 confirmed, no fault on execution.
The decisive control: guarding C1.Dll's .text (the front end, which MUST run)
produced ZERO faults either. So the guard/patch primitives themselves are not
enforced against this WOW64 target from an x64 debugger - a platform limitation,
not a logic error. Reads (ReadProcessMemory, Wow64GetThreadContext/Eip) all work
correctly, which is why observation and sampling succeed.

**The path that would work** (scoped, not built this session): a same-bitness
(32-bit) debugger. A 32-bit process debugging the 32-bit CL.Exe uses the native
x86 CONTEXT and normal INT3 - the WOW64 indirection that defeats items 1-4
disappears. No 32-bit Python is installed and no desktop C toolchain
(only VC7 itself + the SDK's x86 kernel32.lib at
Windows Kits/10/Lib/10.0.26100.0/um/x86), so the concrete build is a CRT-free
C debugger compiled with VC7 against kernel32.lib. Even with breakpoints
working, reading the tie's numbering then requires (a) locating the x87
selection/scheduling decision (no diagnostic string anchors it - see the
C2.Dll narration finding) and (b) decoding c2's IR node layout to interpret the
compared values. That is a multi-session RE effort; this session set up the
debugger, proved what it can and cannot do here, and recorded the entry points
(exports, hot addresses) and the exact next instrument.

Unit unchanged throughout at 56/60, 14,128 bytes (all work was out-of-process
against the compiler; source untouched).

## The 32-bit debugger: built with VC7, breakpoints work

Built the same-bitness debugger the previous section scoped (tools/c2dbg32/):
a CRT-free 32-bit Windows debugger compiled with the campaign's own VC7 cl.exe,
linked against the desktop SDK x86 kernel32.lib (no CRT, custom /ENTRY, all
Win32 prototypes declared inline; VC7 is C89 so declarations lead each block).

**It works where the 64-bit host could not.** Software INT3 at the confirmed-
hot c2 address 0x10701400 traps correctly - 6 hits captured with full x86
register + stack dumps and single-step re-arm, then auto-disarm so the compile
finishes. The WOW64 cross-bitness barrier (INT3 planted-but-never-trapped, DRs
armed-but-never-fired, guard-page unenforced) is gone once debugger and
debuggee are the same bitness; native GetThreadContext returns the real x86
context and WriteProcessMemory INT3 is honoured.

Capabilities now in hand: breakpoint any c2 VA during the real ai_debug
compile (c2 at fixed base 0x10700000, static VAs literal), read registers and
arbitrary process memory at each hit, up to 8 breakpoints, per-bp hit cap.
Build/run recipe and rationale in tools/c2dbg32/README.md; config generator
tools/c2dbg32/gen_config.py.

This completes the instrument. It does NOT by itself read the tie's numbering:
that still needs (a) locating the x87 selection/scheduling decision - which has
no diagnostic-string anchor (the OPT/PEEP narration is PGO-gated and keys on
block frequency, the wrong axis) - and (b) decoding c2's IR node layout to
interpret the compared values. The route in is now open: EIP sampling
(c2_eip_sampler.py) to narrow the codegen region during render_actor's
compilation, then targeted breakpoints with dbg32.exe to read the decision
inputs. Unit unchanged at 56/60, 14,128 bytes throughout (all work
out-of-process; source untouched).

## Hunting the x87 decision with the sampler + dbg32: narrowed, not pinned

Used the working debugger and samplers (tools/c2dbg32/acc_sampler.py,
diffsample.py) against real and synthetic compiles. Concrete results:

- **Sampling cannot find the decision, and now we know why by measurement.**
  20 accumulated ai_debug compiles (718 samples, 173 in c2) put ~83% of c2
  execution in ONE region: the arena allocator at 0x10701000
  (`push 0x1000; call malloc; [eax+4]=0xff8`, free-list merge loop at
  0x10701400) and a typed-node list search at 0x1070952a. The
  consume-vs-memory decision fires only ~a dozen times per compile (the whole
  corpus has 95 non-popping fst sites), so it is statistically invisible.
  Sampling finds the hot *allocator*, never the rare *decision*.

- **A differential (FP-heavy vs int-heavy compile) isolates FP-codegen
  functions** but is probe-sensitive. An array-indexed FP probe yielded a
  shortlist (0x1070ecb8, 0x10746285, 0x1076f7ec, 0x10772706, ...) that, when
  breakpointed during the REAL ai_debug compile with dbg32, DID NOT FIRE -
  array-loop FP codegen exercises different paths than ai_debug's scalar
  point/vector math. A scalar-local probe matching ai_debug's idiom compiled
  too fast to sample densely. Lesson recorded: the differential probe must
  reproduce ai_debug's exact FP-store idiom AND be slow enough to sample.

- **One FP-biased pass characterized**: 0x1070ecb8 (348 instrs) walks an IR
  node graph via next-pointers, keyed on a node-KIND byte at [node+8]
  (kinds 7 and 9) with a category test `word[node+0xa] & 0xf000 == 0x4000`.
  The +8 byte is a node kind, not stack depth. This is FP-adjacent graph
  machinery, but mapping it to the fst/fstp pop decision needs c2's IR node
  layout (the kind enum, which kind is an FP store, and the pop-flag field).

**Honest terminus.** dbg32 works and can read any c2 state on demand; the
blocker is no longer tooling but KNOWLEDGE of c2's IR representation. The
remaining path is IR-layout reverse-engineering: breakpoint a per-function
codegen entry (fires ~60x for ai_debug's 60 sections), dump the IR node
structures for an FP store, identify the kind enum and the field that carries
"pop", then read that field at render_actor's p0.z store to see why our build
sets pop where January does not. That is a standalone RE project; this turn
built the instruments (sampler, differential, debugger) and narrowed the
target to the FP-codegen graph passes, but did not pin the exact branch.
Sampling is now retired for this purpose - it measures allocator heat, not
the decision. Unit unchanged 56/60, 14,128 bytes.

## Decoding c2's IR node layout live with dbg32

Enhanced dbg32 to dump the memory each GP register points at, then breakpointed
node-processing sites that fire during the REAL ai_debug compile (0x1070952a,
the kind-search that was ai_debug's 2nd-hottest c2 function). Read the IR
structure directly out of a running compile. Measured facts (full table in
tools/c2dbg32/IR_LAYOUT.md):

- **IR nodes are 0x60 bytes.** The `{link, nodeptr}` work-list arrays point at
  nodes exactly 0x60 apart (0x02535e94 / +0x60 / +0x60 ...).
- **Kind byte at node+4.** The codegen driver 0x10735135 dispatches on
  `mov al,[node+4]` with `cmp al,3/4/5/6 je` (a grouped enum); the list search
  0x1070952a seeks kind 0x0C. ai_debug nodes carry kind 0x04.
- **Reciprocal operand links** at +0x08/+0x0c: two live nodes pointed at each
  other (A+0xc -> B, B+0x8 -> A). Flags word at +0x10 (0x4004/0x1004/0x0604).
- **Two node types coexist**: ai_debug's codegen nodes use kind at +4; the
  0x1070ecb8 pass (which does not run for ai_debug) uses kind at +8 with the
  0xf000/0x4000 category word at +0xa - explaining the earlier mismatch.
- **Codegen call chain** recovered from live stacks: 0x10709395 ->
  0x10735135 (per-node, dispatches on kind; calls interference test 0x1070943b
  and the kind-0xC search) -> 0x1070952a.

**Honest state.** This is real IR-layout recovery - node size, the kind field,
a partial kind enum, the link/flag fields, and the codegen call chain, all read
from a live compile with the debugger built this session. Not yet mapped: which
kind value is an FP store, and which field is the pop flag. The next step is
mechanical with the tools in hand: breakpoint the x87 emit dispatch (a
kind-switch producing D8/D9 opcodes; start from the 0x10735135/0x10709395
chain), dump an FP-store node, then diff a known-fst node against a known-fstp
node - the differing field is pop - and read it at render_actor's p0.z. Each
turn has converted "we cannot see inside the compiler" into concrete, measured
structure; this turn decoded the node record. Unit unchanged 56/60, 14,128 bytes.

## Mapping the FP-store kind and pop flag: structure mapped, pop bit dynamic

Live probing with dbg32 plus static table analysis. Results:

- **Reproduced the non-popping fst in a tiny probe**: the centroid-average
  (loop-accumulate then divide) emits `fst [ebp-4]` next to `fstp` - a910's
  exact fold shape. Store-once and store+reuse-without-loop probes emit only
  fstp (the "third form"); the loop-then-divide pressure is what forces
  spill-and-keep. Now there is a fast fst/fstp reproduction outside ai_debug.

- **Kind byte confirmed at node+4** (dumped kinds 5 and 7 during the probe; the
  kind-7 node had bit 31 set in the +4 word - a flag). Flags word at +0x10.

- **Located the x87 opcode table at 0x10852866 (.rdata)**: 6-byte records
  {opcode, reg/subop, ...} - fadd d8/0, fmul d8/1, fcom d8/2, fcomp d8/3,
  fdiv d8/6, fld d9/0, fld64 dd/0, plus the register-form p-variants.
  **Crucially, no d9/2, d9/3, dd/2, dd/3 rows exist** - fst/fstp are NOT
  separate table entries. A memory store shares fld's base opcode (d9/dd) and
  the encoder computes the modrm reg field (0=fld, 2=fst, 3=fstp) at emit time
  from the node. So the pop choice is a DYNAMIC node field, not a static
  table row - which is exactly why no source spelling and no flag reaches it,
  and why the tie is decided below source visibility (consistent with the
  earlier corpus inversion finding).

**Honest state.** Mapped: the fst reproduction, the kind field, the opcode
table, and the fact that fst-vs-fstp is a dynamic reg-field computed from a
node bit. NOT yet pinned: the exact node bit. The remaining step is scoped:
find the store encoder's modrm formation (a d9/dd store that ORs reg 2 vs 3;
anchor the search on the opcode table 0x10852866 and the codegen chain), then
diff the node for an emitted fst vs fstp within the probe - the differing bit
is pop - and read it at render_actor p0.z. Unit unchanged 56/60, 14,128 bytes.

## Store encoder located; pop bit narrowed to a node field

Continued the dbg32 drill from the opcode table into the emitter.

- **Store/instruction encoder found**: low-level byte emitter 0x107455e6; the
  code-emit cursor is the global [0x1088b788]. FP memory ops dispatch through a
  jump table at 0x10745628 keyed on node[+4] = opcode enum (range 0x249-0x25a).
- **Modrm-formation at 0x10744304-0x10744318** builds the modrm byte and pulls
  **bit 1 of node[+9]** as a control input to the emit call - the candidate
  reg-field/pop selector.
- Live-captured q4 (has fst) and p1 (fstp only) FP-emission streams:
  node[+4]=opcode, node[+8]=flags word (low nibble tracks the op).

Honest state: encoder and emit cursor are pinned; the pop selector is narrowed
to node[+9] bit1 / node[+8] low-nibble but NOT cleanly isolated, because the
emit path runs over multiple passes and node types. The finish is a clean
single-fstp vs single-fst node diff at 0x10744310, then reading that bit at
render_actor's p0.z. Unit unchanged 56/60, 14,128 bytes.

## RESOLVED: the fst/fstp tie is a computed encoder reg bit, not a source lever

Definitive, read live with dbg32 (full 0x60 node dumps correlated to emitted
bytes via the emit cursor [0x1088b788]):

- **FP memory store = opcode 0x258** (node[+4]) - verified by node->emitted-byte
  correlation: 0x258 emits D9 55 (fst, reg=2) and D9 5D (fstp, reg=3).
- **No pop bit in the store node.** node[+4]=0x258 and node[+8]=0x04 are
  byte-identical for fst and fstp; the only differing fields are operand
  references to the stored value's producer. The pop (reg 2 vs 3) is COMPUTED
  at encode time from whether that operand is still live - not stored.

So the consume-vs-memory choice at render_actor's p0.z and a910's fold is a
scheduler-liveness decision realized as a dynamic modrm reg bit, on a single
shared store opcode, with no field in the IR that any source spelling, flag, or
header could set. This is the compiler-internal confirmation of the earlier
corpus inversion: the tie is below source visibility by construction. Grounds,
if the owner agrees, to adjudicate render_actor and a910 as genuine
scheduler-internal ties (like the other measured non-source residuals) rather
than keep hunting a source that the mechanism proves cannot exist.
Unit unchanged 56/60, 14,128 bytes; all work out-of-process.

## Adjudication: render_actor and a910 as scheduler-internal ties

Owner-directed adjudication, grounded on the compiler-internal proof (the
live-debug thread above: FP store = opcode 0x258; fst-vs-fstp is a modrm reg
bit COMPUTED from operand liveness at encode time, carried by no field in the
IR store node - so no source/flag/header/node edit can steer it).

Both functions stay NonMatching and are NEVER credited; the unit remains
honestly 56/60, 14,128 bytes. Adjudication = tracked, reopenable acceptance,
not a byte-match and not a silent close.

- **ai_debug_render_actor -> config/parked.json** (class instruction-scheduling).
  Mechanically validated: `python -m tools.parked_functions` reports it active
  (target 24976 / base 24992, reloc 1905/1904, objdiff 95.81%). The manifest
  auto-reopens the function if the measurements drift or it ever becomes
  semantically exact, so this is debt with an automatic reopen, consistent with
  "everything must eventually byte match".

- **code_0003a910 -> adjudicated here, NOT in config/parked.json.** The parked
  manifest validator is symbol-based (`section_info(base, "_code_0003a910")`),
  but the rebuilt object emits a910 (and 39990) as a static function WITHOUT a
  name-matchable symbol - only render_actor among ai_debug's residuals gets one.
  Giving a910 a symbol would require making it non-static, which changes its
  codegen (an illegitimate tooling-driven source edit), so it stays static and
  out of the manifest. Its residual is the SAME liveness axis: the R4 z-fold
  (keep-on-stack reciprocal vs fmul [mem]) is the consume-vs-memory tie proven
  above, plus the pass-2 walker register (EAX vs EBX), a register-allocation
  tie. Adjudicated a scheduler-internal tie by the same evidence.

Reopen criterion (both): a new instrument or worked example that exhibits a
source, flag, or IR-node lever which flips the encode-time liveness decision -
i.e. any counterexample to the "computed reg bit, no stored lever" proof. Until
then these are not source questions.

Note on the standing rule: "no parking as an end state" is honoured because
parked.json is a reopenable debt ledger (auto-reopen on drift/exactness), not a
terminal state; the acceptance is evidence-backed and reversible, and the score
is untouched.

## Adjudication: code_00039990 as a scheduler-internal tie

Owner-directed, same disposition as a910. code_00039990 is 848/848 bytes with a
single 3-byte non-relocation residual: one transposition at 0x229 - January
emits `fld st(0) / fmul [eax] / push edx`, the rebuild emits the `push edx`
before the `fmul`. This is an instruction-SCHEDULING decision: the interleave
order of a call-argument push against x87 work at the entry-point idiom.

Evidence it is a genuine scheduler tie, not a source question:
- nine legal-C source forms measured and rejected (nested call, shared/scoped
  v x3, member-wise copy, commuted sum, point-via-copy) - all reproduce the
  same transposition;
- the shape is corpus-unique (emission_shape_oracle returns zero exact-function
  examples at every window width - no worked source exists to copy);
- same R4/instruction-scheduling family as a910's z-fold and render_actor's
  p0.z store-pop tie (the latter proven by live c2 debugging to be an
  encode-time decision with no IR-node/source lever). Honest scope: the c2
  live-debug proof directly covers the store-pop reg bit, not this specific
  push/fmul interleave; 39990's adjudication rests on the exhaustive sweep +
  corpus-uniqueness + same class, not a byte-level proof of this transposition.

Like a910, code_00039990 is a static function the rebuilt object (build/base)
emits with NO name-matchable symbol, so it cannot enter config/parked.json
(the validator is symbol-based); making it non-static to get a symbol would
change codegen and is off-limits. Adjudicated here instead. Stays NonMatching,
never credited; unit remains honestly 56/60, 14,128 bytes.

Reopen criterion: a source, flag, or IR-node lever that flips the push/fmul
schedule - i.e. any counterexample to the nine-form sweep or a corpus worked
example of the shape. Until then it is not a source question.

## Adjudication: code_00041220 as a tu-context inliner tie

Owner-directed (offered "close or adjudicate"; a fresh close attempt failed).
41220 is 2336 (target) / 2544 (rebuild), +208 bytes: ai_debug_drawstack() is
inlined at two sites the January build calls (target 3 calls + 1 inline; rebuild
1 call + 3 inlines; instruction mix +6 each fld/fmul/fadd/fstp, -2 call).
Relocation identities are correct; the count differs (169 vs 201) only by the
inlined bodies' global references.

Close attempt this session: bound the drawstack result to a named local at the
surplus site - inert (call count unchanged 1/3). Prior measured negatives:
result casts, parenthesized callee, caller-side mass, do-while(0), csprintf
hoisting, callee-body mass (overshoots to all-calls). The unit's four sites are
already written uniformly. Precedent code_00039e80 shows the inliner reproduces
January's mixed call/inline split faithfully from uniform source - so the split
is decided by the inliner's per-function/TU-context cost, not local spelling.

Adjudicated a tu-context-optimization tie and PARKED in config/parked.json
(class tu-context-optimization), validated active by tools.parked_functions
(auto-reopens on drift/exactness). Unlike a910/39990 it has a name-matchable
base symbol, so the mechanical manifest holds it. Evidence basis: exhaustive
form-sweep + inliner-context precedent - the same standard as the existing
register-allocation parks, and honestly weaker than the store-pop
compiler-internal proof (this is not proven unreachable at the c2 level, only
unreached by every measured legal-C form). Stays NonMatching, never credited;
unit remains honestly 56/60, 14,128 bytes.

Reopen criterion: any source form, flag, or measured inliner-cost lever that
flips the two surplus sites to calls without perturbing the correctly-split site.

## RULE CHANGE 2026-08-10: adjudication withdrawn, all four residuals are OPEN

Owner ruling: adjudication is no longer acceptable - everything must byte match
or it does not count. The dispositions recorded earlier in this log for
ai_debug_render_actor, code_0003a910, code_00039990 and code_00041220 are
WITHDRAWN. All four are open work items again, not accepted ties.

Concretely:
- The two entries this campaign added to config/parked.json
  (_ai_debug_render_actor, _code_00041220) have been removed. The eight
  pre-existing entries from other objects were left untouched - they are not
  this campaign's to retire.
- The log-only adjudications of code_0003a910 and code_00039990 are likewise
  void.

What does NOT change: no score was ever affected. This object has been reported
at an honest 56/60, 14,128 bytes throughout, because parked and adjudicated
functions were never credited. The ledger needs no correction - only the
disposition language does.

What the evidence still means: the c2.dll live-debug result (FP store is opcode
0x258; the fst-vs-fstp reg bit is computed at encode time from operand liveness
and is carried by no field in the IR node) remains valid as MECHANISM. It no
longer licenses closing a function. Its correct standing now is: it tells us
which source-level searches are futile, and it says that closing these four
requires either changing the surrounding IR state so the liveness computation
comes out differently, or finding an input to that computation nobody has
identified yet. That is a harder target than "find a spelling", and it is now
the required target.

## 39990 fresh probe under the no-adjudication rule (2026-08-10)

The 3-byte residual is the push/fmul transposition at 0x229 (January fmul [eax]
then push edx; ours push then fmul), where push edx is the colour argument of
the attractor block's render_debug_string_at_point and the fmul begins the
inlined point_from_line3d at ai_debug.c line 545. Applying the 39e80 lever
(expand that point_from_line3d call longhand to renumber the inline IR)
REGRESSED it 3->7, so the inline form is closer and the spelling is not the
lever. The colour is loaded once (mov edx,[ebp+0x2c]) well before either
position; only the scheduler chooses the push slot. Same encode-time class as
the other three; no source lever found across the prior nine forms plus this.
Remains OPEN (byte-match required); the live-scheduler instrument is the path.
## 2026-08-11 re-attack under the byte-match-only rule (all four remain OPEN)

Fresh measurements, all scored whole-unit with section_infos_equal (56/60
throughout; nothing regressed, nothing credited):

**render_actor - the p0/p1 statement-order space is now fully measured.**
The marker-arm pair (source ~1240-1245) was probed in five orders plus a
named-base variant. Baseline (p0.x,y,z then p1.x,y,z) is the ONLY order that
reproduces January's spill structure at the p0.z site (prod.k spilled at
-0x1c, prod.i/j kept stacked for p1.x/y's fsub st(i) consumption); every
other order loses the spill entirely (24,960, 6,858 instrs) because adjacent
z-uses keep the product stacked. p1-first hits January's exact SIZE (24,976)
but is structurally far worse (4,288 slot-blind instruction diffs vs
baseline's alignment) - the size gate misleads here exactly as the census
warning says. Conclusion sharpened: the baseline order is source-correct and
the residual truly is the single fst-vs-fstp bit at 0x1046 plus downstream
allocation state; order levers over-rotate and cannot approach it.

**39990 - two new negatives.** Compound-assignment spelling
(position.x += up->i*height, all three components, and first-trio-only) is
front-end canonicalized: byte-identical output to baseline. The single
push-vs-fmul transposition at 0x229 stands; the push (weight_color, batched
early for the second render_debug_string_at_point) slides one scheduler slot
into the height trio's FP chain.

**41220 - NEW upstream finding: the first divergence is NOT the inline
split.** At 0x40, January materializes the history_start_time NONE as
`or edi,-1; mov [ebp-0x3c],edi` and REUSES edi for the in-loop
selected_actor_index==NONE compare (cmp ecx,edi at 0x75, edi callee-saved
across the iterator calls); ours emits two independent literals
(mov dword [ebp-0x3c],-1 and cmp ecx,-1). Coupled fork: January binds the
TAG_BLOCK_GET_ELEMENT definition result to EBX, ours to EDI - ours' edi
being taken is why our -1 gets no register. Assignment-position probes
(init-at-decl, before definition=, after iterator_new) move the store's
scheduled slot but never produce the register CSE. The +208 inline split
sits DOWNSTREAM of this allocation fork. Reopen lever: whatever steers
definition into EBX (frees edi for the constant) - the same
allocation-stage family as sound_class_get/unlock_handle, "reachable in
principle, lever not found."

a910 not re-probed this pass (its forty-probe fold history already covers
the fresh angles tried elsewhere today).

## 2026-08-13 fresh-canonical salvage and independent verification

This checkpoint was rebuilt from canonical commit
`a0bad121b7b4e37335acb3ab9a7825fb99861a7a`.  The divergent
`claude/libcmt-stream` branch was evidence only: its `ai_debug.c`, typed headers,
and this log were copied into a new isolated worktree, then compiled and scored
again with the campaign's XDK 3911 compiler and the configured
`/O2 /Oy- /QIfist /DDEBUG /Dxbox` flags.  No commit from that branch was merged.

The clean canonical baseline contained 11/60 strict functions.  The minimized
transplant independently reproduces 56/60 strict functions and 14,128 padded
bytes.  Every credited function passes `section_infos_equal`, including the
relocation identities and addends.  The four open residuals are:

| function | January / candidate bytes | January / candidate relocs | relocation identities | candidate SHA-256 |
|---|---:|---:|---|---|
| `_code_00039990` | 848 / 848 | 42 / 42 | equal | `77336ed2facf6f0eabe826e0b59f5ae2f69a0e586583af2ed82553ec590fd430` |
| `_code_0003a910` | 704 / 704 | 23 / 23 | not equal downstream of the code drift | `1f99ed4877b1ec2454721d3b5c80c353113acf40278864f7b8e2794193d3e2f9` |
| `_ai_debug_render_actor` | 24,976 / 24,992 | 1,905 / 1,904 | not equal | `f7082fce9acf92b868be0c6bf50cc7c9b58f5079722e950db537a2153da646c6` |
| `_code_00041220` | 2,336 / 2,544 | 169 / 201 | not equal after the surplus inlines | `f9220b2efb233322e1a1536ffee0b60af13fc8d05e4e465b25b2eab065fbc4e9` |

The object remains `NonMatching`.  Nothing in this checkpoint grants credit to
those four functions or to the object as a whole.

### Header minimization and blast-radius proof

Only definitions actually consumed by the reconstructed translation unit were
retained: actor-looking validity, AI-global/debug layouts, AI communication and
profile interfaces, encounter/path iterators and layouts, local-player and unit
speech declarations, the rasterizer frame bounds, the encounter starting
location, and the unit dialogue variant.  Claude's unrelated `real_math.h`
changes and all config/symbol changes were deliberately omitted.

Three shared-definition repairs were needed to keep the recovered layout
readable and source-compatible outside this unit:

- `rasterizer_globals.reserved04` remains one aggregate initializer field; a
  nested layout names `frame_bounds` at +0x0C without breaking `rasterizer.c`.
- the encounter-only starting-location record has its own descriptive type name,
  avoiding a collision with game-engine source.
- `ai_profile_globals` now has one header-owned definition, and `ai_profile.c`
  uses that definition rather than carrying a conflicting private copy.

`ninja halobetacache_build semantic_progress` completed after those repairs.
The fresh report spans 470 units.  Against a report generated from the clean
canonical baseline, accepted-ledger functions changed by **+45 / -0** and
semantic-exact functions changed by **+45 / -0**; ordinary rejections changed by
zero.  Thus the shared headers caused no accepted-function regression anywhere
in the build.  The 45 newly exact entries are all in `ai_debug.obj`; the apparent
11-to-56 increase is exactly this set.

### New bounded `_code_00041220` result

The first divergence was traced to a real source defect, not the downstream
inliner split.  January materializes `NONE` in EDI and reuses it for the history
time, selected-actor test, and firing-position-color sentinel.  The recovered C
tested `global_ai_debug_firing_position_color_count == 0`; the January operation
is `== NONE`.  Correcting the predicate makes VC7 keep `NONE` in EDI and assign
the tag-block definition to EBX, reproducing the January register topology.

A genuinely new lifetime probe named a separate `long no_actor = NONE` carrier.
It was a byte-identical no-op and was restored.  Removing the redundant named
`firing_positions` tag-block alias, while continuing to use the typed
`definition->firing_positions` member directly, then made the candidate prefix
instruction-identical through +0x141.  The first differing branch displacement
there is downstream of the still-present 208-byte surplus: two call sites inline
`ai_debug_drawstack` that January calls out of line.  None of the previously
logged casts, parentheses, caller/callee mass, do/while, or `csprintf` shapes was
repeated.

This is meaningful progress but not completion.  The reopen question is now
narrow: find a natural source/context lifetime that changes the two surplus
inlining decisions without perturbing the now-correct EDI/EBX prefix or the one
site whose mixed inline/call decision already matches January.

### Checkpoint gates

- A forced delete and rebuild of `build/base/source/ai/ai_debug.obj`, followed by
  `halobetacache_build`, `libcmt_build`, and `semantic_progress`, reproduced
  56/60 and 14,128 padded strict bytes with the same four residual hashes above.
- The whole-project semantic report contains 470 units, 3,481 semantic-exact
  functions, and 3,565 accepted exact functions, with zero unit errors.
- `tools.audit_object_admission` reports no admission candidate for AI debug.
  This is the expected fail-closed result: the source remains `NonMatching` and
  the four residuals remain open.
- `python -m unittest discover -s tools -p 'test_*.py'` passes all 179 tests.
- `git diff --check` passes.  The changed source contains no inline assembly,
  `volatile` addition, force-inline directive, optimization pragma, barrier,
  flag change, type-punning codegen cast, comparator exception, or byte patch.
- House/Berth review: recovered structures have named semantic members only
  where target accesses prove them, unidentified spans stay explicitly unknown,
  target assert/string identities remain load-bearing, and the sentinel defect
  is expressed as ordinary readable C rather than a code-generation trick.

The regression gate's clean-canonical manifest correctly cannot be used as a
zero-diff approval for this checkpoint: a deliberate reconstruction changes the
translation unit's section and symbol inventory and adds 45 strict functions.
That comparison reported the additions rather than hiding them.  A clean
checkpoint snapshot followed by a forced rebuild/check is the reproducibility
gate for the retained source; its outcome is recorded in the handoff together
with this ledger.

## Current-canonical integration and strict admission (2026-08-29)

The checkpoint above was transplanted onto canonical commit `71d1693e` only
after AI debug was released from Claude's active lane. The old worktree's two
uncommitted experimental files were treated as read-only evidence and were not
included. The independently audited `punpckhdq/halo` donor remains a verified
no-op for this object: it has no strict exact AI-debug function absent from
canonical.

Current canonical already held 23/60 strict functions and 3,120 padded exact
bytes. The integrated candidate holds 56/60 and 14,128 bytes, for a net gain of
33 functions and 11,008 bytes with no loss from the canonical exact set. A
whole-build comparison covered 568 comparable objects and found zero lost exact
functions anywhere; AI debug was the only object with gains.

The integration also removed representation casts and a raw path-step address
expression from the uncredited residual source. Typed point/vector operations,
subsystem access macros, locally scoped external-layout views, and named debug
flags preserve the exact 56-function set without relying on code-generation
tricks. Every one of the 60 function bodies ends with an explicit `return`, and
the recovered declarations use the current one-parameter-per-line style.

The four residuals remain honest zero-credit work:

| Function | January / candidate bytes | January / candidate relocs | Candidate normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_ai_debug_render_actor` | 24,976 / 24,848 | 1,905 / 1,903 | `e20fcee6e777e36b36fa9e0fa4acfc8ddc46995707cd61aceffda34f092ec2c4` |
| `_code_00039990` | 848 / 848 | 42 / 42 | `77336ed2facf6f0eabe826e0b59f5ae2f69a0e586583af2ed82553ec590fd430` |
| `_code_0003a910` | 704 / 704 | 23 / 23 | `1f99ed4877b1ec2454721d3b5c80c353113acf40278864f7b8e2794193d3e2f9` |
| `_code_00041220` | 2,336 / 2,544 | 169 / 201 | `f9220b2efb233322e1a1536ffee0b60af13fc8d05e4e465b25b2eab065fbc4e9` |

Current gates:

- `halobetacache_build`, `libcmt_build`, and `semantic_progress`: pass;
- semantic report: 470 units, 4,715 semantic exact, 4,725 accepted exact,
  zero unit errors, and zero ordinary-C rejections;
- AI-debug gate: 56 exact, 4 residual, 0 unwritten;
- pre/post exact-set comparison: +33 / -0 functions, +11,008 bytes;
- 568-object whole-build exact-set comparison: no lost exact functions;
- `python -m unittest discover -s tools -p 'test_*.py'`: 205 tests pass;
- object admission audit: no candidate, contradiction, or revocation, which is
  the expected fail-closed result while four functions remain residual;
- no assembly, new `volatile`, `register`, optimization pragma, intrinsic,
  barrier, attribute, byte forcing, raw address access, object patch, tool
  exception, comparator exception, or compiler-flag change.

## Exhaustive later-tree census and `_code_00041220` minimization (2026-08-29)

An exhaustive read-only census checked 1,487 registered worktrees, 1,077 built
`build/base/source/ai/ai_debug.obj` artifacts, and 1,056 distinct object hashes.
A second filesystem scan covered `C:\halo-worktrees`, `C:\tmp`, and the complete
Codex workspace; its six unregistered build artifacts were all older 11/60
objects. The only artifact strictly ahead of current canonical was:

`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\ai-debug-salvage-20260813\build\base\source\ai\ai_debug.obj`

Its raw object SHA-256 is
`f1cabaaba68b527cd93884ce3de293e0f48bbf854aa91cd6b35b25261f74792e`.
It measures 57/60 strict functions and 16,464 padded exact bytes, adding only
`_code_00041220` (2,336 bytes) and losing none of canonical's 56 exact owners.
Transplanting that function's dirty source into the clean canonical integration
tree reproduced the same 57/60 result exactly.

The 57th match is not admissible under the house rules. It depends on a one-use
local function-pointer alias of `ai_debug_drawstack`, followed immediately by a
call through that alias. Replacing only that alias with the direct typed call
returns the object to 56/60. The alias has no semantic selection or callback
purpose and fails the hard-prohibition test: it would not plausibly exist in the
2001 source for a reason other than suppressing one inline decision. It is
therefore recorded as byte-forcing evidence, not credited or retained.

The remaining source changes are ordinary typed C and were minimized separately.
They recover target-backed branch structure, color-pointer indirection, local
lifetimes, assignment order, and the two-arm pursuit/evaluation rendering
topology. With the prohibited alias removed, they preserve the complete 56-owner
exact set and improve the open `_code_00041220` residual as follows:

| measurement | previous canonical | retained ordinary-C subset |
|---|---:|---:|
| candidate bytes | 2,544 | 2,448 |
| candidate relocations | 201 | 185 |
| target bytes / relocations | 2,336 / 169 | 2,336 / 169 |
| candidate normalized SHA-256 | `f9220b2efb233322e1a1536ffee0b60af13fc8d05e4e465b25b2eab065fbc4e9` | `46fab4ca08429eb923a46d573915c2419659690a1c74226612503413cd647f59` |

This is a 112-byte and 16-relocation reduction with no strict regression, but it
earns zero new credit. The remaining difference is exactly one surplus inlined
`ai_debug_drawstack` body. A naturally justified source/context lifetime must
make that site a direct call; a function-pointer alias, invented wrapper, forced
inline/noinline control, or other optimization-only construct is not an
acceptable closeout.
