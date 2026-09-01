import json
import os
from pathlib import Path

from tools.decomp_treemap_multilane import (
    SourceSpec,
    ViewerServer,
    merge_reports,
    normalize_source,
)


def _report(functions, fuzzy=50.0):
    return {
        "units": [
            {
                "name": "source/units/example",
                "measures": {
                    "total_code": str(sum(int(fn["size"]) for fn in functions)),
                    "fuzzy_match_percent": fuzzy,
                },
                "functions": functions,
                "sections": [
                    {
                        "name": ".text",
                        "size": str(sum(int(fn["size"]) for fn in functions)),
                    }
                ],
                "metadata": {},
            }
        ]
    }


def _semantic(**sizes):
    return {
        "accepted_ledger": [
            {
                "unit": "source/units/example",
                "function": name,
                "code_bytes": code_bytes,
                "padded_bytes": padded_bytes,
                "proof_sources": ["semantic-coff"],
            }
            for name, (code_bytes, padded_bytes) in sizes.items()
        ]
    }


def _spec():
    return SourceSpec("canonical", Path("build/report.json"), "canonical")


def test_semantic_overlay_closes_unit_and_hides_local_continuations():
    report = _report(
        [
            {"name": "_owner", "size": "40", "fuzzy_match_percent": 25.0, "address": "0"},
            {"name": "$L123", "size": "8", "address": "40"},
        ]
    )

    normalized = normalize_source(_spec(), report, _semantic(_owner=(40, 48)))
    unit = normalized["units"]["source/units/example"]

    assert unit["pct"] == 100.0
    assert unit["total"] == 40
    assert unit["semanticComplete"] is True
    assert unit["functionOrder"] == ["address:0:size:40"]
    assert next(iter(unit["functions"].values()))["pct"] == 100.0


def test_partial_semantic_overlay_keeps_unit_fuzzy_percentage():
    report = _report(
        [
            {"name": "_exact", "size": "16", "fuzzy_match_percent": 20.0, "address": "0"},
            {"name": "_open", "size": "32", "fuzzy_match_percent": 75.0, "address": "16"},
        ],
        fuzzy=60.0,
    )

    normalized = normalize_source(_spec(), report, _semantic(_exact=(16, 16)))
    unit = normalized["units"]["source/units/example"]
    functions = list(unit["functions"].values())

    assert unit["pct"] == 60.0
    assert unit["semanticComplete"] is False
    assert [fn["pct"] for fn in functions] == [100.0, 75.0]


def test_complete_canonical_does_not_reimport_lane_local_labels(tmp_path):
    report = _report(
        [
            {"name": "_owner", "size": "40", "fuzzy_match_percent": 25.0, "address": "0"},
            {"name": "$L123", "size": "8", "address": "40"},
        ]
    )
    canonical_path = tmp_path / "canonical-report.json"
    lane_path = tmp_path / "lane-report.json"
    canonical_path.write_text("{}", encoding="utf-8")
    lane_path.write_text("{}", encoding="utf-8")
    canonical = normalize_source(
        SourceSpec("canonical", canonical_path, "canonical"),
        report,
        _semantic(_owner=(40, 48)),
    )
    lane = normalize_source(
        SourceSpec("lane", lane_path, "lane"),
        report,
    )

    merged = merge_reports([canonical, lane], [])
    unit = merged["units"][0]

    assert unit["canonicalPct"] == 100.0
    assert unit["unionPct"] == 100.0
    assert len(unit["functions"]) == 1
    assert unit["functions"][0]["name"] == "_owner"


def test_complete_semantic_owner_set_excludes_nonlocal_alias_rows():
    report = _report(
        [
            {"name": "_owner", "size": "40", "fuzzy_match_percent": 25.0, "address": "0"},
            {"name": "_alias", "size": "8", "fuzzy_match_percent": 100.0, "address": "40"},
        ]
    )

    normalized = normalize_source(
        _spec(), report, _semantic(_owner=(40, 48))
    )
    unit = normalized["units"]["source/units/example"]

    assert unit["total"] == 40
    assert [fn["rawName"] for fn in unit["functions"].values()] == ["_owner"]


def test_semantic_size_mismatch_fails_closed():
    report = _report(
        [{"name": "_owner", "size": "40", "fuzzy_match_percent": 25.0, "address": "0"}]
    )

    normalized = normalize_source(
        _spec(), report, _semantic(_owner=(39, 48))
    )
    unit = normalized["units"]["source/units/example"]

    assert unit["pct"] == 50.0
    assert unit["semanticComplete"] is False
    assert next(iter(unit["functions"].values()))["pct"] == 25.0


def test_ordinary_rejection_does_not_render_green():
    report = _report(
        [{"name": "_owner", "size": "40", "fuzzy_match_percent": 100.0, "address": "0"}],
        fuzzy=100.0,
    )
    semantic = {
        "accepted_ledger": [],
        "ordinary_rejected": {
            "unit": "source/units/example",
            "function": "_owner",
            "code_bytes": 40,
        },
    }

    normalized = normalize_source(_spec(), report, semantic)
    unit = normalized["units"]["source/units/example"]

    assert unit["pct"] == 99.998
    assert next(iter(unit["functions"].values()))["pct"] == 99.998


def test_server_ignores_a_stale_semantic_sidecar(tmp_path):
    report_path = tmp_path / "report.json"
    semantic_path = tmp_path / "semantic_report.json"
    report_path.write_text(
        json.dumps(
            _report(
                [
                    {
                        "name": "_owner",
                        "size": "40",
                        "fuzzy_match_percent": 25.0,
                        "address": "0",
                    }
                ]
            )
        ),
        encoding="utf-8",
    )
    semantic_path.write_text(
        json.dumps(_semantic(_owner=(40, 40))),
        encoding="utf-8",
    )
    os.utime(semantic_path, (1, 1))
    os.utime(report_path, (2, 2))

    server = ViewerServer(
        ("127.0.0.1", 0),
        [SourceSpec("canonical", report_path, "canonical")],
        [],
    )
    try:
        envelope = json.loads(server.build_envelope())
    finally:
        server.server_close()

    unit = envelope["units"][0]
    assert unit["canonicalPct"] == 50.0
    assert envelope["sources"][0]["semanticPath"] is None
    assert any("older than objdiff report" in item for item in envelope["warnings"])


def test_parked_reports_do_not_borrow_a_live_semantic_sibling():
    spec = SourceSpec("snapshot", Path("parked/report.json"), "parked")

    assert spec.semantic_path is None
