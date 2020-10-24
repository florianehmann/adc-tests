volatile int value = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("First ADC Tests");
  Serial.flush();

  ADCSRA |= (1 << ADEN);  // enable adc
  ADMUX = (0b11 << 6) | (ADMUX & 0x0f); // set voltage reference | keep input
  ADMUX = (ADMUX & 0xf0) | 8;  // keep reference | select input channel
  ADCSRA |= (1 << ADIE);  // enable adc interrupts
  PRR &= ~(1 << PRADC);  // clear adc bit in power reduction register
  // ADCSRA |= (1 << ADSC);  // set start conversion bit

  // go in ADC noise reduction mode
  SMCR = 0;
  SMCR |= (1 << SE);  // enable sleep mode
  SMCR |= (0b001 << 1);  // select ADC nosie reduction mode
  asm("sleep");  // start sleeping

  ADCSRA &= ~(1 << ADEN);  // disable adc
}

ISR(ADC_vect) {
  cli();
  byte old_sreg = SREG;

  // read ADCL before ADCH
  value = ADCL | (ADCH << 8);  // read conversion result

  Serial.println(value);

  SREG = old_sreg;
  sei();
}

void loop() {}
