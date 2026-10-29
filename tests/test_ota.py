"""Verify OTA configuration without opening a port or flashing any device."""
import hashlib
import json
from pathlib import Path
import runpy
import sys
import tempfile
from types import ModuleType
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
class FakeEnv(dict):
    def subst(self, text):
        return str(self.get(text.lstrip('$'), ''))
    def GetProjectOption(self, name, default=''):
        return self.get(name, default)
    def Replace(self, **values):
        self.update(values)

class OtaConfigTest(unittest.TestCase):
    def run_config(self, env, targets=()):
        scons = ModuleType('SCons')
        script = ModuleType('SCons.Script')
        script.COMMAND_LINE_TARGETS = targets
        with patch.dict(sys.modules, {'SCons': scons, 'SCons.Script': script}):
            runpy.run_path(str(ROOT/'tools/configure_ota.py'), init_globals={'env': env, 'Import': lambda _: None})

    def test_private_credentials_stay_stable_and_host_can_be_overridden(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder); (root/'include').mkdir()
            env = FakeEnv(PROJECT_DIR=folder, upload_protocol='espota')
            self.run_config(env)
            data = json.loads((root/'ota.local.json').read_text())
            self.assertGreaterEqual(len(data['password']),16)
            self.assertEqual(env['UPLOAD_PORT'],data['host'])
            self.assertEqual((root/'ota.local.json').stat().st_mode & 0o777,0o600)
            header = (root/'include/LocalOtaConfig.h').read_text()
            self.assertIn(hashlib.md5(data['password'].encode()).hexdigest(),header)
            self.assertIn('#define OTA_HTTP_PASSWORD '+json.dumps(data['password']),header)
            self.assertIn(hashlib.md5(('admin:Pixel-MB OTA:'+data['password']).encode()).hexdigest(),header)
            env['UPLOAD_PORT']='192.0.2.10'
            self.run_config(env)
            self.assertEqual(env['UPLOAD_PORT'],'192.0.2.10')
            self.assertEqual(json.loads((root/'ota.local.json').read_text()),data)
            with self.assertRaisesRegex(ValueError,'OTA uploadfs disabled'):
                self.run_config(env,['uploadfs'])
            with self.assertRaisesRegex(ValueError,'OTA uploadfs disabled'):
                self.run_config(env,['uploadfsota'])
            env['UPLOADERFLAGS']=['--debug','--progress','-i','$UPLOAD_PORT']
            env['UPLOAD_PROTOCOL']='espota'
            runpy.run_path(str(ROOT/'tools/ota_upload.py'), init_globals={'env':env,'Import':lambda _:None})
            self.assertNotIn('--debug',env['UPLOADERFLAGS'])
            self.assertIn('--auth='+data['password'],env['UPLOADERFLAGS'])

if __name__ == '__main__': unittest.main()
