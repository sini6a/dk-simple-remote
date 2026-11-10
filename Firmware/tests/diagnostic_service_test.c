#define APP_DIAGNOSTICS 1
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/ble/diagnostic_service.c"
const uint8_t primaryServiceUUID[]={0x00,0x28};
const uint8_t characterUUID[]={0x03,0x28};
static unsigned snapshots;
void ble_manager_read_diagnostics(uint8_t out[32])
{ unsigned i; ++snapshots; for(i=0;i<32;i++) out[i]=(uint8_t)(i+snapshots); }
void tmos_memcpy(void *dst,const void *src,uint32_t len) { memcpy(dst,src,len); }
bStatus_t GATTServApp_RegisterService(gattAttribute_t *a,uint16_t count,uint8_t key,gattServiceCBs_t *cb)
{ assert(a==attrs && count==4 && key==GATT_MAX_ENCRYPT_KEY_SIZE && cb==&callbacks); return SUCCESS; }
int main(void)
{
    uint8_t buffer[32]; uint16_t len;
    assert(diagnostic_service_init()==SUCCESS);
    assert(attrs[2].type.len==16 && attrs[2].permissions==GATT_PERMIT_READ);
    assert(callbacks.pfnWriteAttrCB==NULL);
    assert(read_value(1,&attrs[2],buffer,&len,0,22,0)==SUCCESS && len==22 && snapshots==1);
    assert(buffer[0]==1 && buffer[21]==22);
    assert(read_value(1,&attrs[2],buffer+22,&len,22,22,0)==SUCCESS && len==10 && snapshots==1);
    assert(buffer[22]==23 && buffer[31]==32);
    assert(read_value(1,&attrs[2],buffer,&len,32,22,0)==SUCCESS && len==0);
    assert(read_value(1,&attrs[2],buffer,&len,33,22,0)==ATT_ERR_INVALID_OFFSET);
    assert(read_value(1,&attrs[3],buffer,&len,0,32,0)==SUCCESS);
    assert(len==strlen("Reconnect state (hex)") && !memcmp(buffer,label,len));
    assert(snapshots==1); /* Description does not overwrite the latched packet. */
    assert(read_value(1,&attrs[2],buffer,&len,0,32,0)==SUCCESS && snapshots==2);
    puts("PASS: diagnostic GATT read-only permissions, coherent long reads, offsets, description and fresh snapshots");
    return 0;
}
