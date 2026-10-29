#!/usr/bin/env python3
"""Convierte animations_default y, con --upload, las añade al ESP sin borrar otros archivos."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import urllib.error
import urllib.parse
import urllib.request
from awx import encode, encode_compact


def prepare(source, output):
    output.mkdir(parents=True, exist_ok=True)
    manifest, hashes, names = [], set(), set()
    for path in sorted(source.glob('*.json')):
        animation = json.loads(path.read_text(encoding='utf-8'))
        raw = encode(animation)
        digest = hashlib.sha256(raw).hexdigest()
        if digest in hashes:
            continue
        name = re.sub(r'_[0-9a-fA-F]{4}$', '', re.sub(r' \(\d+\)$', '', path.stem))
        if not re.fullmatch(r'[A-Za-z0-9_-]{1,20}', name) or name in names:
            raise ValueError('Nombre inválido o duplicado: ' + name)
        data = encode_compact(animation)
        target = output / (name + '.awx')
        target.write_bytes(data)
        manifest.append(dict(name=name, source=path.name, file=str(target), bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
        hashes.add(digest)
        names.add(name)
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    return manifest


def upload(host, manifest):
    headers = {}
    key = os.environ.get('PIXEL_MB_API_KEY', '')
    if key:
        headers['X-API-Key'] = key
    def request(route, data=None, extra=None):
        req = urllib.request.Request(host.rstrip('/') + route, data=data, headers=dict(headers, **(extra or {})))
        with urllib.request.urlopen(req, timeout=45) as response:
            return response.read()
    status = json.loads(request('/api/status'))
    version = status.get('firmware', '').removeprefix('Pixel-MB ').split('.')
    if len(version) != 3 or not all(v.isdigit() for v in version) or tuple(map(int, version)) < (1, 0, 15):
        raise ValueError('Primero instala Pixel-MB 1.0.15 o posterior (AWX2 y hasta 48 animaciones)')
    existing = {a['name'] for a in json.loads(request('/api/animations'))['animations']}
    if len(existing | {a['name'] for a in manifest}) > 48:
        raise ValueError('La colección supera las 48 animaciones; no se ha enviado nada')
    for item in manifest:
        data = Path(item['file']).read_bytes()
        route = '/api/animations?name=' + urllib.parse.quote(item['name'])
        if item['name'] in existing and request(route) == data:
            print(item['name'] + ': ya coincide', flush=True)
            continue
        boundary = 'PixelMBAnimationUpload'
        body = ('--' + boundary + '\r\nContent-Disposition: form-data; name="file"; filename="' + item['name'] + '.awx"\r\nContent-Type: application/octet-stream\r\n\r\n').encode() + data + ('\r\n--' + boundary + '--\r\n').encode()
        result = json.loads(request(route, body, {'Content-Type': 'multipart/form-data; boundary=' + boundary}))
        if not result.get('ok') or request(route) != data:
            raise ValueError('No se pudo verificar: ' + item['name'])
        print(item['name'] + ': guardada y verificada (' + str(len(data)) + ' bytes)', flush=True)
    final = json.loads(request('/api/status'))
    print('LittleFS: ' + str(final['fsUsed']) + ' / ' + str(final['fsTotal']) + ' bytes', flush=True)


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=root / 'animations_default')
    parser.add_argument('--output', type=Path, default=root / 'work' / 'animations_default')
    parser.add_argument('--host', default='http://192.168.1.50')
    parser.add_argument('--upload', action='store_true')
    args = parser.parse_args()
    manifest = prepare(args.source, args.output)
    print(str(len(manifest)) + ' animaciones únicas, ' + str(sum(a['bytes'] for a in manifest)) + ' bytes', flush=True)
    if args.upload:
        upload(args.host, manifest)


if __name__ == '__main__':
    main()
