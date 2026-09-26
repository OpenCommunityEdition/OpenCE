"""A/B/AB for body type and the real canonical TU/helper context."""
import json
import probe

rows = []
for name, edits, context in (
    ("C_canonical_context", [], "canonical"),
    ("AC_counter_canonical_context_diagnostic", probe.COUNTER, "canonical"),
    ("BC_side_canonical_context_diagnostic", probe.SIDE, "canonical"),
):
    rows.append(probe.run(name, edits, context))
(probe.OUT / "context_measurements.json").write_text(json.dumps(rows, indent=2) + "\n")
