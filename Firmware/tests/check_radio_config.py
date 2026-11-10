"""Preprocess the actual SDK MCU.c to verify every supported radio override."""
from pathlib import Path
import re,subprocess,sys
root=Path(__file__).resolve().parents[1]
cc=sys.argv[1]
includes=['config','src/app','src/drivers','src/ble','src/power']
includes+=['vendor/ch583/EVT/EXAM/'+p for p in ['BLE/HAL/include','BLE/LIB','SRC/StdPeriphDriver/inc','SRC/RVMSIS','BLE/HID_Consumer/Profile/include']]
base=[cc,'-E','-P','-include','config/app_config.h',*['-I'+p for p in includes]]
source='vendor/ch583/EVT/EXAM/BLE/HAL/MCU.c'
for dbm,encoded in [(-16,1),(-12,2),(-8,4),(-5,7),(-3,9),(-1,11),(0,13)]:
    out=subprocess.run([*base,f'-DAPP_TX_POWER_DBM={dbm}',source],cwd=root,text=True,capture_output=True,check=True).stdout
    value=re.search(r'cfg\.TxPower\s*=\s*\(uint32_t\)\s*(0[xX][0-9a-fA-F]+|[0-9]+)',out)
    assert value and int(value[1],0)==encoded,(dbm,value)
bad=subprocess.run([*base,'-DAPP_TX_POWER_DBM=-6',source],cwd=root,text=True,capture_output=True)
assert bad.returncode and 'Unsupported APP_TX_POWER_DBM' in bad.stderr
build=(root/'tools/build.py').read_text()
assert 'flags = ["-include", "config/app_config.h"' in build
print('PASS: actual SDK MCU.c receives encoded TX power at all supported levels; invalid dBm rejected; global firmware include enforced')
