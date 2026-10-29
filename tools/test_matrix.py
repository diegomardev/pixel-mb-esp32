#!/usr/bin/env python3
"""Prueba de regresion con el codigo real de Adafruit NeoMatrix (g++ requerido)."""
from pathlib import Path
import argparse
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--library-dir',type=Path,help='Carpeta Adafruit NeoMatrix; por defecto .pio/libdeps tras pio run')
args=parser.parse_args()
libs=[args.library_dir] if args.library_dir else list((ROOT/'.pio/libdeps').glob('*/Adafruit NeoMatrix'))
libs=[p for p in libs if p and (p/'Adafruit_NeoMatrix.cpp').exists()]
if not libs: raise SystemExit('Compila primero con pio run, o indica --library-dir.')
with tempfile.TemporaryDirectory(prefix='awx-matrix-') as folder:
    executable=Path(folder)/'test-matrix'
    subprocess.run(['g++','-std=c++17','-DARDUINO=10800','-Wall','-Wextra','-Werror',
        '-I',str(ROOT/'tests/neomatrix_stubs'),'-I',str(libs[0]),'-I',str(ROOT/'include'),
        str(libs[0]/'Adafruit_NeoMatrix.cpp'),str(ROOT/'tests/test_matrix.cpp'),'-o',str(executable)],check=True)
    subprocess.run([str(executable)],check=True)
