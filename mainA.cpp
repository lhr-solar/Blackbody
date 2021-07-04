/**
 * Project: Blackbody A Board
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

/** LEDs. */
DigitalOut ledCanTxA(CAN_TX);
DigitalOut ledCanRxA(CAN_RX);
DigitalOut ledTracking(A4);
DigitalOut ledError(A5);

/** Comm. */
static BufferedSerial serialPort(USB_TX, USB_RX); /* USB debugging. */
static CAN canPort(CAN_RX, CAN_TX);

/** Globals. */
static bool sampleEnable;
static bool isReadyIrrad, isReadyTemp;

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

    /* Start thread for sampling and processing sensor data. */
    isReadyIrrad = false;
    isReadyTemp = false;
    LowPowerTicker tickerIrrad;
    tickerIrrad.attach(
        callback(&setReadyIrrad), 
        chrono::milliseconds(1000 / SAMPLE_FREQ_HZ_IRRAD)
    );
    LowPowerTicker tickerTemp;
    tickerTemp.attach(
        callback(&setReadyTemp), 
        chrono::milliseconds(1000 / SAMPLE_FREQ_HZ_TEMP)
    );
    Thread threadTesting;
    threadTesting.start(performTest);

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

/** Sampling sensor data. */
static void sampleTempSensor(void) {
    /* For each temperature sensor active. */
    for (uint8_t i = 0; i < MAX_TEMP_SENSORS; ++i) {
        struct result res = {
            .sensorType = result::TEMPERATURE,
            .sensorId = i, /* TSL2591 only has one I2C address. */
            .value = 0.0
        };

        /* TODO: Sample MAX31865. */

        processTemperatureResult(BLKBDY_TEMP_MEAS, res);
    }
}
static void sampleIrradianceSensor(void) {
    struct result res = {
        .sensorType = result::IRRADIANCE,
        .sensorId = 0x29, /* TSL2591 only has one I2C address. */
        .value = 0.0
    };

    /* TODO: Sample TSL2591. */

    processIrradianceResult(BLKBDY_IRRAD_1_MEAS, res);
}
static void setReadyIrrad(void) { isReadyIrrad = true; }
static void setReadyTemp(void) { isReadyTemp = true; }
static void performTest(void) {
    while (true) {
        if (sampleEnable) {
            if (isReadyIrrad) {
                sampleIrradianceSensor();
                isReadyIrrad = false;
            }
            if (isReadyTemp) {
                sampleTempSensor();
                isReadyTemp = false;
            }
        }
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
                }
                else if (msg.data[0] == 0) { sampleEnable = true; }
                else if (msg.data[0] == 1) { sampleEnable = false; }
                else {
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
static void processIrradianceResult(uint16_t msgId, struct result res) {
    uint32_t value = res.value * 1000;
    static uint32_t sampleId = 0;

    /* CAN. */
    char data[4];
    memcpy(data, &value, 4);
    canPort.write(CANMessage(msgId, data, 4));

    /* Debugging. */
    printf(
        "%02x%03x%01x%03x%05x",
        PRELUDE,
        msgId,
        result::IRRADIANCE,
        sampleId,
        (uint32_t)(res.value * 1000)
    );
    ++sampleId;
}
static void processTemperatureResult(uint16_t msgId, struct result res) {
    uint32_t value = res.value * 1000;
    static uint32_t sampleId = 0;

    /* CAN. */
    uint64_t valueMerged = ((res.sensorId & 0xFF) << 31) | value;
    char data[5];
    memcpy(data, &valueMerged, 5);
    canPort.write(CANMessage(msgId, data, 5));

    /* Debugging. */
    printf(
        "%02x%03x%01x%03x%05x",
        PRELUDE,
        msgId,
        result::TEMPERATURE,
        sampleId,
        (uint32_t)(res.value * 1000)
    );
    ++sampleId;
}
static void processError(uint16_t msgId, uint16_t errorCode, uint16_t errorContext) {
    uint16_t value = (errorCode & 0xFF) << 8 | (errorContext & 0xFF);

    /* CAN. */
    char data[2];
    memcpy(data, &value, 2);
    canPort.write(CANMessage(msgId, data, 2));

    /* Debugging. */
    printf(
        "%02x%03x%03x%04x", 
        PRELUDE,
        msgId,
        errorCode,
        errorContext
    );
}
