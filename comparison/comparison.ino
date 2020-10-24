typedef enum {
  ACQ_VANILLA,
  ACQ_NOISE_REDUCED
} acq_t;
acq_t acq_mode = ACQ_VANILLA;
const uint16_t n_samples = 512;
uint16_t samples[n_samples];
const unsigned long measure_period = 10000;  // µs

void setup_vanilla_acq() {
  analogReference(DEFAULT); // 5V reference
  pinMode(A0, INPUT);
}

uint16_t measure_vanilla() {
  return analogRead(A0);
}

void setup_noise_reduced_acq() {
  ADCSRA |= (1 << ADEN);  // enable adc
  ADMUX = (0b01 << 6) | (ADMUX & 0x0f); // set voltage reference | keep input
  ADMUX = (ADMUX & 0xf0) | 0;  // keep reference | select input channel
  ADCSRA |= (1 << ADIE);  // enable adc interrupts
  PRR &= ~(1 << PRADC);  // clear adc bit in power reduction register

  // go in ADC noise reduction mode
  SMCR = 0;
  SMCR |= (0b001 << 1);  // select ADC nosie reduction mode
}

volatile int adc_nr_value = 0;
ISR(ADC_vect) {
  cli();
  byte old_sreg = SREG;
  SMCR &= ~(1 << SE);  // clear the sleep bit to prevent accidental sleeps

  // read ADCL before ADCH
  adc_nr_value = ADCL | (ADCH << 8);  // read conversion result

  SREG = old_sreg;
  sei();
}

uint16_t measure_noise_reduced() {
  SMCR |= (1 << SE);  // enable sleep mode
  asm("sleep");  // start sleeping / acquiring
  return adc_nr_value;
}

void power_down() {
  SMCR = 0;
  SMCR |= (0b010 << 1);  // select power down mode
  SMCR |= (1 << SE);  // enable sleep mode
  asm("sleep");
}

uint16_t (*measure)();
void setup() {
  Serial.begin(115200);
  Serial.println("# ADC Acquisition Comparison");
  Serial.flush();

  switch (acq_mode) {
    case ACQ_VANILLA:
      Serial.println("# Vanilla Mode");
      Serial.flush();
      setup_vanilla_acq();
      measure = &measure_vanilla;
      break;
    case ACQ_NOISE_REDUCED:
      Serial.println("# ADC Noise Reduction Mode");
      Serial.flush();
      setup_noise_reduced_acq();
      measure = &measure_noise_reduced;
      break;
  }

  // acquire samples
  unsigned long last_measurement = 0;
  for(int i = 0; i < n_samples; i++) {
    unsigned long current_time = micros();
    while (current_time - last_measurement < measure_period) {
      current_time = micros();
    }
    last_measurement = current_time;
    samples[i] = measure();
  }

  // analyze results

  // calculate mean
  unsigned long sample_sum = 0;
  for (int i = 0; i < n_samples; i++) {
    sample_sum += samples[i];
  }
  float mean = float(sample_sum) / n_samples;

  // calculate standard deviation
  float std = 0.0;
  for (int i = 0; i < n_samples; i++) {
    std += pow((samples[i] - mean), 2);
  }
  std /= n_samples - 1;
  std = sqrt(std);

  // output results
  Serial.print("Sample mean: ");
  Serial.print(mean, 2);
  Serial.print('\n');
  Serial.print("Sample std :   ");
  Serial.print(std, 2);
  Serial.print('\n');


  Serial.flush();

  power_down();
}

void loop() {}
