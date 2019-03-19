// PB0 - MOSI, P0 - RELAY
// PB1 - MISO, P1 - LED
// PB2 - ADC1, P2 - SENSOR
// PB3 - ADC3, P3 - USB- / RX
// PB4 - ADC2, P4 - USB+ / TX
// PB5 - NRES, ADC0, P5

// 2019 - Sebastian Żyłowski
// This program reads pressure sensor voltage, average it and output over the uart

/**
 * Sensor calibration:
 * 
 * 1. Turn off device
 * 2. Sink sensor to pump off level (usually very close to complete unsink)
 * 3. Keep PB3 low and power on device
 * 4. Keep PB3 low until LED stop blinking and turns off
 * 5. Sink sensor to pump on level
 * 6. Pull PB3 low shortly
 * 7. Calibration is done and device start working in normal mode
 **/
 
#include <SoftSerial.h>     /* Allows Pin Change Interrupt Vector Sharing */
#include <TinyPinChange.h>  /*  */
#include <avr/eeprom.h>

#define notCONSOLE 
#define STORAGE

// PINS definition
#define RELAY 0
#define LED 1
#define ADC_PIN 2
#define RX 3
#define TX 4
#define ADC_IN A1
#define TEMP_SENSOR (A3+1)

// sensor data average window, for 10-bit adc it can't be higher than 32!
#define AVERAGE_WINDOW 10

// uart buffers
#define UART_BUFFER_SIZE 40
#define UARTRX_BUFFER_SIZE 20

#define RELAY_IDLE      0
#define RELAY_IGNITED   1
#define RELAY_GRACEOFF  2
#define RELAY_OXYGENATE 3
#define RELAY_PUMP_FAILURE 4

#define RELAY_TIME_TO_ON  2
#define RELAY_TIME_TO_OFF 10
#define RELAY_TIME_GRACE 60
#define RELAY_TIME_ON_MAX (5*30)
#define RELAY_TIME_ON_FAILURE (60*60)
#define RELAY_TIME_INACTIVE_MAX (24*60*60)
#define RELAY_TIME_OXYGENATE 60

#define RELAY_OVF_MAX   5
#define RELAY_ON_THRESHOLD 700
#define RELAY_OFF_THRESHOLD 400

#define EEPROM_OFF_THRESHOLD_ADDR ((uint16_t *)2)
#define EEPROM_ON_THRESHOLD_ADDR ((uint16_t *)4)
#define EEPROM_CRC_ADDR ((uint16_t *)6)
#define EEPROM_THRESHOLD_MAGIC 0x7132

void(* resetFunction) (void) = 0;

SoftSerial mySerial(RX, TX); // RX, TX

int samples[AVERAGE_WINDOW] = { 0 };
uint8_t samples_in_buffer = 0;
uint8_t samples_id = 0;

uint8_t relayMode = RELAY_IDLE;
uint8_t pumpFailureCounter = 0;

uint16_t relayCounter = 0;
unsigned long relayOns = 0;
unsigned long relayOnTime = 0;
unsigned long relayOnStart = 0;
unsigned long inactiveTime = 0;
uint16_t pumpOffThreshold = RELAY_OFF_THRESHOLD;
uint16_t pumpOnThreshold = RELAY_ON_THRESHOLD;

#ifdef CONSOLE
char uartrx_buffer[UARTRX_BUFFER_SIZE];
uint8_t uartrx_id = 0;
#endif

char buf[UART_BUFFER_SIZE] = { 0 };

// the setup routine runs once when you press reset:
void setup() 
{
  int i;

  // initialize the digital pin as an output.
  pinMode(RELAY, OUTPUT); // RELAY
  pinMode(LED, OUTPUT); //LED
  pinMode(ADC_PIN, INPUT); // ADC input
  pinMode(TX, OUTPUT);
  pinMode(RX, INPUT_PULLUP);
  digitalWrite(LED, LOW);
  digitalWrite(RELAY, LOW);

#if 0
  // clearing of global variables is not really needed
  for (i = 0; i < AVERAGE_WINDOW; i++)
  {
    samples[i] = 0;
  }
  samples_in_buffer = 0;
  samples_id = 0;
#endif

  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  //mySerial.println("00 BOOT");
#ifdef STORAGE
  if(!ReadThresholdFromEeprom())
  {
    pumpOffThreshold = RELAY_OFF_THRESHOLD;
    pumpOnThreshold = RELAY_ON_THRESHOLD;
    WriteThresholdToEeprom();
  }
#endif
}

#ifdef STORAGE
/* store thresholds with magic word */
void WriteThresholdToEeprom(void)
{
  eeprom_write_word(EEPROM_OFF_THRESHOLD_ADDR, pumpOffThreshold);
  eeprom_write_word(EEPROM_ON_THRESHOLD_ADDR, pumpOnThreshold);
  eeprom_write_word(EEPROM_CRC_ADDR, pumpOffThreshold^pumpOnThreshold^EEPROM_THRESHOLD_MAGIC);
}

/* read and validate thresholds */
bool ReadThresholdFromEeprom(void)
{
  uint16_t data;
  bool result = false;
  
  pumpOffThreshold = eeprom_read_word(EEPROM_OFF_THRESHOLD_ADDR);
  pumpOnThreshold = eeprom_read_word(EEPROM_ON_THRESHOLD_ADDR);
  data = eeprom_read_word(EEPROM_CRC_ADDR);
  data ^= pumpOnThreshold ^ pumpOffThreshold;
  if(pumpOnThreshold > pumpOffThreshold) /* are values logical? */
  {
    result = ( EEPROM_THRESHOLD_MAGIC == data ); /* check magic word */
  }
  return(result);
}
#endif

void BlinkLed(int blinks)
{
  while(blinks)
  {
    digitalWrite(LED, HIGH);  // turn the LED on
    delay(80);          // let voltage regulator to stabilize and reduce noise
    digitalWrite(LED, LOW);   // turn the LED off   
    delay(80);
    blinks--; 
  }
}

// the loop routine runs over and over again forever:
void loop() 
{
  int sensorValue, sensorAverage, temperature, sensorPercentage;
  unsigned long time;

  time = millis() % 1000;
  delay(1000 - time); // always synchronise to full second

  digitalWrite(LED, LOW);   // turn the LED off
  delay(20);          // let voltage regulator to stabilize and reduce noise
  sensorValue = ReadSensorAdc();
  sensorAverage = AverageAdc(sensorValue);
  temperature = ReadTemperatureAdc();


  digitalWrite(LED, HIGH);  // turn the LED on
  time = millis();
  
  if(relayMode != RELAY_IDLE)
    BlinkLed(1+relayMode);     // blink current mode

  if(sensorAverage > pumpOffThreshold)
  {
     sensorPercentage = sensorAverage - pumpOffThreshold;
  } else
  {
    sensorPercentage = 0;
  }
  // calculate current water level in percentage of relay on level - it could be > 100%
  sensorPercentage = (sensorPercentage*100)/(pumpOnThreshold-pumpOffThreshold);
  
  ControlRelay(sensorAverage);
  snprintf(buf, UART_BUFFER_SIZE, "10 %d %d %d", sensorAverage, sensorValue, temperature);

  if( (sensorPercentage<50) && (relayMode==0) )
  {  // if water level < 50% and we are in idle, then wait adequate time and turn off the LED
     delay(sensorPercentage*9);
     digitalWrite(LED, LOW);   // turn the LED off
  }
  
  mySerial.println(buf); // this is time consuming process

  if( (sensorPercentage >= 50) && (relayMode==0) )
  { // if water level >= 50% and we are in idle then wait adequate time and turn off the LED
    time = millis() - time;  // calculate time that already pass
    time %= 1000;
    if(time<(sensorPercentage*9)) // do we still have time till LED should be off?
    {
      delay( (sensorPercentage*9) - time );
    }
     digitalWrite(LED, LOW);   // turn the LED off
  }

#ifdef CONSOLE
  if ( checkSerial() )
  {
    analyzeCommand();
  }
#endif

}

/* RELAY control functions */

void NextRelayMode(uint8_t new_mode)
{
  relayMode = new_mode;
  relayCounter = 0;
  inactiveTime = 0;
}

// increase timer to defined level and then switch to new mode
bool RelayIncreaseTimer(int time_top, uint8_t new_mode)
{
  bool result = false;

  if ( relayCounter < time_top )
  {
    relayCounter++;
  } else
  {
    NextRelayMode(new_mode);
    result = true;
  }

  return (result);
}

// function should be call every 1 second
// It controls relay accordingly to sensor readout and current state.
void ControlRelay(int sensor)
{
  switch (relayMode)
  {
    case RELAY_IGNITED:
      digitalWrite(RELAY, HIGH);
      if ( sensor < pumpOffThreshold )
      {  // water level below threshold, turn off pump and wait grace time
         NextRelayMode(RELAY_GRACEOFF);
         relayOnTime = (millis() - relayOnStart) / 1000;
         pumpFailureCounter = 0;
      } else 
      {
        if( RelayIncreaseTimer(RELAY_TIME_ON_MAX, RELAY_GRACEOFF) )
        { // pump is turned on for too long - possible failure
          pumpFailureCounter++;
          if(pumpFailureCounter >= RELAY_OVF_MAX)
          {
            NextRelayMode(RELAY_PUMP_FAILURE);
          }
        }
      }
      break;
    case RELAY_GRACEOFF:
      digitalWrite(RELAY, LOW);
      RelayIncreaseTimer(RELAY_TIME_GRACE, RELAY_IDLE);
      break;
    case RELAY_OXYGENATE:
      digitalWrite(RELAY, HIGH);
      RelayIncreaseTimer(RELAY_TIME_OXYGENATE, RELAY_GRACEOFF);
      break;
    case RELAY_PUMP_FAILURE:
      digitalWrite(RELAY, LOW);
      RelayIncreaseTimer(RELAY_TIME_ON_FAILURE, RELAY_IDLE);
      break;
    default:         // this is RELAY_IDLE mode
      digitalWrite(RELAY, LOW);
      inactiveTime++;
      if(inactiveTime >= RELAY_TIME_INACTIVE_MAX)
      {
         NextRelayMode(RELAY_OXYGENATE);
         relayOns++;
         relayOnStart = millis();
      } else
      {
        if ( sensor > pumpOnThreshold )
        {
          NextRelayMode(RELAY_IGNITED);
          relayOns++;
          relayOnStart = millis();
        } else relayCounter = 0;
      }
  }
}

#ifdef CONSOLE
/* command line functions */

// function collects incomming serial data until buffer is full or EOL received.
// Returns true if there is full command in buffer
bool checkSerial(void)
{
  char data;
  bool result = false;

  while ( (mySerial.available()) && (result == false) )
  {
    data = mySerial.read();
    if (data == '\n')
    {
      result = true;
      data = 0;  // replace EOL with end of string
    }
    if (uartrx_id < UARTRX_BUFFER_SIZE)
    {
      uartrx_buffer[uartrx_id++] = data;
    }
  }

  return (result);
}

// present device status
bool statusFunction(void)
{ int sensor = ReadSensorAdc();

  snprintf(buf, UART_BUFFER_SIZE, "20 [RELAY] Status: %d, Counter: %d, #ON: %ld, ON time: %ld", relayMode, relayCounter, relayOns, relayOnTime);
  mySerial.println(buf);
  snprintf(buf, UART_BUFFER_SIZE, "21 [SENSOR] Value: %d, Average: %d, idx: %d, #: %d", sensor, AverageAdc(sensor), samples_id, samples_in_buffer);
  mySerial.println(buf);

  return (true);
}

// function checks if command in buffer equals given one.
// If it matches it returns index in input buffer just after the command.
// If not match returns 0
uint8_t checkCommand(char *command)
{
  uint8_t command_length = strlen(command);
  uint8_t result = 0;

  if ( strncmp(uartrx_buffer, command, UARTRX_BUFFER_SIZE) == 0 )
    result = command_length;

  return (result);
}

// function analyzes command in buffer
// it is expected that it is call once command is collected in buffer
void analyzeCommand(void)
{
  bool result = false;

  if ( checkCommand("reset") )
  {
    snprintf(buf, UART_BUFFER_SIZE, "99 REBOOT");
    mySerial.println(buf);
    resetFunction();
  }

  if ( checkCommand("status") )
    result = statusFunction();

  uartrx_id = 0;  // clear input buffer

  if (!result)
  {
    snprintf(buf, UART_BUFFER_SIZE, "80 Unknown Command!");
    mySerial.println(buf);
  }
}
#endif /* CONSOLE */

/* sensor functions */

// function reads value from ADC, puts in buffer, average the buffer and returns the average
int AverageAdc(int sample)
{
  int i, avg = 0;

  samples[samples_id++] = sample;
  if (samples_id >= AVERAGE_WINDOW) // prepare next cell index
    samples_id = 0;
  if (samples_in_buffer < AVERAGE_WINDOW) // if buffer not full, increase number of samples in it
    samples_in_buffer++;

  for (i = 0; i < samples_in_buffer; i++)
    avg += samples[i];

  return (avg / samples_in_buffer);
}

int ReadSensorAdc(void)
{
  int sensor;

  analogReference(DEFAULT);
  delay(1);
  sensor = analogRead(ADC_IN);

  return (sensor);
}

int ReadTemperatureAdc(void)
{
  int sample;

  analogReference(INTERNAL1V1);
  delay(1);
  sample = analogRead(TEMP_SENSOR);

  return (sample);
}
