"""tools/engine_golden.py: which drawn frames are new, the same or changed,
and approving them by name, prefix or all."""

import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import engine_golden  # noqa: E402


class GoldenTests(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        root = Path(self.folder.name)
        self.drawn = root / "drawn"
        self.approved = root / "approved"
        self.drawn.mkdir()
        self.approved.mkdir()
        (self.drawn / "devgulch_station_ai_480x270.png").write_bytes(b"ai")
        (self.drawn / "devgulch_station_hud_480x270.png").write_bytes(b"hud")
        (self.drawn / "devgulch_seam_480x270.png").write_bytes(b"seam")
        (self.approved / "devgulch_station_hud_480x270.png").write_bytes(b"hud")
        (self.approved / "devgulch_seam_480x270.png").write_bytes(b"old seam")

    def tearDown(self):
        self.folder.cleanup()

    def test_each_frame_is_new_the_same_or_changed(self):
        self.assertEqual(
            engine_golden.status(self.drawn, self.approved),
            [
                ("devgulch_seam_480x270.png", "changed"),
                ("devgulch_station_ai_480x270.png", "new"),
                ("devgulch_station_hud_480x270.png", "same"),
            ],
        )

    def test_approving_a_prefix_copies_only_its_frames(self):
        names = engine_golden.approve(self.drawn, self.approved, ["devgulch_station_"])
        self.assertEqual(names, ["devgulch_station_ai_480x270.png", "devgulch_station_hud_480x270.png"])
        self.assertEqual((self.approved / "devgulch_station_ai_480x270.png").read_bytes(), b"ai")
        self.assertEqual((self.approved / "devgulch_seam_480x270.png").read_bytes(), b"old seam")

    def test_approving_all_leaves_nothing_changed(self):
        engine_golden.approve(self.drawn, self.approved, [], every=True)
        self.assertTrue(all(state == "same" for _, state in engine_golden.status(self.drawn, self.approved)))

    def test_approving_nothing_that_matches_copies_nothing(self):
        self.assertEqual(engine_golden.approve(self.drawn, self.approved, ["devgulch_station_zz"]), [])


if __name__ == "__main__":
    unittest.main()
