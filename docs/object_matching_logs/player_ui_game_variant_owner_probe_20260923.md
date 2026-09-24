# Player UI game-variant owner probe (2026-09-23)

This is an evidence-only packet. No source, header, configuration, function
credit, or whole-object label is proposed for admission. The scratch source
experiment was fully reverted.

## Evidence

`source/interface/player_ui.c` and
`source/interface/ui_widget_game_data_input_functions.c` each declare a local
104-byte `struct playlist_profile` with a 0x47-byte opaque middle. This is the
reason for Player UI's `source-layout-incomplete` admission hold, despite its
42/42 exact target functions and exact target-owned data.

The first-party `source/saved games/playlist_profile.c` reads and writes the
same 104-byte record as `struct game_variant` through `playlist_profile_get`,
`playlist_profile_save`, and the default-variant builders. It writes the wide
display name to offset zero, clears `unknown16` at offset 0x16, and reads or
writes `unknown64` as the profile flags. The matching 104-byte `game_variant`
type already lives in its genuine owner, `source/game/game_engine.h`. Player
UI's own cast from `struct playlist_profile *` to `struct game_variant *` at
the saved-game API is further evidence that the local record is a duplicate
view, not a separate serialized type. January's 0x68 copy sizes and accessed
offsets are consistent with this relationship.

The 2011 HCEX PDB's later, *incompatible-size* `game_variant` records a wide
`human_readable_game_description` at offset zero and `flags` at its end. This
supports field meaning only; its size and offsets must not be transplanted
into January.

## Bounded scratch experiment

On baseline `e9e62b786754580e1a6ed271c9bfacc8c69c6d5c`, with the same
canonical objdiff 3.3.1 binary, a temporary four-file packet:

1. Exposed `wchar_t name[12]` and a 16-bit terminal `profile_flags` view in
   the existing 0x68-byte `game_variant` owner type.
2. Replaced Player UI's local playlist struct with `struct game_variant`,
   removing its cast at the saved-game API.
3. Changed `player_ui_get_edit_playlist_profile` to return the owner type.
4. Replaced the second local duplicate in
   `ui_widget_game_data_input_functions.c` and its direct consumers.

`ninja -j8 all_source progress semantic_progress` passed. A frozen full-board
stable sweep changed 7,596 exact functions to 7,595: the unrelated
`_player_profile_3wide_list_update` regressed, with no gain. The return-type
change also exposed two C4133 incompatible-pointer warnings in
`ui_widget_event_handler_functions.c`, whose raw-offset/wide-character local
views require their own typed reconstruction. No source edit was retained;
the source tree was restored to a clean state.

The result is **zero credited code bytes, zero completed objects, and a
rejected packet**. The real relationship of the types is useful evidence, but
the four-file patch is not safe to merge. A local-only Player UI fix would
retain an invented duplicate or add representation casts; neither clears the
source-layout hold.

## Reopen criterion

Coordinate an owner-header and UI-family packet that represents the serialized
record once, in `game_engine.h`, and migrates Player UI, Widget Game Data Input,
and Widget Event Handler together. Keep the 0x68 January layout; use the later
PDB only for naming. The packet must remove the new type warnings, retain
`_player_profile_3wide_list_update` and every other inherited exact function,
pass normal provider links and data/owner audits, and avoid opaque padding or
byte-steering declarations. `ui_widget_game_data_input_functions.c` is in the
concurrent Claude portfolio, so no production edit was made here.
