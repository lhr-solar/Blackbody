/**
 * Project: Blackbody A Board
 * File: mainA.hpp
 * Author: Matthew Yu (2021).
 * Organization: UT Solar Vehicles Team
 * Created on: 05/29/21
 * Last Modified: 07/03/21
 * File Description: Header file for Blackbody boards type A.
 * The Type A board includes both the light sensor and a set of temperature
 * sensors. This board will be installed into the solar vehicle for data
 * collection.
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


/** Includes. */
#include "mbed.h"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include "ComIds.h"
#include "Errors.h"
#include "Fifo.cpp"

/** Defines. */
#define USB_TX USBTX /* A7. */
#define USB_RX USBRX /* A2. */
#define CAN_TX D2
#define CAN_RX D10
#define PRELUDE 0xFF
#define SAMPLE_FREQ_HZ_IRRAD 10
#define SAMPLE_FREQ_HZ_TEMP 2
#define MAX_TEMP_SENSORS 8

/** Struct definitions. */
/** The result struct is used by information sources (i.e. sensors or CAN/serial
    messages from sensors) to format data posting to the user. */
struct result {
    enum type {                 /* Result source. */
        NONE, 
        VOLTAGE, 
        CURRENT, 
        IRRADIANCE, 
        TEMPERATURE, 
        RESERVED1, 
        RESERVED2,
        RESERVED3
    } sensorType;
    uint32_t sensorId;          /* Sample ID of result. */
    float value;                /* Value of the result. */
};

/** Function definitions. */
int mainA(void);

/** Indicator Management. */
static void cycleLed(DigitalOut *dout, uint8_t numCycles, std::chrono::milliseconds delayMs);

/** Sampling sensor data. */
static void sampleTempSensor(void);
static void sampleIrradianceSensor(void);
static void setReadyIrrad(void);
static void setReadyTemp(void);
static void performTest(void);

/** Communication Input Processing. */
static void pollCan(void);

/** Processing. */
static void processIrradianceResult(uint16_t msgId, struct result res);
static void processTemperatureResult(uint16_t msgId, struct result res);
static void processError(uint16_t msgId, uint16_t errorCode, uint16_t errorContext);
