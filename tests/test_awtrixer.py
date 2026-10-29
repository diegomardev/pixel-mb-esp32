import sys,json,unittest,copy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from import_awtrixer import convert
from awx import encode,decode
class AwtrixerTests(unittest.TestCase):
    def setUp(self):self.source=json.loads((ROOT/'tests/fixtures/demo.awtrixer').read_text())
    def test_demo_import(self):
        a=convert(self.source)
        self.assertEqual((a['width'],a['height'],len(a['frames']),a['frameMs']),(8,8,2,100))
        self.assertEqual(encode(decode(encode(a))),encode(a))
        self.assertGreater(len(set(tuple(f) for f in a['frames'])),1)
    def test_layout_and_size(self):
        layer=json.loads(self.source['piskel']['layers'][0]);layer['chunks'][0]['layout'][0][0]=999
        self.source['piskel']['layers']=[json.dumps(layer)]
        with self.assertRaises(ValueError):convert(self.source)
    def test_opacity(self):
        layer=json.loads(self.source['piskel']['layers'][0]);layer['opacity']=0
        self.source['piskel']['layers']=[layer]
        self.assertTrue(all(c=='#000000' for f in convert(self.source)['frames'] for c in f))
    def test_pack(self):
        for name,count in [('aw1_fire',60),('aw1_life',48),('aw2_matrix',35),('aw2_eyes',64)]:
            data=(ROOT/'data/animations'/f'{name}.awx').read_bytes();a=decode(data)
            self.assertEqual((a['width'],a['height'],len(a['frames'])),(32,8,count))
            self.assertEqual(encode(decode(encode(a))),encode(a))
            self.assertGreater(len(set(tuple(f) for f in a['frames'])),1)
if __name__=='__main__':unittest.main()
