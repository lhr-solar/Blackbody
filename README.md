# StandaloneIrradianceBoard

This repository contains the source code for driving the [Blackbody Standalone Irradiance Sensor variant PCB](https://github.com/lhr-solar/Array-StandaloneIrradiancePCB). It is meant specifically for use with the [PV Curve Tracer](https://github.com/lhr-solar/Array-CurveTracerPCB). 

The data is piped through the PV Curve Tracer board and siphoned by the Heliocentric visualizer application -currently a fork of [DeSeCa](https://github.com/dimembermatt/DeviceSerialCapture), to be merged with [ArraySimulation](https://github.com/lhr-solar/Array-Simulation). The data communication protocol is described further below.

## Requirements
To compile and run the code, you need a linked version of mbed OS.

## Communication Protocol

| Name                                      | Direction | ID    | Data Width [L:S]   | Data Type                   | Frequency |
|-------------------------------------------|-----------|-------|--------------------|-----------------------------|-----------|
| Blackbody Temperature Sensors Measurement | Output    | 0x620 | [39 : 32] [31 : 0] | RTD ID; C, signed float*100 | 2 Hz      |
| Blackbody Irradiance Sensor 1 Measurement | Output    | 0x630 | [31 : 0]           | W/m^2, signed float*100     | 10 Hz     |
| Blackbody Irradiance Sensor 2 Measurement | Output    | 0x631 | [31 : 0]           | W/m^2, signed float*100     | 10 Hz     |
| Blackbody Enable/Disable                  | Input     | 0x632 | [0 : 0]            | 1: Halt, 0: Restart         | Async     |
| Blackbody Board Fault                     | Output    | 0x633 | [7 : 0]            | Error ID, enum              | Async     |
