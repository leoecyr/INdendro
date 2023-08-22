#include "HX711.h"
#define DENDROMETER true
#define DEBUG true

// Loadcell setup
// HX711 circuit wiring
#define LOADCELL_DOUT_PIN 4
#define LOADCELL_SCK_PIN 5 // Was 3.  Now 6

HX711 scale;

long dendrometer_read() {
  scale.power_up();

#if defined DEBUG
    //Serial.println("HX711 wakeup complete");
#endif

  // Read dendrometer measurement
  scale.read_average(loadSampleDiscard);
  long dendroVal = scale.read_average(loadSamples);  // FIXME RAW - loadTare; // Must fit in a 16 bit int

#if defined DEBUG
    Serial.print("Average of ");
    Serial.print(loadSamples);
    Serial.print(" RAW HX711 ADC samples:");
    Serial.println(dendroVal);
    //Serial.println("HX711 shutting down");
#endif

  //scale.power_down(); // put the load cell ADC in sleep mode

  return dendroVal;
}

void dendrometer_init() {
  //scale.set_gain(<byte>);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  return;
}
