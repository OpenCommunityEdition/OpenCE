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


def _normalized_at(tmp_path, label, report, semantic=None, kind="lane"):
    report_path = tmp_path / f"{label}-report.json"
    report_path.write_text(json.dumps(report), encoding="utf-8")
    return normalize_source(
        SourceSpec(label, report_path, kind),
        report,
        semantic,
    )


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


def test_partial_semantic_overlay_rolls_function_credit_up_to_unit():
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

    assert abs(unit["pct"] - ((16 * 100.0 + 32 * 75.0) / 48)) < 1e-9
    assert unit["exact"] is False
    assert unit["semanticComplete"] is False
    assert [fn["pct"] for fn in functions] == [100.0, 75.0]
    assert [fn["exact"] for fn in functions] == [True, False]


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
    assert unit["canonicalExact"] is True
    assert unit["bestExact"] is True
    assert unit["unionExact"] is True
    assert unit["bestSource"] == "canonical"
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


def test_near_100_residual_is_not_exact_in_any_view(tmp_path):
    report = _report(
        [
            {
                "name": "_owner",
                "size": "40",
                "fuzzy_match_percent": 99.9995,
                "address": "0",
            }
        ],
        fuzzy=99.9995,
    )
    canonical = _normalized_at(
        tmp_path, "canonical", report, kind="canonical"
    )

    normalized_unit = canonical["units"]["source/units/example"]
    normalized_function = next(iter(normalized_unit["functions"].values()))
    assert normalized_unit["pct"] == 99.9995
    assert normalized_unit["exact"] is False
    assert normalized_function["pct"] == 99.9995
    assert normalized_function["exact"] is False

    merged_unit = merge_reports([canonical], [])["units"][0]
    assert merged_unit["canonicalExact"] is False
    assert merged_unit["bestExact"] is False
    assert merged_unit["unionExact"] is False
    assert merged_unit["functions"][0]["canonicalExact"] is False
    assert merged_unit["functions"][0]["bestExact"] is False
    assert merged_unit["functions"][0]["unionExact"] is False


def test_canonical_rejection_blocks_unproven_raw_100_lane_union(tmp_path):
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

    canonical = _normalized_at(
        tmp_path, "canonical", report, semantic, kind="canonical"
    )
    raw_lane = _normalized_at(tmp_path, "raw-lane", report)
    normalized_unit = canonical["units"]["source/units/example"]
    normalized_function = next(iter(normalized_unit["functions"].values()))

    assert normalized_unit["pct"] == 100.0
    assert normalized_unit["exact"] is False
    assert normalized_function["pct"] == 100.0
    assert normalized_function["exact"] is False

    merged_unit = merge_reports([canonical, raw_lane], [])["units"][0]
    assert merged_unit["canonicalExact"] is False
    assert merged_unit["bestExact"] is False
    assert merged_unit["unionExact"] is False
    assert merged_unit["functions"][0]["canonicalExact"] is False
    assert merged_unit["functions"][0]["bestExact"] is False
    assert merged_unit["functions"][0]["unionExact"] is False


def test_semantic_accepted_lane_clears_canonical_rejection(tmp_path):
    report = _report(
        [{"name": "_owner", "size": "40", "fuzzy_match_percent": 100.0, "address": "0"}],
        fuzzy=100.0,
    )
    rejection = {
        "accepted_ledger": [],
        "ordinary_rejected": {
            "unit": "source/units/example",
            "function": "_owner",
            "code_bytes": 40,
        },
    }

    canonical = _normalized_at(
        tmp_path, "canonical", report, rejection, kind="canonical"
    )
    accepted_lane = _normalized_at(
        tmp_path, "accepted-lane", report, _semantic(_owner=(40, 40))
    )
    merged_unit = merge_reports([canonical, accepted_lane], [])["units"][0]

    assert merged_unit["canonicalExact"] is False
    assert merged_unit["bestExact"] is True
    assert merged_unit["unionExact"] is True
    assert merged_unit["bestSource"] == "accepted-lane"
    assert merged_unit["functions"][0]["canonicalExact"] is False
    assert merged_unit["functions"][0]["bestExact"] is True
    assert merged_unit["functions"][0]["unionExact"] is True


def test_out_of_bounds_semantic_owner_fails_closed():
    report = _report(
        [{"name": "_owner", "size": "40", "fuzzy_match_percent": 25.0, "address": "0"}]
    )

    normalized = normalize_source(
        _spec(), report, _semantic(_owner=(40, 100))
    )
    unit = normalized["units"]["source/units/example"]
    function = next(iter(unit["functions"].values()))

    assert unit["pct"] == 50.0
    assert unit["exact"] is False
    assert unit["semanticComplete"] is False
    assert function["pct"] == 25.0
    assert function["exact"] is False
    assert function["semanticAccepted"] is False


def test_duplicate_semantic_owner_records_fail_closed():
    report = _report(
        [{"name": "_owner", "size": "40", "fuzzy_match_percent": 25.0, "address": "0"}]
    )
    semantic = _semantic(_owner=(40, 40))
    semantic["accepted_ledger"].append(dict(semantic["accepted_ledger"][0]))

    normalized = normalize_source(_spec(), report, semantic)
    unit = normalized["units"]["source/units/example"]
    function = next(iter(unit["functions"].values()))

    assert unit["pct"] == 50.0
    assert unit["exact"] is False
    assert unit["semanticComplete"] is False
    assert function["pct"] == 25.0
    assert function["exact"] is False
    assert function["semanticAccepted"] is False


def test_linked_metadata_is_revoked_when_effective_code_is_not_exact(tmp_path):
    report = _report(
        [{"name": "_owner", "size": "40", "fuzzy_match_percent": 75.0, "address": "0"}],
        fuzzy=75.0,
    )
    report["units"][0]["metadata"]["complete"] = True

    canonical = _normalized_at(
        tmp_path, "canonical", report, kind="canonical"
    )
    normalized_unit = canonical["units"]["source/units/example"]
    normalized_function = next(iter(normalized_unit["functions"].values()))

    assert normalized_unit["exact"] is False
    assert normalized_unit["linked"] is False
    assert normalized_function["exact"] is False
    assert normalized_function["linked"] is False

    merged_unit = merge_reports([canonical], [])["units"][0]
    assert merged_unit["canonicalExact"] is False
    assert merged_unit["canonicalLinked"] is False
    assert merged_unit["anyLinked"] is False
    assert merged_unit["functions"][0]["canonicalLinked"] is False
    assert merged_unit["functions"][0]["anyLinked"] is False


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


def test_server_refreshes_discovered_source_inventory(tmp_path):
    canonical_path = tmp_path / "canonical.json"
    lane_path = tmp_path / "lane.json"
    canonical_path.write_text(json.dumps(_report([])), encoding="utf-8")
    lane_path.write_text(json.dumps(_report([])), encoding="utf-8")
    canonical = SourceSpec("canonical", canonical_path, "canonical")
    lane = SourceSpec("lane", lane_path, "lane")
    state = {"include_lane": False}

    def discover():
        return ([canonical, lane] if state["include_lane"] else [canonical]), []

    server = ViewerServer(
        ("127.0.0.1", 0),
        [canonical],
        [],
        source_discovery=discover,
    )
    try:
        first = json.loads(server.build_envelope())
        state["include_lane"] = True
        second = json.loads(server.build_envelope(force=True))
    finally:
        server.server_close()

    assert [source["label"] for source in first["sources"]] == ["canonical"]
    assert [source["label"] for source in second["sources"]] == [
        "canonical",
        "lane",
    ]


def test_parked_reports_do_not_borrow_a_live_semantic_sibling():
    spec = SourceSpec("snapshot", Path("parked/report.json"), "parked")

    assert spec.semantic_path is None
