/*
 * NOTE: You will find a variety of gloal variables and in-place manipulation
 * of those globals.  There is improvement that can be made.  However, the use
 * of globals is mostly intentional to keep our memory footprint low.
 * With debug off we are at 70% of our 2kB memory.  debug on we are at 81%.
 * TODO Optimization and organization.
 */
#include "SPI.h"
#include "RH_RF95.h"
#include "LowPower.h"
#include "AES.h"
#include "HX711.h"

#include "node-identity.h";

#define DEBUG true
//#define RADIO true

#define TX_DELAY_SEC 8 //1800 // 1/2 hour -- will be divided into multiples of 8s

// FIXME Most of these global initialization steps should be modularized and placed
//  in their own header/library files.

// Type changes for different sensor arrangements and for new or updated schemas
#define PACKET_TYPE 0x0000
/*
 * Packet format for ^THIS^ packet type
 * 16 bytes total, composed of following 8 unsigned integers
 * 
 * |<---- CONSTANT ----------------->|<------------ VARIABLE -------------------->|
 * |                                 |<packet id->|<---------sensor data--------->|
 * device_id, device_ver, packet_type, sequence_id, meas_0, meas_1, meas_2, meas_3
 */

unsigned int packet_vars[8]; // 0-2 are fixed; 3-7 vary at runtime

#ifdef RADIO
// LoRa radio configuration
// 5-23 dBm
#define TX_POWER  15
#define RFM95_CS  8
#define RFM95_INT 3
#define RFM95_RST 4
#define RF95_FREQ 434.0   // For 900MHz hardware: 915.0

// Singleton instance of the radio driver

RH_RF95 rf95(RFM95_CS, RFM95_INT);
#endif

// Thermistor setup
// 
#define THERMISTOR_PIN A3
#define THERMISTOR_POWER_PIN 5
int thermValue = 0;  // variable to store the value coming from the sensor

#define BAT_LVL_PIN A2
int batLvl = 0;

// Signaling LED
#define LED_PIN 13

// Loadcell setup
// HX711 circuit wiring
#define LOADCELL_DOUT_PIN 2
#define LOADCELL_SCK_PIN 6 // Was 3.  Now 6
#define LOADCELL_MIN 62800  //0
#define LOADCELL_SAMPLES 100
long loadLvl = 0;
HX711 scale;

// Device info
#define DEVICE_NAME "INdendro v"
#define DEVICE_TYPE "Pro-mini"
#define DEVICE_VOLTAGE 3.3
#define DEVICE_FREQ 8

#ifdef RADIO
// Encryption setup
#define MAX_PACKET_BYTES 16

byte clear_packet[MAX_PACKET_BYTES];
byte cipher_packet[MAX_PACKET_BYTES];
AES128 aes128;
#endif

char txt_buf[128]; // ASCII representation of printable text

void setup() 
{
#ifdef DEBUG
    Serial.begin(9600);
    delay(100);
    pinMode(LED_PIN, OUTPUT);
#endif
  
  show_device_id();

  setup_thermistor(true);

  setup_dendrometer(true);

#ifdef RADIO
  setup_radio(true);
  aes128.setKey(key,16);
#endif

}


int seq_num = 0;  // packet sequence counter
void loop()
{
  // Enable power to thermistor and stabilize
  digitalWrite(THERMISTOR_POWER_PIN, HIGH);
  delay(1000); // stabilize temperature
  thermValue = analogRead(THERMISTOR_PIN);
  digitalWrite(THERMISTOR_POWER_PIN, LOW);

  // Read battery level at BL0:  +VCC--/\3.3M/\--BL0--/\1.0M/\--GND
  //analogReference(INTERNAL1V1);
  analogReference(INTERNAL);
  batLvl = analogRead(BAT_LVL_PIN);
  analogReference(DEFAULT);

  // Read dendrometer measurement
  loadLvl = scale.read_average(LOADCELL_SAMPLES) - LOADCELL_MIN; // Must fit in 16 bit int
    
#ifdef DEBUG
  // LED turned on during activity for debug
  digitalWrite(LED_PIN, HIGH);

  Serial.print("Raw ADC (avg. of ");
  Serial.print(LOADCELL_SAMPLES);
  Serial.print("): ");  
  Serial.println(loadLvl);
  
  Serial.print("Thermistor reading: ");
  Serial.println(thermValue);
  
  Serial.print("Battery Level*[1/3.3]: ");
  Serial.println(batLvl);
#endif

  // Packet type 0x0000 format
  // |<---- CONSTANT ----------------->|<------------ VARIABLE -------------------->|
  // device_id, device_ver, packet_type, sequence_id, meas_0, meas_1, meas_2, meas_3)
  //
  // Prepare packet for encryption
  packet_vars[0] = DEVICE_ID;
  packet_vars[1] = DEVICE_FILTER_ID;
  packet_vars[2] = PACKET_TYPE;
  packet_vars[3] = seq_num;
  packet_vars[4] = (int)loadLvl; // Dendro tension load
  packet_vars[5] = 0x0000; // Empty
  packet_vars[6] = thermValue;
  packet_vars[7] = batLvl;

#ifdef RADIO
  memcpy(clear_packet, packet_vars, 16);
  encrypt_packet(); // encrypted output in cipher_packet

#ifdef DEBUG
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
#endif - DEBUG

  rf95.send((uint8_t *)cipher_packet, MAX_PACKET_BYTES);
  
#ifdef DEBUG
  Serial.println("waitPacketSent()...");
#endif - DEBUG
  
  rf95.waitPacketSent();

#ifdef DEBUG
  Serial.println("Packet sent");
  digitalWrite(LED_PIN, LOW);
#endif - DEBUG
#endif - RADIO

  seq_num++;

  deep_sleep(TX_DELAY_SEC);

#ifdef RADIO
  setup_radio(DEBUG);
#endif
  
  setup_thermistor(DEBUG);
  setup_dendrometer(DEBUG);
}

//
// Routines to support setup() and loop()
void show_device_id() {
  sprintf(txt_buf, "%s%X on %s %nv %nMHz LoRa", DEVICE_NAME, DEVICE_ID, DEVICE_TYPE, DEVICE_VOLTAGE, DEVICE_FREQ);
  Serial.println(txt_buf);
}

void setup_dendrometer(bool debug_out) {
  
  //scale.set_gain(<byte>);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  // FIXME I don't think these are needed when using raw ADC values
  //scale.set_scale(2280.f);  // this value is obtained by calibrating the scale with known weights; see the README for details
  //scale.tare();             // reset the scale to 0

  scale.power_up();
  if(debug_out) {
    loadLvl = scale.read_average(LOADCELL_SAMPLES) - LOADCELL_MIN; // Must fit in 16 bit int
    Serial.print("Average of 100 RAW HX711 ADC samples:");
    Serial.println(loadLvl);
  }
  
  return;
}

void setup_thermistor(bool debug_out) {
  //
  // Set device power pins and LED as OUTPUT:
  pinMode(THERMISTOR_POWER_PIN, OUTPUT);
}

#ifdef RADIO
void setup_radio(bool debug_out) {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  // Reset LoRa radio
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);

  while (!rf95.init()) {
    if(debug_out) {
      Serial.println("LoRa radio init failed");
      Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    }
    delay(1000);
    //while (1);
  }
  
  if(debug_out) { Serial.println("LoRa radio init OK!"); }

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    if(debug_out) { Serial.println("setFrequency failed"); }
    while (1);
  }

  if(debug_out) {
    Serial.print("Set Freq to: ");
    Serial.println(RF95_FREQ);
  }
    
  // This module has the PA_BOOST transmitter pin, permitting powers from 5 to 23 dBm:
  rf95.setTxPower(TX_POWER, false);
  if(debug_out) {
    Serial.print("Set Tx power to: ");
    Serial.print(TX_POWER);
    Serial.println(" dBm");
  }
    
  //
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
}
#endif - RADIO

void deep_sleep(int seconds) {
#ifdef RADIO
  // Shutdown the LoRa radio
  rf95.sleep();
  digitalWrite(RFM95_CS, LOW);
#endif

  scale.power_down();              // put the ADC in sleep mode

  // A low budget long, low-power sleep
  int tx_delay_count = round(seconds / 8);

  // count of 8s sleeps -- 225 = 1800s
  for(int slp_cnt = 0; slp_cnt < tx_delay_count; slp_cnt++) {
    // Enter power down state for 8 s with ADC and BOD module disabled
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);      
  }

#ifdef RADIO
  // FIXME ideally this would should be in a lightweight, radio_wake()
  digitalWrite(RFM95_CS, HIGH);
  rf95.setModeRx();
#endif
  

  // After sleeping for seconds, return to main loop
  return;
}

#ifdef RADIO
void encrypt_packet() {
  //
  //  input:  byte clear_packet[]
  //  output: byte cipher_packet[]
  //
  int blk_cnt_max = MAX_PACKET_BYTES / 16;  // AES 128 has 16 byte keys for 16 bytes of data
  for(int blk_offset = 0; blk_offset < blk_cnt_max; blk_offset++) {
    aes128.encryptBlock(&cipher_packet[blk_offset * 16], &clear_packet[blk_offset * 16]); //cypher->output block and cleartext->input block
  }
}

// Specific to dendrometer packet type 0x00
char *decode_to_text_00(int rssi) {
  // Eight unsigned integers are encoded in 16 bytes
  unsigned int temp_vals[8];
  memcpy(temp_vals, clear_packet, 16);
  sprintf(txt_buf, "%X,%X,%X,%X, %d,%d,%d,%d, %d\n", temp_vals[0], temp_vals[1], temp_vals[2], temp_vals[3], temp_vals[4], temp_vals[5], temp_vals[6], temp_vals[7], rssi);
  //device_id, device_ver, packet_type, sequence_id, meas_0, meas_1, meas_2, meas_3);
  return txt_buf;
}
#endif - RADIO
