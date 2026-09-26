import importlib.util
from pathlib import Path
import pytest

SPEC = importlib.util.spec_from_file_location("coupled_probe", Path(__file__).with_name("coupled_probe.py"))
MOD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MOD)


def factor(old, new, count=1):
    return {"hypothesis": "test", "prediction": "test",
            "edits": [{"old": old, "new": new, "count": count}]}


def test_exact_count_edit():
    assert MOD.apply_factor("a a b", factor("a", "c", 2)) == "c c b"


def test_no_partial_or_ambiguous_apply():
    with pytest.raises(ValueError):
        MOD.apply_factor("a a b", factor("a", "c"))
    with pytest.raises(ValueError):
        MOD.apply_factor("a a b", factor("z", "c"))


def test_requires_stated_prediction():
    f = factor("a", "c")
    del f["prediction"]
    with pytest.raises(ValueError):
        MOD.apply_factor("a b", f)
