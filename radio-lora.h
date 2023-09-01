#include "SPI.h"
#include "RH_RF95.h"
#include "AES.h"

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
 // 0-2 are fixed; 3-7 vary at runtime
byte clear_packet[MAX_PACKET_BYTES];
byte cipher_packet[MAX_PACKET_BYTES];


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

#if defined DEBUG_RADIO

void radio_packet_debug() {
  for(i; i < 7; i++) {
    Serial.print(clear_packet[i], HEX);
    Serial.print(":");
  }
  Serial.println(clear_packet[7], HEX);
}
#endif

void radio_init() {
#if defined DEBUG_RADIO
  Serial.println("radio_init()");
#endif

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
#if defined DEBUG_RADIO
      Serial.println("LoRa radio init failed");
      Serial.println("Uncomment '#define SERIAL_DEBUG_RADIO' in RH_RF95.cpp for detailed debug info");
#endif
    delay(1000);
  }

#if defined DEBUG_RADIO
  Serial.println("LoRa radio init OK!");
#endif
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
#if defined DEBUG_RADIO
      Serial.println("setFrequency failed");
#endif

    while (1);
  }

#if defined DEBUG_RADIO
      Serial.print("Set Freq to: ");
      Serial.println(RF95_FREQ);
#endif
    
  // This module has the PA_BOOST transmitter pin, permitting powers from 5 to 23 dBm:
  rf95.setTxPower(TX_POWER, false);
#if defined DEBUG_RADIO
    Serial.print("Set Tx power to: ");
    Serial.print(TX_POWER);
    Serial.println(" dBm");
#endif
    
  //
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  aes128.setKey(key,16);

#if defined debug
  Serial.println("radio_init() Complete");
#endif
}

void radio_send() {
  rf95.send((uint8_t *)cipher_packet, MAX_PACKET_BYTES);  
  rf95.waitPacketSent();

#if defined DEBUG_RADIO
  Serial.println("Packet sent");
  radio_packet_debug();
#endif

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
  //  Manipulated in-place
  //  input: unsigned int packet_vars[8]
  //  interim:  byte clear_packet[]
  //  output: byte cipher_packet[]
  //
  memcpy(clear_packet, packet_vars, 16);
    
  int blk_cnt_max = MAX_PACKET_BYTES / 16;  // AES 128 has 16 byte keys for 16 bytes of data
  for(i = 0; i < blk_cnt_max; i++) {
    aes128.encryptBlock(&cipher_packet[i * 16], &clear_packet[i * 16]); //cypher->output block and cleartext->input block
  }
}
