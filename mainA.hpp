/**
 * Project: Blackbody A
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
#include <Misc/ComIds.hpp>
#include <Misc/Errors.hpp>
#include "IrradI2cSensor.hpp"
#include "TempSpiSensor.hpp"

/** Defines. */
#define USB_TX USBTX /* A7. */
#define USB_RX USBRX /* A2. */
#define CAN_TX D2
#define CAN_RX D10
#define I2C_SDA D4
#define I2C_SCL D5
#define SPI_SDI D11
#define SPI_SDO D12
#define SPI_CLK D13
#define SPI_CS0 A1
#define SPI_CS1 A0
#define SPI_CS2 D8
#define SPI_CS3 D9
#define SPI_CS4 A3
#define SPI_CS5 D6
#define SPI_CS6 A6
#define SPI_CS7 D3
#define PRELUDE 0xFF
#define SAMPLE_FREQ_HZ_IRRAD 10
#define SAMPLE_FREQ_HZ_TEMP 2
#define QUEUE_SIZE 100

/** Function definitions. */
/** Main routine. */
int mainA(void);

/** Indicator Management. */
static void cycleLed(DigitalOut *dout, uint8_t numCycles, std::chrono::milliseconds delayMs);

/** Communication Input Processing. */
static void pollCan(void);

/** Processing. */
static void processIrradianceResult(float data);
static void processTemperatureResult(float data);
static void processError(uint16_t msgId, uint16_t errorCode, uint16_t errorContext);
