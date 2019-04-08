#ifndef _CONFIG__
#define _CONFIG__

// PB0 - MOSI, P0 - RELAY
// PB1 - MISO, P1 - LED
// PB2 - ADC1, P2 - SENSOR
// PB3 - ADC3, P3 - USB- / RX
// PB4 - ADC2, P4 - USB+ / TX
// PB5 - NRES, ADC0, P5

#define CALIBRATION

// PINS definition
#define RELAY 0
#define LED 1
// if ADC_PIN is changed, modify ADC_IN below and ADC1D in SensorSetup()!!
#define ADC_PIN 2
#define RX_PIN 3
#define TX_PIN 4

// ADC MUX input where external sensor is connected, needs to be changed if you change ADC_PIN!!
#define ADC_IN 1

// ADC MUX where internal temperature sensor is located
#define TEMP_SENSOR 15

// uart buffers
#define UART_BUFFER_SIZE 40
#define UARTRX_BUFFER_SIZE 20

// state machine - usually don't need to change
#define RELAY_IDLE         0
#define RELAY_IGNITED      1
#define RELAY_IGNITED_OFF  2
#define RELAY_GRACEOFF     3
#define RELAY_OXYGENATE    4
#define RELAY_PUMP_FAILURE 5

// here are timings for specific states 
// everything here is defined in seconds
#define RELAY_TIME_TO_ON  2UL
#define RELAY_TIME_TO_OFF 10UL
#define RELAY_TIME_GRACE 120UL
#define RELAY_TIME_ON_MAX (10*60UL)
#define RELAY_TIME_ON_FAILURE (60*60UL)
#define RELAY_TIME_INACTIVE_MAX (24*60*60UL)
#define RELAY_TIME_OXYGENATE 60UL

// number of unsuccessful pumping out to declare pump failure
#define RELAY_FAILURES_MAX   5

// default pump on threshold - overwritten by calibration routine
#define RELAY_ON_THRESHOLD 500
// default pump off threshold - overwritten by calibration routine
#define RELAY_OFF_THRESHOLD 60
// off threshold shift
#define OFF_THRESHOLD_SHIFT 10
// minimum threshold on & off difference to declare successfull calibration
#define MIN_THRESHOLD_DISTANCE 50

#define EEPROM_OFF_THRESHOLD_ADDR ((uint16_t *)2)
#define EEPROM_ON_THRESHOLD_ADDR ((uint16_t *)4)
#define EEPROM_CRC_ADDR ((uint16_t *)6)
#define EEPROM_THRESHOLD_MAGIC 0x7132

#endif
