"""MCC particle-system marker ownership and per-view visibility, using real code."""
from pathlib import Path
import subprocess

import pytest
from harness import function, enum_with
from test_mcc_identity import compile_tool

ROOT = Path(__file__).resolve().parents[1]


@pytest.fixture(scope="module")
def particles(tmp_path_factory):
    work = tmp_path_factory.mktemp("mcc-particles")
    adapter = (ROOT / "port/linux/game/mcc_particles.c").read_text()
    definitions = (ROOT / "source/effects/particle_system_definitions.h").read_text()
    fixture = (ROOT / "tools/harness/tests/mcc_particles.c").read_text()
    implementation = enum_with(definitions, "_particle_system_type_nodraw_first_person_bit")
    implementation += "\n" + "\n".join(function(adapter, name) for name in (
        "mcc_particle_system_local_player", "mcc_particle_system_markers", "mcc_particle_system_visible"))
    source = work / "particles.c"
    source.write_text(fixture.replace("/* IMPLEMENTATION */", implementation))
    return compile_tool(work, [source])


@pytest.mark.parametrize("case", ["markers", "views", "invalid", "lifecycle", "legacy"])
def test_particle_system_views(particles, case):
    result = subprocess.run([str(particles), case], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
