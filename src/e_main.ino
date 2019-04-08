// 2019 - Sebastian Żyłowski
// This program reads pressure sensor voltage, average it and output over the uart

// PB0 - MOSI, P0 - RELAY
// PB1 - MISO, P1 - LED
// PB2 - ADC1, P2 - SENSOR
// PB3 - ADC3, P3 - USB- / RX
// PB4 - ADC2, P4 - USB+ / TX
// PB5 - NRES, ADC0, P5

#include <SoftSerial.h>     /* Allows Pin Change Interrupt Vector Sharing */
#include <TinyPinChange.h>  /*  */

void(* resetFunction) (void) = 0;

SoftSerial mySerial(RX_PIN, TX_PIN); // RX, TX

char buf[UART_BUFFER_SIZE] = { 0 };

// the setup routine runs once when you press reset:
void setup() 
{
  int i;

  // initialize the digital pin as an output.
  pinMode(LED, OUTPUT); //LED
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT_PULLUP);
  digitalWrite(LED, LOW);

  SensorSetup();
#ifdef CALIBRATION
  CalibrationSetup();
#endif
  ControllerSetup();
  
  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  //mySerial.println("00 BOOT");
}

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

int CalculateSensorPercentage(int sensor)
{
  int percentage;
  
  // calculate current water level in percentage of threshold on level - it could be > 100%
  if(sensor > pumpOffThreshold)
  {
     percentage = sensor - pumpOffThreshold;
  }
  percentage = (percentage*10)/( (pumpOnThreshold-pumpOffThreshold)/10);

  return(percentage);
}

// this routine assumes that we starter it just after passing 0ms (must be syncronised to full second)
void IndicateStatus(int sensorPercentage, uint8_t mode)
{ unsigned long time;

  if( mode == RELAY_IDLE )
  { // if we are in idle then wait adequate time and turn off the LED
    time = millis() - time;  // calculate time that already pass
    time %= 1000;

    if(sensorPercentage > 100) // in case level is higher than 100% limit it
    {
      sensorPercentage = 100;
    }      
    if(time<(sensorPercentage*9)) // do we still have time till LED should be off?
    {
      delay( (sensorPercentage*9) - time );
    }
  } else
  {
      BlinkLed(1+mode);     // blink current mode
  }
  
  digitalWrite(LED, LOW);   // turn the LED off  
}

// the loop routine runs over and over again forever:
void loop() 
{
  int sensorValue, temperature, sensorPercentage;
  unsigned long time;

  time = millis() % 1000;
  delay(1000 - time); // always synchronise to full second

  sensorValue = ReadSensorAdc();
  temperature = ReadTemperatureAdc();

  digitalWrite(LED, HIGH);  // turn the LED on
  
  ControllerLoop(sensorValue);
  sensorPercentage = CalculateSensorPercentage(sensorValue);

  snprintf(buf, UART_BUFFER_SIZE, "10 %d %d %d %d", sensorValue, temperature, sensorPercentage, relayMode);
  mySerial.println(buf); // this is time consuming process

  IndicateStatus(sensorPercentage, relayMode);
}
