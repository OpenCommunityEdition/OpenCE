"""Tests for the January<->HCEA cross-build bridge."""

import json
import tempfile
import unittest
from pathlib import Path

from tools.crossbuild_bridge import (
    TIER_ALIAS_FAMILY,
    TIER_AMBIGUOUS,
    TIER_EXACT_UNIQUE,
    TIER_EXACT_UNIQUE_CROSSOBJECT,
    TIER_JANUARY_ONLY,
    TIER_KNOWN_EVOLVED,
    build_correspondence,
    hcea_unit_relpath,
    is_anonymous_symbol,
    january_unit_relpath,
    normalize_coff_symbol,
    parse_dia_dump,
    summarize_census,
)

DIA_SAMPLE = """\


*** SYMBOLS



** Module: obj\\xbox\\pch.obj

CompilandEnv   : obj = "E:\\BT\\455758\\core\\lib\\obj\\xbox\\pch.obj"
CompilandEnv   : src = "obj\\xbox\\pch_hdr.src"

Function       : static, [00001000][0003:00000F00], len = 00000010, engine_glue_helper

** Module: ..\\..\\..\\build\\x360\\RELEASE_CACHE\\halo\\action_alert.obj

CompilandDetails:
\tLanguage: C
\tFrontend Version: Major = 16, Minor = 0, Build = 11886, QFE = 0
\tBackend Version: Major = 16, Minor = 0, Build = 11886, QFE = 0

CompilandEnv   : src = "D:\\Projects\\code\\HCEX\\sources\\ai\\action_alert.c"

Function       : static, [018264D0][0003:014764D0], len = 00000080, action_alert_setup
                 Function attribute:
Data           :   enregistered bl, Param, Type: long, actor_index
Data           :   enregistered ah, Param, Type: short, move_position_order

Function       : static, [01826550][0003:01476550], len = 00000004, action_alert_begin
Data           :   enregistered bl, Param, Type: long, actor_index

Function       : static, [01826558][0003:01476558], len = 00000174, moved_function
Data           :   enregistered bl, Param, Type: long, actor_index

Function       : static, [018266D0][0003:014766D0], len = 00000054, duplicated_helper

** Module: ..\\..\\..\\build\\x360\\RELEASE_CACHE\\halo\\actions.obj

CompilandEnv   : src = "D:\\Projects\\code\\HCEX\\sources\\ai\\actions.c"

Function       : static, [01830000][0003:01480000], len = 00000020, duplicated_helper

Function       : static, [01830040][0003:01480040], len = 00000030, evolved_function
Data           :   enregistered bl, Param, Type: long, actor_index
Data           :   enregistered ah, Param, Type: short, extra_parameter
"""


def _sample_modules():
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "dump.txt"
        path.write_text(DIA_SAMPLE, encoding="utf-8")
        return parse_dia_dump(path)


def _january_units():
    def function(symbol, size, accepted=False, fuzzy=None):
        return {"symbol": symbol, "size": size,
                "fuzzy_match_percent": fuzzy, "accepted_exact": accepted}

    return [
        {"unit": "source/ai/action_alert", "complete": False, "functions": [
            function("_action_alert_setup", 128),
            function("_action_alert_begin", 16, accepted=True),
            function("_code_00123450", 32),
            function("_duplicated_helper", 48),
            function("_january_removed", 64),
        ]},
        {"unit": "source/ai/actions", "complete": False, "functions": [
            function("_moved_function", 372),
            function("_evolved_function", 48),
        ]},
    ]


class NormalizeTests(unittest.TestCase):
    def test_cdecl(self):
        self.assertEqual(normalize_coff_symbol("_actor_delete"),
                         ("actor_delete", "cdecl", None))

    def test_stdcall(self):
        self.assertEqual(normalize_coff_symbol("_DirectSoundCreate@12"),
                         ("DirectSoundCreate", "stdcall", 12))

    def test_fastcall(self):
        self.assertEqual(normalize_coff_symbol("@fast_helper@8"),
                         ("fast_helper", "fastcall", 8))

    def test_cplusplus_untouched(self):
        name = "?render@@YAXXZ"
        self.assertEqual(normalize_coff_symbol(name), (name, "c++", None))

    def test_anonymous(self):
        self.assertTrue(is_anonymous_symbol("_code_00123450"))
        self.assertTrue(is_anonymous_symbol("code_00123450"))
        self.assertFalse(is_anonymous_symbol("_codegen_helper"))


class DiaParseTests(unittest.TestCase):
    def test_modules_functions_params(self):
        modules = _sample_modules()
        self.assertEqual(len(modules), 3)
        alert = modules[1]
        self.assertEqual(alert["language"], "C")
        self.assertEqual(alert["frontend_version"], "16.0.11886")
        self.assertEqual(len(alert["functions"]), 4)
        setup = alert["functions"][0]
        self.assertEqual(setup["name"], "action_alert_setup")
        self.assertEqual(setup["length"], 0x80)
        self.assertEqual(setup["scope"], "static")
        self.assertEqual([p["name"] for p in setup["params"]],
                         ["actor_index", "move_position_order"])
        self.assertEqual(setup["params"][1]["type"], "short")

    def test_unit_relpath_mapping(self):
        modules = _sample_modules()
        self.assertIsNone(hcea_unit_relpath(modules[0]))
        self.assertEqual(hcea_unit_relpath(modules[1]), "ai/action_alert")
        self.assertEqual(january_unit_relpath("source/ai/action_alert"),
                         "ai/action_alert")


class CorrespondenceTests(unittest.TestCase):
    def _tiers(self, known_evolved=None):
        result = build_correspondence(
            _january_units(), _sample_modules(), known_evolved)
        return {(m["january_unit"], m["january_symbol"]): m
                for m in result["mappings"]}, result

    def test_accepted_functions_not_mapped(self):
        mappings, _ = self._tiers()
        self.assertNotIn(("source/ai/action_alert", "_action_alert_begin"),
                         mappings)

    def test_exact_unique(self):
        mappings, _ = self._tiers()
        record = mappings[("source/ai/action_alert", "_action_alert_setup")]
        self.assertEqual(record["tier"], TIER_EXACT_UNIQUE)
        self.assertEqual(record["hcea_param_count"], 2)

    def test_crossobject_moved_function(self):
        mappings, _ = self._tiers()
        record = mappings[("source/ai/actions", "_moved_function")]
        self.assertEqual(record["tier"], TIER_EXACT_UNIQUE_CROSSOBJECT)
        self.assertEqual(record["hcea_unit_relpath"], "ai/action_alert")

    def test_ambiguous_duplicate(self):
        mappings, _ = self._tiers()
        record = mappings[("source/ai/action_alert", "_duplicated_helper")]
        self.assertEqual(record["tier"], TIER_ALIAS_FAMILY)
        self.assertEqual(record["hcea_total_candidates"], 2)

    def test_anonymous_and_removed_are_january_only(self):
        mappings, _ = self._tiers()
        self.assertEqual(
            mappings[("source/ai/action_alert", "_code_00123450")]["tier"],
            TIER_JANUARY_ONLY)
        self.assertEqual(
            mappings[("source/ai/action_alert", "_january_removed")]["tier"],
            TIER_JANUARY_ONLY)

    def test_known_evolved_never_exact(self):
        mappings, _ = self._tiers(
            {"evolved_function": "HCEA added extra_parameter"})
        record = mappings[("source/ai/actions", "_evolved_function")]
        self.assertEqual(record["tier"], TIER_KNOWN_EVOLVED)

    def test_engine_glue_never_joins(self):
        _, result = self._tiers()
        self.assertNotIn("obj/xbox/pch", result["hcea_units"])
        glue = [f for f in result["hcea_only_functions"]
                if f["hcea_name"] == "engine_glue_helper"]
        self.assertEqual(glue, [])

    def test_summary_counts(self):
        _, result = self._tiers()
        summary = summarize_census(result)
        # action_alert_setup plus evolved_function (no evolved list passed here)
        self.assertEqual(summary[TIER_EXACT_UNIQUE]["functions"], 2)
        self.assertEqual(summary[TIER_JANUARY_ONLY]["functions"], 2)
        self.assertEqual(
            summary[TIER_EXACT_UNIQUE_CROSSOBJECT]["january_code_bytes"], 372)


if __name__ == "__main__":
    unittest.main()
