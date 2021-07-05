/**
 * Project: Blackbody
 * File: TempSpiSensor.hpp
 * Author: Matthew Yu (2021).
 * Organization: UT Solar Vehicles Team
 * Created on: 07/04/21
 * Last Modified: 07/04/21
 * File Description: Header file that implements the MAX31865 
 * temperature sensor.
 */

/** Includes. */
#include <Sensor/Sensor.hpp>

class TempSpiSensor : public Sensor {
    private:
        SPI * spi;

    public:
        TempSpiSensor(SPI *sensor, EventQueue *queue = NULL, void (*processFnc)(float data) = NULL) : Sensor(queue, processFnc) {
            spi = sensor;
        }
    
    private:
        virtual void _sampleData(void) {
            float temperature = 0;

            /* TODO: calibration here. */
            
            data = temperature;
        }
};
