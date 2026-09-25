"""CPU-tuning diagnostic, not a proposed production flag change.

The adjacent swap separates a byte-load from its dependent indexed load.
G5/G6/G7 controls test whether the block-order effect is CPU-sensitive.
"""
import json
import probe

rows = [probe.run("flag_" + flag[1:] + "_diagnostic", [], "canonical", [flag]) for flag in ("/G5", "/G6", "/G7")]
(probe.OUT / "flags_measurements.json").write_text(json.dumps(rows, indent=2) + "\n")
