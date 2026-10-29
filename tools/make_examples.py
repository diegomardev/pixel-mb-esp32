"""Iconos originales de demostración, sin descargar la galería de AWTRIX."""
from pathlib import Path
import colorsys
import json
from awx import encode

ROOT = Path(__file__).resolve().parents[1]
ICONS = {
    'adquio': ('#ff00ff', ['00011000','00100100','01000010','01000010','01111110','01000010','01000010','00000000']),
    '1027': ('#00ff50', ['00000010','00001110','00111110','01111100','01111000','00110000','01000000','10000000']),
    '1019': ('#ff4030', ['00111100','01000010','10100101','10000001','10011001','10100101','01000010','00111100']),
    '609': ('#ff8020', ['00011000','00100100','00110100','00110100','00110100','01111000','01111000','00110000']),
    '693': ('#0099ff', ['00010000','00011000','00111100','00111100','01111110','01111110','00111100','00000000']),
}
for name, (color, rows) in ICONS.items():
    original = [color if c == '1' else '#000000' for row in rows for c in row]
    dim = ['#' + ''.join(f'{int(c[i:i+2],16)//2:02x}' for i in (1,3,5)) for c in original]
    animation = dict(width=8, height=8, frameMs=400, frames=[original, dim])
    (ROOT / 'data/animations' / (name + '.awx')).write_bytes(encode(animation))
    (ROOT / 'examples' / (name + '.json')).write_text(json.dumps(animation, indent=2) + '\n')
frames = []
for f in range(12):
    frame = []
    for y in range(8):
        for x in range(32):
            rgb = colorsys.hsv_to_rgb((x / 32 + f / 12) % 1, 1, 0.35 if y % 2 else 0.7)
            frame.append('#' + ''.join(f'{int(c*255):02x}' for c in rgb))
    frames.append(frame)
full = dict(width=32, height=8, frameMs=100, frames=frames)
(ROOT / 'data/animations/rainbow.awx').write_bytes(encode(full))
(ROOT / 'examples/rainbow.json').write_text(json.dumps(full, indent=2) + '\n')
