# `virtual_keyboard.obj` Fable salvage admission — 2026-09-02

## Result

The raw Fable wave was imported as an immutable snapshot from commit
`06b3b1982b527f512231f9c4460b980a6241cd87`.  Reconciliation retains the
complete selector and event processor, but grants strict credit only where the
hardened COFF comparison proves it.  The unit advances from 17 exact, zero
residual, and three unwritten functions to 18 exact, one honestly parked
residual, and one unwritten function.  The strict gain is
`_virtual_keyboard_process_internal`, 848 padded and meaningful code bytes.
All 17 inherited exact functions remain exact.

| Function | January size / relocs / normalized SHA-256 | Candidate | Result |
| --- | --- | --- | --- |
| `_virtual_keyboard_process_internal` | 848 / 72 / `21ed27cb544a6130f43b2ca43956ab9a748fc2f881469c0ead2c60a144f8cad8` | identical | strict exact |
| `_virtual_keyboard_select` | 1,088 / 123 / `28fa08bf4fb555466006d076f32c672ca72ddd07a165e4c51e1cb54f02a0e141` | 1,088 / 123 / `025bd53f5f2169bb649702adf79e4603dd2a13ac0b5967aca9297e7e983565d9` | 93.7518% fuzzy park |
| `_virtual_keyboard_render_internal` | 1,616 / 95 / `f52389e69e9dbb4f8cbeaf41e8ad788704fec98dc5fbff916156df5285960425` | no body | unwritten |

The strict comparator resolves the process function's static local
`time_of_last_tab` to the same target BSS destination.  Objdiff reports the
function as fuzzy because csplit and MSVC present that private relocation
ownership differently; the semantic COFF report independently admits all 848
bytes with the target hash and all 72 relocations.

## Proven names and ownership

The HCEX PDB compiland
`..\..\..\build\x360\SHIP\halo\virtual_keyboard.obj` authenticates the private
names `virtual_keyboard_cancel`, `virtual_keyboard_get_current_character`,
`virtual_keyboard_render_internal`, `virtual_keyboard_backspace`,
`virtual_keyboard_select`, and `virtual_keyboard_process_internal`, including
the static local `time_of_last_tab`.  The remaining navigation and character
helper names are corroborated by the independent HaloCEA reconstruction and
their behavior:

- `virtual_keyboard_tab_left`, `virtual_keyboard_tab_right`,
  `virtual_keyboard_tab_up`, and `virtual_keyboard_tab_down`;
- `virtual_keyboard_get_character`;
- `virtual_keyboard_free_space_in_text_buffer`.

`config/symbols.json` records the target-proven private linkage rather than
retaining `code_<address>` labels.  The 0x160-byte private rectangle table is
named `_keyboard_rect`, matching the PDB's `keyboard_rect[44]`; it is not
claimed as reconstructed data while the renderer remains unwritten.

Two small helpers remain externally emitted in the partial source even though
the later PDB describes the complete program's ownership differently.  Making
them private before their natural renderer caller exists causes VC7 to remove
or inline their exact sections.  The source therefore preserves January's
observable emission schedule without a keepalive, fake call, forced noinline,
or other matching construct; the target symbol map still carries the strongest
authenticated ownership evidence.

## Source and house-rule reconciliation

- The complete event loop and selector were checked against January control
  flow, relocation destinations, assertion strings, and the independent HCEA
  implementations.  Both implement coherent virtual-keyboard behavior; no
  target-only duplicate branch or nonsensical coincidence is retained.
- Raw numeric key codes, UI sounds, and saved-game errors were replaced with
  their authenticated enum constants.  The keyboard layout table uses the
  `_vkey_*` domain rather than hexadecimal indices.
- Virtual-keyboard tag access goes through the typed
  `virtual_keyboard_definition_get` and `virtual_keyboard_key_get` macros;
  the caret bitmap uses `BITMAP_GROUP_TAG`.
- The buffer type is the cseries `word`.  Declarations are initialized where
  natural, parameters use one line each, no-argument lists use `void`, and
  every void body ends in an explicit `return;`.
- Unicode, UI, bitmap, and saved-game APIs are included through their owner
  headers.  `saved_game_file_name_unique` is declared in
  `source/saved games/saved_game_files.h`, not in this consumer `.c`.
- Compact spacing is retained only inside four `match_assert` predicates.
  January's decorated assertion literals prove those exact source spellings;
  normalizing their spaces would change runtime diagnostic strings.
- No inline assembly, forced inline/noinline, volatile or register forcing,
  pragma, raw address, fabricated dependency, undefined behavior, or
  `point_from_line3d` emission is present.

## Honest residual boundary

`_virtual_keyboard_select` is a complete ordinary-C implementation of save-name
validation, mode toggles, edit/navigation actions, character insertion, the
`.fortune` path, feedback, and one-shot shift reset.  It has January's exact
1,088-byte envelope and all 123 relocations.  The fail-closed blocker
classifier returns `UNKNOWN`; the remaining instruction and relocation-order
differences are not grounds for optimizer tricks.  The prior natural HCEA
adaptation was structurally worse at 1,120 bytes.  The retained candidate is
therefore recorded as an `unclassified` fuzzy park in `config/parked.json`.

The renderer stays unwritten.  The earlier complete HCEA rendering donor was
1,568 bytes with 87 relocations versus January's 1,616 bytes and 95
relocations, and also emitted a different rectangle-table ownership topology.
No stronger renderer evidence arrived with this Fable wave, so that rejected
body is not reintroduced.

## Verification

- Final isolated gate: 18 exact, one residual, one unwritten; 2,400 exact
  padded bytes of 5,104.
- Full 572-unit rebuild and report generation: pass; semantic audit has zero
  unit errors and recognizes `_virtual_keyboard_process_internal` as a hidden
  exact function with its target hash.
- Objdiff unit report: selector 93.7518%; process 89.38053% before hardened
  private-relocation resolution.
- Fail-closed selector classifier: `UNKNOWN`, with equal padded size and
  relocation count.
- Park, admission, fake-match, Python-tooling, exact-set, and canonical
  integration results are recorded by the admitting commit after replay.

## Do not repeat

- Do not respell the compact assertion predicates: the spaces are embedded in
  January runtime strings.
- Do not add keepalives or forced inlining merely to change helper ownership.
- Do not promote the selector from similarity percentage alone.
- Do not retry the already-rejected HCEA renderer without new authenticated
  evidence that explains both its code and ownership gaps.
