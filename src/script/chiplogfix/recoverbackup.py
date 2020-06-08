import json
import sys
import os
from pathlib import Path
import copy
import traceback

rootp = os.path.abspath(sys.argv[1])
print('scanning tree from: {}'.format(rootp))

for path in Path(rootp).rglob('chip_log.json.bak'):
    print('process {}'.format(path))
    os.replace(path, path.parent / 'chip_log.json')