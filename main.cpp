/**
 * Project: Standalone Irradiance Board Firmware
 * File: main.cpp
 * Author: Matthew Yu (2021).
 * Organization: UT Solar Vehicles Team
 * Created on: 05/24/21
 * Last Modified: 06/12/21
 * File Description: This file describes the operation and execution of the
 * Standalone Irradiance Board for the UT LHR Solar Vehicles Team. 
 * L432KC Pinout:
 * https://os.mbed.com/media/uploads/bcostm/nucleo_l432kc_2017_10_09.png
 */

/** Library Imports. */
#include "mbed.h"
#include <chrono> 

/** Custom Imports. */
#include <src/Error/Errors.h>
#include <src/Sensor/Sensor.h>
#include <src/Message/Message.h>
#include <src/ComDevice/ComDevice.h>
#include <src/I2cSensor/IrradianceI2cSensor.h>

#define IRRAD_SENSOR_ID 0x630
#define CAN_Tx D2
#define CAN_Rx D10
#define USB_TX USBTX
#define USB_RX USBRX
#define SDA D0
#define SCL D1
#define HEARTBEAT_LED D3
#define PERIOD_HEARTBEAT_MS 500

/** Variable Declarations. */
bool captureFlag = false;
enum DeviceState {OFF, ON, ERR} deviceState = ON;

/** Heartbeat LED. */
DigitalOut ledHeartbeat(HEARTBEAT_LED);
LowPowerTicker tickHeartbeat;

/** Communication Device. */
#define COM_RATE_MS 300
// ComDevice transceiver(ComDevice::CAN, CAN_TX, CAN_RX);
ComDevice transceiver(ComDevice::SERIAL, USB_TX, USB_RX);

/** TSL2591 Sensor Device. */
#define SEN_RATE_MS 500
IrradianceI2cSensor irradSensor(SDA, SCL);
LowPowerTicker tickSendData;

/** Error Code. */
uint8_t errorCode = ERR_NONE;

/** Function Declaration and Implementation. */
void heartbeat(void) { ledHeartbeat = !ledHeartbeat; }

void raiseCaptureFlag(void) {
    captureFlag = true;
}

void raiseError(uint8_t errorCode) {
    Message message = Message(0x633, (uint64_t) errorCode);
    /* Send message over com. */
    transceiver.sendMessage(&message);
}

int main() {
    /* Initialize heartbeat to toggle at 0.5*3 s (OFF). */
    ledHeartbeat = 1;
    tickHeartbeat.attach(
        &heartbeat, 
        std::chrono::milliseconds(PERIOD_HEARTBEAT_MS*3));

    /* Accept only input message IDs defined in the README.md. */
    transceiver.addCanIdFilter(0x632);
    /* Initialize ComDevice to spit out data to serial. */
    transceiver.startMs(COM_RATE_MS);

    /* Reset variable is set when the device was recently in an OFF or ERROR
       state. */
    DeepSleepLock lock;
    bool reset = true;
    while (true) {
        /* Sidenote: if this way of writing programs seems familiar, it's because
           it is emulating Verilog. The first block updates the state, and the
           second block acts on the state. */

        /* Get latest message in com buffer. */
        Message message;
        if (transceiver.getMessage(&message)) {
            uint16_t msgId = message.getMessageID();
            switch (msgId) {
                case 0x632: { /* Blackbody Enable/Disable. */
                        uint64_t data = message.getMessageDataU();
                        if (data == 0) deviceState = ON;
                        else if (data == 1) deviceState = OFF;
                        else {
                            deviceState = ERR;
                            errorCode = ERR_INVALID_CAN_DATA;
                        }
                    }
                    break;
                default:
                    deviceState = ERR;
                    errorCode = ERR_INVALID_CAN_ID;
            }
        }

        switch (deviceState) {
            case ON:
                /* Nominal operation. */
                /* On reset. */
                if (reset) {
                    irradSensor.startMs(SEN_RATE_MS);
                    tickSendData.attach(
                        &raiseCaptureFlag, 
                        std::chrono::milliseconds(SEN_RATE_MS));
                    tickHeartbeat.attach(
                        &heartbeat, 
                        std::chrono::milliseconds(PERIOD_HEARTBEAT_MS));
                    reset = false;
                }

                /* On sensor capture. */
                if (captureFlag) {
                    /* Capture buffered sensor data. */
                    float data = 10.12;//irradSensor.getValue();
                    Message message = Message(IRRAD_SENSOR_ID, (int64_t) (data * 100));

                    /* Send message over com. */
                    transceiver.sendMessage(&message);
                    captureFlag = false;
                }
                break;
            case OFF:
                /* Device is sleeping until notified. */
                if (!reset) {
                    tickSendData.detach();
                    tickHeartbeat.attach(
                        &heartbeat, 
                        std::chrono::milliseconds(PERIOD_HEARTBEAT_MS*3));
                    reset = true;
                }
                break;
            default:
                errorCode = ERR_BAD_STATE;
                reset = false;
            case ERR:
                /* Error event. How this is handled depends on the board, but in
                   this case we raise an error and turn off the heartbeat. The
                   board can be restarted through com like the OFF state. */
                if (!reset) {
                    tickSendData.detach();
                    tickHeartbeat.detach();
                    ledHeartbeat = 0;
                    raiseError(errorCode);
                    reset = true;
                }
                break;
        }
    }
}
