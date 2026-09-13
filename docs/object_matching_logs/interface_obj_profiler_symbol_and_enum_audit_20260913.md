# `interface.obj` profiler symbol and enum audit (2026-09-13)

## Decision

The two January targets at file offsets `0xCE900` and `0xCEA10` are now
identified as `_render_debug_profile_stall_tick` and
`_render_debug_profile`, respectively.  The names are admitted, but neither
body is admitted by this audit and **zero exact code bytes are credited**.

The Opus helper is byte-exact when compiled as a natural private helper of its
real caller, but its color switch is still written with raw case labels
`1`, `2..19`, `21`, and `26`.  The 27-value stall enum and its enumerator names
could not be recovered from January, the supplied symbol build, HCEA, Marathon,
or the local source corpus.  Admitting those numeric cases would violate the
campaign rule requiring semantic enum constants and would turn a compiler match
into an unsupported source-authenticity claim.  The exact helper therefore
remains blocked, not credited.

The caller remains the best complete reconstruction known, but is fuzzy and
also depends on that blocked helper.  It remains donor evidence only.

## Name provenance

The authenticated local symbol atlas supplies three mutually corroborating
records for each function:

- January `2002-01-14_2002_debug__cachebeta_exe`, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`,
  maps the exact target VAs `0x4CE900` and `0x4CEA10` to the two names (the
  atlas conservatively labels these two records `fuzzy`).
- The independent `2001-09-25_2001_retail__cachebeta_xbe` map corpus, atlas
  identity `7eacac85f93a4630b52142fe5f458acc017c081720b75919c9aaad41b6119dd6`,
  exposes `_render_debug_profile_stall_tick` at `0xDDD70` and
  `_render_debug_profile` at `0xDDE80`, both owned by `interface.obj`.
- The independent `2002-01-14_2002_debug__cachebeta_xbe` corpus, atlas
  identity `b49631768ccad84f0b6612227b750e9de8c9af844c9d40e11d76f006ec93dc8f`,
  exposes the same names and owner at `0xE0300` and `0xE0410` with exact-tier
  records.

The function order is stable across all three corpora: `profile_graph_toggle`,
the private stall-tick helper, the profiler caller, then
`interface_splitscreen_render`.  January's existing
`interface_draw_fullscreen_overlays` also calls the second target in precisely
the expected profiler slot.

The supplied 2020 Halo cache symbol executable is cross-build evidence only
(SHA-256 `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`).
Its analyzed large profiler-shaped routine at `0x0063E740` is adjacent to a
compiler-emitted local-name packet containing `graph_bounds`,
`graph_screen_bounds`, `tab_stops`, `objects_information`,
`memory_information`, `effects_information`, `drawingbuf`,
`drawingbuf_counts`, `current_values`, `last_values`,
`largest_stall_type`, and `largest_stall_msec`.  Those names independently
support the reconstructed caller's semantics, but the packet contains no stall
enum or enumerator names and cannot authorize the numeric switch.

Accordingly, `config/symbols.json` and the existing call/prototype use the
semantic names.  This is a naming correction only, not a match claim.

## Donor measurements

The retained donor is
`C:/halo-worktrees/opus-30k-ui-script-profiles-20260906` at `08d6bea92`.
Compiling that complete `interface.c` directly against the current canonical
tree with both names aliased to the still-stale generated target labels gives:

```text
EXACT       272  _code_000ce900
residual   2272  _code_000cea10  [size 2256!=2272, sha]
UNWRITTEN   928  _interface_draw_screen
== exact 15  residual 2  unwritten 1  (of 18 listed)
```

The helper contributes 271 meaningful bytes inside its 272-byte padded
section.  It is naturally emitted only through the genuine profiler caller;
no standalone retention or fake reference is used.  Despite its byte identity,
the unresolved enum contract makes its admissible/credited delta **0 bytes**.

The caller's native donor-lane investigation reached a 2,272-byte candidate
with all 119 relocation identities and the same 647 normalized instruction
shapes; only VC7 stack-slot coloring remained.  Against today's canonical
include/type environment the same source is 2,256 bytes.  In both environments
it is fuzzy and receives **0 bytes**.

Copying the donor file wholesale would also discard today's exact 928-byte
`_interface_draw_screen`, so it is categorically not an integration path.

## Focused verification

The canonical file before this audit gated at 15 exact, one residual, and two
unwritten functions.  Since the target split object was generated before the
symbol rename, a direct post-rename gate reports the 32-byte overlay caller as
relocation-identity-only residual.  The required temporary name alias proves
that this is solely stale target naming:

```text
python -m tools.campaign.gate source/interface/interface --all \
  --alias render_debug_profile=code_000cea10 \
  --forbid-emitted-symbol _point_from_line3d

== emitted-symbol guard passed (1 forbidden names checked)
EXACT        32  _interface_draw_fullscreen_overlays
EXACT       928  _interface_draw_screen
residual    448  _interface_get_weapon_hud_index  [sha]
UNWRITTEN   272  _code_000ce900
UNWRITTEN  2272  _code_000cea10
== exact 15  residual 1  unwritten 2  (of 18 listed)
```

Regenerating the split target from the renamed configuration during the
orchestrator's full build will remove the temporary alias requirement.  No
source body, header, compiler option, or inline schedule changed here, and the
`_point_from_line3d` emitted-symbol guard passes.

## Reopen criterion

Reopen this packet only when an authoritative Bungie/XDK header, matching PDB,
source file, or independently authenticated symbol/type record recovers the
27 stall categories and the enumerator names used by this switch.  Until then:

- `_render_debug_profile_stall_tick`: exact machine code known, blocked source,
  zero credit;
- `_render_debug_profile`: best complete fuzzy donor, zero credit;
- no `config/parked.json` entry is added because neither blocked body is
  retained in canonical source.
