/**
 * Sensor calibration:
 * 
 * 1. Turn off device
 * 2. Sink sensor to pump off level (or fully unsink)
 * 3. Keep PB3 low and power on device
 * 4. Release PB3 when LED starts blinking
 * 5. Sink sensor to pump on level
 * 6. Pull PB3 low shortly
 * 7. If calibration is successfull 3 quick LED blinks are visible
 * 8. If calibration fails 3 long LED blinks signal error
 * 9. Device starts working in normal mode
 **/

#ifdef CALIBRATION

#include <avr/eeprom.h>

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

void ShowLed(int high_time, int low_time)
{
  digitalWrite(LED, HIGH);   // turn the LED on
  delay(high_time);          // let voltage regulator to stabilize and reduce noise
  digitalWrite(LED, LOW);    // turn the LED off   
  delay(low_time); 
}

void Show3Led(int high_time, int low_time)
{
  ShowLed(high_time, low_time); 
  ShowLed(high_time, low_time); 
  ShowLed(high_time, low_time); 
}

void CalibrationSetup(void)
{
  if(!ReadThresholdFromEeprom())
  {
    pumpOffThreshold = RELAY_OFF_THRESHOLD;
    pumpOnThreshold = RELAY_ON_THRESHOLD;
    WriteThresholdToEeprom();
  }
  if(digitalRead(RX_PIN)==0)
  { int lowValue, highValue;
  
     while(digitalRead(RX_PIN)==0)
     {
       ShowLed(250, 250); // slow blinking as long as key is pressed
     }
     lowValue = ReadSensorAdc()+ OFF_THRESHOLD_SHIFT;
     while(digitalRead(RX_PIN))
     {
       ShowLed(100, 100); // fast blinking until key is pressed
     }
     highValue = ReadSensorAdc();
     if(lowValue + MIN_THRESHOLD_DISTANCE < highValue)
     {
       pumpOffThreshold = lowValue;
       pumpOnThreshold = highValue;
       WriteThresholdToEeprom();
       // signal correct calibration
       Show3Led(200, 100); 
     } else
     {
       // signal error
       Show3Led(800, 200);
     }
  }
}

#endif
