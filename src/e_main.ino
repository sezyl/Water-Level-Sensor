// 2019 - Sebastian Ĺ»yĹ‚owski
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
#ifdef STORAGE
#endif
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

// the loop routine runs over and over again forever:
void loop() 
{
  int sensorValue, temperature, sensorPercentage;
  unsigned long time;

  time = millis() % 1000;
  delay(1000 - time); // always synchronise to full second

  digitalWrite(LED, LOW);   // turn the LED off
  delay(20);          // let voltage regulator to stabilize and reduce noise
  sensorValue = ReadSensorAdc();
  temperature = ReadTemperatureAdc();


  digitalWrite(LED, HIGH);  // turn the LED on
  time = millis();
  
  if(relayMode != RELAY_IDLE)
    BlinkLed(1+relayMode);     // blink current mode

  if(sensorValue > pumpOffThreshold)
  {
     sensorPercentage = sensorValue - pumpOffThreshold;
  } else
  {
    sensorPercentage = 0;
  }
  // calculate current water level in percentage of relay on level - it could be > 100%
  sensorPercentage = (sensorPercentage*100)/(pumpOnThreshold-pumpOffThreshold);
  
  ControllerLoop(sensorValue);
  snprintf(buf, UART_BUFFER_SIZE, "10 %d %d %d %d", sensorValue, temperature, sensorPercentage, relayMode);

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

  snprintf(buf, UART_BUFFER_SIZE, "20 [RELAY] Status: %d, Counter: %d, #ON: %ld", relayMode, relayCounter, relayOns);
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
