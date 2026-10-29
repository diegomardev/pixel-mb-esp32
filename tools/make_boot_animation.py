"""Export the actual C++ boot renderer as a replayable Pixel-MB JSON/AWX."""
from pathlib import Path
import json
import struct
import subprocess
import tempfile
from awx import encode

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as folder:
    exe = Path(folder) / 'boot-renderer'
    raw = Path(folder) / 'frames.raw'
    subprocess.run(['g++', '-std=c++17', '-I'+str(root/'include'), str(root/'tests/test_boot_logo.cpp'), '-o', str(exe)], check=True)
    subprocess.run([str(exe), str(raw)], check=True)
    pixels = raw.read_bytes()
frames = []
for start in range(0, len(pixels), 512):
    frame = []
    for color in struct.unpack('<256H', pixels[start:start+512]):
        r, g, b = (color >> 11) & 31, (color >> 5) & 63, color & 31
        frame.append('#%02x%02x%02x' % ((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)))
    frames.append(frame)
animation = dict(name='Pixel-MB', source='Pixel-MB boot renderer: include/BootLogo.h', width=32, height=8, frameMs=40, frames=frames)
(root/'examples/pixel-mb.json').write_text(json.dumps(animation, indent=2)+'\n')
(root/'data/animations/pixel-mb.awx').write_bytes(encode(animation))
print('Pixel-MB animation: %s frames, 40 ms/frame, %s seconds' % (len(frames), len(frames)*.04))
