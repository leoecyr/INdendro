#include "SPI.h"
#include "RH_RF95.h"
#include "AES.h"

#define DEBUG true

// Type changes for different sensor arrangements and for new or updated schemas
#define PACKET_TYPE 0x0000

// Encryption setup
#define MAX_PACKET_BYTES 16
/*
 * Packet format for ^THIS^ packet type
 * 16 bytes total, composed of following 8 unsigned integers
 * 
 * |<---- CONSTANT ----------------->|<------------ VARIABLE -------------------->|
 * |                                 |<packet id->|<---------sensor data--------->|
 * device_id, device_ver, packet_type, sequence_id, meas_0, meas_1, meas_2, meas_3
 */

// LoRa radio configuration
// 5-23 dBm
#define TX_POWER  15
#define RFM95_CS  8
#define RFM95_INT 3
#define RFM95_RST 4
#define RF95_FREQ 434.0   // For 900MHz hardware: 915.0

// instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// instance of the encryption driver
AES128 aes128;

//char txt_buf[80]; // Buffer for debugging purposes

byte clear_packet[MAX_PACKET_BYTES];
byte cipher_packet[MAX_PACKET_BYTES];


void radio_init() {

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
  if(DEBUG) {
      Serial.println("LoRa radio init failed");
      Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
  }

    delay(1000);
    //while (1);
  }

  if(DEBUG) {
  Serial.println("LoRa radio init OK!");
  }

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    if(DEBUG) {
      Serial.println("setFrequency failed");
    }
    while (1);
  }

    if(DEBUG) {
      Serial.print("Set Freq to: ");
      Serial.println(RF95_FREQ);
    }
    
  // This module has the PA_BOOST transmitter pin, permitting powers from 5 to 23 dBm:
  rf95.setTxPower(TX_POWER, false);
    if(DEBUG) {
    Serial.print("Set Tx power to: ");
    Serial.print(TX_POWER);
    Serial.println(" dBm");
    }
    
  //
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  aes128.setKey(key,16);
}

void radio_sleep() {
  // Shutdown the LoRa radio
  rf95.sleep();
  digitalWrite(RFM95_CS, LOW);
}


void radio_wake() {
  // FIXME ideally this would should be in a lightweight, radio_wake()
  digitalWrite(RFM95_CS, HIGH);
  rf95.setModeRx();
}

void encrypt_packet() {
  //
  //  input:  byte clear_packet[]
  //  output: byte cipher_packet[]
  //
  int blk_cnt_max = MAX_PACKET_BYTES / 16;  // AES 128 has 16 byte keys for 16 bytes of data
  for(i = 0; i < blk_cnt_max; i++) {
    aes128.encryptBlock(&cipher_packet[i * 16], &clear_packet[i * 16]); //cypher->output block and cleartext->input block
  }
}

void radio_packet_debug() {
  for(i; i < 7; i++) {
    Serial.print(clear_packet[i]);
  }
  Serial.println(clear_packet[7]);
}

/* Debugging only
// Specific to dendrometer packet type 0x00
char *decode_to_text_00(int rssi) {
  // Eight unsigned integers are encoded in 16 bytes
  unsigned int temp_vals[8];
  memcpy(temp_vals, clear_packet, 16);
  sprintf(txt_buf, "%X,%X,%X,%X, %d,%d,%d,%d, %d\n", temp_vals[0], temp_vals[1], temp_vals[2], temp_vals[3], temp_vals[4], temp_vals[5], temp_vals[6], temp_vals[7], rssi);
  //device_id, device_ver, packet_type, sequence_id, meas_0, meas_1, meas_2, meas_3);
  return txt_buf;
}
*/
