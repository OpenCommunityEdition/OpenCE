import json

from tools.link_probe import diagnostic_summary, manifest_objects


def test_manifest_excludes_missing_units_and_requires_compiled_inputs(tmp_path):
    config = tmp_path / "config"
    config.mkdir()
    (config / "config.json").write_text(
        json.dumps(
            {
                "projects": [
                    {
                        "name": "halo",
                        "objects": [
                            {"name": "source/ready.c", "status": "NonMatching"},
                            {"name": "source/unbuilt.c", "status": "Matching"},
                            {"name": "source/synthetic.c", "status": "MISSING"},
                        ],
                    }
                ]
            }
        ),
        encoding="utf-8",
    )
    ready = tmp_path / "build" / "base" / "source" / "ready.obj"
    ready.parent.mkdir(parents=True)
    ready.write_bytes(b"object")

    objects, missing = manifest_objects(tmp_path)

    assert objects == [ready.resolve()]
    assert [entry["source"] for entry in missing] == ["source/unbuilt.c"]

    replacement = tmp_path / "replacement.obj"
    replacement.write_bytes(b"new object")
    objects, missing = manifest_objects(
        tmp_path,
        excluded_sources={"source/unbuilt.c"},
        replacements={"source/ready.c": replacement},
    )
    assert objects == [replacement.resolve()]
    assert not missing


def test_diagnostic_summary_distinguishes_references_from_unique_symbols():
    output = "\n".join(
        [
            "a.obj : error LNK2019: unresolved external symbol _missing referenced in function _a",
            "b.obj : error LNK2001: unresolved external symbol _missing",
            "c.obj : error LNK2005: _duplicate already defined in d.obj",
            "LINK : fatal error LNK1120: 1 unresolved externals",
        ]
    )

    summary = diagnostic_summary(output)

    assert summary["counts"]["LNK2019"] == 1
    assert summary["counts"]["LNK2001"] == 1
    assert summary["counts"]["LNK2005"] == 1
    assert summary["unique_unresolved_seen"] == 1
    assert summary["unique_duplicate_definitions"] == 1
    assert summary["duplicate_symbols"] == ["_duplicate"]
    assert summary["linker_unresolved_count"] == 1
