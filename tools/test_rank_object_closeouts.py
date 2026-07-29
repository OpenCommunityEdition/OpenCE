import unittest

from tools.rank_object_closeouts import build_rankings


class RankObjectCloseoutsTests(unittest.TestCase):
	def _unit(self, name, functions, data_total=0, data_matched=0):
		return {
			"name": name,
			"metadata": {"progress_categories": ["halobetacache"]},
			"measures": {
				"total_functions": len(functions),
				"total_data": str(data_total),
				"matched_data": str(data_matched),
				"matched_data_percent": (
					100.0 * data_matched / data_total
					if data_total else 100.0),
			},
			"functions": [
				{"name": function, "size": str(size)}
				for function, size in functions
			],
		}

	def _config(self, entries):
		return {
			"projects": [{
				"units": [
					{"name": unit + ".c", "status": status}
					for unit, status in entries
				]
			}]
		}

	def test_semantic_ledger_overrides_ordinary_function_count(self):
		report = {"units": [
			self._unit("source/a", [("_exact", 16), ("$L1", 16)])
		]}
		semantic = {"accepted_ledger": [
			{"unit": "source/a", "function": "_exact"},
			{"unit": "source/a", "function": "$L1"},
		]}
		rankings = build_rankings(
			report,
			semantic,
			self._config([("source/a", "NonMatching")]),
			{"entries": []})
		self.assertEqual([], rankings[0]["remaining_functions"])

	def test_matching_units_are_not_candidates(self):
		report = {"units": [self._unit("source/a", [("_f", 16)])]}
		rankings = build_rankings(
			report,
			{"accepted_ledger": []},
			self._config([("source/a", "Matching")]),
			{"entries": []})
		self.assertEqual([], rankings)

	def test_parked_residual_sorts_after_open_closeout(self):
		report = {"units": [
			self._unit("source/parked", [("_f", 16)]),
			self._unit("source/open", [("_g", 32)]),
		]}
		config = self._config([
			("source/parked", "NonMatching"),
			("source/open", "NonMatching"),
		])
		parked = {"entries": [{
			"unit": "source/parked",
			"function": "_f",
		}]}
		rankings = build_rankings(
			report, {"accepted_ledger": []}, config, parked)
		self.assertEqual("source/open", rankings[0]["unit"])
		self.assertEqual("NEAR_CLOSEOUT", rankings[0]["class"])
		self.assertEqual("PROVEN_BLOCKED", rankings[1]["class"])

	def test_data_only_gap_is_first_class_closeout(self):
		report = {"units": [
			self._unit("source/data", [("_f", 16)], 12, 8)
		]}
		semantic = {"accepted_ledger": [
			{"unit": "source/data", "function": "_f"},
		]}
		rankings = build_rankings(
			report,
			semantic,
			self._config([("source/data", "NonMatching")]),
			{"entries": []})
		self.assertEqual("DATA_CLOSEOUT", rankings[0]["class"])
		self.assertEqual(4, rankings[0]["raw_data_gap"])

	def test_existing_object_ledger_requires_review_before_dispatch(self):
		report = {"units": [
			self._unit("source/old", [("_f", 16)]),
			self._unit("source/fresh", [("_g", 32)]),
		]}
		config = self._config([
			("source/old", "NonMatching"),
			("source/fresh", "NonMatching"),
		])
		rankings = build_rankings(
			report,
			{"accepted_ledger": []},
			config,
			{"entries": []},
			ledger_units={"source/old"})
		self.assertEqual("source/fresh", rankings[0]["unit"])
		self.assertEqual("LEDGER_REVIEW", rankings[1]["class"])
		self.assertTrue(rankings[1]["object_ledger_present"])


if __name__ == "__main__":
	unittest.main()
