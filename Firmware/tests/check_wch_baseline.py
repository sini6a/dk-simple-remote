from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
v=(root/'vendor/ch583/EVT/EXAM/BLE/HID_Consumer/Profile/hiddev.c').read_bytes().decode('gb18030',errors='replace')
a=(root/'src/ble/wch_hiddev.c').read_text(encoding='utf-8')
def function(text,name):
    m=re.search(r'(?m)^(?:static\s+)?(?:void|uint8_t|uint16_t|bStatus_t)\s+'+name+r'\([^;]*?\)\s*\{',text)
    assert m,name
    start=text.index('{',m.start()); end=start+1; depth=1
    while depth:
        if text[end]=='{': depth+=1
        elif text[end]=='}': depth-=1
        end+=1
    return text[m.start():end]
def normalize(text):
    text=re.sub(r'#if DEFAULT_SCAN_PARAM_NOTIFY_TEST == TRUE.*?#endif','',text,flags=re.S)
    text=re.sub(r'\b(?:Batt_AddService|ScanParam_AddService)\(\);','',text)
    text=re.sub(r'\b(?:Batt_Register|ScanParam_Register|Batt_HandleConnStatusCB|ScanParam_HandleConnStatusCB)\([^;]*\);','',text)
    text=text.replace('ble_manager_bond_event(state, status);','')
    text=re.sub(r'/\*.*?\*/|//[^\r\n]*','',text,flags=re.S)
    return re.sub(r'\s+','',text)
names=['HidDev_Init','HidDev_Report','hidDevDisconnected','hidDevGapStateCB','hidDevPairStateCB',
       'hidDevHighAdvertising','hidDevLowAdvertising','hidDevInitialAdvertising','hidDevBondCount',
       'HidDev_ReadAttrCB','HidDev_WriteAttrCB','hidDevSendReport','HidDev_sendNoti']
for name in names: assert normalize(function(v,name))==normalize(function(a,name)),name
print('PASS: thirteen HID lifecycle/report/advertising/attribute/transport functions match pinned official WCH source except declared optional-service omissions and observer hook')
