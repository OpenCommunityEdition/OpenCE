# Controller button remapping

Gamepad Setup contains **Button Remapping**. The player selector edits P1-P4
independently, using the same input-port assignments as controller detection.
Layouts belong to those slots on this installation, not to device serial numbers
or saved player profiles. Unconfigured slots continue using their profile preset.

Select an action and press the desired controller button, or cycle its assignment
with left/right. Capture waits for the opening press to be released. Back/Select,
Start/Options or keyboard Escape cancels capture; B/Circle remains assignable.
Only the selected port can supply the new binding. Disconnection cancels capture.
Names follow the detected family even with Auto Button Detection off.
Unknown or disconnected controllers use familiar Xbox labels; internal
physical-position tokens are never displayed as menu button names.

Each action has one button. Assigning a button already used by another action
swaps the two, so no gameplay action becomes unreachable. An unused button can
replace the selected action directly. Use / Reload remains
combined. D-pad Left is also available as a custom binding; when assigned,
it acts as that action instead of movement. Other D-pad directions,
stick axes, Start and Back retain their existing roles. Menu navigation, keyboard bindings, touch controls and chat-keyboard
navigation remain independent. Save writes pending layouts; Cancel discards them.
Defaults clears the selected slot's custom override when saved, restoring its
profile preset. A custom layout takes precedence over subsequent profile button
preset changes until cleared; sensitivity and stick layout still follow the profile.

For example, select P2, choose Jump and press the button currently assigned to
Melee. After Save, those actions exchange physical buttons for P2 alone. Its
gameplay Jump and Melee prompts follow the new assignments, in the selected Button
Style, with the existing Xbox/CE glyph geometry.

## Reading route and integration

1. [controller_remap.c](../../port/linux/src/controller_remap.c) owns configuration
   parsing, physical-position tokens, family labels and conflict swapping.
   `controller_remap_read` caches validated complete layouts by `config_changes`.
   Empty, incomplete, duplicate or unknown tokens fall back to the profile.
2. [controller_remap_menu.inl](../../port/linux/game/controller_remap_menu.inl)
   owns pending edits and capture state. `pad_editor_event` handles Save, Cancel,
   Defaults and selection; `pad_editor_update` supplies live names and help.
   [port_settings.py](../../tools/port_settings.py), `_controller_screen`, generates
   the submenu. `ce_menus.py --settings-only` also registers it for embedding.
3. [controller_remap_input.c](../../port/linux/src/controller_remap_input.c),
   `controller_remap_input`, translates only physical controller button pressure
   into the existing profile's logical buttons. The engine retains its original
   action interpretation and hold timers. `controller_remap_merge` then uses the
   existing maximum-pressure/greatest-stick-deflection merge rules.
4. [xinput_sdl.c](../../port/linux/src/xinput_sdl.c), `XInputGetState`, is the minimal
   integration point needed to separate physical input before keyboard and touch
   merge. SDL port assignment and its original state reader remain intact. Capture
   suppresses the edited menu's input until release. Other players still in gameplay
   retain their input. The keyboard capture service supplies Escape cancellation
   and its existing release guard.
5. [controller_remap_game.c](../../port/linux/game/controller_remap_game.c) reads
   stock profile preferences and menu context through existing interfaces. It never
   changes the profile or game-input structure. `controller_remap_prompt` maps the
   viewport to its controller and changes a local copy of prompt preferences.
   The small `port:` hook in
   [hud_messaging.c](../../source/interface/hud_messaging.c) is necessary because
   that copy is otherwise inaccessible to the feature. It changes button identity,
   not glyph size, spacing, anchoring or map loading. Xbox and CE retain their
   existing icon renderers and ownership boundaries.

## State, persistence and checks

`input.controller_p1` through `input.controller_p4` default to empty strings.
Nonempty values contain ten comma-separated physical-position names, in this
order: Jump, Switch Grenade, Use / Reload, Switch Weapon, Melee, Flashlight,
Throw Grenade, Fire, Crouch, Zoom. Choose ten unique entries from `south,east,west,north,rb,lb,lt,rt,ls,rs,dpad_left`. Physical south is A on Xbox, Cross on PlayStation and B
on Switch; changing Button Style cannot change its meaning.

Capture has no map-owned allocations. Closing the screen cancels it; a watchdog
also cancels abandoned capture if the screen stops polling. The captured press is
consumed until release. Disconnects reset transient per-port input state without
erasing saved layouts. Entering gameplay or changing layouts suppresses buttons
already held until release, preventing an opening/closing press from becoming a
game action. No new simulation flags, network messages or profile file fields are
introduced; the network version is unchanged.

[Remapping tests](../../tools/test_controller_remap.py) execute the production
parser, input transform, capture state machine, prompt mapping and menu handlers.
They cover slot isolation, conflict swaps, malformed configuration, trigger
pressure, menu passthrough, release guards, disconnects, cancellation, abandoned
screens, Save/Cancel/Defaults and generated submenu registration. These tests do
not establish physical driver behavior. Runtime menu captures, map-family prompt
checks and device playtesting remain distinct validation steps.
