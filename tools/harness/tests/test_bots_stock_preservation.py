"""Stock decision algorithms remain the 7ea7b0bf7 implementation.
The manifest hashes original function bodies, excluding admission/dispatch APIs.
"""
import hashlib
import json
import re
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import read, function, build, run

def test_stock_algorithms_preserved():
    expected = json.loads(Path(__file__).with_name("stock_bots_functions.json").read_text())
    source = read("port/linux/game/bots.c")
    for name, digest in expected.items():
        actual = re.sub(r"\s+", "", function(source, name))
        assert hashlib.sha256(actual.encode()).hexdigest() == digest, name

def test_enhanced_controller_is_selected_before_stock_loop():
    source = function(read("port/linux/game/bots.c"), "bots_update_actions")
    dispatch = source.index("enhanced_controller_update(actions)")
    assert source.index("return;", dispatch) < source.index("data_iterator_new")
    enhanced = read("port/linux/game/enhanced_bots_controller.c")
    assert "bots_enhanced_running" not in enhanced
    assert 'include "bots.c"' not in enhanced


def test_controller_ownership_across_map_changes():
    source = read("port/linux/game/bots.c")
    code = function(source, "bots_initialize_for_new_map") + function(source, "bots_dispose_from_old_map")
    status, output = run(build("bots_controller_lifecycle", (("under_test.inc", code),)), "ownership")
    assert status == 0, output
