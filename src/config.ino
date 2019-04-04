#ifndef _CONFIG_H_
#define _CONFIG_H_

// PB0 - MOSI, P0 - RELAY
// PB1 - MISO, P1 - LED
// PB2 - ADC1, P2 - SENSOR
// PB3 - ADC3, P3 - USB- / RX
// PB4 - ADC2, P4 - USB+ / TX
// PB5 - NRES, ADC0, P5

#define notCONSOLE 
#define CALIBRATION

// PINS definition
#define RELAY 0
#define LED 1
#define ADC_PIN 2
#define RX_PIN 3
#define TX_PIN 4
#define ADC_IN 1
#define TEMP_SENSOR 15

// sensor data average window, for 10-bit adc it can't be higher than 32!
#define AVERAGE_WINDOW 3

// uart buffers
#define UART_BUFFER_SIZE 40
#define UARTRX_BUFFER_SIZE 20

#define RELAY_IDLE         0
#define RELAY_IGNITED      1
#define RELAY_IGNITED_OFF  2
#define RELAY_GRACEOFF     3
#define RELAY_OXYGENATE    4
#define RELAY_PUMP_FAILURE 5

#define RELAY_TIME_TO_ON  2UL
#define RELAY_TIME_TO_OFF 10UL
#define RELAY_TIME_GRACE 120UL
#define RELAY_TIME_ON_MAX (10*60UL)
#define RELAY_TIME_ON_FAILURE (60*60UL)
#define RELAY_TIME_INACTIVE_MAX (24*60*60UL)
#define RELAY_TIME_OXYGENATE 60UL

#define RELAY_OVF_MAX   5
#define RELAY_ON_THRESHOLD 500
#define RELAY_OFF_THRESHOLD 60
#define OFF_THRESHOLD_SHIFT 10
#define MIN_THRESHOLD_DISTANCE 50

#define EEPROM_OFF_THRESHOLD_ADDR ((uint16_t *)2)
#define EEPROM_ON_THRESHOLD_ADDR ((uint16_t *)4)
#define EEPROM_CRC_ADDR ((uint16_t *)6)
#define EEPROM_THRESHOLD_MAGIC 0x7132

#endif
