"""scenarios/elevator.c: the player on a platform 0.25 high; a second in, it rises 5 units in 2 seconds, and at 5
seconds comes down."""

TOP = 0.25
CUES = [(1, "device_set_position elevator 1"), (5, "device_set_position elevator 0")]
PLAY = dict(seconds=10, input="look:1", cues=CUES)


def rides(session, player):
    """on the platform throughout, alive: at the top in between, back at the bottom by the end, never under it"""
    heights = [z for _, _, z in session.places(player)]
    assert session.deaths(player) == 0
    assert len(heights) == len(session.seconds), heights
    assert abs(heights[0] - TOP) < 0.05, heights
    assert max(heights) > TOP + 4.95, heights
    assert abs(heights[-1] - TOP) < 0.05, heights
    assert min(heights) > TOP - 0.05, heights


def test_rides_it_up_and_down(play):
    rides(play("elevator"), 0)
