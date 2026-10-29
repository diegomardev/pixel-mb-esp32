#!/usr/bin/env python3
"""Comprueba el formateo y dibujo real del reloj fijo (g++ requerido)."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='awx-clock-') as folder:
    binary=Path(folder)/'test-clock'
    subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',
        '-I',str(ROOT/'include'),str(ROOT/'tests/test_clock.cpp'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
