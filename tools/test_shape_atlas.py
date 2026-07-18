import unittest

from tools.shape_atlas import build_atlas


def entry(unit, function, status, size=32, relocation_types=(20,), sha256="shape"):
	return {
		"unit": unit,
		"function": function,
		"status": status,
		"size": size,
		"shape_key": [size, len(relocation_types), sha256, tuple(relocation_types)],
	}


class ShapeAtlasTests(unittest.TestCase):
	def test_exact_donor_creates_transfer_group(self):
		transfers, bulk = build_atlas([
			entry("source/a", "_donor", "exact"),
			entry("source/b", "_recipient", "absent"),
		])
		self.assertEqual(len(transfers), 1)
		self.assertEqual(transfers[0]["donors"][0]["function"], "_donor")
		self.assertEqual(transfers[0]["recipients"][0]["function"], "_recipient")
		self.assertEqual(bulk, [])

	def test_three_unsolved_members_create_bulk_group(self):
		transfers, bulk = build_atlas([
			entry("source/a", "_a", "absent"),
			entry("source/b", "_b", "implemented"),
			entry("source/c", "_c", "absent"),
		])
		self.assertEqual(transfers, [])
		self.assertEqual(len(bulk), 1)
		self.assertEqual(bulk[0]["total_bytes"], 96)

	def test_padding_difference_does_not_transfer(self):
		transfers, bulk = build_atlas([
			entry("source/a", "_donor", "exact", size=544),
			entry("source/b", "_near", "absent", size=543),
		])
		self.assertEqual(transfers, [])
		self.assertEqual(bulk, [])

	def test_relocation_type_difference_does_not_transfer(self):
		transfers, bulk = build_atlas([
			entry("source/a", "_donor", "exact", relocation_types=(20,)),
			entry("source/b", "_near", "absent", relocation_types=(6,)),
		])
		self.assertEqual(transfers, [])
		self.assertEqual(bulk, [])

	def test_two_unsolved_members_are_not_bulk(self):
		transfers, bulk = build_atlas([
			entry("source/a", "_a", "absent"),
			entry("source/b", "_b", "implemented"),
		])
		self.assertEqual(transfers, [])
		self.assertEqual(bulk, [])


if __name__ == "__main__":
	unittest.main()
