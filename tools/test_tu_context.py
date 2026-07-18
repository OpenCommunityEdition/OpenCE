import unittest

from tools.tu_context import classify_candidate


class TranslationUnitContextTests(unittest.TestCase):
	def test_starved_near_helper_is_blocked_on_callers(self):
		self.assertEqual(
			classify_candidate(True, 82.63, False, ["_pool_new"], True, True),
			"blocked-on-callers",
		)

	def test_isolated_equal_shape_is_a_park_candidate(self):
		self.assertEqual(
			classify_candidate(True, 95.31, True, [], True, True),
			"park-candidate",
		)

	def test_low_similarity_function_remains_structural(self):
		self.assertEqual(
			classify_candidate(True, 11.58, False, ["_unit_update"], True, True),
			"structural",
		)

	def test_missing_base_function_is_structural(self):
		self.assertEqual(
			classify_candidate(False, 0.0, False, [], False, False),
			"structural",
		)

	def test_nonisolated_same_shape_waits_for_callers(self):
		self.assertEqual(
			classify_candidate(True, 98.0, True, ["_caller"], True, True),
			"blocked-on-callers",
		)


if __name__ == "__main__":
	unittest.main()
