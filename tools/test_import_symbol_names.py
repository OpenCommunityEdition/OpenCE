import argparse
from contextlib import redirect_stdout
import io
import unittest

from tools.campaign.import_symbol_names import (
	Candidate,
	build_import_plan,
	detect_source_collisions,
	is_placeholder_symbol,
	parse_file_offset,
	print_import_report,
	replace_c_identifiers,
	replace_exact_json_strings,
	select_candidates,
	symbol_to_c_identifier,
	update_symbols_text,
)


class ImportSymbolNamesTests(unittest.TestCase):
	def test_mapping_uses_file_offset_and_preserves_canonical_fields(self):
		canonical = [
			{
				"file_offset": 0x10,
				"flags": 32,
				"name": "_code_00000010",
				"canonical_note": "keep",
			}
		]
		donor = [
			{
				"file_offset": 0x10,
				"flags": 999,
				"name": "_real_function",
				"static": True,
			}
		]

		plan = build_import_plan(canonical, donor)

		self.assertEqual(
			plan.candidates,
			(Candidate(0x10, "_code_00000010", "_real_function", True),),
		)
		text = (
			'[\r\n'
			'{ "file_offset": 16, "flags": 32, "name": "_code_00000010", '
			'"canonical_note": "keep" },\r\n'
			']\r\n'
		)
		self.assertEqual(
			update_symbols_text(text, plan.candidates),
			'[\r\n'
			'{ "file_offset": 16, "flags": 32, "name": "_real_function", '
			'"canonical_note": "keep", "static": true },\r\n'
			']\r\n',
		)

	def test_cdecl_stdcall_and_fastcall_decoration(self):
		self.assertTrue(is_placeholder_symbol("_code_00158450@8"))
		self.assertTrue(is_placeholder_symbol("_bss_0031ce38"))
		self.assertTrue(is_placeholder_symbol("_rdata_00256c64"))
		self.assertFalse(is_placeholder_symbol("@code_00158450@8"))
		self.assertFalse(is_placeholder_symbol("_actual_name@8"))
		self.assertEqual(symbol_to_c_identifier("_cdecl_name"), "cdecl_name")
		self.assertEqual(symbol_to_c_identifier("_stdcall_name@8"), "stdcall_name")
		self.assertEqual(symbol_to_c_identifier("@fastcall_name@12"), "fastcall_name")
		self.assertEqual(symbol_to_c_identifier("__private_name"), "_private_name")
		self.assertIsNone(symbol_to_c_identifier("?cpp_name@@YAXXZ"))

	def test_both_real_names_are_preserved_as_conflicts(self):
		canonical = [
			{"file_offset": 32, "flags": 32, "name": "_canonical_name"}
		]
		donor = [
			{"file_offset": 32, "flags": 32, "name": "_different_real_name"}
		]

		plan = build_import_plan(canonical, donor)

		self.assertEqual(plan.candidates, ())
		self.assertEqual(len(plan.real_name_conflicts), 1)
		self.assertEqual(plan.real_name_conflicts[0].canonical_name, "_canonical_name")

	def test_duplicate_offset_aliases_do_not_make_pairing_ambiguous(self):
		canonical = [
			{"file_offset": 40, "flags": 32, "name": "_code_00000028"},
			{"file_offset": 72, "flags": 32, "name": "_shared_alias"},
			{"file_offset": 72, "flags": 32, "name": "__shared_alias"},
		]
		donor = [
			{"file_offset": 40, "flags": 32, "name": "_recovered_name"},
			{"file_offset": 72, "flags": 32, "name": "__shared_alias"},
			{"file_offset": 72, "flags": 32, "name": "_shared_alias"},
		]

		plan = build_import_plan(canonical, donor)

		self.assertEqual(
			plan.candidates,
			(Candidate(40, "_code_00000028", "_recovered_name"),),
		)
		self.assertEqual(plan.real_name_conflicts, ())

	def test_multiline_entry_retains_line_endings_and_indentation(self):
		text = (
			"[\r\n"
			"  {\r\n"
			"    \"file_offset\": 48,\r\n"
			"    \"flags\": 32,\r\n"
			"    \"name\": \"_data_00000030\"\r\n"
			"  }\r\n"
			"]\r\n"
		)
		candidate = Candidate(48, "_data_00000030", "_named_table", True)

		updated = update_symbols_text(text, [candidate])

		self.assertEqual(
			updated,
			"[\r\n"
			"  {\r\n"
			"    \"file_offset\": 48,\r\n"
			"    \"flags\": 32,\r\n"
			"    \"name\": \"_named_table\",\r\n"
			"    \"static\": true\r\n"
			"  }\r\n"
			"]\r\n",
		)

	def test_exact_json_strings_do_not_touch_evidence_substrings(self):
		text = (
			'{"function":"_code_00000040",'
			'"evidence":"mentions _code_00000040 but is not a name"}\n'
		)

		updated, count = replace_exact_json_strings(
			text, {"_code_00000040": "_named_function"}
		)

		self.assertEqual(count, 1)
		self.assertEqual(
			updated,
			'{"function":"_named_function",'
			'"evidence":"mentions _code_00000040 but is not a name"}\n',
		)

	def test_c_rewrite_changes_tokens_but_not_comments_strings_or_substrings(self):
		text = (
			"void code_00000050(void);\n"
			"void call(void) { code_00000050(); code_00000050_extra(); }\n"
			"// code_00000050 remains documentary\n"
			'char const *s = "code_00000050";\n'
		)

		updated, counts = replace_c_identifiers(
			text, {"code_00000050": "named_function"}
		)

		self.assertEqual(counts["code_00000050"], 2)
		self.assertIn("void named_function(void);", updated)
		self.assertIn("named_function(); code_00000050_extra();", updated)
		self.assertIn("// code_00000050 remains documentary", updated)
		self.assertIn('"code_00000050"', updated)

	def test_same_file_source_collision_is_reported(self):
		candidates = [
			Candidate(0x60, "_code_00000060", "_named_function")
		]
		sources = {
			"source/example.c": (
				"void code_00000060(void);\nvoid named_function(void);\n"
			)
		}

		collisions = detect_source_collisions(sources, candidates)

		self.assertEqual(len(collisions), 1)
		self.assertEqual(collisions[0].path, "source/example.c")
		self.assertIn("coexist", collisions[0].reason)

	def test_exclusion_offset_parser_accepts_hex_and_decimal(self):
		self.assertEqual(parse_file_offset("4660"), 0x1234)
		self.assertEqual(parse_file_offset("004660"), 0x1234)
		self.assertEqual(parse_file_offset("0x1234"), 0x1234)
		with self.assertRaises(argparse.ArgumentTypeError):
			parse_file_offset("1234h")
		with self.assertRaises(argparse.ArgumentTypeError):
			parse_file_offset("-1")

	def test_excluded_candidates_are_enumerated_in_report(self):
		selected = [Candidate(0x70, "_code_00000070", "_selected")]
		excluded = [Candidate(0x80, "_code_00000080", "_excluded")]
		output = io.StringIO()

		with redirect_stdout(output):
			print_import_report(selected, excluded, [0x90], [], [], verbose=False)

		report = output.getvalue()
		self.assertIn("placeholder-to-real candidates: 1", report)
		self.assertIn("0x00000070: _code_00000070 -> _selected", report)
		self.assertIn("excluded donor candidates: 1", report)
		self.assertIn("0x00000080: _code_00000080 -> _excluded", report)
		self.assertIn("unused exclusion offsets: 1", report)
		self.assertIn("0x00000090", report)

	def test_excluding_collision_leaves_safe_partial_apply_plan(self):
		colliding = Candidate(0xA0, "_code_000000a0", "_existing_name")
		safe = Candidate(0xB0, "_code_000000b0", "_safe_name", True)
		sources = {
			"source/example.c": (
				"void code_000000a0(void);\n"
				"void existing_name(void);\n"
				"void code_000000b0(void);\n"
			)
		}
		self.assertEqual(
			len(detect_source_collisions(sources, [colliding, safe])), 1
		)

		selection = select_candidates([colliding, safe], [0xA0])

		self.assertEqual(selection.candidates, (safe,))
		self.assertEqual(selection.excluded, (colliding,))
		self.assertEqual(selection.unused_offsets, ())
		self.assertEqual(detect_source_collisions(sources, selection.candidates), [])

		symbols_text = (
			"[\n"
			'{ "file_offset": 160, "flags": 32, "name": "_code_000000a0" },\n'
			'{ "file_offset": 176, "flags": 32, "name": "_code_000000b0" }\n'
			"]\n"
		)
		updated = update_symbols_text(symbols_text, selection.candidates)
		self.assertIn('"name": "_code_000000a0"', updated)
		self.assertIn('"name": "_safe_name", "static": true', updated)


if __name__ == "__main__":
	unittest.main()
