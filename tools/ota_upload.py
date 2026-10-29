"""Configure authentication after the platform creates its uploader arguments."""
from pathlib import Path
import json
Import('env')
if env.subst('$UPLOAD_PROTOCOL') == 'espota':
    config = json.loads((Path(env.subst('$PROJECT_DIR')) / 'ota.local.json').read_text())
    # espota debug logging prints the password. Keep normal progress without it.
    flags = [flag for flag in env.get('UPLOADERFLAGS', []) if flag not in ('--debug', '-d')]
    env.Replace(UPLOADERFLAGS=flags + ['--host_port=8266', '--auth=' + config['password']])
