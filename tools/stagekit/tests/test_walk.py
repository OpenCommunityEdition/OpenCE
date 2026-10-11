"""scenarios/walk.c: a biped in the chamber, the scripted bot on its controller."""

PLAY = dict(seconds=30)


def test_stands_on_the_floor(play):
    """z 0 between jumps, never below"""
    places = play("walk").places()
    assert min(z for _, _, z in places) == 0.0
    assert sum(z == 0.0 for _, _, z in places) > len(places) // 2


def test_moves(play):
    assert max(abs(x) + abs(y) for x, y, _ in play("walk").places()) > 1.0


def test_jumps_less_than_a_unit(play):
    """(jump velocity 0.08 a tick under the engine's gravity: a peak of about 0.9, sampled once a second)"""
    assert 0.2 < max(z for _, _, z in play("walk").places()) < 1.0


def test_walks_on_the_floor(play):
    """from a second on the floor to the next, it has moved: not only in its jumps"""
    places = play("walk").places()
    grounded = [(a, b) for a, b in zip(places, places[1:]) if a[2] == 0.0 and b[2] == 0.0]
    assert grounded and all(abs(a[0] - b[0]) + abs(a[1] - b[1]) > 0.05 for a, b in grounded), grounded
