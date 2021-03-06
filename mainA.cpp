/**
 * Project: Blackbody A
 * File: mainA.cpp
 * Author: Matthew Yu (2021).
 * Organization: UT Solar Vehicles Team
 * Created on: 05/29/21
 * Last Modified: 07/03/21
 * File Description: Implementation file for Blackbody boards type A.
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
#include "mainA.hpp"
#include <chrono>

/** Data structures. */
static EventQueue queue(QUEUE_SIZE * EVENTS_EVENT_SIZE);

/** LEDs. */
DigitalOut ledCanTxA(CAN_TX);
DigitalOut ledCanRxA(CAN_RX);
DigitalOut ledTracking(A4);
DigitalOut ledError(A5);

/** Comm. */
static BufferedSerial serialPort(USB_TX, USB_RX); /* USB debugging. */
static CAN canPort(CAN_RX, CAN_TX);

/** Sensor. */
static I2C i2c(I2C_SDA, I2C_SCL);
static IrradI2cSensor tsl2591(&i2c, &queue, &processIrradianceResult);
#define SELECT SPI_CS0
static SPI spi(SPI_SDO, SPI_SDI, SPI_CLK, SELECT);
static TempSpiSensor max31865(&spi, &queue, &processTemperatureResult);

/** Main routine. */
int mainA() {
    /* Setup serial comm. */
    serialPort.set_baud(9600);
    serialPort.set_format(
        8,                      /* bits */ 
        BufferedSerial::None,   /* parity */ 
        1                       /* stop bit */ 
    );

    /* Cycle LEDs. */
    cycleLed(&ledCanTxA, 4, std::chrono::milliseconds(100));
    cycleLed(&ledCanRxA, 4, std::chrono::milliseconds(100));
    cycleLed(&ledTracking, 4, std::chrono::milliseconds(100));
    cycleLed(&ledError, 4, std::chrono::milliseconds(100));

    /* Start execution of the TSL2591 sensor. */
    tsl2591.start(chrono::milliseconds(1000 / SAMPLE_FREQ_HZ_IRRAD));

    /* Start execution of the MAX31865 sensor. */
    max31865.start(chrono::milliseconds(1000 / SAMPLE_FREQ_HZ_TEMP));

    /* Start threads for output message processing. */
    Thread threadProcessing;
    threadProcessing.start(callback(&queue, &EventQueue::dispatch_forever));
    
    while (true) {
        pollCan();
        ThisThread::sleep_for(chrono::milliseconds(100));
    }
}

/** Indicator Management. */
static void cycleLed(DigitalOut *dout, uint8_t numCycles, std::chrono::milliseconds delayMs) {
    for (uint8_t i = 0; i < numCycles; ++i) {
        *dout = 1;
        ThisThread::sleep_for(delayMs);
        *dout = 0;
        ThisThread::sleep_for(delayMs);
    }
}

/** Communication Input Processing. */
static void pollCan(void) {
    static CANMessage msg;
    if (canPort.read(msg)) {
        /* Handle data based on message ID. */
        uint16_t msgId = msg.id;
        switch (msgId) {
            case BLKBDY_EN_DIS:
                /* DATA should be 1 byte wide and is either 1:HALT or 0:RESTART. 
                   Everything else is an error. */
                if (msg.len != 1) { 
                    processError(BLKBDY_FAULT, ERR_INVALID_MSG_DATA_LEN, msg.len);
                } else if (msg.data[0] == 0) { 
                    tsl2591.start(chrono::milliseconds(1000 / SAMPLE_FREQ_HZ_IRRAD));
                    max31865.start(chrono::milliseconds(1000 / SAMPLE_FREQ_HZ_TEMP));
                } else if (msg.data[0] == 1) {
                    tsl2591.stop();
                    max31865.stop();
                } else {
                    processError(BLKBDY_FAULT, ERR_INVALID_MSG_DATA, msg.data[0]);
                }
                break;

            /* These may be received from another board on the CAN network,
               although this is unlikely. Ignore. */
            case BLKBDY_TEMP_MEAS:
            case BLKBDY_IRRAD_1_MEAS:
            case BLKBDY_IRRAD_2_MEAS:
            case BLKBDY_FAULT:
            default:
                break;
        }
    }
}

/** Processing. */
static void processIrradianceResult(float data) {
    uint64_t value = data * 1000;

    /* CAN. */
    char dataPack[5];
    memcpy(dataPack, &value, 5);
    canPort.write(CANMessage(BLKBDY_IRRAD_1_MEAS, dataPack, 5));

    /* Debugging. */
    printf(
        "%02x%04x%10llx",
        PRELUDE,
        BLKBDY_IRRAD_1_MEAS,
        value
    );
}
static void processTemperatureResult(float data) {
    uint32_t value = data * 1000;

    /* CAN. */
    char dataPack[4];
    memcpy(dataPack, &value, 4);
    canPort.write(CANMessage(BLKBDY_TEMP_MEAS, dataPack, 4));

    /* Debugging. */
    printf(
        "%02x%04x%08x",
        PRELUDE,
        BLKBDY_TEMP_MEAS,
        value
    );
}
static void processError(uint16_t msgId, uint16_t errorCode, uint16_t errorContext) {
    uint16_t value = (errorCode & 0xFF) << 8 | (errorContext & 0xFF);

    /* CAN. */
    char data[2];
    memcpy(data, &value, 2);
    canPort.write(CANMessage(msgId, data, 2));

    /* Debugging. */
    printf(
        "%02x%04x%04x", 
        PRELUDE,
        msgId,
        value
    );
}
