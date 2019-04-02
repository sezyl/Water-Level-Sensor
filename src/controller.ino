// 2019 - Sebastian Żyłowski
// This program reads pressure sensor voltage, average it and output over the uart

#include "config.h"


uint8_t relayMode = RELAY_IDLE;
uint8_t pumpFailureCounter = 0;

uint16_t relayCounter = 0;
unsigned long relayOns = 0;
unsigned long relayOnTime = 0;
unsigned long relayOnStart = 0;
unsigned long inactiveTime = 0;
uint16_t pumpOffThreshold = RELAY_OFF_THRESHOLD;
uint16_t pumpOnThreshold = RELAY_ON_THRESHOLD;

// the setup routine runs once when you press reset:
void ControllerSetup() 
{
  // initialize relay pin.
  pinMode(RELAY, OUTPUT); // RELAY
  digitalWrite(RELAY, LOW);
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
void ControllerLoop(int sensor)
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

