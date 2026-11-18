import hashlib
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zlib
from tools import db1_ending_punctuation as ending
ROOT=Path(__file__).resolve().parents[1]
class EndingPunctuationTests(unittest.TestCase):
    def test_unqualified_sources_rejected(self):
        for data in [b'',bytes(1024),bytes(1048576)]:
            with self.subTest(size=len(data)):
                with self.assertRaises(ValueError):ending.repair(data)
    def test_output_is_exclusive_local_and_source_preserved(self):
        (ROOT/'local').mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(prefix='ending-punct-test-',dir=ROOT/'local') as folder:
            source=Path(folder)/'source.md';source.write_bytes(b'source')
            output=Path(folder)/'derived.md'
            ending.write_derived(source,output,b'derived')
            self.assertEqual(output.read_bytes(),b'derived')
            with self.assertRaises((ValueError,FileExistsError)):ending.write_derived(source,output,b'other')
            with self.assertRaises(ValueError):ending.write_derived(source,source,b'other')
            alias=Path(folder)/'hardlink.md';os.link(source,alias)
            with self.assertRaises((ValueError,FileExistsError)):ending.write_derived(source,alias,b'other')
            self.assertEqual(source.read_bytes(),b'source')
            self.assertEqual(output.read_bytes(),b'derived')
    def test_output_outside_local_rejected_without_creation(self):
        with tempfile.TemporaryDirectory(prefix='ending-punct-outside-') as folder:
            output=Path(folder)/'derived.md'
            with self.assertRaises(ValueError):ending.write_derived(ROOT/'local/source.md',output,b'data')
            self.assertFalse(output.exists())
@unittest.skipUnless(os.environ.get('DB1_ENDING_TEST_ROM'),'private ROM qualification opt-in')
class PrivateEndingPunctuationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.source=Path(os.environ['DB1_ENDING_TEST_ROM']).read_bytes()
    def test_exact_target_and_only_five_bytes_plus_checksum(self):
        original=bytearray(self.source);result=ending.repair(original)
        self.assertEqual(bytes(original),self.source)
        self.assertIsInstance(result,bytes);self.assertEqual(len(result),1048576)
        self.assertEqual(hashlib.sha256(result).hexdigest(),ending.TARGET_SHA256)
        self.assertEqual(f'{zlib.crc32(result):08X}',ending.TARGET_CRC32)
        allowed={0x18e,0x18f}|{a for a,_,_ in ending.EDITS}
        changed={i for i,(a,b) in enumerate(zip(self.source,result)) if a!=b}
        self.assertEqual(changed,allowed)
        self.assertEqual(int.from_bytes(result[0x18e:0x190],'big'),sum(int.from_bytes(result[i:i+2],'big')for i in range(512,len(result),2))&65535)
        self.assertEqual(self.source[0x295be:0x29cae],result[0x295be:0x29cae])
    def test_wrong_alias_and_already_repaired_rejected(self):
        for data in [ending.repair(self.source),self.source[:1000],bytes(1048576)]:
            with self.assertRaises(ValueError):ending.repair(data)
    def test_table_guard_independent_of_source_guard(self):
        data=bytearray(self.source);data[ending.TABLE_START]^=1
        with patch.object(ending,'SOURCE_SHA256',hashlib.sha256(data).hexdigest()):
            with self.assertRaisesRegex(ValueError,'table'):ending.repair(data)
    def test_individual_byte_guard_independent_of_table_hash(self):
        data=bytearray(self.source);data[ending.EDITS[0][0]]=0x28
        with patch.object(ending,'SOURCE_SHA256',hashlib.sha256(data).hexdigest()),patch.object(ending,'TABLE_SHA256',hashlib.sha256(data[ending.TABLE_START:ending.TABLE_START+ending.TABLE_LENGTH]).hexdigest()):
            with self.assertRaisesRegex(ValueError,'byte guard'):ending.repair(data)
    def test_output_identity_checked(self):
        with patch.object(ending,'TARGET_SHA256','0'*64):
            with self.assertRaisesRegex(ValueError,'output'):ending.repair(self.source)
    def test_loaded_safe_punctuation_shapes_and_ink(self):
        def decode(address):
            n=int.from_bytes(self.source[address:address+2],'big');address+=3;out=bytearray()
            while len(out)<n:
                op=self.source[address];address+=1;count=(op&127)+1
                if op<128:out.extend(self.source[address:address+count]);address+=count
                else:
                    distance=self.source[address]+1;address+=1
                    for _ in range(count):out.append(out[-distance])
                self.assertLessEqual(len(out),n)
            return bytes(out)
        primary=decode(0x7c10);loaded=decode(0x6fc00)
        expected={0x99:('........',)*5+('..##....','..##....','........'),0x9b:('........','...#....','..###...','..###...','...#....','...#....','........','...#....')}
        for tile,rows in expected.items():
            raw=loaded[(tile-0x74)*32:(tile-0x74+1)*32]
            primary_code=0x27 if tile==0x99 else 0x29
            self.assertEqual(raw,primary[(primary_code-2)*32:(primary_code-1)*32])
            pixels=[(raw[y*4+x//2]>>(0 if x&1 else 4))&15 for y in range(8)for x in range(8)]
            self.assertEqual(set(pixels),{0,1})
            self.assertEqual(tuple(''.join('#'if v else'.'for v in pixels[y*8:y*8+8])for y in range(8)),rows)
            self.assertLess(tile*32+31,0x16c0)
        for tile in [0x9a,0xa3]:self.assertNotIn(tile,self.source[ending.TABLE_START:ending.TABLE_START+ending.TABLE_LENGTH])
if __name__=='__main__':unittest.main()
