// 2019 - Sebastian Żyłowski
// These are controller routines


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
{ int last_sensor;

  switch (relayMode)
  {
    case RELAY_IGNITED:
      digitalWrite(RELAY, HIGH);
      if ( sensor < pumpOffThreshold )
      {  // water level below threshold, turn off pump and wait grace time
         NextRelayMode(RELAY_IGNITED_OFF);
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
    case RELAY_IGNITED_OFF:
      if(last_sensor<sensor)  //if water level is going up, turn the pump off
      {
        NextRelayMode(RELAY_GRACEOFF);
      } else
      {
        RelayIncreaseTimer(RELAY_TIME_TO_OFF, RELAY_GRACEOFF);
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
      } else
      {
        if ( sensor > pumpOnThreshold )
        {
          NextRelayMode(RELAY_IGNITED);
          relayOns++;
        } else relayCounter = 0;
      }
  }
  last_sensor = sensor;
}
