#include "LowPower.h"


// GLOBALS -- Variables requiring visibility across modules
int thermValue = 9999;  // variable to store the value coming from the sensor
int batLvl = 0;
int dendroVal = 9999;

const int loadSampleDiscard = 3;  // count of samples to discard before averaging
const int loadSamples = 10;  // Count of samples averaged

unsigned int packet_vars[8]; 

int i;  // local iterator used multiply in multiple modules NOT recursively OR ELSE
// END GLOBALS


// Module code inclusion/exclusion
#include "node-identity.h";
#include "power-mgmt.h";
#include "dendrometer.h";
#include "thermistor.h";
#include "radio-lora.h";


// Module enable/disable
//#define DEBUG true
#define DENDROMETER true
#define THERMISTOR true
#define RADIO true

#define BAUD 9600
#define TX_DELAY_SEC 1800 // 1/2 hour -- will be divided into multiples of 8s

// Signaling LED
#define LED_PIN 13



void setup() 
{
#if defined DEBUG
  Serial.begin(BAUD);
  Serial.println(DEVICE_VERSION);
#endif

  delay(100);

#if defined THERMISTOR
  thermistor_init();
#endif

#if defined DENDROMETER
  dendrometer_init();
#endif

#if defined RADIO
  radio_init();
#endif
}


int seq_num = 0;  // packet sequence counter
void loop() {
  // Read battery level at BAT_LVL_PIN:  +VCC--/\3.3M/\--BAT_LVL_PIN--/\1.0M/\--GND
  //analogReference(INTERNAL1V1);
  analogReference(INTERNAL);
  batLvl = analogRead(BAT_LVL_PIN);
  analogReference(DEFAULT);
  
#if defined THERMISTOR
    // Read thermistor voltage at A4: +(Vcc)THERMISTOR_POWER_PIN--/\--15k--/\--THERMISTOR_PIN--/\--Thermistor--/\--GND
    thermistor_read(); // Result left in global to decrease runtime memory requirements
#endif

#if defined DENDROMETER
    dendrometer_read(); // Result left in global #MEMMGMT
#endif

#if defined DEBUG
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
  packet_vars[0] = DEVICE_ID;
  packet_vars[1] = DEVICE_FILTER_ID;
  packet_vars[2] = 0x0000; //PACKET_TYPE; // FIXME needs to come from radio-lora.h
  packet_vars[3] = seq_num;
  packet_vars[4] = dendroVal; // Dendro tension load
  packet_vars[5] = 0x0000; // Empty
  packet_vars[6] = thermValue;
  packet_vars[7] = batLvl;

#if defined RADIO
#if defined DEBUG
    pinMode(LED_PIN, OUTPUT);
    // LED turned on until packet transmit is complete
    digitalWrite(LED_PIN, HIGH);
#endif // - DEBUG

    encrypt_packet(); // encrypted output in cipher_packet
    //radio_wake();
    radio_send();
    delay(1000);
    //radio_sleep();

#if defined DEBUG
    digitalWrite(LED_PIN, LOW);
    Serial.println("Packet send complete.  Radio sleeping.");
#endif // - DEBUG

#endif // - RADIO
  
  seq_num++;
  
  deep_sleep(TX_DELAY_SEC);
}
