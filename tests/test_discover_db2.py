import unittest
from tools.discover_db2 import discover


class DiscoverDb2Tests(unittest.TestCase):
    def test_rejects_dump_that_does_not_match_the_qualified_db2_special_hash(self):
        for data in [b'', b'not the real DB2 Special dump', bytes(512)]:
            with self.subTest(data=data), self.assertRaises(ValueError):
                discover(data)
