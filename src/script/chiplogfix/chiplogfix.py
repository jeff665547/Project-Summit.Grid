import json
import sys
import os
from pathlib import Path
import copy
import traceback

inst_path = Path(os.path.abspath(__file__)).parent.parent.parent
chipspec_path = inst_path / 'etc' / 'private' / 'chip.json'
chipspec = json.load(open(chipspec_path, 'r'))

l1b_shift_map = {
    'A': [0, 0],
    'B': [2480, 0],
    'C': [2480, 2480],
    'D': [0, 2480]
}

def fix_if_L1Bsp(jchip_log, chip_log_path):
    if jchip_log['chip']['name'] == 'lassen33':
        print('L1B sp chip log found: {}'.format(chip_log_path))
        # TODO: shift
        # TODO: comp
        jchip_log['chip']['name'] = 'lassen-comp33'
        jchip_log['chip']['spec']['name'] = 'lassen-comp'
        rfid_str = Path(chip_log_path).parent.parent.name
        print(rfid_str)
        rfid = rfid_str.split('-')
        print(rfid)
        comp = 'A'
        if len(rfid) < 1:
            print('not expected RFID name, use "A" as default component name')
        else:
            trayt_at = rfid[0].split('@')
            if len(trayt_at) != 2:
                print('not expected RFID name, use "A" as default component name')
        comp = trayt_at[1]
        jchip_log['chip']['comp'] = comp
        jchip_log['chip']['spec']['shift'] = l1b_shift_map[comp]
        return True
    return False

def fix_if_L2C_banff(jchip_log, path):
    global chipspec
    rfid_str = path.parent.parent.name
    rfid = rfid_str.split('-')
    if len(rfid) < 1:
        return False
    tray_type = rfid[0]
    if tray_type != 'L2C':
        return False
    if jchip_log['chip']['name'] != 'banff33':
        return False
    jchip_log['chip']['name'] = 'lassen-comp33'
    jchip_log['chip']['spec'] = chipspec['lassen-comp']
    return True

def fix_if_banff_marker_error(jchip_log):
    if jchip_log['chip']['name'] != 'banff33':
        return False
    
    for chn in jchip_log['channels']:
        if chn['marker_type'] == 'AM1':
            chn['marker_type'] = 'AM1E'
    return True
    
def fix_if_yz01_marker_error(jchip_log):
    if jchip_log['chip']['name'] != 'yz0177' and jchip_log['chip']['name'] != 'yz01-4122':
        return False
    
    for chn in jchip_log['channels']:
        if chn['marker_type'] == 'AM1':
            chn['marker_type'] = 'AM1E'
        elif chn['marker_type'] == 'AM3':
            chn['marker_type'] = 'AM5B'
    return True

def fix_if_kenai_marker_error(jchip_log):
    if jchip_log['chip']['name'] != 'kenai77':
        return False
    
    for chn in jchip_log['channels']:
        if chn['marker_type'] == 'AM1':
            chn['marker_type'] = 'AM5B'
        elif chn['marker_type'] == 'AM3':
            chn['marker_type'] = 'AM1E'
    return True

def fix_if_lassen_marker_error(jchip_log):
    if jchip_log['chip']['name'] != 'lassen-comp33':
        return False
    
    for chn in jchip_log['channels']:
        if chn['marker_type'] == 'AM1E':
            chn['marker_type'] = 'AM5B'
        elif chn['marker_type'] == 'AM5B':
            chn['marker_type'] = 'AM1E'
        elif chn['marker_type'] == 'AM1':
            chn['marker_type'] = 'AM5B'
        elif chn['marker_type'] == 'AM3':
            chn['marker_type'] = 'AM1E'
    return True

def process_chip_log_path(path):
    chip_log_path = path.absolute()
    print('found chip log: {}'.format(chip_log_path))
    chip_log_file = open(chip_log_path, 'r')
    jchip_log = json.load(chip_log_file)
    backup_chip_log_path = (path.parent / 'chip_log.json.bak').absolute()

    if os.path.exists(backup_chip_log_path):
        print('chip_log.json.bak exist')
        print('chip log has been processed, skip')
        return
    if not jchip_log['scan_success']:
        print('chip scan not success, skip')
        return
    if not 'chip' in jchip_log:
        print('chip log format incorrect, ignore')
        return
    if not 'name' in jchip_log['chip']:
        print('chip log format incorrect, ignore')
        return

    backup_chip_log = copy.deepcopy(jchip_log)

    hasfix = False
    print('chip type issue fix')
    if fix_if_L1Bsp(jchip_log, chip_log_path): 
        print('L1B SP chip process done')
        hasfix = True
    elif fix_if_L2C_banff(jchip_log, chip_log_path):
        print('L2C(banff) chip process done')
        hasfix = True
    else:
        print('no chip type issue found')

    print('channel marker issue fix')

    if fix_if_banff_marker_error(jchip_log):
        print('banff marker fix done')
        hasfix = True
    elif fix_if_yz01_marker_error(jchip_log):
        print('yz01 marker fix done')
        hasfix = True
    elif fix_if_kenai_marker_error(jchip_log):
        print('kenai marker fix done')
        hasfix = True
    elif fix_if_lassen_marker_error(jchip_log):
        print('lassen marker fix done')
        hasfix = True
    else:
        print('no marker type issue found')

    if hasfix:
        backup_chip_log_file = open(backup_chip_log_path, 'w')
        json.dump(backup_chip_log, backup_chip_log_file, indent=2)
        chip_log_file.close()
        chip_log_file = open(chip_log_path, 'w')
        json.dump(jchip_log, chip_log_file, indent=2)

rootp = os.path.abspath(sys.argv[1])
print('scanning tree from: {}'.format(rootp))

for path in Path(rootp).rglob('chip_log.json'):
    try:
        process_chip_log_path(path)
    except:
        print('chip log: {} process failed, ignored'.format(path))