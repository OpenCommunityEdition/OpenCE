# Voice chat between the Mac and a Linux machine (manual test)

Needs: the Mac (PowerSpark) with a microphone and speakers or headphones, a
Linux machine (the Mac Pro) with a headset, both on one network, the same
`HALO_PORT_NETWORK_VERSION`, and the same map. Use headphones to avoid echo.

Settings (`config.toml`, or the environment):

- `audio.voice_chat = "push_to_talk"` on both (then `"open_mic"` below).
- `controls.push_to_talk` is V by default.
- `network.voice_mode` on the host: `"team_global_enemy_proximity"` by default;
  for the test set `network.voice_lobby = true` (default) and test in the lobby.

Test 1, Mac hosts, Linux joins
1. Mac: Multiplayer > Create Game > LAN. Linux: Join Game > LAN.
2. On the Mac, hold V and say "one two three". The Linux player should hear it
   within about half a second. Release V: it stops at once.
3. On Linux, hold V and speak: the Mac player hears it. Both at once: both hear
   each other.
4. The Mac's log (`HALO_MAC_ENGINE_LOG=1`) should show `voice chat: microphone open`
   at the first press, and not before. Nothing should show it with
   `audio.voice_chat = "off"`.

Test 2, Linux hosts, Mac joins. As above (the host relays the voice).

Test 3, open mic. Set `audio.voice_chat = "open_mic"` on the Mac: speaking
(not silence or keys) sends; a quiet room sends nothing. Set `"off"`: nothing
sends, but the Mac still hears the others.

Test 4, in a game. Start the game: voices follow `network.voice_mode`
(teammates everywhere, enemies near). Mute voice on the scoreboard works.

Test 5, devices. `audio.input_device = "<a name from the Mac's list>"` (or
`HALO_AUDIO_INPUT_DEVICE`) opens that microphone; an unknown name uses the
default.

Report any silence, clipping, a pitch that is off (a wrong sample rate would
sound too high or too low), or a delay that grows.
