// adc0.c modified for 4342 Project
// Rolando Rosales 1001850424

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <shell.h>
#include "gpio.h"
#include "nvic.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"

#define ADC_CTL_DITHER 0x00000040
#define BUFFER_SIZE 10

#define BLUE_MIC PORTE,2
#define GREEN_MIC PORTE,1
#define WHITE_MIC PORTD,3

uint32_t holdoff = 80;
uint32_t h = 80;
uint16_t backoff = 200;
uint16_t b = 0;
uint16_t th = 600;
uint16_t time_constant = 10;
uint16_t t = 0;
int16_t aoa = 0;
uint8_t i = 0;
char str[MAX_CHARS];
char fail_flag = '\0';
bool always_aoa = false;
bool display_tdoa = false;
bool display_fail = false;
bool mics_detected = false;

uint16_t blue_raw = 0;
uint16_t blue_sum = 0;
uint16_t blue_arr[BUFFER_SIZE];
uint16_t blue_avg = 0;
uint16_t blue_time = 0;
uint16_t blue_time_last = 0;
bool blue_active = false; 

uint16_t green_raw = 0;
uint16_t green_sum = 0;
uint16_t green_arr[BUFFER_SIZE];
uint16_t green_avg = 0;
uint16_t green_time = 0;
uint16_t green_time_last = 0;
bool green_active = false; 

uint16_t white_raw = 0;
uint16_t white_sum = 0;
uint16_t white_arr[BUFFER_SIZE];
uint16_t white_avg = 0;
uint16_t white_time = 0;
uint16_t white_time_last = 0;
bool white_active = false; 

// Initialize Hardware
void initAdc0Ss1()
{
    // Enable clocks
    SYSCTL_RCGCADC_R |= SYSCTL_RCGCADC_R0;
    _delay_cycles(16);

    // Set pins for analog function
    selectPinAnalogInput(BLUE_MIC);                     // AIN1
    selectPinAnalogInput(GREEN_MIC);                    // AIN2
    selectPinAnalogInput(WHITE_MIC);                    // AIN4

    // Configure ADC
    ADC0_ACTSS_R &= ~ADC_ACTSS_ASEN1;                   // disable sample sequencer 1 (SS1) for programming
    ADC0_PC_R = ADC_PC_SR_1M;                           // select 1Msps rate
    ADC0_EMUX_R = ADC_EMUX_EM1_ALWAYS;                  // set EMUX to continuous for SS1
    ADC0_SSCTL1_R = ADC_SSCTL1_END2 | ADC_SSCTL1_IE2;   // mark third sample as the end
    ADC0_SSMUX1_R = (2 << 0) + (4 << 4) + (1 << 8);     // set AIN for 1, 2, 4
    enableNvicInterrupt(INT_ADC0SS1);
    ADC0_IM_R |= ADC_IM_MASK1;
    ADC0_CTL_R &= ~ADC_CTL_DITHER;                      // turn-off dithering if no averaging
    ADC0_ACTSS_R |= ADC_ACTSS_ASEN1;                    // enable SS1 for operation
}

void adc0Ss1Isr()
{
    blue_raw = ADC0_SSFIFO1_R;
    green_raw = ADC0_SSFIFO1_R;
    white_raw = ADC0_SSFIFO1_R;

    t++;
    if (t == time_constant)
    {
        if (blue_raw)
        {
            blue_sum -= blue_arr[i];
            blue_sum += blue_raw;
            blue_arr[i] = blue_raw;
            blue_avg = blue_sum / BUFFER_SIZE;
        }
        if (green_raw)
        {
            green_sum -= green_arr[i];
            green_sum += green_raw;
            green_arr[i] = green_raw;
            green_avg = green_sum / BUFFER_SIZE;
        }
        if (white_raw)
        {
            white_sum -= white_arr[i];
            white_sum += white_raw;
            white_arr[i] = white_raw;
            white_avg = white_sum / BUFFER_SIZE;
        }

        i = (i + 1) % BUFFER_SIZE;
        t = 0;
    }

    if (!blue_active && blue_raw > (blue_avg + (th - b)))
    {
        blue_active = true;
        b = backoff;
    }
    if (!green_active && green_raw > (green_avg + (th - b)))
    {
        green_active = true;
        b = backoff;
    }
    if (!white_active && white_raw > (white_avg + (th - b)))
    {
        white_active = true;
        b = backoff;
    }

    if (blue_active || green_active || white_active)
    {
        if (blue_active)
            blue_time--;
        if (green_active)
            green_time--;
        if (white_active)
            white_time--;

        h--;
    }

    if (h == 0)
    {
        if (blue_time != holdoff && green_time != holdoff && white_time != holdoff)
            mics_detected = true;
        else
        {
            fail_flag = 'h';
            mics_detected = false;
        }

        if (mics_detected || display_fail)
        {
            blue_time_last = blue_time;
            green_time_last = green_time;
            white_time_last = white_time;

            if (!blue_time_last)
                aoa = 90+2*(white_time_last - green_time_last);
            else if (!green_time_last)
                aoa = 210+2*(blue_time_last - white_time_last);
            else if (!white_time_last)
                aoa = 330+2*(green_time_last - blue_time_last);

            if (aoa > 360 && aoa <= 720)
                aoa = aoa - 360;
            if (aoa > 360)
                fail_flag = 'a';

            if (display_fail && fail_flag != '\0')
            {
                if (fail_flag == 'h')
                    putsUart0("At least one mic did not get detected, consider lower threshold\n");
                if (fail_flag == 'a')
                    putsUart0("AoA larger than 360° detected, consider lower holdoff\n");
            }

            if (always_aoa)
            {
                snprintf(str, sizeof(str), "AoA Detected\t%"PRId16"°\n", aoa);
                putsUart0(str);
                if (display_tdoa)
                {
                    snprintf(str, sizeof(str), "Blue Mic TDoA\t%"PRIu16"\n", blue_time_last);
                    putsUart0(str);
                    snprintf(str, sizeof(str), "Green Mic TDoA\t%"PRIu16"\n", green_time_last);
                    putsUart0(str);
                    snprintf(str, sizeof(str), "White Mic TDoA\t%"PRIu16"\n", white_time_last);
                    putsUart0(str);
                }
                putsUart0("\n");
            }
        }

        fail_flag = '\0';
        h = holdoff;
        b = 0;
        blue_time = holdoff;
        green_time = holdoff;
        white_time = holdoff;
        blue_active = false;
        green_active = false;
        white_active = false;
    }

    ADC0_ISC_R |= ADC_ISC_IN1;
}

