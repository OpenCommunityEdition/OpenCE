import contextlib
import io
import json
import struct
import tempfile
import unittest
from pathlib import Path

from tools import coff_compare as cc
from tools.campaign import alndiff, namegap, tinfo
from tools.campaign._coff_tools import default_object_path, normalize_unit


CODE_FLAGS = 0xE0000060


def build_functions(entries):
    """Build a COFF object from ``(name, bytes)`` function entries."""

    sections = []
    symbols = []
    for index, (name, code) in enumerate(entries, start=1):
        sections.append({
            "name": ".text",
            "size": len(code),
            "raw_data": code,
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


class CampaignCoffPathTests(unittest.TestCase):
    def test_normalizes_spaced_windows_style_unit(self):
        self.assertEqual(
            "source/saved games/saved_game_files",
            normalize_unit(r"source\saved games\saved_game_files.c"),
        )

    def test_rejects_unit_path_escape(self):
        with self.assertRaises(ValueError):
            normalize_unit("source/../config/symbols.json")

    def test_default_object_path_appends_suffix_without_replacing_dot(self):
        path = default_object_path(Path("C:/repo"), "split", "source/a/file.v1")
        self.assertEqual("file.v1.obj", path.name)


class AlignedDiffTests(unittest.TestCase):
    def test_normalizes_only_immediate_control_flow_destination(self):
        self.assertEqual(
            "jne <branch-target>",
            alndiff.normalize_instruction("jne", "0x1234"),
        )
        self.assertEqual(
            "call dword ptr [eax]",
            alndiff.normalize_instruction("call", "dword ptr [eax]"),
        )
        self.assertEqual(
            "mov eax, 0x1234",
            alndiff.normalize_instruction("mov", "eax, 0x1234"),
        )

    def test_sequence_alignment_is_not_an_index_zip(self):
        target = [
            alndiff.Instruction(0, "push ebp", "push ebp"),
            alndiff.Instruction(1, "mov ebp, esp", "mov ebp, esp"),
            alndiff.Instruction(3, "ret", "ret"),
        ]
        ours = [
            alndiff.Instruction(0, "push ebp", "push ebp"),
            alndiff.Instruction(1, "nop", "nop"),
            alndiff.Instruction(2, "mov ebp, esp", "mov ebp, esp"),
            alndiff.Instruction(4, "ret", "ret"),
        ]
        changed = [
            opcode for opcode in alndiff.aligned_opcodes(target, ours)
            if opcode[0] != "equal"
        ]
        self.assertEqual([("insert", 1, 1, 1, 2)], changed)

    def test_disassembly_masks_and_annotates_relocation(self):
        relocation = struct.pack("<LLH", 1, 1, cc.IMAGE_REL_I386_REL32)
        obj = cc.load(cc.build_coff(
            sections=[{
                "name": ".text",
                "size": 6,
                "raw_data": b"\xe8\0\0\0\0\xc3",
                "reloc_data": relocation,
                "reloc_count": 1,
                "flags": CODE_FLAGS,
            }],
            symbols=[
                {
                    "name": "_caller",
                    "section": 1,
                    "value": 0,
                    "type": 0x20,
                    "storage": 2,
                },
                {
                    "name": "_callee",
                    "section": 0,
                    "value": 0,
                    "type": 0x20,
                    "storage": 2,
                },
            ],
        ))
        instructions = alndiff.disassemble_function(obj, "_caller")
        self.assertEqual(2, len(instructions))
        self.assertIn("<branch-target>", instructions[0].normalized)
        self.assertIn("_callee", instructions[0].normalized)


class TargetInfoTests(unittest.TestCase):
    def test_bss_preview_is_never_materialized(self):
        data = bytearray(cc.build_coff(
            sections=[{
                "name": ".bss",
                "size": 16,
                "raw_data": b"\0" * 16,
                "flags": cc.IMAGE_SCN_CNT_UNINITIALIZED_DATA,
            }],
            symbols=[{
                "name": "_state",
                "section": 1,
                "value": 0,
                "type": 0,
                "storage": 2,
            }],
        ))
        struct.pack_into("<L", data, cc.COFF_HEADER_SIZE + 20, 0)
        records = tinfo.data_records(cc.load(bytes(data)), preview_limit=512)
        self.assertEqual(1, len(records))
        self.assertTrue(records[0]["uninitialized"])
        self.assertIsNone(records[0]["preview"])

    def test_function_json_cli_reads_explicit_object(self):
        obj_data = cc.build_coff(
            sections=[{
                "name": ".text",
                "size": 1,
                "raw_data": b"\xc3",
                "flags": CODE_FLAGS,
            }],
            symbols=[{
                "name": "_leaf",
                "section": 1,
                "value": 0,
                "type": 0x20,
                "storage": 2,
            }],
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory, "leaf.obj")
            path.write_bytes(obj_data)
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                result = tinfo.main([
                    "--object", str(path),
                    "--function", "_leaf",
                    "--json",
                ])
        self.assertEqual(0, result)
        report = json.loads(output.getvalue())
        self.assertEqual("function", report["mode"])
        self.assertEqual("_leaf", report["records"][0]["name"])


class NameGapTests(unittest.TestCase):
    def test_proposes_only_bijective_strict_section_match(self):
        target = build_functions([("_code_00100000", b"\x90\xc3")])
        ours = build_functions([("_semantic_leaf", b"\x90\xc3")])
        proposals, ambiguities = namegap.find_name_gaps(
            target,
            ours,
            "source/test/leaf",
        )
        self.assertEqual([], ambiguities)
        self.assertEqual(1, len(proposals))
        self.assertEqual("_semantic_leaf", proposals[0]["candidate_name"])
        self.assertFalse(proposals[0]["authoritative"])
        self.assertTrue(proposals[0]["requires_manual_provenance_review"])

    def test_rejects_non_bijective_same_shape_pairing(self):
        target = build_functions([
            ("_code_00100000", b"\x90\xc3"),
            ("_code_00100010", b"\x90\xc3"),
        ])
        ours = build_functions([("_semantic_leaf", b"\x90\xc3")])
        proposals, ambiguities = namegap.find_name_gaps(
            target,
            ours,
            "source/test/leaf",
        )
        self.assertEqual([], proposals)
        self.assertEqual(2, len(ambiguities))

    def test_json_cli_is_explicitly_non_authoritative_and_read_only(self):
        target_data = cc.build_coff(
            sections=[{
                "name": ".text",
                "size": 1,
                "raw_data": b"\xc3",
                "flags": CODE_FLAGS,
            }],
            symbols=[{
                "name": "_code_00100000",
                "section": 1,
                "value": 0,
                "type": 0x20,
                "storage": 2,
            }],
        )
        ours_data = cc.build_coff(
            sections=[{
                "name": ".text",
                "size": 1,
                "raw_data": b"\xc3",
                "flags": CODE_FLAGS,
            }],
            symbols=[{
                "name": "_semantic_leaf",
                "section": 1,
                "value": 0,
                "type": 0x20,
                "storage": 2,
            }],
        )
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            target_path = root / "target.obj"
            ours_path = root / "ours.obj"
            sentinel = root / "symbols.json"
            target_path.write_bytes(target_data)
            ours_path.write_bytes(ours_data)
            sentinel.write_text("do not edit", encoding="utf-8")
            before = sentinel.read_bytes()
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                result = namegap.main([
                    "source/test/leaf",
                    "--root", str(root),
                    "--target-object", str(target_path),
                    "--ours-object", str(ours_path),
                    "--json",
                ])
            after = sentinel.read_bytes()
        self.assertEqual(0, result)
        self.assertEqual(before, after)
        report = json.loads(output.getvalue())
        self.assertFalse(report["authoritative"])
        self.assertEqual(1, report["proposal_count"])
        self.assertIn("manual", report["notice"].lower())


if __name__ == "__main__":
    unittest.main()
