
// Arduino pin assignment
#define PIN_IR A0

// configurable parameters
#define LOOP_INTERVAL 100    // Loop Interval (unit: msec)
#define _EMA_ALPHA    0.7   // EMA weight of new sample (range: 0 to 1)
                            // Setting EMA to 1 effectively disables EMA filter.

// global variables
unsigned long last_sampling_time; // unit: msec
int ema;                          // Voltage values from the IR sensor (0 ~ 1023)

void setup()
{
  Serial.begin(1000000);    // 1,000,000 bps

  last_sampling_time = 0;

  return;
  while(1) {
    while (Serial.available() == 0)
      ;
    Serial.read();  // wait for <Enter> key
    ir_sensor_filtered(10, 2);
  }
}

void loop()
{
  unsigned int raw, filtered;           // Voltage values from the IR sensor (0 ~ 1023)

  // wait until next sampling time.
  if (millis() < (last_sampling_time + LOOP_INTERVAL))
    return;
  last_sampling_time += LOOP_INTERVAL;

  // Take a median value from multiple measurements
  filtered = ir_sensor_filtered(10, 2);

  // Take a single measurement
  raw = analogRead(PIN_IR);

  // Calculate EMA
  ema = _EMA_ALPHA * ema + (1.0 - _EMA_ALPHA) * raw;

  // Oupput the raw, filtered, and EMA values for comparison purpose
  Serial.print("MIN:"); Serial.print(0);              Serial.print(",");
  Serial.print("RAW:"); Serial.print(raw);            Serial.print(",");
  Serial.print("EMA:"); Serial.print(ema + 100);      Serial.print(",");
  Serial.print("FLT:"); Serial.print(filtered + 200); Serial.print(",");
  Serial.print("MAX:"); Serial.println(900);
}

int compare(const void *a, const void *b) {
  return (*(unsigned int *)a - *(unsigned int *)b);
}

unsigned int ir_sensor_filtered(unsigned int n, int verbose)
{
  // Eliminate spiky noise of an IR distance sensor by repeating measurement and taking a middle value
  // n: number of measurement repetition
  // position: the percentile of the sample to be taken (0.0 <= position <= 1.0)
  // verbose: 0 - normal operation, 1 - observing the internal procedures, and 2 - calculating elapsed time.
  // Example 1: ir_sensor_filtered(n, 0.5, 0) => return the median value among the n measured samples.
  // Example 2: ir_sensor_filtered(n, 0.0, 0) => return the smallest value among the n measured samples.
  // Example 3: ir_sensor_filtered(n, 1.0, 0) => return the largest value among the n measured samples.

  // The output of Sharp infrared sensor includes lots of spiky noise.
  // To eliminate such a spike, ir_sensor_filtered() performs the following two steps:
  // Step 1. Repeat measurement n times and collect n * position smallest samples, where 0 <= postion <= 1.
  // Step 2. Return the position'th sample after sorting the collected samples.

  // returns 0, if any error occurs

  if(n < 3) return 0;

  unsigned long start_time = millis();

  unsigned int minVal = 1023;
  unsigned int maxVal = 0;
  unsigned int sum = 0;

  for(unsigned int i = 0; i < n; i++){
    unsigned int v = analogRead(PIN_IR);
    if(v < minVal) minVal = v;
    if(v > maxVal) maxVal = v;
    sum += v;

    if(verbose == 1){
      Serial.print(" ");
      Serial.print(v);
    }
  }

  unsigned int result = (sum - minVal - maxVal) / (n - 2);

  if(verbose >= 2){
    Serial.print("Elapsed: "); Serial.print(millis() - start_time); Serial.println("ms");
    Serial.print("Memory usage: "); Serial.print(sizeof(minVal) + sizeof(maxVal) + sizeof(sum)); Serial.println("bytes");
  }
  
  return result;
}
