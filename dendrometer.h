#include <Adafruit_NAU7802.h>

Adafruit_NAU7802 nau;
int32_t dendroValRaw;

void dendrometer_read() {

  // Read dendrometer measurement
  nau.enable(true);
  while (! nau.available()) {
    delay(1);
  }

  // Fit the measured value into 16 bits
  dendroValRaw = nau.read();

  // Throw away the bottom 4 bits of the LSB and the top 12 bits of the MSW to fit in the 16 bit pakcet
  dendroValRaw = dendroValRaw >> 4;
  dendroVal = (int)dendroValRaw;
  
  nau.enable(false);
}

void dendrometer_init() {
  if (! nau.begin()) {
#if defined DEBUG
    Serial.println("Failed to find NAU7802");
#endif
    delay(1);
  }
  
  nau.setLDO(NAU7802_3V0);
/*  Serial.print("LDO voltage set to ");
  switch (nau.getLDO()) {
    case NAU7802_4V5:  Serial.println("4.5V"); break;
    case NAU7802_4V2:  Serial.println("4.2V"); break;
    case NAU7802_3V9:  Serial.println("3.9V"); break;
    case NAU7802_3V6:  Serial.println("3.6V"); break;
    case NAU7802_3V3:  Serial.println("3.3V"); break;
    case NAU7802_3V0:  Serial.println("3.0V"); break;
    case NAU7802_2V7:  Serial.println("2.7V"); break;
    case NAU7802_2V4:  Serial.println("2.4V"); break;
    case NAU7802_EXTERNAL:  Serial.println("External"); break;
  }
*/
  nau.setGain(NAU7802_GAIN_128);
/*  Serial.print("Gain set to ");
  switch (nau.getGain()) {
    case NAU7802_GAIN_1:  Serial.println("1x"); break;
    case NAU7802_GAIN_2:  Serial.println("2x"); break;
    case NAU7802_GAIN_4:  Serial.println("4x"); break;
    case NAU7802_GAIN_8:  Serial.println("8x"); break;
    case NAU7802_GAIN_16:  Serial.println("16x"); break;
    case NAU7802_GAIN_32:  Serial.println("32x"); break;
    case NAU7802_GAIN_64:  Serial.println("64x"); break;
    case NAU7802_GAIN_128:  Serial.println("128x"); break;
  }
*/
  nau.setRate(NAU7802_RATE_10SPS);
/*  Serial.print("Conversion rate set to ");
  switch (nau.getRate()) {
    case NAU7802_RATE_10SPS:  Serial.println("10 SPS"); break;
    case NAU7802_RATE_20SPS:  Serial.println("20 SPS"); break;
    case NAU7802_RATE_40SPS:  Serial.println("40 SPS"); break;
    case NAU7802_RATE_80SPS:  Serial.println("80 SPS"); break;
    case NAU7802_RATE_320SPS:  Serial.println("320 SPS"); break;
  }
*/

  // Take 10 readings to flush out readings
  for (i=0; i<10; i++) {
    while (! nau.available()) delay(1);
    nau.read();
  }

  while (! nau.calibrate(NAU7802_CALMOD_INTERNAL)) {
#if defined DEBUG
    Serial.println("Failed to calibrate internal offset, retrying!");
#endif
    delay(1000);
  }

  while (! nau.calibrate(NAU7802_CALMOD_OFFSET)) {
#if defined DEBUG
    Serial.println("Failed to calibrate system offset, retrying!");
#endif
    delay(1000);
  }

  return;
}
