# Static retention and linkage-sensitive code: research and one repair

Research started at canonical `30f71413`, against read-only donor `fe6e1331`.
The isolated physics correction subsequently committed as `25da8a5e` and is
not part of the storage result. The owner authorized investigation, with no
invented uses and no landing before independent review. This report separates
one reviewed, source-backed repair from diagnostic-only candidates.

## Scope correction and evidence limits

RF-BQ's final c-drop class contains **20 rows**, all currently emitted and
already strict function-byte exact at the research baseline. Thirteen occur in
donor Matching units and seven in NonMatching units. Two are SDK wrapper bodies
inside a Halo TU, not Halo-authored code. The preliminary 29 is **not** 20 Halo
plus nine vendor rows: it was an own-object, zero-named-reference cohort. Nine
rows later classified a-src/a-hdr were included; one even has a cross-TU
reference. These are ownership/source-recovery opportunities, not 20 new
function matches.

For the final 20, the bounded census examined 833 January split objects and
113,672 nondebug relocations, resolving symbol/section/addend destinations,
plus 832,958 decoded executable instructions and 60,922 PE HIGHLOW sites.
It found no incoming direct or address-taken reference. Raw E8/E9 scanning and
absolute-start-address controls agree. This does not rule out arbitrary
computed/encoded pointers. Split storage is reconstruction metadata, not
independent proof of original linkage. Primary January PDB public records have
no public at these RVAs; absence alone is also not a static declaration.

**No surviving machine reference does not mean no source use.** A call can
inline while the private out-of-line body remains emitted. Conversely, the
current source losing a function when marked static does not prove original
external linkage or authorize an artificial retaining call.

The historical `research/fifty_objects_20260925/w/saved_game_files/LEDGER.md`
assertion that three such helpers were "provably external" is superseded.
September's original map has a Static symbols heading at line 19268 and names
take_mapfile_mutex, release_mapfile_mutex and enumerate_default_profiles at
19377/19378/19386. Canonical already has all three static, emitted and exact,
with genuine calls. These are solved controls, not members of the live 20.
Existing object_lights L0/L1/L2 evidence likewise demonstrates static-only
deletion followed by retention through real first-party-attested calls. No
redundant synthetic retention lab was needed.

## Reviewed repair: virtual_keyboard_get_current_character

The current source had a correct, already-exact 48-byte helper but hand-expanded
its only genuine use, leaving the helper external merely to retain emission.
The repair restores the call at character insertion and marks the existing
local prototype and definition static. Exactly one ordered symbols.json entry
at RVA `0xe5080` gains `static: true`. No shared header changes.

Evidence is positive, not just a missing-reference inference:

- September's first-party map lists this name under Static symbols.
- Original HCEX PDB records `static wchar_t
  virtual_keyboard_get_current_character()` and its private select caller.
- HCEX's actual PPC code calls that helper at `0x82d40718`, immediately after
  insertion memmove at `0x82d40714`, and stores the wchar into the cursor at
  `0x82d40728`.
- January select `+0x306` calls csmemmove; `+0x30b/+0x312` load signed row and
  column; `+0x319` multiplies row by 11; `+0x31c` loads the layout entry;
  `+0x325` calls get_character; `+0x330` stores AX into the cursor. January's
  surviving current-character helper performs exactly that computation.

Later PPC code is corroboration, not transplanted January source or ABI.
The January dataflow, existing return type, cursor sequencing, modifier/fallback
behavior and actual site all agree. This is a meaningful helper call, not an
empty call whose position was selected for compiler effects. The unrelated
bink/hardware-geometry wrapper holds remain in force.

This is a coupled reconstruction: static alone dropped the body; historical
call-only probes were inert. The preregistered current-base call-only control
is again inert, while **call plus static** retains the exact helper with correct
private storage. Independent review and four additional root stock-/W3 builds
confirm the result. No declaration-count or local-name search was used.

The actual minimal source (without the scratch snapshot's extra EOF blank)
was rebuilt during reconciliation and equals the reviewed candidate. All 44
nondebug sections and 44 semantic definitions remain. All code/data payloads,
resolved relocations, section flags/alignment and COMDAT properties are
unchanged; only this helper's storage changes 2 to 3. Three incidental `$L`
names advance by three ordinals, with unchanged section/offset/type/storage and
resolved branch destinations. Equality here is relocation-semantic, not literal
raw symbol-table identity.

The helper remains 48 bytes / four relocations, normalized SHA256
`6c7664929f604e417a5c59ec61d8076dd5c769ef691de0d29e3ab5dfaa064659`.
There is **no new helper definition**, COMMON, undefined reference or surplus
provider. This retains a TU-private function, not an extra shared-header COMDAT;
the strict-caller folded-inline exception is not needed. No external consumer
was found in the source/header/object census. Stock warnings remain zero;
diagnostic `/W3` keeps 14 inherited warnings without any new one.

Canonical gates at the landing:

- Full build, source/target storage agreement and independent whole-TU review
  pass; only this one target object changes among all 833 split objects.
- Whole-board 8,252-row sweep: 7,641 exact, zero gains/losses.
- Keyboard stays 18/20 exact and NonMatching. Both residual bodies are
  unchanged. No object admission or new code/data credit.
- Parks remain 72 / zero stale / zero invalid; no park file edit in this packet.
- Admission and all 26 inherited fake-match leads are unchanged.
- Tests: 1,161 passed, five skipped, 26 subtests passed.
- Scoped whitespace checks pass; unrelated source and user dirt are preserved.

This resolves **one** of the 20 c-drop storage rows, leaving **19**. Halo totals
remain 1,598,242 / 1,770,166 meaningful code bytes and 390 / 468 complete
objects. The missing original references for the other 19 are not invented.

## c-code: two different mechanisms, neither landed

`mp_sound_queue_count` is a **data object**, not a function. Static-only retains
the same 44-byte BSS, count at +0 and queue at +4, but inserts `TEST EAX,EAX`
after update's `DEC EAX` and before its count store/JE. Padding absorbs the two
bytes (80 bytes/seven relocs before and after), but exactness falls 6/6 to 5/6.
DEC already supplies the relevant zero flag; this is not a BSS-layout change.
An alias/VN/condition-code-pass explanation remains unproven.

A first-party demo PDB types the counter as static long, not the reconstructed
one-member aggregate. A separately preregistered four-cell comparison removed
the wrapper, its size assertion and macro alias completely, with no filler:

| Definition | Wrapper | Authentic scalar form |
|---|---|---|
| External | 6/6 exact | Whole nondebug object identical to external wrapper |
| Static | 5/6; extra TEST | Whole nondebug object identical to static wrapper |

Thus scalarization neither cures nor causes this linkage-sensitive difference
in the measured contexts. Both new scalar forms and all warnings/layout checks
are retained, but neither a debit nor a partial cleanup is landed here. The
next useful work is authentic update-source or compiler condition-code evidence,
not more BSS ordering or arbitrary spelling probes.

Making `virtual_keyboard_free_space_in_text_buffer` static has a different
effect: its exact 32-byte body survives, but select's space-arm call is inlined.
Select grows 1088/123 to 1104/124 bytes/relocations; the default arm already had
expanded arithmetic. No new width or proven private-ABI effect occurs. It remains
held and external. The current-character repair above does not approve this
second helper change or select's separate switch-tail packet.

The c-code raw-reference diagnostic initially mishandled REL32 addends and
counted adjacent backspace calls. The corrected raw instruction is
`E8 52 FD FF FF` at the genuine call: operand file offset 940458 plus four minus
686 equals helper file offset 939776. The original bad receipt is retained as
a negative, and independent review verified the correction. There is one live
helper call, not two.

## Local receipts / continuing work

Full inventory, original map/PDB/image records, raw scans, bounded probes and
negative controls remain under `scratch/astra_storage_research_20260927/`:
`c_drop/REPORT.md`, `c_drop/VK_REPORT.md`, `c_code/REPORT.md`, its independent
`review/`, and `c_code/scalar_followup/`. Root's independent builds, frozen
baseline, minimal-source checks, split census and full gates are in
`scratch/astra_vk_storage_20260927/`.

Private reference binaries and compiler assets are not committed. Research-only
patches remain preserved. No other storage/linkage change, exception, whole
object certification or publication is implied by this repair.
