"""Verifica que la colección embebida conserva cada animación sin pérdidas."""
import re
import subprocess
import sys
import unittest
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class DefaultPackTest(unittest.TestCase):
    def test_embedded_collection(self):
        subprocess.run([sys.executable, str(ROOT / 'tools/embed_web.py')], check=True)
        header = (ROOT / 'include/DefaultAnimations.h').read_text()
        sources = sorted((ROOT / 'data/animations').glob('*.awx'))
        self.assertEqual(len(sources), 42)
        self.assertLessEqual(len(sources), 48)
        for index, source in enumerate(sources):
            with self.subTest(animation=source.stem):
                match = re.search(r'DEFAULT_ANIMATION_' + str(index) + r'\[\].*?\{(.*?)\};', header, re.S)
                self.assertIsNotNone(match)
                packed = bytes(int(value, 16) for value in re.findall(r'0x([0-9a-f]{2})', match[1]))
                self.assertEqual(zlib.decompress(packed), source.read_bytes())
                self.assertIn('{"' + source.stem + '", DEFAULT_ANIMATION_' + str(index) + ',', header)


if __name__ == '__main__':
    unittest.main()
