#include "LowPower.h"


// GLOBALS -- Variables requiring visibility across modules
int thermValue = 9999;  // variable to store the value coming from the sensor
int batLvl = 0;
int dendroVal = 9999;

const int loadSampleDiscard = 3;  // count of samples to discard before averaging
const int loadSamples = 10;  // Count of samples averaged


//unsigned int packet_vars[8]; // 0-2 are fixed; 3-7 vary at runtime


int i;  // local iterator used multiply in multiple modules NOT recursively OR ELSE
// END GLOBALS


// Module code inclusion/exclusion
#include "node-identity.h";
#include "power-mgmt.h";
#include "dendrometer.h";
#include "thermistor.h";
#include "radio-lora.h";


// Module enable/disable
#define DEBUG true
#define DENDROMETER true
#define THERMISTOR true

// Global defines
#define BAUD 9600
#define TX_DELAY_SEC 1 //1800 // 1/2 hour -- will be divided into multiples of 8s

// Signaling LED
#define LED_PIN 13



void setup() 
{
#if defined DEBUG
  Serial.begin(BAUD);
  Serial.println(DEVICE_VERSION);
#endif

  delay(100);
  pinMode(LED_PIN, OUTPUT);

#if defined THERMISTOR
  thermistor_init();
#endif

#if defined DENDROMETER
  dendrometer_init();
#endif

#if defined RADIO
  radio_init();
#endif

#if defined DEBUG
  Serial.println("Completed setup()");
#endif
}


int seq_num = 0;  // packet sequence counter
void loop() {
#if defined DEBUG
  Serial.println("Entered loop()");
#endif

  // Read battery level at BL0:  +VCC--/\3.3M/\--BL0--/\1.0M/\--GND
  //analogReference(INTERNAL1V1);
  analogReference(INTERNAL);
  batLvl = analogRead(BAT_LVL_PIN);
  analogReference(DEFAULT);
  
#if defined THERMISTOR
    thermistor_read(); // Result left in global to decrease runtime memory requirements
#endif

#if defined DENDROMETER
    dendrometer_read(); // Result left in global #MEMMGMT
#endif

#if defined DEBUG
    // LED turned on during activity for debug
    // Remains on until packet transmit is complete
    //digitalWrite(LED_PIN, HIGH);


    Serial.print("Battery Level*[1/3.3]: ");
    Serial.println(batLvl);
    Serial.print("Thermistor reading: ");
    Serial.println(thermValue);
    Serial.print("dendroVal: ");
    Serial.println(dendroVal);
 
#endif

  // Packet type 0x0000 format
  // |<---- CONSTANT ----------------->|<------------ VARIABLE -------------------->|
  // device_id, device_ver, packet_type, sequence_id, meas_0, meas_1, meas_2, meas_3)
  //
  // Prepare packet for encryption
  clear_packet[0] = DEVICE_ID;
  clear_packet[1] = DEVICE_FILTER_ID;
  clear_packet[2] = 0x0000; //PACKET_TYPE; // FIXME needs to come from radio-lora.h
  clear_packet[3] = seq_num;
  clear_packet[4] = dendroVal; // Dendro tension load
  clear_packet[5] = 0x0000; // Empty
  clear_packet[6] = thermValue;
  clear_packet[7] = batLvl;

#if defined RADIO
  //memcpy(clear_packet, packet_vars, 16);
  encrypt_packet(); // encrypted output in cipher_packet

#if defined DEBUG
    //unsigned int pkttmp[8];
    //memcpy(pkttmp, clear_packet, 16);
    //Serial.println("Cleartext packet payload:");
    //Serial.println( decode_to_text_00(0) );  // RSSI = 0 cause there is no RS
    radio_packet_debug();
    Serial.println("Encrypted packet payload:");
    for(i=0; i<MAX_PACKET_BYTES; i++){
      Serial.print(cipher_packet[i], HEX);
      Serial.print(" ");
    }
    Serial.println("Sending encrypted packet...");
#endif

  rf95.send((uint8_t *)cipher_packet, MAX_PACKET_BYTES);

#if defined DEBUG
  Serial.println("waitPacketSent()...");
#endif
  
  rf95.waitPacketSent();

#if defined DEBUG
    Serial.println("Packet sent");
#endif

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

//#if defined DENDROMETER
//  dendrometer_init();
//#endif
}

/*void show_device_id() {
  //Serial.println(DEVICE_NAME . DEVICE_ID . DEVICE_TYPE . DEVICE_VOLTAGE . DEVICE_FREQ . " MHz LoRa");
}*/
