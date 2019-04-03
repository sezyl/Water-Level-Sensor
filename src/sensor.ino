// 2019 - Sebastian Żyłowski
// These are sensor reading and averaging routines

/* sensor functions */

void SensorSetup(void)
{  
  pinMode(ADC_PIN, INPUT); // ADC input
}

#if 0
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
#endif
// read ADC three times, reject one sample which is far from others
// average remaining two samples and return as result
int ReadAdc(uint8_t adc)
{ int read1, read2, read3;
  int avg_final, delta, delta_final;

  read1 = analogRead(adc);
  read2 = analogRead(adc);
  read3 = analogRead(adc);

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

  analogReference(DEFAULT);
  delay(1);
  sensor = ReadAdc(ADC_IN);

  return (sensor);
}

int ReadTemperatureAdc(void)
{
  int sample;

  analogReference(INTERNAL1V1);
  delay(1);
  sample = ReadAdc(TEMP_SENSOR);

  return (sample);
}
