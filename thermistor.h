
// Thermistor setup
// 
#define THERMISTOR_PIN A3
#define THERMISTOR_POWER_PIN 5

void thermistor_init() {
#if defined DEBUG
  Serial.println("thermistor_init()");
#endif

  pinMode(THERMISTOR_POWER_PIN, OUTPUT);
}

void thermistor_read() {
  // Enable power to thermistor and stabilize
  digitalWrite(THERMISTOR_POWER_PIN, HIGH);
  //delay(1000); // stabilize temperature -- no, just elevate; right?  Or, is there wide variation in ADC conversion time?  Compare.
  thermValue = analogRead(THERMISTOR_PIN);
  digitalWrite(THERMISTOR_POWER_PIN, LOW);
}
