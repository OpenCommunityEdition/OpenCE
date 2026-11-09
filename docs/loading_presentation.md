# Loading presentation

`display.widescreen_loading` defaults to true. PC Settings > Video > Graphics
provides the Widescreen Loading toggle, also accessible through the PC-style
pause menu's Settings. Xbox menus retain their existing controls; the setting
can also be changed in config.toml or with HALO_WIDESCREEN_LOADING.
Xbox, Custom Edition and MCC maps use it.

The installation's original loading picture, mask, sweep, feedback blur,
glow trails, wobble and fade remain. Widescreen expands the entire composition
horizontally; false restores the central 4:3 framing. No replacement art is
bundled. The Xbox chant plays in either mode, under master/effects volume.

Read `progress_bar_begin`, `progress_bar_render` and `progress_bar_end` in
[progress_bar.c](../source/interface/progress_bar.c), then
[loading_widescreen.c](../port/linux/src/loading_widescreen.c) and
`front_buffer_take` in [d3d8_gl.c](../port/linux/src/d3d8_gl.c).
The feature module latches the setting for a loading session and resets it
at completion or shutdown. Drawing and feedback capture must agree on width:
at 1920x1080 both use the whole frame when enabled, or its central 1440x1080
region when disabled. The feedback texture remains 640x480 in both modes.
These small engine hooks are necessary because projection and front-buffer
capture are private to their respective implementations. Map loading and
simulation contracts are unchanged.

MCC conversion keeps its own stage tracking and draw scheduling in
[mcc_loading.c](../port/linux/game/mcc_loading.c). `mcc_loading_begin` starts
the same stock loading renderer, `mcc_loading_pulse` displays its progress,
and `mcc_loading_end` closes it on success or failure. Consequently the same
framing choice, mask, feedback and chant apply without a second MCC effect.
The MCC pulse draws only this standalone effect while tags are incomplete;
it does not run ordinary gameplay or menu rendering during conversion.

The game already supplies the chant's samples, four voices, pitches and volume
envelopes. `mix_buffers` and the DirectSound buffer functions in
[dsound_sdl.c](../port/linux/src/dsound_sdl.c) implement their playback.
Buffers own decoded samples, with mutable state protected by the mixer lock.
Stop, restart and shutdown work without waiting for another ordinary frame.
Existing stream mixing, including cut fades, retains its path.

[test_loading_widescreen.py](../tools/test_loading_widescreen.py) exercises
production drawing/capture at 4:3, widescreen and ultrawide sizes and session
reset. [test_loading_chant.py](../tools/test_loading_chant.py) checks the actual
samples, buffer ownership, pitch, volume, looping and lifecycle. These tests
do not establish physical audio output or complete map compatibility.
