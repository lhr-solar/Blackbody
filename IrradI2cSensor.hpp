/**
 * Project: Blackbody
 * File: IrradI2cSensor.hpp
 * Author: Matthew Yu (2021).
 * Organization: UT Solar Vehicles Team
 * Created on: 07/04/21
 * Last Modified: 07/04/21
 * File Description: Header file that implements the TSL2591 
 * irradiance sensor.
 */

/** Includes. */
#include <Sensor/Sensor.hpp>

class IrradI2cSensor : public Sensor {
    private:
        I2C * i2c;

    public:
        IrradI2cSensor(I2C *sensor, EventQueue *queue = NULL, void (*processFnc)(float data) = NULL) : Sensor(queue, processFnc) {
            i2c = sensor;
        }
    
    private:
        virtual void _sampleData(void) {
            float irradiance = 0;

            /* TODO: calibration here. */
            
            data = irradiance;
        }
};
