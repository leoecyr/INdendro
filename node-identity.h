#define DEVICE_ID 0x0000

// Device info
#define DEVICE_NAME "INdendro"
#define DEVICE_TYPE "Pro-mini"
#define DEVICE_VOLTAGE 3.3
#define DEVICE_FREQ 8
#define DEVICE_VERSION "INdendro Pro-mini 3.3v 8MHz dendrometer"

// Encryption setup
//
// Change this to increase data integrity and security
//
//  If this node is to work on an existing network, use the key for that network
//
// 16 byte (128bit) encryption key
const byte key[16]={
  0x00, 0x00, 0x00, 0x00,
  0x04, 0x04, 0x04, 0x04,
  0x08, 0x08, 0x08, 0x08,
  0xff, 0xff, 0xff, 0xff
  };
  
//  TODO: Change this to match your network
//
// This is used as a casual way to filter devices with the same encryption keys
//
// Since packets are encrypted, it is about a 1/2^16 probability of accidental data pollution
//  and then, the data injected will not consisten
#define DEVICE_FILTER_ID 0x0000     //FIXME CHECKSUM or other solution
