import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path

from tools import coff_compare as cc
from tools.campaign import unparked


CODE_FLAGS = 0xE0000060


def build_functions(entries):
    sections = []
    symbols = []
    for index, (name, body) in enumerate(entries, start=1):
        sections.append({
            "name": ".text",
            "size": len(body),
            "raw_data": body,
            "flags": CODE_FLAGS,
        })
        symbols.append({
            "name": name,
            "section": index,
            "value": 0,
            "type": 0x20,
            "storage": 2,
        })
    return cc.load(cc.build_coff(sections=sections, symbols=symbols))


class OpenPoolClassificationTests(unittest.TestCase):
    def test_excludes_exact_and_parked_functions(self):
        target = build_functions([
            ("_exact", b"\x90\xc3"),
            ("_residual", b"\x90\xc3"),
            ("_unwritten", b"\x90\xc3"),
            ("_parked", b"\x90\xc3"),
        ])
        candidate = build_functions([
            ("_exact", b"\x90\xc3"),
            ("_residual", b"\x91\xc3"),
            ("_parked", b"\x91\xc3"),
        ])

        rows = unparked.classify_unit(
            "source/example",
            target,
            candidate,
            {("source/example", "_parked")},
        )

        self.assertEqual(
            {("_residual", "residual"), ("_unwritten", "UNWRITTEN")},
            {(row.function, row.status) for row in rows},
        )
        residual = next(row for row in rows if row.function == "_residual")
        self.assertEqual("sha", residual.difference)

    def test_missing_candidate_object_marks_every_function_unwritten(self):
        target = build_functions([
            ("_first", b"\xc3"),
            ("_second", b"\x90\xc3"),
        ])
        rows = unparked.classify_unit("source/example", target, None, set())
        self.assertEqual(2, len(rows))
        self.assertTrue(all(row.status == "UNWRITTEN" for row in rows))


class OpenPoolCliTests(unittest.TestCase):
    def test_json_cli_is_generic_and_read_only(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            target_path = root / "build" / "split" / "source" / "sample.obj"
            candidate_path = root / "build" / "base" / "source" / "sample.obj"
            parked_path = root / "config" / "parked.json"
            target_path.parent.mkdir(parents=True)
            candidate_path.parent.mkdir(parents=True)
            parked_path.parent.mkdir(parents=True)

            target_path.write_bytes(cc.build_coff(
                sections=[{
                    "name": ".text",
                    "size": 2,
                    "raw_data": b"\x90\xc3",
                    "flags": CODE_FLAGS,
                }],
                symbols=[{
                    "name": "_sample",
                    "section": 1,
                    "value": 0,
                    "type": 0x20,
                    "storage": 2,
                }],
            ))
            candidate_path.write_bytes(cc.build_coff(
                sections=[{
                    "name": ".text",
                    "size": 2,
                    "raw_data": b"\x91\xc3",
                    "flags": CODE_FLAGS,
                }],
                symbols=[{
                    "name": "_sample",
                    "section": 1,
                    "value": 0,
                    "type": 0x20,
                    "storage": 2,
                }],
            ))
            parked_path.write_text(
                json.dumps({"entries": []}),
                encoding="utf-8",
            )
            before = parked_path.read_bytes()

            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                result = unparked.main([
                    "source/sample",
                    "--json",
                    "--root",
                    str(root),
                ])

            self.assertEqual(0, result)
            document = json.loads(output.getvalue())
            self.assertEqual(1, document["summary"]["functions"])
            self.assertEqual("source/sample", document["rows"][0]["unit"])
            self.assertEqual(before, parked_path.read_bytes())


if __name__ == "__main__":
    unittest.main()
