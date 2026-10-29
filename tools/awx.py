#!/usr/bin/env python3
"""Convierte JSON del editor <-> AWX1/AWX2; solo biblioteca estándar de Python."""
import argparse
import json
import re
import struct
from pathlib import Path

HEADER = struct.Struct('<4sBBHHH')

def validate(animation):
    if not isinstance(animation, dict):
        raise ValueError('Se espera un objeto de animación')
    w, h, ms = (animation.get(key) for key in ('width', 'height', 'frameMs'))
    frames = animation.get('frames')
    if not (type(w) is int and 1 <= w <= 32 and type(h) is int and 1 <= h <= 8
            and type(ms) is int and 20 <= ms <= 5000
            and isinstance(frames, list) and 1 <= len(frames) <= 120):
        raise ValueError('Dimensiones, frameMs o número de fotogramas inválidos')
    for frame in frames:
        if (not isinstance(frame, list) or len(frame) != w * h or any(
                not isinstance(c, str) or not re.fullmatch(r'#[0-9a-fA-F]{6}', c) for c in frame)):
            raise ValueError('Cada fotograma contiene width*height colores #RRGGBB')
    return animation


def encode(animation):
    a = validate(animation)
    result = bytearray(HEADER.pack(b'AWX1', a['width'], a['height'], len(a['frames']), a['frameMs'], 0))
    for frame in a['frames']:
        for color in frame:
            n = int(color[1:], 16)
            r, g, b = n >> 16, (n >> 8) & 255, n & 255
            result.extend(struct.pack('<H', ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)))
    return bytes(result)


def encode_compact(animation):
    raw = encode(animation)
    _, w, h, count, ms, _ = HEADER.unpack_from(raw)
    output = bytearray(HEADER.pack(b'AWX2', w, h, count, ms, 0))
    frame_bytes = w * h * 2
    for offset in range(12, len(raw), frame_bytes):
        frame = raw[offset:offset + frame_bytes]
        runs = bytearray()
        i = 0
        while i < len(frame):
            j = i + 2
            while j < len(frame) and j - i < 510 and frame[j:j+2] == frame[i:i+2]:
                j += 2
            runs.extend(bytes([(j-i)//2]) + frame[i:i+2])
            i = j
        output.extend(struct.pack('<H', len(runs) if len(runs) < len(frame) else 0))
        output.extend(runs if len(runs) < len(frame) else frame)
    return bytes(output) if len(output) < len(raw) else raw


def decode(data):
    if len(data) < HEADER.size:
        raise ValueError('Archivo truncado')
    magic, w, h, count, ms, reserved = HEADER.unpack_from(data)
    if magic not in (b'AWX1', b'AWX2') or reserved or not (1 <= w <= 32 and 1 <= h <= 8 and 1 <= count <= 120):
        raise ValueError('Cabecera inválida')
    if magic == b'AWX2':
        expanded = bytearray(HEADER.pack(b'AWX1', w, h, count, ms, 0))
        offset = 12
        for _ in range(count):
            if offset + 2 > len(data): raise ValueError('Fotograma truncado')
            size, = struct.unpack_from('<H', data, offset)
            offset += 2
            length = size or w * h * 2
            chunk = data[offset:offset+length]
            if len(chunk) != length: raise ValueError('Fotograma truncado')
            offset += length
            if not size: expanded.extend(chunk); continue
            if size % 3: raise ValueError('RLE inválido')
            frame = bytearray()
            for i in range(0, size, 3):
                if not chunk[i]: raise ValueError('RLE vacío')
                frame.extend(chunk[i+1:i+3] * chunk[i])
            if len(frame) != w * h * 2: raise ValueError('RLE inválido')
            expanded.extend(frame)
        if offset != len(data): raise ValueError('Datos sobrantes')
        return decode(bytes(expanded))
    if len(data) != HEADER.size + w * h * count * 2:
        raise ValueError('Tamaño incorrecto')
    frames = []
    colors = struct.unpack_from('<' + 'H' * (w * h * count), data, HEADER.size)
    for start in range(0, len(colors), w * h):
        frame = []
        for n in colors[start:start + w * h]:
            r = int(((n >> 11) & 31) * 255 / 31 + 0.5)
            g = int(((n >> 5) & 63) * 255 / 63 + 0.5)
            b = int((n & 31) * 255 / 31 + 0.5)
            frame.append(f'#{r:02x}{g:02x}{b:02x}')
        frames.append(frame)
    return validate(dict(width=w, height=h, frameMs=ms, frames=frames))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--compact', action='store_true', help='AWX2 RLE sin pérdida si ocupa menos')
    args = parser.parse_args()
    if args.input.suffix.lower() == '.json':
        args.output.write_bytes((encode_compact if args.compact else encode)(json.loads(args.input.read_text(encoding='utf-8'))))
    else:
        args.output.write_text(json.dumps(decode(args.input.read_bytes()), indent=2) + '\n', encoding='utf-8')

if __name__ == '__main__':
    main()
