#include "LowPower.h"

#include "globals.h";
#include "node-identity.h";
#include "power-mgmt.h";
#include "dendrometer.h";
#include "thermistor.h";
//#include "radio-lora.h";

#define DEBUG true;

#define DENDROMETER true
// Dendrometer value is 24 bits
long dendroVal = 999999; // FIXME testing value


#define BAUD 9600
#define TX_DELAY_SEC 1 //1800 // 1/2 hour -- will be divided into multiples of 8s


#define BAT_LVL_PIN A2
int batLvl = 0;

// Signaling LED
#define LED_PIN 13


unsigned int packet_vars[8]; // 0-2 are fixed; 3-7 vary at runtime

void setup() 
{
  Serial.begin(BAUD);
  Serial.println(DEVICE_VERSION);
  
  delay(100);
  pinMode(LED_PIN, OUTPUT);
  
  //show_device_id();

#if defined THERMISTOR
  thermistor_init();
#endif

#if defined DENDROMETER
  dendrometer_init();
#endif

#if defined RADIO
  radio_setup();
#endif

}


int seq_num = 0;  // packet sequence counter
void loop()
{
  /*

*/
  // Read battery level at BL0:  +VCC--/\3.3M/\--BL0--/\1.0M/\--GND
  //analogReference(INTERNAL1V1);
  /*
  analogReference(INTERNAL);
  batLvl = analogRead(BAT_LVL_PIN);
  analogReference(DEFAULT);
*/
#if defined DENDROMETER
    dendroVal = dendrometer_read();
#endif

#if defined DEBUG
/*
    // LED turned on during activity for debug
    // Remains on until packet transmit is complete
    //digitalWrite(LED_PIN, HIGH);

    Serial.print("Battery Level*[1/3.3]: ");
    Serial.println(batLvl);

    Serial.print("Thermistor reading: ");
    Serial.println(thermValue);
    Serial.print("dendroVal: ");
    Serial.println(dendroVal);
    */

#endif

  // Packet type 0x0000 format
  // |<---- CONSTANT ----------------->|<------------ VARIABLE -------------------->|
  // device_id, device_ver, packet_type, sequence_id, meas_0, meas_1, meas_2, meas_3)
  //
  // Prepare packet for encryption
  packet_vars[0] = DEVICE_ID;
  packet_vars[1] = DEVICE_FILTER_ID;
  packet_vars[2] = 0x0000; //PACKET_TYPE; // FIXME needs to come from radio-lora.h
  packet_vars[3] = seq_num;
  packet_vars[4] = (int)dendroVal; // Dendro tension load
  packet_vars[5] = 0x0000; // Empty
  packet_vars[6] = thermValue;
  packet_vars[7] = batLvl;

#if defined RADIO
  memcpy(clear_packet, packet_vars, 16);
  encrypt_packet(); // encrypted output in cipher_packet

  if(DEBUG) {
    unsigned int pkttmp[8];
    memcpy(pkttmp, clear_packet, 16);
    Serial.println("Cleartext packet payload:");
    Serial.println( decode_to_text_00(0) );  // RSSI = 0 cause there is no RS
    Serial.println("Encrypted packet payload:");
    for(int j=0;j<MAX_PACKET_BYTES;j++){
      Serial.print(cipher_packet[j], HEX);
      Serial.print(" ");
    }
    Serial.println("Sending encrypted packet...");
  }

  rf95.send((uint8_t *)cipher_packet, MAX_PACKET_BYTES);
  
  if(DEBUG) {
    Serial.println("waitPacketSent()...");
  }
  
  rf95.waitPacketSent();

  if(DEBUG) {
    Serial.println("Packet sent");
  }

#endif - RADIO

#if defined DEBUG
    digitalWrite(LED_PIN, LOW);
#endif
  
  seq_num++;
/*
  if(DEBUG) {
    Serial.print("Sleeping for ");
    Serial.println(TX_DELAY_SEC);
  }

  deep_sleep(TX_DELAY_SEC);

  if(DEBUG) {
    Serial.println("Waking up");
  }
*/

  delay(8000);

#if defined RADIO
  radio_init();
#endif

#if defined THERMISTOR
  thermistor_init();
#endif

#if defined DENDROMETER
  dendrometer_init();
#endif
}

/*void show_device_id() {
  //Serial.println(DEVICE_NAME . DEVICE_ID . DEVICE_TYPE . DEVICE_VOLTAGE . DEVICE_FREQ . " MHz LoRa");
}*/
