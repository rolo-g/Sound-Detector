// 4342 Project main
// Rolando Rosales 1001850424

#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <shell.h>
#include "clock.h"
#include "gpio.h"
#include "adc0.h"
#include "uart0.h"
#include "wait.h"
#include "tm4c123gh6pm.h"

void initHw(void)
{
    initSystemClockTo40Mhz();

    enablePort(PORTE);
    enablePort(PORTD);
}

void printHelp(void)
{
    char str[MAX_CHARS];

    putsUart0("Commands:\n");
    // reset
    putsUart0(" reset\t\tResets the hardware\n"); 
    // average
    putsUart0(" avg (always)\tDisplays average value in DAC & SPL units for each mic,\n");
    putsUart0(" \t\toptionally use always to toggle displaying the avg every sec\n"); 
    // time constant
    putsUart0(" tc T\t\tSet time constant of average filter to T\n"); 
    snprintf(str, sizeof(str), "\t\tcurrent: %"PRIu16"\n", time_constant);
    putsUart0(str);
    // backoff
    putsUart0(" backoff B\tSet the backoff between first and subsequent mic\n"); 
    putsUart0(" \t\tsignal threshold levels\n"); 
    snprintf(str, sizeof(str), "\t\tcurrent: %"PRIu16"\n", backoff);
    putsUart0(str);
    // holdoff
    putsUart0(" holdoff H\tSet minimum time before next event cant be detected\n"); 
    snprintf(str, sizeof(str), "\t\tcurrent: %"PRIu32"\n", holdoff);
    putsUart0(str);
    // aoa
    putsUart0(" aoa (always)\tReturn most current AoA in theta and phi, optionally\n"); 
    putsUart0(" \t\tuse always to toggle the auto display the AoA once detected\n"); 
    // tdoa
    putsUart0(" tdoa ON|OFF\tSet the display of the TDoA data for qualified events\n"); 
    putsUart0(" \t\twhen the AoA data is shown\n"); 
    // fail
    putsUart0(" fail ON|OFF\tSet the display of the partial data set of data from\n");
    putsUart0(" \t\tfrom the sensors when there is no qualified event\n");
    // th
    putsUart0(" thresh TH\tSet the peak threshold before the mics are activated\n");
    snprintf(str, sizeof(str), "\t\tcurrent: %"PRIu16"\n", th);
    putsUart0(str);
    // clr
    putsUart0(" clr\t\tClears terminal window\n"); 
    // help
    putsUart0(" help\t\tDisplays this message again\n\n");
}

void displayMicAverage(uint16_t avg)
{
    char str[MAX_CHARS];

    float dac = ((float)avg/4096.0) * 3.3;
    float spl = 20.0*log10(dac/3.3) + 44.0 + 94.0;

    snprintf(str, sizeof(str), "DAC: %.3fV SPL: %.3f dB\n", dac, spl);
    putsUart0(str);
}

int main(void)
    {
    initHw();    
    initUart0();
    initAdc0Ss1();

    USER_DATA data;
    uint32_t temp = 0;
    char str[MAX_CHARS];
    char* ptr = NULL;
    bool valid = true;
    bool always_avg = false;

    data.fieldCount = 0;
    clearField(&data);

    printHelp();

    while (true)
    {
        if (always_avg)
        {
            waitMicrosecond(5e5);
            putsUart0("Blue mic\t");
            displayMicAverage(blue_avg);
            putsUart0("Green mic\t");
            displayMicAverage(green_avg);
            putsUart0("White mic\t");
            displayMicAverage(white_avg);
            putsUart0("\n");
        }

        if (kbhitUart0())
        {
            getsUart0(&data);
            parseFields(&data);

            if (isCommand(&data, "reset", 0) || isCommand(&data, "r", 0))
            {
                putsUart0("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
                putsUart0("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
                NVIC_APINT_R = NVIC_APINT_VECTKEY | NVIC_APINT_SYSRESETREQ;
            }

            if (isCommand(&data, "average", 0) || isCommand(&data, "avg", 0))
            {
                ptr = getFieldString(&data, 1);
                if (stringsEqual("always", ptr))
                {
                    putsUart0("Always print average toggled\n\n");
                    always_avg ^= true;
                }
                else
                {
                    putsUart0("Blue mic\t");
                    displayMicAverage(blue_avg);
                    putsUart0("Green mic\t");
                    displayMicAverage(green_avg);
                    putsUart0("White mic\t");
                    displayMicAverage(white_avg);
                    putsUart0("\n");
                }

                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "tc", 0))
            {
                temp = getFieldInteger(&data, 1);
                if (temp)
                {
                    time_constant = temp;
                    snprintf(str, sizeof(str), "Average filter time constant set to %"PRIu8"\n\n", time_constant);
                    putsUart0(str);
                    temp = 0;
                }
                else
                {
                    snprintf(str, sizeof(str), "Time constant set to current value of %"PRIu8" due to invalid input\n\n", time_constant);
                    putsUart0(str);
                }

                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "backoff", 0) || isCommand(&data, "bo", 0))
            {
                temp = getFieldInteger(&data, 1);
                if (temp)
                {
                    backoff = temp;
                    snprintf(str, sizeof(str), "Backoff set to %"PRIu16"\n\n", backoff);
                    putsUart0(str);
                    temp = 0;
                }
                else
                {
                    snprintf(str, sizeof(str), "Backoff set to current value of %"PRIu16" due to invalid input\n\n", backoff);
                    putsUart0(str);
                }

                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "holdoff", 0) || isCommand(&data, "ho", 0))
            {
                temp = getFieldInteger(&data, 1);
                if (temp)
                {
                    holdoff = temp;
                    snprintf(str, sizeof(str), "Holdoff set to %"PRIu32"\n\n", holdoff);
                    putsUart0(str);
                    temp = 0;
                }
                else
                {
                    snprintf(str, sizeof(str), "Holdoff set to current value of %"PRIu32" due to invalid input\n\n", holdoff);
                    putsUart0(str);
                }

                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "aoa", 0))
            {
                char* test = getFieldString(&data, 1);
                if (stringsEqual("always", test))
                {
                    putsUart0("Always print angle of arrival toggled\n\n");
                    always_aoa ^= true;
                }
                else
                {
                    snprintf(str, sizeof(str), "Latest AoA\t%"PRIu16"°\n", aoa);
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

                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "tdoa", 1))
            {
                ptr = getFieldString(&data, 1);
                if (stringsEqual("ON", ptr))
                {
                    putsUart0("Time distance of arrival displayed on event\n\n");
                    display_tdoa = true;
                    valid = true;
                }
                if (stringsEqual("OFF", ptr))
                {
                    putsUart0("Time distance of arrival no longer displayed\n\n");
                    display_tdoa = false;
                    valid = true;
                }
                if (!valid)
                    putsUart0("TDoA must be set 'ON' or 'OFF'\n\n");
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "fail", 1) || isCommand(&data, "f", 1))
            {
                ptr = getFieldString(&data, 1);
                if (stringsEqual("ON", ptr))
                {
                    putsUart0("Displaying failed events\n\n");
                    display_fail = true;
                    valid = true;
                }
                if (stringsEqual("OFF", ptr))
                {
                    putsUart0("No longer displaying failed events \n\n");
                    display_fail = false;
                    valid = true;
                }
                if (!valid)
                    putsUart0("Fail must be set 'ON' or 'OFF'\n\n");
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "thresh", 0) || isCommand(&data, "th", 0))
            {
                temp = getFieldInteger(&data, 1);
                if (temp)
                {
                    th = temp;
                    snprintf(str, sizeof(str), "Threshold set to %"PRIu16"\n\n", th);
                    putsUart0(str);
                    temp = 0;
                }
                else
                {
                    snprintf(str, sizeof(str), "Threshold set to current value of %"PRIu16" due to invalid input\n\n", th);
                    putsUart0(str);
                }

                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "clr", 0) || isCommand(&data, "c", 0))
            {
                putsUart0("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
                putsUart0("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (isCommand(&data, "help", 0) || isCommand(&data, "h", 0))
            {
                printHelp();
                valid = true;
                clearField(&data);
                data.fieldCount = 0;
            }

            if (!valid)
            {
                putsUart0("Command not valid\n\n");
                valid = true;
            }

            clearField(&data);
        }
    }
}
