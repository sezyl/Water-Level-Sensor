// 2019 - Sebastian Żyłowski
// These are sensor reading and averaging routines

/* sensor functions */

void SensorSetup(void)
{  
  pinMode(ADC_PIN, INPUT); // ADC input
  // enable ADC with highest precision
  ADCSRA = _BV(ADEN) | 7;
  // set input mux to sensor
  ADMUX = ADC_PIN;
  // disable input buffer for ADC1 - this needs to be changed if sensor pin is changed!!
  DIDR0 |= _BV(ADC1D);
}

int MyAnalogRead()
{
  // start ADC conversion
  ADCSRA |= _BV(ADSC);
  // wait for end of conversion
  while(bit_is_set(ADCSRA, ADSC));

  return(ADCW);
}

// read ADC three times, reject one sample which is far from others
// average remaining two samples and return as result
int ReadAdc()
{ int read1, read2, read3;
  int avg_final, delta, delta_final;

  read1 = MyAnalogRead();
  read2 = MyAnalogRead();
  read3 = MyAnalogRead();

  delta_final = abs(read1 - read2);
  avg_final = (read1 + read2)/2;

  delta = abs(read2 - read3);
  if(delta<delta_final)
  {
    delta_final = delta;
    avg_final = (read2 + read3)/2;
  }

  delta = abs(read3 - read1);
  if(delta<delta_final)
  {
    avg_final = (read1 + read3)/2;
  }
  
  return(avg_final);
}

int ReadSensorAdc(void)
{
  int sensor;

  ADMUX = ADC_IN;
  delay(2);
  sensor = ReadAdc();

  return (sensor);
}

int ReadTemperatureAdc(void)
{
  int sample;

  ADMUX = _BV(REFS1) | TEMP_SENSOR;
  delay(2);
  sample = ReadAdc();

  return (sample);
}
