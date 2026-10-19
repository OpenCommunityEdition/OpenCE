"""What every StageKit scenario does (README.md): its map needs no correction, and played, the game raises no
exception, the player is in it, and played again it plays the same."""

import subprocess

import pytest
import runner
from runner import SCENARIOS


def test_scenarios_list(stagekit):
    listed = subprocess.run([str(stagekit), "--list"], capture_output=True, text=True, check=True).stdout
    assert sorted(line.split()[0] for line in listed.splitlines()) == SCENARIOS


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_map_needs_no_correction(stagekit, scenario, tmp_path):
    assert runner.validate(runner.generate(stagekit, scenario, tmp_path)) == 0


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_no_engine_exception(play, scenario):
    assert "EXCEPTION" not in play(scenario).debug


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_player_plays(play, scenario):
    session = play(scenario)
    assert len(session.places()) == len(session.seconds) > 0


@pytest.mark.parametrize("scenario", SCENARIOS)
def test_plays_the_same_every_time(play, scenario):
    """the game's whole log of each second of the slice, every player's every field, the same"""
    assert play(scenario, 1).logged() == play(scenario).logged()
