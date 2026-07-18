import unittest

from tools.symbol_aliases import pair_unique_aliases


def entry(name, shape="same", size=16, relocations=1):
	return {
		"function": name,
		"measurement_key": shape,
		"size": size,
		"relocation_count": relocations,
		"normalized_sha256": shape,
	}


class SymbolAliasTests(unittest.TestCase):
	def test_unique_cross_name_shape_is_reported(self):
		aliases = pair_unique_aliases([entry("_code_1")], [entry("_sdk_name")])
		self.assertEqual(len(aliases), 1)
		self.assertEqual(aliases[0]["target_function"], "_code_1")
		self.assertEqual(aliases[0]["base_function"], "_sdk_name")

	def test_same_name_is_not_an_alias(self):
		self.assertEqual(pair_unique_aliases([entry("_same")], [entry("_same")]), [])

	def test_duplicate_target_shape_is_ambiguous(self):
		self.assertEqual(
			pair_unique_aliases([entry("_a"), entry("_b")], [entry("_sdk")]),
			[],
		)

	def test_duplicate_base_shape_is_ambiguous(self):
		self.assertEqual(
			pair_unique_aliases([entry("_code")], [entry("_a"), entry("_b")]),
			[],
		)

	def test_different_shapes_do_not_pair(self):
		self.assertEqual(
			pair_unique_aliases([entry("_code", "one")], [entry("_sdk", "two")]),
			[],
		)


if __name__ == "__main__":
	unittest.main()
