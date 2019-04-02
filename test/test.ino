#line 2 "test.ino"

//#include <Arduino.h>
#include <AUnit.h>
#include "./config.h"

// Tests

extern int samples[AVERAGE_WINDOW];
extern uint8_t samples_in_buffer;
extern uint8_t samples_id;

test(sensor, check_init) {
  uint16_t sensor_data[AVERAGE_WINDOW] = { 0 };

  assertTrue(memcmp(sensor_data, samples, AVERAGE_WINDOW));
  assertTrue(0 == samples_in_buffer);
  assertTrue(0 == samples_id);
}

int calculate_average(int last_sample, int samples)
{ int i = 0, j = samples;

  if(j>AVERAGE_WINDOW)
     j = AVERAGE_WINDOW;

  while(j>0)
  { i += last_sample;
    j--;
    last_sample--;
  }

  return(i/j);
}

test(sensor, table_fill) {
  int sensor_avg, i;
  int exp_avg, sample;
 
  for(i=0;i<AVERAGE_WINDOW;i++)
  { 
    sample = AVERAGE_WINDOW+i;
    exp_avg = calculate_average(sample, i+1);
    sensor_avg = AverageAdc(AVERAGE_WINDOW + i);
    assertTrue(exp_avg == sensor_avg);
    // verify table index
    assertTrue( (i+1) == samples_id);
    // verify samples count
    assertTrue( (i+1) == samples_in_buffer);
    // verify the sample landed in table as expected
    assertTrue(sample == samples[i]); 
  }
  
  sensor_avg = AverageAdc(2 * AVERAGE_WINDOW);
  exp_avg = calculate_average(2 * AVERAGE_WINDOW, AVERAGE_WINDOW);
  assertTrue( sensor_avg == exp_avg );

  // table index should go to 0 for circular buffer
  assertTrue( 0 == samples_id );

  // number of samples in table should not increase
  assertTrue( AVERAGE_WINDOW == samples_in_buffer );
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
