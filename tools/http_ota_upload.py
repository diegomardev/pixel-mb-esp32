"""Upload firmware with HTTP Digest authentication, without a reverse connection."""
import json
from pathlib import Path
import sys
import urllib.request

root = Path(__file__).resolve().parents[1]
config = json.loads((root / 'ota.local.json').read_text())
firmware = Path(sys.argv[1])
if firmware.name != 'firmware.bin':
    raise SystemExit('Only firmware.bin uploads are supported; filesystem uploads are disabled.')
url = 'http://' + config['host'] + '/update'
manager = urllib.request.HTTPPasswordMgrWithDefaultRealm()
manager.add_password('Pixel-MB OTA', url, 'admin', config['password'])
opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), urllib.request.HTTPDigestAuthHandler(manager))
boundary = 'PixelMBFirmwareBoundaryESP32'
body = (f'--{boundary}\r\nContent-Disposition: form-data; name="firmware"; filename="firmware.bin"\r\n'
        'Content-Type: application/octet-stream\r\n\r\n').encode() + firmware.read_bytes() + f'\r\n--{boundary}--\r\n'.encode()
request = urllib.request.Request(url, data=body, headers={'Content-Type': 'multipart/form-data; boundary=' + boundary})
print('HTTP OTA:', config['host'], '(Digest authentication; password hidden)', flush=True)
try:
    with opener.open(request, timeout=90) as response:
        result = json.load(response)
    if result.get('ok') is not True:
        raise ValueError('ESP did not confirm a successful update')
except Exception as error:
    raise SystemExit('HTTP OTA failed: ' + str(error))
print('HTTP OTA: SUCCESS; ESP rebooting. Saved animations and settings retained.')
