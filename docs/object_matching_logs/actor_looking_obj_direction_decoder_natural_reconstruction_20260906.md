# ActorLooking direction decoder / movement caller (2026-09-06)

## Fixed first natural source checkpoint

Baseline: `d2c0babb171a51de60fe0cb9ebba4bedab1c6f61`, published to both
authorized GitHub refs. This packet reconstructs two previously unwritten
functions with no shared-header edits and no change to either concurrent
Opus reservation. It is not a claim that ActorLooking is complete.

| Function | January meaningful / padded | Candidate padded / relocations | Ordinary fuzzy |
| --- | --- | --- | --- |
| `actor_look_decode_direction` (private) | 1,624 / 1,632 | 1,616 / 99 (target 98) | 65.322365% |
| `actor_look_affect_movement` (public) | 122 / 128 | 128 / 4 (target 4) | 92.38636% |

Neither function earns exact credit. The first gate object and the ordinary
Ninja object have identical normalized function bytes and relocation records.
The whole strict census remains **6,261** with **zero inherited regressions**;
the unit is now two exact / two residual / twelve unwritten. Canonical exact
code remains **917,456 meaningful bytes**, 6,215 credited functions and
391/833 Matching objects at this checkpoint.

The private function's target normalized SHA-256 is
`4f3714fab510f0c407dc0792b3cbea003ba4d1dfdb4aee74d201d9a9e5dc14a9`;
first/actual candidate is
`bea2ec692aedec4276c07fb076ad2dd53888277627541c42fb56dea1670b3008`.
The public target is
`5d7d253b925542d2e1c1c21c83c6a2f2049df1a6fe051b6f713b377c47f85dde`;
first/actual candidate is
`d096436ec233acf74823c8ce959e2ded832eb046b0c8b441be976c5e6cf5dafa`.

No alternative declaration orders, equivalent expressions, inline annotations,
pragmas, compiler flags, fake exported helpers, representation casts, literal
instruction emission, or register controls were tried. The only pre-compile
signature correction removed `const` from `specification` to follow the
authenticated private prototype. The local `look_point` also follows DIA.

## Source, ABI and owner evidence

Root and an independent worker read the complete January byte streams and
relocations. HCEX DIA corroborates a static, non-inline, non-assembly
`unsigned char actor_look_decode_direction(long, struct direction_specification *,
union real_vector3d *)`. Repository `boolean` has that byte-return ABI.
The matching source uses a genuine static declaration in the defining C file,
not an external emission facade. Config offset **93296 / 0x16C70** is renamed
from `_code_00016c70` to `_actor_look_decode_direction`, with `static: true`.

All six January decoder references are same-TU calls: five in
`actor_look_update` at +0x268, +0x2A5, +0x2EA, +0x9FE and +0xC5F, and one in
the reconstructed public movement caller at +0x57. The five update call sites
remain unwritten; this is an acknowledged incomplete compiler context, not
an invented caller graph. Both target and natural candidate pass specification
in EBX, output direction in EDI and actor index on the stack. The sole
outside movement-caller reference is `actors.obj::actor_update` at +0x103.

`actors.h` already owns the complete relevant actor fields, direction/fire/
danger/priority enums, `actor_get` and `actor_path_has_path`; `props.h` owns
the prop fields and typed access macros; `objects.h`/`object_types.h` own
object access and masks; `units.h` owns `unit_get_head_position`.
`real_math.h` and `cseries.h` own the math, assertions, formatting and temporary
buffer. Only the real `units/units.h` include is added. The public prototype
already resides in `actor_looking.h`.

The local HCEA reference at
`research-cache/halocea-full-blobs-20260830/src/blam/ai/actor_look_decode_direction.c`
(relative to the outer campaign workspace, not this worktree) corroborates
the seven source forms. Its later release/debug differences do not override
January. In particular, movement's priority threshold is **3**, expressed by
the current genuine `_primary_priority_aiming`, not HCEA's misleading facing
name. January loads object type at +0x64, matching the current header rather
than HCEX's later layout. The locally cached Marathon 2 `monsters.c` supplies
only a contemporary example of named states, typed accessors and subsystem
organization, not byte or layout authority.

## Raw hex and original diagnostic behavior

The original frozen January ActorLooking object SHA-256 is
`443e269f49461dcaf5ff3804fc8770cc5579798427289c60f56a1ce533a226de`.
The decoder has two seven-entry absolute relocation tables at function
offsets **0x620** and **0x63C**, followed by eight NOP bytes at 0x658.
These are table data and padding, not instructions, despite a naive linear
disassembler producing apparent `outsb`, `xchg` and arithmetic there.

The dispatch table resolves to 0x7B, 0x1E3, 0x16E, 0x207, 0x22F, 0x249,
0x279. The diagnostic table resolves to 0x321, 0x4B5, 0x394, 0x504, 0x539,
0x56E, 0x5D0. Raw payload, table destinations and padding are checked by
`scratch/actor_looking_decoder_owner_audit_20260906.py`.

The movement diagnostic genuinely prints point **x, z, z**. At both +0x356
and +0x360, hex `D9 86 14 05 00 00` loads actor +0x514; +0x36A loads
actor +0x50C. Right-to-left varargs and the typed field layout prove the typo.
The source marks it as a preserved original bug and states that a corrected
diagnostic should use y for the second component. This is not an invented
nonsensical expression used to steer the compiler.

The invalid-prop diagnostic also deliberately passes owner **0**, not NONE:
the failed `datum_try_and_get` leaves EAX zero, then the varargs path pushes
it unchanged; the successful path first loads `[eax+4]`, owner_actor_index.

January checks null specification/direction at lines349/350, uses a fatal
NULL-message assertion at449, emits the custom nonfatal diagnostic at526,
then validates the normal again at529 with a fatal formatted assertion.
All stages and original strings are retained, including the two otherwise
seemingly redundant normal checks.

## Regression and ownership checkpoint

Before and ordinary-first captures freeze all 572 base and all 833 split
hashes, the source, symbol config, and original target. Only the ActorLooking
base and split object hashes change. The all-section comparator preserves
all **14 inherited runtime records** with no changed or lost section, new
COMMON, writable data, or definition transition. New sections are exactly
the two functions, 21 read-only string/real sections and nine ordinary
math-helper fallbacks. Every new read-only payload has an exact January
definition witness in the complete split census. No `point_from_line3d`
definition or reference is introduced.

Before manifest SHA-256:
`5edecbfb16f421a7eedafb89121ef42348cee3ecdbfe45ce8a0f272b6263e3ef`.
Ordinary-first manifest:
`513b356efb3903cc6f3d0f012e5506e6152601c0d4a09c5843cb89d7346003e8`.
Actual frozen object:
`scratch/actor-looking-decoder-first-20260906/source/ai/actor_looking.obj`,
SHA-256 `0d3378d771f780f909a6d7d29d2c9eec14cfd65f5901c6efbec82a5fd737b175`.
The original target is frozen beneath the corresponding `before` directory;
the independent gate object is `scratch/actor-looking-decoder-first-20260906.obj`.

All nine new helpers are ordinary SELECTANY COMDATs from unchanged math
headers. Their bodies match all other current definitions except one
`vector_from_points3d` scheduling variant in BreakableSurfaces (one independent
pointer load transposed with `fld`; same 48-byte size and zero relocations).
Four helpers also have existing NODUPLICATES definitions in ActionCharge:
square_root, scale_vector3d, magnitude3d and normalize3d. Whole-object VC7
diagnostic links in **both input orders** report LNK2005 for all four, plus
32 unresolved imports. No FORCE/IGNORE options or fake stubs are used. This
is a real link-policy veto: ActorLooking is not complete or linkable here.
No helper suppression or header-order trick is used to conceal it.

The decoder's first material code difference is the candidate calling
`valid_real_normal3d` at +0xC0 where January inlines its first normal check;
the final check remains a call. This explains the extra relocation, but not
an exact overall classification of the remaining block layout. Movement has
duplicated return/store tails rather than January's shared tail. These are
honest residuals, not claimed compiler ties. Reopen only with authentic
source/type/complete-caller-context evidence, not equivalent-spelling searches.

Root also inspected January +0xBB through +0x110 against the real helper
+0x06 through +0x50. Both compute the same x87 product/sum/subtraction and
comparison sequence, and both use `fst` rather than `fstp` for the finite-value
bit check. The target retains squared magnitude below the difference while
the helper discards its component temporaries earlier. The mere presence of
a call therefore does not establish an extra float-rounding step or a known
precision defect. Boundary behavior still needs actual execution evidence;
finite tests cannot establish exhaustive equivalence.

## Independently replayed behavior and partial-source admission

Root read the entire frozen runtime oracle and independently reproduced its
report byte-for-byte. The tool executes the real decoder and movement caller
for January, first gate and ordinary Ninja objects. The reached
`normalize3d`, `vector_from_points3d` and `valid_real_normal3d` bodies also
execute as actual machine code; they are not semantic stand-ins. All nine
loaded helper bodies are raw-identical to authenticated January physical
owners in ActionAlert, ActionCharge and ActorCombat. The other six helper
bodies are loaded but not reached in this corpus.

**32 authored cases** (26 direct decoder, six real public caller) under x87
control words **0x027F and 0x037F** yield **192 target/first/actual entry
executions**. All tested semantic signatures agree, first/actual observations
are identical, and no FPSW difference is observed. The corpus covers all
seven direction types, absent/zero-result paths, unit/general/missing object
paths, priorities 2/3/4, movement path short circuits, original null/default
assertions and denormalized-vector diagnostics. Typed call arguments and
ordering, full 4 KiB seeded data pages and tails, permitted writes, caller
stack, normal-return nonvolatiles, FPCW/FPTAG/TOP and direction flag pass.
Fatal paths are checked at the nonreturning `system_exit` boundary, not
misrepresented as successful function returns.

Four adjacent float32 bit patterns bracket the lower and upper normal-vector
validity bounds: lower outside/inside **0x3F7FDF39 / 0x3F7FDF3A**, upper
inside/outside **0x3F801061 / 0x3F801062**. All roles agree in both precision
modes. No tested precision divergence exists; this finite corpus is not a
proof covering every floating-point input, exception mode or pointer alias.

Three separate mutations of relocated candidate bytes in memory are all
detected: the public priority threshold changes 3 to 4 at +0x43; the decoder's
second movement-point z load changes to y at +0x2F7; and the failed-prop owner
changes zero to one at +0x34C. No on-disk COFF input is modified.

Engine datum/object/path/head/origin APIs, assertions, exit and CRT formatting
are explicitly bounded models. Formatting evidence is the actual machine
stack words, decoded types, formats and downstream assertion arguments;
the Python-rendered string is not claimed to reproduce the original CRT for
every float. This is a bounded function-graph test, not whole-engine or
complete-object linkage evidence.

Frozen scratch script SHA-256:
`a40ca1bd386050bf7c46c0132aa230ead3e70446aa23b26992f8a1268ec8ce92`.
Both worker and independent root replay report SHA-256:
`d54e7384eacd5229444878d2a88d417e22e98100c0d3934e3c8226fa704c335b`.
Public tool `tools/audit/actor_looking_decoder_runtime.py` SHA-256:
`8785df698c64a4b6f5702d7fd53edb41793b99a31eb326b40cada9d61eea42e0`.
Its independent public replay report SHA-256:
`66fc6e1ad0805a2e27b5eaf7d4e2d574207689ee9052012a158da07fb67c0bf2`.
Promotion changes only the default scratch report name and removes an extra
blank line at EOF. Private COFF and
executable inputs stay local and hash-pinned; none is published.

The two natural bodies are accepted as **fuzzy partial-source progress**,
with zero exact credit and explicit unclassified parks. The documented
four-helper link conflict and five unwritten update call sites remain open;
ActorLooking is not admitted as a complete object. RenderDebug was separately
published as `2fbd3e3bb6917e4ee5c47288ad4ed3f5b7173f03`, so the current exact
total is **917,904 meaningful bytes**, **6,217 credited functions**, **6,263
strict functions** and **391/833 Matching objects**, unchanged by this packet.

Final Ninja succeeds. The fresh 8,245-owner strict snapshot preserves all
6,263 current exacts with no gains or losses. Park validation is **294 active,
zero stale and zero invalid**; admission has zero candidates/contradictions/
revocations and five existing rejections. The final frozen manifest
`scratch/actor-looking-decoder-final-20260906.json` has SHA-256
`b9f4d5ed7396f087e399991a93a6227bca1f82a8e9e1c760297b66c3229612d3`.
Against the first ordinary capture, all 833 split hashes, ActorLooking source,
symbol configuration and frozen target remain unchanged. Of all 572 base
objects, only the independently published RenderDebug packet differs.
ActorLooking's actual object retains its pinned SHA-256 above.
The complete tools suite passes **606 tests plus 26 subtests**, including
12 fixture-free ActorLooking oracle contract tests. The test file SHA-256 is
`341c21be568c1b50ecb4d4e6d34b155e8fd122a4bca6040aaa75fea2242d1bfa`.
These pure contract/mutation tests do not read private game inputs and are
not counted as additional target machine-code executions. The scoped batch
is ready for commit; remote publication is independently verified afterward.
