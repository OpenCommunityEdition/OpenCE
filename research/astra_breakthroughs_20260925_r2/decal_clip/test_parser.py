"""Regression tests for the pinned debugger's single-hex-digit hit formatter."""
import unittest
from unittest.mock import patch
import common as C

class ParserTests(unittest.TestCase):
    def test_lt_full_ordinals_with_interleaving_and_wrap(self):
        lines=[]
        for i in range(1,34):
            lines.append(f'LT bp1 #{i%16:x} eip=0x1072e92a eax=0x00000001 stk: 0x00000002 chain0: 0x00000003')
            if i<=4: lines.append(f'LT bp2 #{i:x} eip=0x10758346 eax=0x00000004')
        with patch.object(C.Path,'read_text',return_value='\n'.join(lines)):
            events=C.lt_events(C.OUT)
        bp1=[e for e in events if e['bp']==1]
        self.assertEqual([e['hit'] for e in bp1],list(range(1,34)))
        self.assertEqual(bp1[21]['hit'],22)
        self.assertEqual(bp1[21]['printed_hit_low_nibble'],6)
        self.assertEqual(bp1[0]['stack'],[2])
        self.assertEqual(bp1[0]['chains'],{0:[3]})

    def test_missing_lt_fails_closed(self):
        with patch.object(C.Path,'read_text',return_value='LT bp1 #2 eax=0x00000001'):
            with self.assertRaises(AssertionError): C.lt_events(C.OUT)

    def test_watch_full_ordinals(self):
        records=[]
        for i in range(1,34):
            records.append(f'WP0 #{i%16:x} @0x00000004 now=0x00000021 writer_eip=0x10743ecf\n        eax=0x00000021\n        stk: 0x00000001\n        rec@0x00000000: 0x00000000 0x00000021')
        with patch.object(C.Path,'read_text',return_value='\n'.join(records)):
            events=C.watch_events(C.OUT)
        self.assertEqual([e['hit'] for e in events],list(range(1,34)))
        self.assertEqual(events[16]['record'],[0,33])

    def test_missing_watch_fails_closed(self):
        with patch.object(C.Path,'read_text',return_value='WP0 #2 @0x00000004 now=0x00000021 writer_eip=0x10743ecf\n'):
            with self.assertRaises(AssertionError): C.watch_events(C.OUT)

if __name__=='__main__': unittest.main()
