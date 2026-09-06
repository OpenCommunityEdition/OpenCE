"""Fixture-free contract tests for the bounded geosphere runtime oracle."""

import copy
import json
from pathlib import Path
import struct

import pytest

pytest.importorskip("unicorn")
from tools.audit import geometry_geosphere_runtime_differential as oracle


def cases_by_name():
    return {case["name"]: case for case in oracle.all_cases()}


def case_named(base_name, fpcw=0x027F):
    return cases_by_name()["%s_fpcw_%04x" % (base_name, fpcw)]


def test_case_population_is_bounded_and_has_two_explicit_x87_modes():
    cases = oracle.all_cases()
    assert len(cases) == len(cases_by_name()) == 20
    assert {case["fpcw"] for case in cases} == {0x027F, 0x037F}
    assert sum(case["fpcw"] == 0x027F for case in cases) == 10
    assert sum(case["fpcw"] == 0x037F for case in cases) == 10
    assert {
        case["name"].rsplit("_fpcw_", 1)[0]
        for case in cases
    } == {
        "valid_segments_2",
        "valid_segments_3",
        "production_segments_16",
        "outer_allocation_failure",
        "vertices_allocation_failure",
        "strips_allocation_failure",
        "subdivision_allocation_failure",
        "first_face_failure_segments_2",
        "first_face_failure_segments_3",
        "capacity_assertion_segments_1",
    }
    production = [case for case in cases if case.get("production_caller")]
    assert len(production) == 2
    assert {case["segment_count"] for case in production} == {16}


def test_numeric_diagnostic_float_order_handles_signed_zero_and_neighbors():
    ordered = oracle.ordered_float_int
    assert ordered(0x80000000) == ordered(0x00000000) == 0x80000000
    assert ordered(0x80000001) == 0x7FFFFFFF
    assert ordered(0x00000001) == 0x80000001
    assert ordered(0xC0000000) < ordered(0xBF800000) < ordered(0x80000001)
    assert ordered(0x80000001) < ordered(0x00000000) < ordered(0x00000001)


@pytest.mark.parametrize(
    "segments,vertices,triangles,strips,calculations",
    [
        (1, 6, 8, 8, 0),
        (2, 18, 32, 16, 12),
        (3, 38, 72, 24, 32),
        (16, 1026, 2048, 128, 1020),
    ],
)
def test_geometry_count_model(segments, vertices, triangles, strips, calculations):
    assert oracle.geosphere_counts(segments) == (vertices, triangles)
    case = case_named(
        "capacity_assertion_segments_1" if segments == 1 else
        "production_segments_16" if segments == 16 else
        "valid_segments_%d" % segments
    )
    graph = oracle.expected_graph_counts(case)
    helpers = oracle.expected_helper_counts(case)
    assert graph["_geosphere_new"] == 1
    assert graph["_subdivide_triangle"] == 8
    assert graph["_calculate_vertex"] == calculations
    assert helpers["_normalize3d"] == calculations
    assert all(helpers[name] == 0 for name in oracle.MATH_HELPERS if name != "_normalize3d")
    assert strips == 8 * segments


def malloc_events(case):
    return [event for event in oracle.expected_events(case) if event["api"] == "_debug_malloc"]


@pytest.mark.parametrize(
    "base_name,failure_index,expected_frees",
    [
        ("outer_allocation_failure", 0, []),
        ("vertices_allocation_failure", 1, [2, 3]),
        ("strips_allocation_failure", 2, [1, 3]),
        ("subdivision_allocation_failure", 3, [1, 2]),
    ],
)
def test_root_and_child_failure_call_order_is_explicit(base_name, failure_index, expected_frees):
    case = case_named(base_name)
    events = oracle.expected_events(case)
    mallocs = [event for event in events if event["api"] == "_debug_malloc"]
    frees = [event for event in events if event["api"] == "_debug_free"]
    assert len(mallocs) == (1 if failure_index == 0 else 4)
    assert mallocs[failure_index]["result"] == "NULL"
    assert [event["args"][0] for event in frees] == [
        oracle.allocation_label(index) for index in expected_frees
    ]
    assert all(event["args"][2] == oracle.SOURCE_FILE for event in mallocs)
    assert all(event["args"][1] == oracle.SOURCE_FILE for event in frees)


def test_per_face_failures_and_capacity_assertion_preserve_original_outcomes():
    face2 = case_named("first_face_failure_segments_2")
    face3 = case_named("first_face_failure_segments_3")
    capacity = case_named("capacity_assertion_segments_1")

    assert len(malloc_events(face2)) == len(malloc_events(face3)) == len(malloc_events(capacity)) == 12
    assert malloc_events(face2)[4]["result"] == "NULL"
    assert [event["api"] for event in oracle.expected_events(face2)[-2:]] == [
        "_debug_free", "_debug_free",
    ]
    assert oracle.expected_events(face3)[-2:] == oracle.assertion_events(99)
    assert oracle.expected_events(capacity)[-2:] == oracle.assertion_events(98)
    assert oracle.expected_graph_counts(face2)["_calculate_vertex"] == 12
    assert oracle.expected_graph_counts(face3)["_calculate_vertex"] == 31


def exact_info(spec):
    size, relocation_count, normalized_sha256 = spec
    return {
        "size": size,
        "relocation_count": relocation_count,
        "normalized_sha256": normalized_sha256,
    }


@pytest.mark.parametrize(
    "name,spec",
    list(oracle.FUNCTION_SPECS["target"].items())
    + list(oracle.FUNCTION_SPECS["compiled"].items())
    + list(oracle.HELPER_SPECS.items()),
)
def test_coff_section_guards_accept_only_pinned_metadata(name, spec):
    assert all(oracle.section_guard(exact_info(spec), spec, name).values())


@pytest.mark.parametrize("field", ("size", "relocation_count", "normalized_sha256"))
def test_coff_section_guards_reject_mutations(field):
    spec = oracle.FUNCTION_SPECS["target"]["_geosphere_new"]
    info = exact_info(spec)
    info[field] = "0" * 64 if field == "normalized_sha256" else info[field] + 1
    with pytest.raises(AssertionError, match="section guard failed"):
        oracle.section_guard(info, spec, "synthetic")


def symbolic_relocation(name):
    return {
        "address": 0,
        "type": oracle.cc.IMAGE_REL_I386_REL32,
        "target": ["symbol", name, 0],
        "symbolic_target": ["symbol", name, 0],
    }


def test_direct_relocation_graph_counts_only_real_graph_and_public_stubs():
    info = {
        "relocations": [
            symbolic_relocation("_subdivide_triangle"),
            symbolic_relocation("_debug_malloc"),
            symbolic_relocation("_debug_malloc"),
            symbolic_relocation("??_C@_synthetic"),
            {
                "address": 4,
                "type": oracle.cc.IMAGE_REL_I386_REL32,
                "target": ["internal", 12],
                "symbolic_target": ["internal", 12],
            },
        ],
    }
    assert oracle.direct_relocation_counts(info) == {
        "_subdivide_triangle": 1,
        "_debug_malloc": 2,
    }


def test_sha256_guard_uses_only_supplied_synthetic_file(tmp_path):
    path = tmp_path / "synthetic.obj"
    path.write_bytes(b"fixture-free geosphere guard")
    expected = oracle.sha256_path(path)
    assert oracle.verify_hash(path, expected, "synthetic")["sha256"] == expected
    with pytest.raises(AssertionError, match="SHA-256 mismatch"):
        oracle.verify_hash(path, "0" * 64, "synthetic")


def typed_source_text(cast=False):
    pointer = "(real_vector3d *)&vertex" if cast else "&vertex"
    return """
static void calculate_vertex(
    struct geosphere *sphere)
{
    real_vector3d vertex;
    set_real_vector3d(&vertex,
        1.f, 0.f, 0.f);
    normalize3d(%s);
    set_real_point3d(&sphere->vertices[new_vertex], vertex.i, vertex.j, vertex.k);
}
/* ---------- private code */
""" % pointer


def test_typed_source_guard_accepts_semantic_local_and_rejects_cast(tmp_path):
    source = tmp_path / "geometry.c"
    source.write_text(typed_source_text(), encoding="utf-8")
    assert all(oracle.verify_typed_source(source).values())
    source.write_text(typed_source_text(cast=True), encoding="utf-8")
    with pytest.raises(AssertionError, match="typed calculate_vertex source guard"):
        oracle.verify_typed_source(source)


def synthetic_manifests(tmp_path):
    units = ["unit-%d" % index for index in range(6)]
    before = {
        "phase": "before",
        "units": units,
        "frozen_files": {
            "build/split/source/math/geometry.obj": {"sha256": oracle.DEFAULT_HASHES["target"]},
        },
    }
    final = {
        "phase": "final",
        "units": units,
        "objects": {
            "source/math/geometry": {"sha256": oracle.DEFAULT_HASHES["actual"]},
        },
        "frozen_files": {
            "source/math/geometry.c": {"sha256": oracle.DEFAULT_HASHES["source"]},
            "source/math/geometry.h": {"sha256": oracle.DEFAULT_HASHES["header"]},
            "config/symbols.json": {"sha256": oracle.DEFAULT_HASHES["config"]},
        },
    }
    before_path = tmp_path / "before.json"
    final_path = tmp_path / "final.json"
    before_path.write_text(json.dumps(before), encoding="utf-8")
    final_path.write_text(json.dumps(final), encoding="utf-8")
    return {"before_manifest": before_path, "final_manifest": final_path}, before, final


def test_manifest_guard_accepts_exact_synthetic_ledger_and_rejects_owner_mutation(tmp_path):
    paths, unused_before, final = synthetic_manifests(tmp_path)
    assert all(oracle.verify_manifests(paths).values())
    final["objects"]["source/math/geometry"]["sha256"] = "0" * 64
    paths["final_manifest"].write_text(json.dumps(final), encoding="utf-8")
    with pytest.raises(AssertionError, match="manifest provenance guard"):
        oracle.verify_manifests(paths)


def allocation_calls(case):
    result = []
    for index, event in enumerate(malloc_events(case)):
        result.append({
            "index": index,
            "label": oracle.allocation_label(index),
            "address": 0 if event["result"] == "NULL" else oracle.allocation_address(index),
            "size": event["args"][0],
        })
    return result


def write_strip_stream(heap, pointer, segment_count, face_count):
    cursor = pointer - oracle.HEAP
    for unused_face in range(face_count):
        for row in range(1, segment_count + 1):
            header = 2 * row + 1
            struct.pack_into("<h", heap, cursor, header)
            cursor += 2
            struct.pack_into("<" + "h" * header, heap, cursor, *([0] * header))
            cursor += 2 * header


def synthetic_outcome(case):
    heap = bytearray(oracle.HEAP_SEED)
    calls = allocation_calls(case)
    failure_at = case.get("failure_at")
    segment_count = case["segment_count"]
    vertex_count, triangle_count = oracle.geosphere_counts(segment_count)
    terminal = "system_exit" if "expected_assertion_line" in case else "return"

    eax = 0 if failure_at == 0 else oracle.allocation_address(0)
    if failure_at != 0:
        vertices = 0 if failure_at == 1 else oracle.allocation_address(1)
        strips = 0 if failure_at == 2 else oracle.allocation_address(2)
        if failure_at in (1, 2, 3):
            strip_count = 0
        else:
            strip_count = (7 if failure_at == 4 else 8) * segment_count
        struct.pack_into(
            "<h2xIIhhh2x",
            heap,
            oracle.allocation_address(0) - oracle.HEAP,
            segment_count,
            vertices,
            strips,
            vertex_count,
            triangle_count,
            strip_count,
        )
        if vertices and failure_at not in (1, 2, 3):
            for index in range(vertex_count):
                struct.pack_into(
                    "<3f", heap, vertices - oracle.HEAP + 12 * index, 1.0, 0.0, 0.0,
                )
        if strips and failure_at not in (1, 2, 3):
            write_strip_stream(heap, strips, segment_count, 7 if failure_at == 4 else 8)

    frame = oracle.p32(oracle.STOP) + oracle.p32(segment_count)
    outcome = {
        "hook_errors": [],
        "execution_error": None,
        "terminal": terminal,
        "events": oracle.expected_events(case),
        "allocation_calls": calls,
        "allocations": {},
        "graph_counts": oracle.expected_graph_counts(case),
        "helper_counts": oracle.expected_helper_counts(case),
        "normalize_inputs": [struct.pack("<3f", 1.0, 0.0, 0.0)] *
            oracle.expected_helper_counts(case)["_normalize3d"],
        "heap": bytes(heap),
        "heap_sha256": oracle.sha256_bytes(heap),
        "eax": eax,
        "eip": oracle.STOP if terminal == "return" else oracle.STUB_ADDRESSES["_system_exit"],
        "esp": oracle.ENTRY_SP + 4 if terminal == "return" else oracle.ENTRY_SP - 32,
        "eflags": 0x202,
        "fpcw": case["fpcw"],
        "fpsw": 0,
        "registers": {
            oracle.SAVED_REGISTER_NAMES[register]: value
            for register, value in oracle.SAVED_REGISTERS.items()
        },
        "caller_frame": frame,
        "expected_caller_frame": frame,
    }
    oracle.augment_geometry(outcome)
    return outcome


@pytest.mark.parametrize(
    "base_name",
    (
        "valid_segments_2",
        "valid_segments_3",
        "outer_allocation_failure",
        "vertices_allocation_failure",
        "strips_allocation_failure",
        "subdivision_allocation_failure",
        "first_face_failure_segments_2",
        "first_face_failure_segments_3",
        "capacity_assertion_segments_1",
    ),
)
@pytest.mark.parametrize("fpcw", (0x027F, 0x037F))
def test_semantic_validator_accepts_modeled_layout_failures_and_explicit_fpcw(base_name, fpcw):
    case = case_named(base_name, fpcw)
    outcome = synthetic_outcome(case)
    assert oracle.validate_geometry(case, outcome) == []
    canonical = oracle.canonical_outcome(outcome)
    assert canonical["fpcw"] == fpcw
    assert canonical["fpu_top"] == 0


@pytest.mark.parametrize(
    "mutation",
    (
        "hook_error",
        "execution_error",
        "terminal",
        "allocator_clear",
        "graph_count",
        "helper_count",
        "caller_frame",
        "fpcw",
        "fpu_stack",
        "vertex_state",
        "strip_index",
        "eip",
        "esp",
        "direction_flag",
        "callee_saved",
    ),
)
def test_semantic_validator_rejects_state_call_and_abi_mutations(mutation):
    case = case_named("valid_segments_2")
    outcome = synthetic_outcome(case)
    if mutation == "hook_error":
        outcome["hook_errors"] = ["synthetic"]
    elif mutation == "execution_error":
        outcome["execution_error"] = "synthetic"
    elif mutation == "terminal":
        outcome["terminal"] = "other"
    elif mutation == "allocator_clear":
        outcome["events"][0]["args"][1] = 1
    elif mutation == "graph_count":
        outcome["graph_counts"]["_subdivide_triangle"] -= 1
    elif mutation == "helper_count":
        outcome["helper_counts"]["_normalize3d"] -= 1
    elif mutation == "caller_frame":
        outcome["caller_frame"] = b"changed"
    elif mutation == "fpcw":
        outcome["fpcw"] ^= 0x100
    elif mutation == "fpu_stack":
        outcome["fpsw"] |= 1 << 11
    elif mutation == "vertex_state":
        raw = bytearray(outcome["vertices"])
        struct.pack_into("<3f", raw, 0, 2.0, 0.0, 0.0)
        pointer = outcome["sphere"]["vertices_pointer"] - oracle.HEAP
        heap = bytearray(outcome["heap"])
        heap[pointer:pointer + len(raw)] = raw
        outcome["heap"] = bytes(heap)
        oracle.augment_geometry(outcome)
    elif mutation == "strip_index":
        pointer = outcome["sphere"]["strips_pointer"] - oracle.HEAP
        heap = bytearray(outcome["heap"])
        struct.pack_into("<h", heap, pointer + 2, outcome["sphere"]["vertex_count"])
        outcome["heap"] = bytes(heap)
        oracle.augment_geometry(outcome)
    elif mutation in ("eip", "esp"):
        outcome[mutation] ^= 4
    elif mutation == "direction_flag":
        outcome["eflags"] |= 0x400
    elif mutation == "callee_saved":
        outcome["registers"]["esi"] ^= 1
    assert oracle.validate_geometry(case, outcome)


def test_synthetic_outcome_negative_controls_detect_all_components_without_mutation():
    case = case_named("valid_segments_2")
    canonical = oracle.canonical_outcome(synthetic_outcome(case))
    reference = {case["name"]: canonical}
    untouched = copy.deepcopy(reference)
    result = oracle.run_negative_controls(reference)
    assert result["pass"]
    assert set(result["controls"]) == {
        "allocator_argument",
        "external_call_order",
        "same_object_private_count",
        "real_math_call_count",
        "normalize_input",
        "heap_state",
        "owned_vertex_state",
        "eax",
        "esp",
        "callee_saved",
        "caller_frame",
        "sha256_guard",
    }
    assert all(result["controls"].values())
    assert reference == untouched


def test_pure_contract_tests_do_not_read_frozen_campaign_files(monkeypatch):
    def forbidden(*unused_args, **unused_kwargs):
        raise AssertionError("unexpected frozen-file access")

    monkeypatch.setattr(Path, "read_bytes", forbidden)
    monkeypatch.setattr(Path, "read_text", forbidden)
    assert len(oracle.all_cases()) == 20
    assert oracle.geosphere_counts(16) == (1026, 2048)
    case = case_named("valid_segments_2")
    outcome = synthetic_outcome(case)
    assert oracle.validate_geometry(case, outcome) == []
    assert oracle.run_negative_controls({case["name"]: oracle.canonical_outcome(outcome)})["pass"]
