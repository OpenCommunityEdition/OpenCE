"""scenarios/obstacle.c: a wall from x 1.0 to 1.5, y -10 to 10, 2 high; the player is 0.25 round."""

PLAY = dict(seconds=90)
FACE = 1.0 - 0.25


def along(play):
    return [x for x, y, _ in play("obstacle").places() if abs(y) < 10.0]


def test_never_in_or_past_the_wall(play):
    assert max(along(play)) <= FACE + 1e-3


def test_stopped_at_its_face(play):
    assert any(x > FACE - 0.01 for x in along(play)), "the player never reached the wall"
