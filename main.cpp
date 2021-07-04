/**
 * Project: Blackbody Boards
 * File: main.cpp
 * Author: Matthew Yu (2021).
 * Organization: UT Solar Vehicles Team
 * Created on: 05/29/21
 * Last Modified: 07/03/21
 * File Description: Determines execution and/or building of Blackbody
 * board type A or B.
 * 
 * L432KC Pinout:
 * https://os.mbed.com/media/uploads/bcostm/nucleo_l432kc_2017_10_09.png
 * Note: The following pins must be reserved during STLink debugging:
 * - PA11 | D10 | USP_DM
 * - PA12 | D2  | USB_DP
 * - PA13 | N/A | USB_NOE
 * - PC14 | D7  | RCC_OSC32_IN
 * - PC15 | D8  | RCC_OSC32_OUT
 * - PA14 | N/A | SYS_JTCK_SWCLK
 * - PA15 | N/A | SYS_JTDI
 * - PB3  | D13 | SYS_JTDO_SW0
 * - PA13 | N/A | SYS_JTMS_SWDIO
 * - PB4  | D12 | SYS_JTRST
 * - PB7  | D4  | SYS_PVD_IN
 * - PA0  | A0  | SYS_WKUP1
 * - PA2  | A7  | SYS_WKUP4
 * L432KC specific.
 */

/** Defines. */
#define TYPE_A 0
#define TYPE_B 1
#define BOARD_TYPE TYPE_A

/** Includes. */
#if BOARD_TYPE == TYPE_A
    #include "mainA.hpp"
    int main(void) { return mainA(); }
#elif BOARD_TYPE == TYPE_B
    #include "mainB.hpp"
    int main(void) { return mainB(); }
#else
    int main(void) {
        while(true);
        return 1;
    }
#endif