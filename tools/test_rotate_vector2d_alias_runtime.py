"""Fail-closed loader and original-byte alias-replay regression checks."""
import pytest

from tools.audit import rotate_vector2d_alias_runtime as runtime


def require_original_artifacts():
    if not all(path.is_file() for path in runtime.OBJECTS.values()):
        pytest.skip("local original and frozen candidate COFF evidence is unavailable")


def test_changed_object_is_rejected_before_parsing(tmp_path, monkeypatch):
    changed = tmp_path / "changed.obj"
    changed.write_bytes(b"not the pinned object")
    monkeypatch.setitem(runtime.OBJECTS, "target", changed)
    with pytest.raises(AssertionError, match="SHA mismatch"):
        runtime.load_body("target")


def test_repair_is_exact_original_code():
    require_original_artifacts()
    target, _ = runtime.load_body("target")
    repaired, _ = runtime.load_body("repaired")
    old, _ = runtime.load_body("old_negative")
    assert target == repaired
    assert target != old


@pytest.mark.parametrize("control", runtime.FPCWS)
def test_genuine_old_body_exposes_in_place_bug(control):
    require_original_artifacts()
    _, *values = runtime.CASES[0]
    bits = tuple(values)
    outputs = {}
    for role in runtime.OBJECTS:
        body, _ = runtime.load_body(role)
        outputs[role] = [runtime.run(body, bits, control, alias)["output_hex"]
                         for alias in (False, True)]
    assert outputs["target"][0] == outputs["target"][1]
    assert outputs["repaired"] == outputs["target"]
    assert outputs["old_negative"][0] != outputs["old_negative"][1]
