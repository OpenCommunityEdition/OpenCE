"""scenarios/vibrate.c: a Custom Edition map's startup script calls Halo PC's player_effect_set_max_vibrate."""

PLAY = dict(seconds=3)


def test_call_given_this_builds_function(play):
    """the relinker gives the call player_effect_set_max_rumble, not a constant that does nothing"""
    session = play("vibrate")
    logs = session.log + session.debug
    assert "player_effect_set_max_vibrate', which this build does not have" not in logs
    assert "custom edition: 1 script calls and 0 engine global references were given this build's index" in logs


def test_scripts_load(play):
    """the game finds the call by its name in the string data as it loads the map (hs_compile.c)"""
    session = play("vibrate")
    logs = session.log + session.debug
    assert "missing function" not in logs
    assert "you need to recompile" not in logs
