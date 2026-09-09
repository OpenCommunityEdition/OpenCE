import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

from tools import coff_compare as cc
from tools.campaign import unpark


CODE_FLAGS = 0xE0000060


def build_functions(names):
    sections = []
    symbols = []
    for index, name in enumerate(names, start=1):
        sections.append({
            "name": ".text",
            "size": 2,
            "raw_data": b"\x90\xc3",
            "flags": CODE_FLAGS,
        })
        symbols.append({
            "name": name,
            "section": index,
            "value": 0,
            "type": 0x20,
            "storage": 2,
        })
    return cc.build_coff(sections=sections, symbols=symbols)


class FormattingTests(unittest.TestCase):
    def test_removes_first_adjacent_and_last_entries_without_reserializing(self):
        raw = (
            b'{\r\n  "version": 1,\r\n  "entries": [\r\n'
            b'    {"unit":"u0","function":"_f0","class":"x"},\r\n'
            b'    {"unit":"u1","function":"_f1","class":"x"},\r\n'
            b'    {"unit":"u2","function":"_f2","class":"x"},\r\n'
            b'    {"unit":"u3","function":"_f3","class":"x"},\r\n'
            b'    {"unit":"u4","function":"_f4","class":"x"}\r\n'
            b'  ],\r\n  "tail": {"compact":true}\r\n}\r\n'
        )
        expected = (
            b'{\r\n  "version": 1,\r\n  "entries": [\r\n'
            b'    {"unit":"u1","function":"_f1","class":"x"}\r\n'
            b'  ],\r\n  "tail": {"compact":true}\r\n}\r\n'
        )
        keys = {
            ("u0", "_f0"),
            ("u2", "_f2"),
            ("u3", "_f3"),
            ("u4", "_f4"),
        }

        result = unpark.remove_entries_preserving_format(raw, json.loads(raw), keys)

        self.assertEqual(expected, result)

    def test_unknown_removal_key_fails_closed(self):
        raw = b'{\n  "entries":[{"unit":"u","function":"_f","class":"x"}]\n}'
        with self.assertRaisesRegex(unpark.UnparkError, "requested entry is absent"):
            unpark.remove_entries_preserving_format(
                raw,
                json.loads(raw),
                {("u", "_missing")},
            )


class CliTests(unittest.TestCase):
    def test_help_exits_without_opening_inputs(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            with self.assertRaisesRegex(SystemExit, "0"):
                unpark.main(["--help"])
        self.assertIn("--write", output.getvalue())

    def test_dry_run_is_default_and_write_is_explicit(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            target = root / "target.obj"
            candidate = root / "candidate.obj"
            report = root / "objdiff.json"
            parked = root / "config" / "parked.json"
            parked.parent.mkdir()
            object_bytes = build_functions(["_exact", "_assembly"])
            target.write_bytes(object_bytes)
            candidate.write_bytes(object_bytes)
            report.write_text(json.dumps({"units": [
                {
                    "name": "source/example",
                    "target_path": "target.obj",
                    "base_path": "candidate.obj",
                },
                {"name": "external/unbuilt"},
            ]}), encoding="utf-8")
            parked.write_bytes(
                b'{\n  "version": 1,\n  "entries": [\n'
                b'    {"unit":"source/example","function":"_exact","class":"x"},\n'
                b'    {"unit":"source/example","function":"_assembly","class":"asm-implemented"}\n'
                b'  ]\n}\n'
            )
            original = parked.read_bytes()
            arguments = [
                "--root", str(root),
                "--report", str(report),
                "--parked", str(parked),
            ]

            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                result = unpark.main(arguments)
            self.assertEqual(0, result)
            self.assertEqual(original, parked.read_bytes())
            self.assertIn("dry run", output.getvalue())

            with contextlib.redirect_stdout(io.StringIO()):
                result = unpark.main(arguments + ["--write"])
            self.assertEqual(0, result)
            self.assertEqual(
                ["_assembly"],
                [entry["function"] for entry in json.loads(parked.read_bytes())["entries"]],
            )


if __name__ == "__main__":
    unittest.main()
