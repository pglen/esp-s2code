| Supported Targets | ESP32 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- |

# ADC test

### Hardware Required

* A development board with ESP32, ESP32-S and ESP32-C SoC
* A USB cable for power supply and programming

Please refer to the `channel` array defined in `adc_dma_example_main.c`. These GPIOs should be connected to voltage sources (0 ~ 3.3V). If other ADC unit(s) / channel(s) are selected in your application,
feel free to modify these definitions.

### Configure the project

```
idf.py menuconfig
```

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

For frequency analyzer test:

 Sample period:  98 micro sec

 1000 / 98 ~~  10 kHz

 Mental map for sample sizes:

  1 sample          10 khz
  2 samples         5 khz
  4 samples         2.5 khz
  8 samples         1.25 khz
 16 samples         600 hz
 32 samples         300 hz
 64 samples         150 khz
128 samples         75  hz


