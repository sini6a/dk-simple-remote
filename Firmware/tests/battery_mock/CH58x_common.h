#include <stdint.h>
#define RB_ADC_START 1
#define RB_ADC_DATA 4095
#define RB_ADC_POWER_ON 1
#define RB_ADC_BUF_EN 2
#define RB_ADC_OFS_TEST 8
#define RB_TKEY_PWR_ON 1
#define RB_TEM_SEN_PWR_ON 128
#define CH_INTE_VBAT 14
extern uint8_t R8_ADC_CFG,R8_ADC_CHANNEL,R8_TKEY_CFG,R8_TEM_SENSOR;
extern uint16_t R16_ADC_DATA;
uint8_t *adc_control(void);
#define R8_ADC_CONVERT (*adc_control())
void SYS_DisableAllIrq(unsigned long *s);
void SYS_RecoverIrq(unsigned long s);
void ADC_InterBATSampInit(void);
