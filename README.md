# Blackbody

This repository contains the source code for driving the PCBs defined in
[Array-Blackbody-A-PCB
repository](https://github.com/lhr-solar/Array-Blackbody-A-PCB) and
[Array-Blackbody-B-PCB](https://github.com/lhr-solar/Array-Blackbody-B-PCB). It
is meant to be used with the PV test platform, which consists of the
[Array-CurveTracerPCB](https://github.com/lhr-solar/Array-CurveTracerPCB) 
and the Heliocentric visualizer application -currently a fork of
[DeSeCa](https://github.com/dimembermatt/DeviceSerialCapture), to be merged with
[ArraySimulation](https://github.com/lhr-solar/Array-Simulation).

---
## Requirements
To compile and run the code, you need a linked version of mbed OS 6.

Additional requirements include pulling at least release 0.1.0 of
[Mbed-Shared-Components](https://github.com/lhr-solar/Mbed-Shared-Components);
this release contains the communication message IDs and error IDs that are used
in the firmware.

---
## TODO
- Move Errors, ComIds to Mbed-Shared-Components.
- Add exception handling to bad sensor data (post-calibration).
- Migrate serial input processing, CAN input processing to class.
- Create common com input processing class that encompasses the above.
- Handle invalid input data as an exception, not an error.

---
## Operation

The Blackbody boards operates on a two thread scheme: 
- A primary thread doing initialization and *input processing*. This includes
  capturing messages from other devices and managing them.
- A secondary thread doing experiments. This thread is always active, but will
  only execute an experiment at a fixed rate and when enabled. Once the test is
  complete, it will post its results to CAN.

### Nominal operation loop

In a typical operation loop, the primary thread initializes the peripherals
(which include a I2C and SPI device) and spawns a second thread. The primary
thread then continues to poll for CAN messages across the CAN network, looking
for an enable/disable command. This command determines the operation of the
secondary thread.

The secondary thread executes a set of experiments at a fixed rate, only when
the device is enabled. It interrogates the peripheral devices and upon receiving
a response, sends them out immediately via CAN.

### Error handling

The firmware also has a message encoding and error handling scheme. Software
exceptions and critical errors are either handled or cause a software halt,
spinning in an error loop in one or several threads and turning on the ERROR
LED if available. Prior to the error loop, an error message is sent outbound to
the relevant parties containing a predefined error code indicating what caused
the issue.

---
## Communication

Communication between the Blackbody boards and other parties are defined in the
following sections. Typically, messages with IDs that are not explicitly defined
for each context will be treated as software exceptions and reported to the user.

## CAN communication protocol.

The CAN communication protocol concerns the transmission of messages from the
Blackbody boards. All messages are input sans the Blackbody Enable/Disable
command; the user, through the PV Curve Tracer can disable external sensor input
messages through this command.

| Name                                      | Direction | ID    | Data Width [L:S]   | Data Type                    | Frequency |
|-------------------------------------------|-----------|-------|--------------------|------------------------------|-----------|
| Blackbody Temperature Sensors Measurement | O         | 0x620 | [39 : 32] [31 : 0] | RTD ID; C, signed float*1000 | 2 Hz      |
| Blackbody Irradiance Sensor 1 Measurement | O         | 0x630 | [31 : 0]           | W/m^2, signed float*1000     | 10 Hz     |
| Blackbody Irradiance Sensor 2 Measurement | O         | 0x631 | [31 : 0]           | W/m^2, signed float*1000     | 10 Hz     |
| Blackbody Enable/Disable                  | I         | 0x632 | [0 : 0]            | 1: Halt, 0: Restart          | Async     |
| Blackbody Board Fault                     | O         | 0x633 | [15 : 8][7 : 0]    | Error ID, error context      | Async     |

### Serial alternative communication protocol.
In the event of a CAN failure or testing, the following serial communication
protocol, separate to the serial communication protocol with PC, can be used.

Using the same table as above:
- 1 byte 0xFF prelude.
- 2 bytes of ID.
- x bytes according to the data width.

---
## Serial communication protocol with PC

This protocol is purely for debug communication with a user over USB USART. Two
message types are currently supported, and they follow the following format:

- 1 byte 0xFF prelude.
- 1.5 bytes of the CAN ID or MSG ID.
- x.y bytes of resulting data fields.


### Blackbody result.
Blackbody to PC.
```js
Bitmap                      | Contents                          | Data Width
[55:48] - byte 6            | 0xFF                              | 0xFF
[47:40] - byte 5            | CAN ID/MSG ID                     | 0xFFF
[39:36] - byte 4, nibble 2  | CAN ID/MSG ID                     |
[35:32] - byte 4, nibble 1  | Measurement Type                  | 0xF
[31:24] - byte 3            | Sample ID                         | 0xFFF
[23:20] - byte 2, nibble 2  | Sample ID                         |
[19:16] - byte 2, nibble 1  | Value (xxxx.yyy * 1000)           | 0xFFFFF
[15:8]  - byte 1            | Value                             |
[7:0]   - byte 0            | Value                             |
```

### Blackbody exception.
Blackbody to PC.
```js
Bitmap                      | Contents                          | Data Width
[47:40] - byte 5            | 0xFF                              | 0xFF
[39:32] - byte 4            | CAN ID/MSG ID                     | 0xFFF
[31:28] - byte 3, nibble 2  | CAN ID/MSG ID                     | 
[27:24] - byte 3, nibble 1  | Error Code                        | 0xFFF
[23:16] - byte 2            | Error Code                        |
[15:8]  - byte 1            | Error Context                     | 0xFFFF
[7:0]   - byte 0            | Error Context                     |
```
