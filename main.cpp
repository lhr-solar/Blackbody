#include "mbed.h"
#include <chrono> 


// main() runs in its own thread in the OS

// CAN Rx is pin D10, CAN Tx is pin D2
#define CAN_Rx D10
#define CAN_Tx D2

std::chrono::milliseconds csec(100);
Ticker ticker;
// Initialize CAN driver and UART driver. CAN driver communicates with IV curve tracer, UART communicates with computer over USB
CAN can1(CAN_Rx, CAN_Tx);


// L432KC pin reference https://os.mbed.com/media/uploads/bcostm/nucleo_l432kc_2017_10_09.png

/* CAN IDs

Irradiance Sensor 1 Measurement	Irradiance/RTD output	x630	"Data:
[31:0] : Irradiance measurement (W/m^2, signed float)
Frequency: 10Hz"

Irradiance/RTD Board Enable/Disable command	Irradiance/RTD input	x632	"Data: [0:0]
1 - Halt the board measurement and output and put into idle/low power state.
0 - Restart the board measurement and output.
Frequency: On demand in specific occasions."

Irradiance/RTD Board Fault	Irradiance/RTD output	x633	"Data: [7:0]: Error ID
Frequency: On Demand"
*/


/*This measurement method is supposed to capture integration result from TSL2591 sensor, 
convert it to W/m^2, and send it across USB UART and CAN
*/
void measure()
{
    static double measurement;
    //Step 1: measure TSL2591 device over I2C
    //int write()
    //Step 2: linearize output, convert to W/m^2
    //Step 3: send across CAN and UART
    unsigned char measurement_normalized = measurement;
    if(can1.write(CANMessage(0x630, &measurement_normalized, 8))) 
    {
        printf("Measurement (W/m^2): %f\n", measurement);
    }
}

void sendError(const unsigned char *error)
{
    if(can1.write(CANMessage(0x633, error, 8))) 
    {
        printf("Program error  %s\n", error);
        
    }
    //Go into low power mode (stop transmitting, stop measuring)
    //To stop measuring, stop ticker
    ticker.detach();
    //Turn back on when curve tracer gives signal to restart measurment/outputt (ID 0x632, data 0)
}

int main()
{
    ticker.attach(&measure, std::chrono::microseconds(csec));

    CANMessage msg;
    while (true) 
    {
        if(can1.read(msg)) 
        {
            printf("Message received: %d\n\n", msg.data[0]); 
            //Halt or restart based on message ID 0x632 (implement later)
            //If CAN message and ID match restart, reattach ticker
            ticker.attach(&measure, std::chrono::microseconds(csec));
        }
    }
}

