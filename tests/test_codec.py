import json
import sys
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from awx import encode, decode

class CodecTests(unittest.TestCase):
    def test_all_examples_roundtrip(self):
        for path in sorted((ROOT / 'examples').glob('*.json')):
            with self.subTest(path=path.name):
                original = json.loads(path.read_text())
                data = encode(original)
                binary = path.with_suffix('.awx')
                if not binary.exists():
                    binary = ROOT/'data/animations'/(path.stem+'.awx')
                if binary.exists():
                    self.assertEqual(decode(data), decode(binary.read_bytes()))
                self.assertEqual(data, encode(decode(data)))

    def test_known_colors(self):
        a = dict(width=4, height=1, frameMs=20, frames=[['#ff0000','#00ff00','#0000ff','#ffffff']])
        self.assertEqual(encode(a)[12:], bytes.fromhex('00f8 e007 1f00 ffff'))

    def test_reject_bad_files(self):
        data = (ROOT/'data/animations/1027.awx').read_bytes()
        for candidate in (b'',data[:11],data[:-1],data+b'\0',b'NOPE'+data[4:]):
            with self.assertRaises(ValueError): decode(candidate)

    def test_maximum_animation(self):
        data = encode(dict(width=32,height=8,frameMs=5000,frames=[['#123456']*256 for _ in range(120)]))
        self.assertEqual(len(data),61452)
        self.assertEqual(encode(decode(data)),data)

    def test_bad_json(self):
        for obj in (None,{},dict(width=True,height=8,frameMs=100,frames=[[]]),
                    dict(width=8,height=8,frameMs=19,frames=[['#ffffff']*64]),
                    dict(width=8,height=8,frameMs=100,frames=[['bad']*64])):
            with self.assertRaises(ValueError): encode(obj)

if __name__=='__main__': unittest.main()
