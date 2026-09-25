"""Offline regression assertions over the observed, timestamp-checked packet."""
from pathlib import Path
import json
import unittest

HERE=Path(__file__).resolve().parent
DATA=json.loads((HERE/'measurements.json').read_text())

def row(label,name,kind='scout'):
    return next(x for x in DATA[kind][label]['ordering'] if x['name']==name)

class MechanismTests(unittest.TestCase):
    def test_storage_class_not_changed(self):
        a,b=[DATA['scout'][x]['created_computed_left'] for x in ['scalar_left','aggregate_left']]
        self.assertEqual(a,b)
        self.assertEqual(a['size'],12)
        self.assertEqual(row('scalar_left','_vehicle_left:computed')['flags'],row('aggregate_left','_vehicle_left:computed')['flags'])

    def test_count_crosses_unchanged_seven(self):
        a=row('scalar_left','_vehicle_left:computed')
        b=row('aggregate_left','_vehicle_left:computed')
        self.assertEqual((a['count'],a['rank'],b['count'],b['rank']),(6,34,8,33))
        self.assertEqual(row('scalar_left','_vehicle_horizontal_forward')['count'],7)
        self.assertEqual(row('aggregate_left','_vehicle_horizontal_forward')['count'],7)
        for label in ['scalar_left','aggregate_left']:
            data=DATA['scout'][label]
            for name,total in data['addition_counts'].items():
                self.assertEqual(total,row(label,name)['count'])
            keys=[(x['size'],-x['count']) for x in data['ordering']]
            self.assertEqual(keys,sorted(keys))

    def test_complete_four_target_interference_sets_equal(self):
        self.assertEqual(DATA['scout']['scalar_left']['interference'],DATA['scout']['aggregate_left']['interference'])
        graph=DATA['scout']['scalar_left']['interference']
        self.assertIn('_vehicle_left:computed',graph['_applied_alpha'])
        self.assertNotIn('_vehicle_horizontal_forward',graph['_applied_alpha'])
        self.assertNotIn('_vehicle_horizontal_forward',graph['_vehicle_left:computed'])

    def test_packing_choice_explains_four_bytes(self):
        scalar=DATA['scout']['scalar_left']['packing']
        aggregate=DATA['scout']['aggregate_left']['packing']
        choose=lambda rows,n:next(x for x in rows if x['name']==n)
        self.assertEqual(choose(scalar,'_vehicle_horizontal_forward')['group'],9)
        self.assertEqual(choose(aggregate,'_vehicle_horizontal_forward')['group'],13)
        self.assertEqual(choose(scalar,'_vehicle_left:computed')['group'],13)
        self.assertEqual(choose(aggregate,'_vehicle_left:computed')['group'],13)

    def test_independent_prediction_and_initialized_control(self):
        self.assertEqual(row('control_scalar','_left','independent')['count'],4)
        self.assertEqual(row('control_aggregate','_left','independent')['count'],6)
        self.assertEqual(row('control_scalar','_forward','independent')['count'],5)
        self.assertEqual(row('control_aggregate','_forward','independent')['count'],5)
        self.assertEqual((row('control_scalar','_left','independent')['rank'],row('control_scalar','_forward','independent')['rank']),(5,4))
        self.assertEqual((row('control_aggregate','_left','independent')['rank'],row('control_aggregate','_forward','independent')['rank']),(4,5))
        for label,size in [('control_scalar',32),('control_aggregate',28)]:
            self.assertEqual(-min(x['offset'] for x in DATA['independent'][label]['final']),size)

    def test_negative_interface_control_unchanged(self):
        a,b=[DATA['independent'][x] for x in ['control_aggregate','control_const_interface']]
        self.assertEqual(a['ordering'],b['ordering'])
        self.assertEqual(a['final'],b['final'])
        self.assertEqual(a['section'],b['section'])

    def test_full_object_observation_checks(self):
        self.assertGreaterEqual(len(DATA['stock_checks']),18)
        for check in DATA['stock_checks'].values():
            self.assertTrue(check['full_object_timestamp_only'])
            self.assertTrue(set(check['differing_offsets']) <= set(range(4,8)))

    def test_generic_cli_direct_toy_packing(self):
        data=json.loads((HERE/'generic_cli_smoke.json').read_text())
        for label in ['scalar','aggregate']:
            self.assertTrue(data[label]['stock_check']['full_object_timestamp_only'])
            self.assertTrue(data[label]['compiler_disk_hashes_unchanged'])
        choose=lambda label,n:next(x for x in data[label]['packing'] if x['name']==n)
        self.assertEqual(choose('scalar','_forward')['group'],choose('scalar','_alpha')['group'])
        self.assertNotEqual(choose('scalar','_left')['group'],choose('scalar','_alpha')['group'])
        self.assertEqual(choose('aggregate','_forward')['group'],choose('aggregate','_left')['group'])
        self.assertNotEqual(choose('aggregate','_left')['group'],choose('aggregate','_alpha')['group'])

if __name__=='__main__': unittest.main()
