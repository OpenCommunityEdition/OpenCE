import unittest

from tools.audit_semantic_matches import relocation_shape_matches


def info(size=16, digest="same", relocations=None):
    return {
        "size": size,
        "normalized_sha256": digest,
        "relocations": relocations or [],
    }


class RelocationShapeTests(unittest.TestCase):
    def test_accepts_symbol_ownership_difference(self):
        target = info(relocations=[{
            "address": 4,
            "type": 6,
            "target": ["symbol", "_target_owner", 12],
        }])
        base = info(relocations=[{
            "address": 4,
            "type": 6,
            "target": ["symbol", "_candidate_owner", 0],
        }])
        self.assertTrue(relocation_shape_matches(target, base))

    def test_rejects_relocation_count_difference(self):
        target = info(relocations=[])
        base = info(relocations=[{
            "address": 4,
            "type": 6,
            "target": ["symbol", "__except_list", 0],
        }])
        self.assertFalse(relocation_shape_matches(target, base))

    def test_rejects_relocation_address_or_type_difference(self):
        target = info(relocations=[{
            "address": 4,
            "type": 6,
            "target": ["symbol", "_value", 0],
        }])
        moved = info(relocations=[{
            "address": 8,
            "type": 6,
            "target": ["symbol", "_value", 0],
        }])
        retyped = info(relocations=[{
            "address": 4,
            "type": 20,
            "target": ["symbol", "_value", 0],
        }])
        self.assertFalse(relocation_shape_matches(target, moved))
        self.assertFalse(relocation_shape_matches(target, retyped))

    def test_rejects_size_or_normalized_bytes_difference(self):
        target = info()
        self.assertFalse(relocation_shape_matches(target, info(size=32)))
        self.assertFalse(relocation_shape_matches(target, info(digest="different")))


if __name__ == "__main__":
    unittest.main()
