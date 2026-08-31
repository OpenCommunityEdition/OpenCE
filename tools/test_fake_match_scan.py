import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from tools.fake_match_scan import scan_paths, scan_text


class FakeMatchScanTests(unittest.TestCase):
	def test_comments_and_literals_do_not_create_findings(self):
		findings = scan_text(
			'char const *text = "if (ready) { }";\n'
			"/* value = value; */\n"
			"// goto done; done:\n"
			"void real_code(void)\n"
			"{\n"
			"\tvalue = value;\n"
			"}\n",
			"sample.c",
		)
		self.assertEqual([(item.rule, item.line) for item in findings], [
			("self-assignment", 6),
		])

	def test_member_assignment_from_same_named_parameter_is_not_self_assignment(self):
		findings = scan_text(
			"state_data->goal = goal;\n"
			"result->i = i;\n"
			"actual = actual;\n"
			"object->field = object->field;\n"
			"array[index] = array[index];\n"
			"*pointer = *pointer;\n",
			"assignments.c",
		)
		self.assertEqual([(item.rule, item.line) for item in findings], [
			("self-assignment", 3),
			("self-assignment", 4),
			("self-assignment", 5),
			("self-assignment", 6),
		])

	def test_empty_and_identical_branches_are_classified_separately(self):
		findings = scan_text(
			"if (a) { }\n"
			"if (b) { } else { work(); }\n"
			"if (c) { work(); } else { }\n"
			"if (d) { work(); } else { work(); }\n",
			"branches.c",
		)
		self.assertEqual([item.rule for item in findings], [
			"empty-if",
			"empty-then-else",
			"empty-else",
			"identical-branches",
		])

		literal_difference = scan_text(
			'if (condition) { copy("first"); } else { copy("second"); }\n',
			"literal-branches.c",
		)
		self.assertEqual(literal_difference, [])

	def test_simple_fixed_conditions_are_review_leads(self):
		findings = scan_text(
			"if (ready && !ready) { work(); }\n"
			"if (state.value > state.value) { work(); }\n"
			"if (number != number) { handle_nan(); }\n",
			"conditions.c",
		)
		self.assertEqual([item.rule for item in findings], [
			"fixed-boolean-condition",
			"self-order-comparison",
		])

	def test_goto_and_codegen_controls_are_found_without_comment_noise(self):
		findings = scan_text(
			"// __asm nop\n"
			"goto finish;\n"
			"/* explanation */\n"
			"finish:\n"
			"__asm nop\n"
			"value = *(volatile long *)&source;\n"
			"#pragma optimize(\"\", off)\n"
			"__declspec(noinline) void helper(void);\n",
			"controls.c",
		)
		self.assertEqual([item.rule for item in findings], [
			"goto-next-label",
			"inline-assembly",
			"volatile-reinterpret-cast",
			"codegen-directive",
			"codegen-directive",
		])

	def test_directory_scan_is_sorted_and_skips_known_vendor_trees(self):
		with tempfile.TemporaryDirectory() as temporary_directory:
			root = Path(temporary_directory)
			(root / "z.c").write_text("goto out; out:\n", encoding="utf-8")
			(root / "a.c").write_text("value = value;\n", encoding="utf-8")
			vendor = root / "source" / "memory" / "zlib"
			vendor.mkdir(parents=True)
			(vendor / "vendor.c").write_text("__asm nop\n", encoding="utf-8")

			summary, errors = scan_paths([root])
			self.assertEqual(errors, [])
			self.assertEqual(summary.files_scanned, 2)
			self.assertEqual([item.rule for item in summary.findings], [
				"self-assignment",
				"goto-next-label",
			])
			self.assertEqual(len(summary.skipped_vendored), 1)
			vendor_only, errors = scan_paths([vendor])
			self.assertEqual(errors, [])
			self.assertEqual(vendor_only.files_scanned, 0)
			self.assertEqual(len(vendor_only.skipped_vendored), 1)

			included, errors = scan_paths([root], include_vendored=True)
			self.assertEqual(errors, [])
			self.assertEqual(included.files_scanned, 3)
			self.assertEqual([item.rule for item in included.findings], [
				"self-assignment",
				"inline-assembly",
				"goto-next-label",
			])

	def test_rule_filter_is_applied_after_scanning(self):
		with tempfile.TemporaryDirectory() as temporary_directory:
			path = Path(temporary_directory) / "sample.c"
			path.write_text("value = value;\n__asm nop\n", encoding="utf-8")
			summary, errors = scan_paths(
				[path],
				rule_ids=frozenset(("inline-assembly",)),
			)
			self.assertEqual(errors, [])
			self.assertEqual([item.rule for item in summary.findings], ["inline-assembly"])

	def test_cli_json_and_exit_codes_are_stable(self):
		script = Path(__file__).with_name("fake_match_scan.py")
		with tempfile.TemporaryDirectory() as temporary_directory:
			path = Path(temporary_directory) / "sample.c"
			path.write_text("value = value;\n", encoding="utf-8")
			completed = subprocess.run(
				[
					sys.executable,
					str(script),
					"--format",
					"json",
					"--fail-on-findings",
					str(path),
				],
				capture_output=True,
				text=True,
				check=False,
			)
			self.assertEqual(completed.returncode, 1, completed.stderr)
			payload = json.loads(completed.stdout)
			self.assertIn("does not prove", payload["disclaimer"])
			self.assertEqual(payload["files_scanned"], 1)
			self.assertEqual(payload["findings"][0]["rule"], "self-assignment")
			ordinary_exit = subprocess.run(
				[sys.executable, str(script), "--format", "json", str(path)],
				capture_output=True,
				text=True,
				check=False,
			)
			self.assertEqual(ordinary_exit.returncode, 0, ordinary_exit.stderr)
			self.assertEqual(ordinary_exit.stdout, completed.stdout)

			missing = subprocess.run(
				[sys.executable, str(script), str(path.with_name("missing.c"))],
				capture_output=True,
				text=True,
				check=False,
			)
			self.assertEqual(missing.returncode, 2)
			self.assertIn("path does not exist", missing.stderr)

	def test_direct_script_help_and_rule_listing(self):
		script = Path(__file__).with_name("fake_match_scan.py")
		help_result = subprocess.run(
			[sys.executable, str(script), "--help"],
			capture_output=True,
			text=True,
			check=False,
		)
		self.assertEqual(help_result.returncode, 0, help_result.stderr)
		self.assertIn("--fail-on-findings", help_result.stdout)

		list_result = subprocess.run(
			[sys.executable, str(script), "--list-rules"],
			capture_output=True,
			text=True,
			check=False,
		)
		self.assertEqual(list_result.returncode, 0, list_result.stderr)
		self.assertIn("self-assignment", list_result.stdout)
		self.assertIn("inline-assembly", list_result.stdout)


if __name__ == "__main__":
	unittest.main()
