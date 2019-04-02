// 2019 - Sebastian Żyłowski
// These are sensor reading and averaging routines

int samples[AVERAGE_WINDOW] = { 0 };
uint8_t samples_in_buffer = 0;
uint8_t samples_id = 0;

/* sensor functions */

void SensorSetup(void)
{  
  pinMode(ADC_PIN, INPUT); // ADC input
}

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
