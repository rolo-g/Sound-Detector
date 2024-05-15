// ADC0 Library - Modified for 4342 Project
// Rolando Rosales 1001850424

#ifndef ADC0_H_
#define ADC0_H_

extern uint32_t backoff;
extern uint16_t time_constant;
extern uint16_t holdoff;
extern uint16_t th;
extern uint16_t blue_avg;
extern uint16_t green_avg;
extern uint16_t white_avg;
extern uint16_t blue_time_last;
extern uint16_t green_time_last;
extern uint16_t white_time_last;
extern uint16_t aoa;
extern char fail_flag;
extern bool always_aoa;
extern bool display_tdoa;
extern bool display_fail;

void initAdc0Ss1();

#endif
