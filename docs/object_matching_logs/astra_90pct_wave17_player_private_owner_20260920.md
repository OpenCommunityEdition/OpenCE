> Frozen evidence/probe report; later admission is recorded only in the completed wave17 summary. The private player helper adds one meaningful byte; all other lanes earn zero credit.

# Original private players owner: one coherent test is now justified

**QUALIFIED FOR A BOUNDED TEST; natural emission remains unmeasured.**
This independent packet creates no source shape, invokes no C compiler and
changes no header, source, configuration or target. Credit remains zero.
Potential function size is only **1 meaningful / 16 padded bytes**.
Reproduce with `audit.py`; complete raw locations, hashes and checks are in
`audit.json`.

## Newly acquired primary identity evidence

Both original 2001 beta MAPs explicitly list
`_player_examine_nearby_unit` under **Static symbols**, marked `f`, owned by
`players.obj`:

| Build | MAP line | Segment 1 offset | MAP VA | XBE file offset |
| --- | ---: | --- | --- | --- |
| August 15 | 19,863 | `0x9D6C0` | `0x0049DAC0` | `0x9E6C0` |
| September 25 | 21,223 | `0xA8850` | `0x004A8C30` | `0xA9850` |
| January target | configured original slot | — | `0x004AA180` | `0xAA180` |

The XBE header and section-table parser independently identifies `.text` at
raw offset `0x1000`, VA `0x11000`; these are not assumed from the filename.
Both corresponding 16-byte windows are exactly `C3` followed by 15 `90`
bytes, identical to January. Input hashes match root's acquisition manifest.
August's `1749betaP.xbe` filename does not prove a pristine unmodified image;
the independent September evidence carries the stronger neighbor result.

Both MAPs place the helper immediately after
`_players_compute_local_player_count`, 32 bytes later. September preserves
the full five-owner consecutive sequence and January-sized instruction
windows:

| Owner | Padded window | September normalized instructions equal January |
| --- | ---: | --- |
| `players_compute_combined_pvs` | 368 | yes |
| `players_compute_local_player_count` | 32 | yes |
| `player_examine_nearby_unit` | 16 | yes, no relocation masking needed |
| `unit_should_autopick_weapon` | 144 | yes |
| `player_reset_action_result` | 32 | yes |

August additionally matches the complete local-count and helper windows;
its combined-PVS/reset windows differ and autopick is absent from this
sequence. The comparison masks January relocation positions in neighboring
functions. It is evidence for correspondence, not a claim that all resolved
cross-build destinations match or that the complete source revision is the
same.

This closes the earlier *unsupported inferred name/public-linkage premise*
enough for a source test: the earlier original private owner, exact leaf and
anchored September sequence identify the January slot by narrow cross-build
correspondence. A direct January local-symbol record is still unavailable.
Binary owner order is not claimed to recover source declaration order.

## Type, call graph and remaining emission condition

Packet13's independent raw HCEA `S_LPROC32` and procedure/argument records
already establish **private `void(long player_index, long unit_index)`**;
DIA marks it inlined. The old Boolean-reconstruction premise is superseded.
This packet rechecks all three saved later x86 PE slices against the actual
hashed input: two genuine type-zero dispatch arms pass the enclosing player
handle and selected object handle to the empty framed return, discard eight
argument bytes and consume no return value.

Current `players.c` has a real `_object_type_biped` case that immediately
continues the object loop. Replacing that case with the authenticated empty
void call and loop continuation has initialized, already selected operands
and no new behavior or undefined return value. The only proposed source
test is a **private** typed empty definition plus that genuine dispatch
call, with root removing the false public Boolean declaration from the
actual owner header after consumer inventory. No public stand-in, fake call,
address anchor, forced emission or metadata change is permitted.

Ordinary VC7 may still inline the call and omit the standalone body. That
outcome must close the test, without an emission workaround. A retained
body must pass all **66 current strict exact controls**, full runtime
owner/data/COMMON/point checks and the complete genuine-header consumer
checks. The configured January split currently labels this inferred owner
external; that synthetic linkage is not a reason to emit a false public
definition or rewrite target metadata. Source ownership and admissibility
remain explicit review requirements.

The packet13 zero-reference census is historical evidence and was not
rerun across the tree. Current focused reads confirm the target body exists
and the base body is absent. No gain or landing is recommended before the
single coherent test and independent review succeed.

Evidence ownership: **RELEASED**. Root was notified of the concrete new
premise before any source preparation or compiler invocation.
