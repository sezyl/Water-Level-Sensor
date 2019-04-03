#line 2 "test.ino"

//#include <Arduino.h>
#include <AUnit.h>
#include "./config.h"

// define it
#define INTERNAL1V1 0

// Tests

extern int samples[AVERAGE_WINDOW];
extern uint8_t samples_in_buffer;
extern uint8_t samples_id;

test(sensor, check_init) {
  uint16_t sensor_data[AVERAGE_WINDOW] = { 0 };

  assertFalse(memcmp(sensor_data, samples, AVERAGE_WINDOW));
  assertEqual(0, samples_in_buffer);
  assertEqual(0, samples_id);
}

int calculate_average(int last_sample, int samples)
{ int i = 0, j;

  if(samples>AVERAGE_WINDOW)
     samples = AVERAGE_WINDOW;

  j = samples;
  
  while(j>0)
  { i += last_sample;
    j--;
    last_sample--;
  }

  return(i/samples);
}

test(sensor, table_fill) {
  int sensor_avg, i;
  int exp_avg, sample;
 
  for(i=0;i<AVERAGE_WINDOW;i++)
  { 
    sample = AVERAGE_WINDOW+i;
    exp_avg = calculate_average(sample, i+1);
    // verify table index
    assertEqual( i, samples_id);

    sensor_avg = AverageAdc(AVERAGE_WINDOW + i);

    assertEqual(exp_avg, sensor_avg);
    // verify samples count
    assertEqual( (i+1), samples_in_buffer);
    // verify the sample landed in table as expected
    assertEqual(sample, samples[i]); 
  }
  
  // table index should go to 0 for circular buffer
  assertEqual( 0, samples_id );

  sensor_avg = AverageAdc(2 * AVERAGE_WINDOW);
  exp_avg = calculate_average(2 * AVERAGE_WINDOW, AVERAGE_WINDOW);
  assertEqual( sensor_avg, exp_avg );

  // number of samples in table should not increase
  assertEqual( AVERAGE_WINDOW, samples_in_buffer );
}

#if 1
extern uint8_t relayMode;
extern uint8_t pumpFailureCounter;
extern uint16_t relayCounter;
extern unsigned long relayOns;
extern unsigned long relayOnTime;
extern unsigned long relayOnStart;
extern unsigned long inactiveTime;
extern uint16_t pumpOffThreshold;
extern uint16_t pumpOnThreshold;
#endif

test(controller, check_init)
{
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, pumpFailureCounter);
  assertEqual(0, (int)relayCounter);
  assertEqual(0UL, relayOns);
  assertEqual(0UL, relayOnTime);
  assertEqual(0UL, relayOnStart);
  assertEqual(0UL, inactiveTime);
  assertEqual(RELAY_OFF_THRESHOLD, (int)pumpOffThreshold);
  assertEqual(RELAY_ON_THRESHOLD, (int)pumpOnThreshold);
}

test(controller, next_relay_mode)
{
  NextRelayMode(RELAY_PUMP_FAILURE);
  assertEqual(RELAY_PUMP_FAILURE, relayMode);

  relayCounter = 5;
  inactiveTime = 6;
  
  NextRelayMode(RELAY_IDLE);
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0UL, inactiveTime);
}

test(controller, relay_increase_timer)
{
  // initialize state to IDLE
  NextRelayMode(RELAY_IDLE);
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, (int)relayCounter);

  // increase timer twice
  assertFalse(RelayIncreaseTimer(2, RELAY_PUMP_FAILURE));
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(1, (int)relayCounter);
  assertFalse(RelayIncreaseTimer(2, RELAY_PUMP_FAILURE));
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(2, (int)relayCounter);
  
  // Following count should result in state change
  assertTrue(RelayIncreaseTimer(2, RELAY_PUMP_FAILURE));
  // on state change counter should be reset
  assertEqual(0, (int)relayCounter);
  // verify that new state is set
  assertEqual(RELAY_PUMP_FAILURE, relayMode);
}

test(controller, check_thresholds)
{
  int i;
  
  // initialize state to IDLE
  NextRelayMode(RELAY_IDLE);
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);

  // verify it stays stable at RELAY_IDLE
  for(i=0;i<10;i++)
  {
    assertEqual(i, (int)inactiveTime);
    ControllerLoop(RELAY_OFF_THRESHOLD);
    assertEqual(RELAY_IDLE, relayMode);
  }
  // now indicate high level
  ControllerLoop(RELAY_ON_THRESHOLD+1);
  assertEqual(RELAY_IGNITED, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);

  // verify it stays stable at RELAY_IGNITED
  for(i=0;i<10;i++)
  {
    assertEqual(i, (int)relayCounter);
    ControllerLoop(RELAY_ON_THRESHOLD+1);
    assertEqual(RELAY_IGNITED, relayMode);
  }

  // now indicate mid level, we expect it will stay in same mode
  for(;i<20;i++)
  {
    assertEqual(i, (int)relayCounter);
    ControllerLoop(RELAY_ON_THRESHOLD-1);
    assertEqual(RELAY_IGNITED, relayMode);
  }

  // now indicate low level, it should went to grace off mode
  ControllerLoop(RELAY_OFF_THRESHOLD-1);
  assertEqual(RELAY_GRACEOFF, relayMode);
  assertEqual(0, (int)relayCounter);

  for(i=0;i<RELAY_TIME_GRACE;i++)
  {
     ControllerLoop(RELAY_OFF_THRESHOLD-1);
     assertEqual(RELAY_GRACEOFF, relayMode);
     assertEqual(i+1, (int)relayCounter);
  }

  // now we are at end of grace time, next loop should switch it to RELAY_IDLE
  ControllerLoop(RELAY_OFF_THRESHOLD-1);
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);
}

// check that pump is not allowed to be on too long
test(controller, check_max_on_time)
{
  int i;

  // initialize pump failure counter
  pumpFailureCounter = 0;

  // initialize state to IDLE
  NextRelayMode(RELAY_IDLE);
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);

  // now indicate high level
  ControllerLoop(RELAY_ON_THRESHOLD+1);
  assertEqual(RELAY_IGNITED, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);

  // keep pump on maximum allowed time
  for(i=0;i<RELAY_TIME_ON_MAX;i++)
  {
    ControllerLoop(RELAY_ON_THRESHOLD+1);
    assertEqual(RELAY_IGNITED, relayMode);
    assertEqual(i+1, (int)relayCounter);
    assertEqual(0, (int)inactiveTime);
  }

  // now maximum allowed time is passed, should turn pump off
  ControllerLoop(RELAY_ON_THRESHOLD+1);
  assertEqual(RELAY_GRACEOFF, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);
  // pump failure should be noticed
  assertEqual(1, pumpFailureCounter);

  // wait till end of grace time
  for(i=0;i<RELAY_TIME_GRACE;i++)
  {
     ControllerLoop(RELAY_OFF_THRESHOLD-1);
     assertEqual(RELAY_GRACEOFF, relayMode);
     assertEqual(i+1, (int)relayCounter);
  }

  // now we are at end of grace time, next loop should switch it to RELAY_IDLE
  ControllerLoop(RELAY_OFF_THRESHOLD-1);
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);

  // it should be remembered that pump has failed
  assertEqual(1, pumpFailureCounter);
}

// keep sensor level low and check that after long inactivity pump will be on
// to allow air entering into sensor
test(controller, check_oxygenation)
{
  int i;
  long li;
  
  // initialize state to IDLE
  NextRelayMode(RELAY_IDLE);
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);

  // keep it in RELAY_IDLE as long as allowed
  for(li=1;li<RELAY_TIME_INACTIVE_MAX;li++)
  {
    ControllerLoop(RELAY_OFF_THRESHOLD-1);
    assertEqual(li, (long)inactiveTime);
    assertEqual(RELAY_IDLE, relayMode);
  }

  // now maximum allowed time is passed, pump should be on to allow air in
  ControllerLoop(RELAY_OFF_THRESHOLD-1);
  assertEqual(RELAY_OXYGENATE, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);

  // wait till end of oxygenation routine
  for(i=0;i<RELAY_TIME_OXYGENATE;i++)
  {
     ControllerLoop(RELAY_OFF_THRESHOLD-1);
     assertEqual(RELAY_OXYGENATE, relayMode);
     assertEqual(i+1, (int)relayCounter);
  }

  // now maximum allowed time is passed, should turn pump off
  ControllerLoop(RELAY_OFF_THRESHOLD-1);
  assertEqual(RELAY_GRACEOFF, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);

  // wait till end of grace time
  for(i=0;i<RELAY_TIME_GRACE;i++)
  {
     ControllerLoop(RELAY_OFF_THRESHOLD-1);
     assertEqual(RELAY_GRACEOFF, relayMode);
     assertEqual(i+1, (int)relayCounter);
  }

  // now we are at end of grace time, next loop should switch it to RELAY_IDLE
  ControllerLoop(RELAY_OFF_THRESHOLD-1);
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);
}

// keep sensor level high and check that pump failure mode works correctly
test(controller, pump_failure)
{
   int i, j;
   
  // initialize state to IDLE
  NextRelayMode(RELAY_IDLE);
  assertEqual(RELAY_IDLE, relayMode);
  assertEqual(0, (int)relayCounter);
  assertEqual(0, (int)inactiveTime);

  for(j=pumpFailureCounter;j<RELAY_OVF_MAX-1;j++)
  {
    assertEqual(j, pumpFailureCounter);
    assertEqual(RELAY_IDLE, relayMode);
    ControllerLoop(RELAY_ON_THRESHOLD+1);
    for(i=0;i<=RELAY_TIME_ON_MAX;i++)
    {
      assertEqual(RELAY_IGNITED, relayMode);
      ControllerLoop(RELAY_ON_THRESHOLD+1);
    }
    for(i=0;i<=RELAY_TIME_GRACE;i++)
    {
      assertEqual(RELAY_GRACEOFF, relayMode);
      ControllerLoop(RELAY_ON_THRESHOLD+1);
    }
  }
  assertEqual(RELAY_IDLE, relayMode);
  ControllerLoop(RELAY_ON_THRESHOLD+1);
  for(i=0;i<=RELAY_TIME_ON_MAX;i++)
  {
    assertEqual(RELAY_IGNITED, relayMode);
    ControllerLoop(RELAY_ON_THRESHOLD+1);
  }
  // now the pump was turn on too long for too many times
  // we should be in pump failure mode
  assertEqual(RELAY_OVF_MAX, pumpFailureCounter);
  for(i=0;i<=RELAY_TIME_ON_FAILURE;i++)
  {
    assertEqual(RELAY_PUMP_FAILURE, relayMode);
    ControllerLoop(RELAY_ON_THRESHOLD+1);
  }
  // time in failure passed, should be in idle again
  assertEqual(RELAY_IDLE, relayMode);
  
  // still keep high level simulating pump failure
  ControllerLoop(RELAY_ON_THRESHOLD+1);
  // we should be allowed to on pump for one cycle
  for(i=0;i<=RELAY_TIME_ON_MAX;i++)
  {
    assertEqual(RELAY_IGNITED, relayMode);
    ControllerLoop(RELAY_ON_THRESHOLD+1);
  }
  // pump failure counter should continue counting
  assertEqual(RELAY_OVF_MAX+1, pumpFailureCounter);
  // after one pumping cycle we should immediately move to failure mode again
  for(i=0;i<=RELAY_TIME_ON_FAILURE;i++)
  {
    assertEqual(RELAY_PUMP_FAILURE, relayMode);
    ControllerLoop(RELAY_ON_THRESHOLD+1);
  }
  // time in failure passed, should be in idle again
  assertEqual(RELAY_IDLE, relayMode);
  
  // still keep high level simulating pump failure
  ControllerLoop(RELAY_ON_THRESHOLD+1);

  // now shorten pumping out cycle..
  for(i=0;i<=RELAY_TIME_ON_MAX-10;i++)
  {
    assertEqual(RELAY_IGNITED, relayMode);
    ControllerLoop(RELAY_ON_THRESHOLD+1);
  }
  // ..and simulate that water level decreased
  ControllerLoop(RELAY_OFF_THRESHOLD-1);
  // we should immediately move to grace off mode
  assertEqual(RELAY_GRACEOFF, relayMode);
  // and pump failure counter should be reset
  assertEqual(0, pumpFailureCounter);
  for(i=0;i<RELAY_TIME_GRACE;i++)
  {
    ControllerLoop(RELAY_OFF_THRESHOLD-1);
    assertEqual(RELAY_GRACEOFF, relayMode);
  }
  // now we should move to idle state
  ControllerLoop(RELAY_OFF_THRESHOLD-1);
  assertEqual(RELAY_IDLE, relayMode);
}

void setup() {

  delay(1000);
  Serial.begin(115200);
  while(!Serial);

}

void loop() 
{
  aunit::TestRunner::run();
  delay(1);

}
