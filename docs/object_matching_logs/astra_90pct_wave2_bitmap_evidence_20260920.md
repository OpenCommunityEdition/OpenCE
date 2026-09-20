# Wave 2: authenticated bitmap extraction state machine

The fresh tools-build investigation closes the specific source-evidence gap
that kept `_extract_sequence` parked. A scratch-only candidate is strict exact
at720 meaningful/padded bytes and28 relocations, preserving all20 inherited
exact functions in `bitmap_extract.c`. No source/config landing or credit is
claimed by this research result; independent review and root integration follow.

## Primary evidence

Input: `C:/Users/isabe/Downloads/haloleak2024/halo1/h1_tags/tool_symbols.exe`.
Its fresh SHA256 and PE timestamp are recorded in
`scratch/astra-wave2/tools-source-xrefs.json`. The scanner also checked
guerilla, sapien and halo_tag symbols builds. All four contain65 bitmap_extract
source-assertion xrefs, including the complete extraction state machine absent
from the previously used cache build. Tool entry0x00B96A10 corresponds to
Guerilla0x00960600, Sapien0x00C678D0 and tag build0x004E1C40 by distinctive
assertion strings and algorithm. These are cross-build correspondences, not
recovered original private symbol names or proof of January source revision.

The read-only PE/Capstone scanner initially labels starts as prologue
candidates; this function has a complete coherent prologue, matching two-word
parameter use, three distinctive source assertions, complete loop/trim/call
graph, RTC epilogue and terminal return at0x00B96EA8. Bytes after the return
are descriptor data/padding, not additional code. The raw reconnaissance
disassembly intentionally preserves its broader span; use the verified code
boundary above when analyzing it.

`scratch/astra-wave2/tool_symbols.exe-b96a10-prologue-candidate.asm` supplies
the following independent unoptimized statements:

- 0xB96A74/0xB96A76 compare bottom and top and accept equality; January's
  assertion literal is `bottom>=top`. The inherited `bottom > top` assertion
  and spaced literal were incorrect.
- 0xB96ADA..0xB96ADC initializes x only after the assertions.
- 0xB96B08..0xB96B26 writes rectangle fields x0,y0,x1,y1 in that order.
  The inner test checks x against width before checking state against2.
- 0xB96B5B and0xB96B5F initialize two independent booleans. The bottom-color
  arm writes the second flag at0xB96BB9; the non-top-color arm writes the
  bitmap flag at0xB96BD1 and dispatches extraction states0/1.
- 0xB96BF3..0xB96C08 sets state1 and x0 in state0, then falls into the
  normal bounds updates. The fall-through is meaningful source control flow.
- 0xB96C73..0xB96C9C explicitly implements
  `(found_bottom_reference || single_sequence) && !found_bitmap`, with an
  inner state1 test and state2 store. Immediately afterwards,
  0xB96CA0..0xB96CB6 independently tests `state==1 && !found_bitmap` and
  writes state2 again. This independently authenticates the redundant
  flag-dependent branch, rather than inferring it solely because January
  requires it for exact bytes. The two stores are on distinct tested paths.
- 0xB96CE1..0xB96CEA copies the8-byte bounds into a second rectangle before
  checking the trim flag. The trim scans use that working rectangle, update
  its y0/y1 and pass it to the bitmap extraction call at0xB96E67.
- The RTC descriptor at0xB96EAC names `bounds` (8 bytes atEBP-28) and
  `adjusted_bounds` (8 bytes atEBP-116). Raw records are preserved in
  `scratch/astra-wave2/extract-rtc.json`. The protected aggregate names and
  widths are direct evidence; they do not reconstruct unrecorded scalar
  declaration order or literal lexical blocks.

The old `seqe.c` is a patch carrier after this fresh authentication, not a
source authority. The actual candidate improves its fidelity to the evidence:
it names the working rectangle `adjusted_bounds`, uses that rectangle in the
first trim scan and expresses the two state-ending tests as independent if
statements. The previous donor used `bitmap_bounds` and an else-if. Those
differences are byte-neutral in the current compiler, and the authentic form
still gates exact. Only one evidence-based shape was compiled.

## Semantics and admission boundary

The redundant flag branch does not change the state machine's final result:
for state1/no bitmap, either the first or second test advances to2; all other
states retain their prior behavior. The original separate statements have
direct first-party machine evidence. All booleans and rectangle members are
initialized before use, the scan bounds and array sizes are preserved, and
the separate rectangle is a used trimming result, not declaration filler.
The short state values0/1/2 are an internal integer state machine; no original
enum names are claimed. A comment marks the intentional state0 fall-through.

The corrected assertion admits an empty top==bottom row exactly as January
and the tools build do. The body then advances x without finding a bitmap
and returns success. No uninitialized read, fake dependency, forced inline,
foreign helper/prototype or manual bytes are introduced.

The park described missing evidence for apparently redundant source logic.
This new unoptimized machine witness plus fresh strict January comparison
satisfies that source-context gap. Existing user instructions explicitly
authorize independently authenticated branch-owned statements; no new owner
ruling is requested or inferred as necessary from obsolete lane wording.

## Reproduction and controls

- `scratch/astra_wave2_tools_xrefs.py`: four-build source assertion scan and
  preserved primary bytes/disassembly. No input file or database is modified.
- `scratch/astra_wave2_extract_candidate.py`: unique function-only patch,
  scratch body/diff/JSON and distinct gate output.
- `scratch/astra-wave2/extract-authenticated-gate.txt`:21/21 exact;
  point_from_line3d emission guard passes.
- `scratch/astra-wave2/extract-relocdiff.txt`: all28 positions, types,
  addends and semantic destinations agree; no relocation alias exception.
- `scratch/astra-wave2/extract-alndiff.txt`: alignment-aware comparison.
- `scratch/astra-wave2/extract-admission-candidate.json`: full fingerprints,
  preserved20-function control set and zero inherited losses.

The four unrelated strict branch-store controls from wave1 and its five new
accepted functions establish that authenticated source statements can survive
apparently equivalent simplifications differently. This result remains a
bounded source-preservation rule, not permission for arbitrary duplicated
logic or a universal compiler scheduling law.
