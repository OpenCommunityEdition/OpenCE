"""Real syntax allocation, interpretation, collection and saved console ownership."""
import os
from pathlib import Path
import re
import subprocess
import sys

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import ROOT, enum_with, function, mutated, read, structure


def declaration(source, name):
    return re.search(r"^[^\n]*\b" + re.escape(name) + r"\[.*?^};", source, re.M | re.S).group()


@pytest.fixture(scope="session")
def console_state_tool(tmp_path_factory, request):
    work = tmp_path_factory.mktemp("hs-console-state")
    runtime = read("source/hs/hs_runtime.c")
    data = read("source/memory/data.c")
    data_h = read("source/memory/data.h")
    syntax = read("source/hs/hs_scenario_definitions.h")
    checkpoint = read("port/linux/game/mcc_checkpoint.c")
    compiler = read("source/hs/hs_compile.c")
    hs = read("source/hs/hs.c")
    types = "\n".join(structure(runtime, name) for name in
        ("hs_stack_frame", "hs_thread_datum", "hs_runtime_globals", "hs_global_datum"))
    types += "\n" + "\n".join(structure(data_h, name) for name in ("data_array", "datum_header"))
    types += "\n" + "\n".join(structure(syntax, name) for name in ("hs_syntax_node", "hs_script"))
    types += "\n" + "\n".join(enum_with(runtime, name) for name in
        ("_hs_thread_type_script =", "_hs_syntax_node_primitive_bit =", "HS_THREAD_STACK_SIZE =", "_hs_thread_in_function_call_bit ="))
    types += "\n" + enum_with(syntax, "_hs_script_startup =")
    types += "\n" + enum_with(read("source/hs/hs.h"), "_hs_type_void")
    types += "\n" + enum_with(read("source/hs/hs.h"), "_hs_function_begin =")
    types += "\n" + enum_with(read("source/hs/hs.h"), "NUMBER_OF_HS_OBJECT_TYPES =")
    types += "\n" + "\n".join(line for line in read("source/hs/hs.h").splitlines()
        if line.startswith("#define HS_TYPE_IS_OBJECT"))
    types += "\n" + enum_with(hs, "_hs_node_refusal_none")
    types += "\n" + structure(compiler, "hs_compile_globals") + "\n" + structure(compiler, "hs_tokenizer")
    definitions = "\n".join(line for text in (checkpoint, read("port/linux/game/mcc_campaign.h"),
        read("port/linux/game/mcc_object_names.h")) for line in text.splitlines() if line.startswith("#define MCC_"))
    data_code = "\n".join(function(data, name) for name in ("data_initialize", "data_verify", "data_usable",
        "datum_initialize", "datum_get", "datum_try_and_get", "data_delete_all", "data_make_valid",
        "data_make_invalid", "datum_new", "datum_delete", "data_next_index"))
    storage = function(checkpoint, "mcc_checkpoint_script_storage") + "\n" + re.sub(r'^#include .*$', '',
        read("port/linux/game/mcc_script_threads.inl"), flags=re.M)
    runtime_code = function(read("source/hs/hs.c"), "hs_node_gc") + "\n" + "\n".join(function(runtime, name)
        for name in ("hs_stack_push", "hs_stack_allocate", "hs_evaluate", "hs_return", "hs_thread_delete",
                     "hs_thread_main", "hs_runtime_evaluate", "hs_runtime_update"))
    valid = runtime[runtime.index("#define valid_thread("):runtime.index("\n\n", runtime.index("#define valid_thread("))]
    console = read("port/linux/game/hs_console_state.h") + "\n" + re.sub(r'^#include .*$', '',
        read("port/linux/game/hs_console_state.inl"), flags=re.M)
    policy = "\n".join(function(hs, name) for name in
        ("hs_syntax_try_get", "hs_syntax_node_check", "hs_console_syntax_node_valid"))
    compile_code = "\n".join(function(compiler, name) for name in
        ("hs_parse_string", "hs_compile_initialize", "hs_compile_dispose", "hs_compile_expression"))
    mcc_calls = "\n".join(function(read("port/linux/game/mcc_scripts.c"), name) for name in
        ("mcc_script_call_arguments_valid", "mcc_script_call_valid"))
    # hs_can_cast only checks these function pointers for presence. Keep the
    # real table's shape and NULL entries without linking conversion effects.
    cast_table = declaration(runtime, "typecasting_procedures")
    start = cast_table.index("=")
    cast_table = cast_table[:start] + re.sub(r"\b(?:hs_|object_list_)\w+\b",
        "test_cast_available", cast_table[start:])
    casts = declaration(hs, "hs_object_type_masks") + "\n" + cast_table
    casts += "\n" + "\n".join(function(read("port/linux/game/mcc_scripts.c"), name)
        for name in ("mcc_script_ai_cast", "mcc_script_can_cast"))
    casts += "\n" + "\n".join(function(runtime, name)
        for name in ("hs_object_type_can_cast", "hs_can_cast"))
    fault = getattr(request, "param", None)
    if fault == "reuse-live-source":
        console = mutated(console, "if (!marked[slot])", "if (TRUE)")
    elif fault == "ignore-console-root":
        console = mutated(console, "if (hs_console_state->roots[slot] == root) marked[slot] = TRUE;", "(void)root;")
    elif fault == "ignore-global-value":
        console = mutated(console, "hs_console_source_mark_value(marked, global->value);", "(void)global;")
    elif fault == "omit-syntax-restore":
        console = mutated(console, "memcpy(data, hs_console_state->nodes, hs_syntax_data->maximum_count * sizeof(struct hs_syntax_node));", "(void)data;")
    elif fault == "allow-foreign-result":
        console = mutated(console, "if (result != (uintptr_t)&live->result &&", "if (FALSE && result != (uintptr_t)&live->result &&")
    elif fault == "allow-invalid-cast":
        policy = mutated(policy, "hs_can_cast(actual_type, node->type);", "TRUE;")
    elif fault == "allow-function-name-value":
        console = mutated(console, "|| !hs_type_valid(next->type)))", "))")
    for name, text in {"types.inc": types, "definitions.inc": definitions, "data.inc": data_code,
                       "storage.inc": storage, "runtime.inc": runtime_code, "valid.inc": valid,
                       "console.inc": console, "policy.inc": policy, "compile.inc": compile_code,
                       "mcc_calls.inc": mcc_calls, "casts.inc": casts}.items():
        (work / name).write_text(text)
    zlib = ROOT / "port/third_party/zlib"
    binary = work / ("console.exe" if os.name == "nt" else "console")
    command = [os.environ.get("CC", "clang"), "-m32", "-std=gnu89", "-O2", "-Wall", "-Werror",
        "-Wno-unused-function", "-Wno-unused-variable", "-D_CRT_SECURE_NO_WARNINGS", "-DZ_PREFIX",
        "-Dz_errmsg=z_port_errmsg", "-I"+str(zlib), "-I"+str(work),
        str(ROOT / "tools/harness/tests/hs_console_state.c"), str(zlib / "crc32.c"), "-o", str(binary)]
    compiled = subprocess.run(command, capture_output=True, text=True)
    assert compiled.returncode == 0, compiled.stdout + compiled.stderr
    return binary


CASES = ["console_gc", "console_reuse", "scenario_gc", "standalone_gc", "console_no_gc",
         "repeat_restore", "reload", "concurrent_text", "global_text", "pool_full", "false_pin",
         "unaligned_value", "discarded_frame", "source_bounds", "capacity_boundary", "compile_text", "compile_failure", "compile_dispose",
         "inactive_permanent", "long_sleep_reuse", "mcc_console_call", "valid_casts"]
CASES += ["invalid_" + name for name in ("signature", "name", "capacity", "count", "actual", "data", "salt",
    "cycle", "string", "string_end", "source_root", "source_owner", "call", "stack", "frame_size", "result", "previous",
    "thread_array", "permanent", "literal_cast", "global_cast", "return_cast", "unparsed",
    "predicate", "primitive_flags", "global_index", "script_index", "script_kind", "script_cast", "name_argument")]


@pytest.mark.parametrize("mode", ["native", "mcc"])
@pytest.mark.parametrize("case", CASES)
def test_saved_console_lifetime(console_state_tool, mode, case):
    result = subprocess.run([str(console_state_tool), mode, case], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize("console_state_tool,case", [
    ("reuse-live-source", "compile_text"), ("ignore-console-root", "concurrent_text"),
    ("ignore-global-value", "global_text"), ("omit-syntax-restore", "console_reuse"),
    ("allow-foreign-result", "invalid_result"),
    ("allow-invalid-cast", "invalid_literal_cast"),
    ("allow-function-name-value", "invalid_name_argument"),
], indirect=["console_state_tool"])
def test_console_regressions_detect_faults(console_state_tool, case):
    result = subprocess.run([str(console_state_tool), "native", case], capture_output=True, text=True)
    assert result.returncode in (1, 3), result.stdout + result.stderr
