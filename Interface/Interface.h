
#include "Interface_Drive.h"


// COM_PROTOCOL_VERSION
// The protocol version is used on every packet sent to verify both sides speek the same language.
// Devices should attempt to work with lower protocol versions where possible.
#define COM_PROTOCOL_VERSION 0


// Interface Defines;
#define COM_STATE_KEEP_ALIVE_TIME_IN_MS  100
#define COM_MAX_SIGNAL_LOST_TIME_IN_MS   (COM_STATE_KEEP_ALIVE_TIME_IN_MS*3)

// Sync Codes:
// #define 32BIT_COM_SYNCPATTERN 0xD42E0F75
#define COM_SYNCPATTERN_8BIT  0xD3


// Packet type defines:
typedef enum {
    COM_FEATURE_DRIVE,
    COM_FEATURE_ARM,
    COM_FEATURE_RELAYS,
    COM_FEATURE_AUTO,
    COM_FEATURE_SENSORS
} COM_FEATURE_EN;

// Packet type defines:
typedef enum {
    COM_PACKETTYPE_STATE,      // Absolute target state.
    COM_PACKETTYPE_RELATIVE,   // Relative parameters like up/down, etc.
    COM_PACKETTYPE_REL_ACK,    // Acknowledgement for relative parameters. Sender retries if not received.
    COM_PACKETTYPE_STATUS      // Status info sent from robot to controllers.
} COM_PACKETTYPE_EN;

// ----------------------------------------------------------------------
// Relative Control Structs:
// ----------------------------------------------------------------------

// Relative control header and acknowledgement struct;
// Used in relative control struct and for packet type: COM_PACKETTYPE_REL_ACK
typedef struct {
    uint8_t   sequenceNumber;  // Number in the sequence - used for controller acknowledgement.
                               // Loops to 0 after 255. Global accross all parameters.
    uint16_t  parmToControl;   // Which parameter is being controlled.
} COM_RELATIVE_HEADER_ST;

// Relative control struct;
// Used packet type: COM_PACKETTYPE_RELATIVE
typedef struct {
    COM_RELATIVE_HEADER_ST  header;
    uint32_t                value;   // Parameter specific value. TODO: Consider more structure here...
} COM_RELATIVE_ST;


// ----------------------------------------------------------------------
// Actual structures sent with sync, protocol version, and checksum:
// ----------------------------------------------------------------------
// What gets sent is header+payload.
// Header  - The structure below.
// Payload - One of:
//              COM_CONTROLPARMS_ST
//              COM_STATUSPARMS_ST
//              COM_RELATIVE_ST
typedef struct {
    uint8_t               sync=COM_SYNCPATTERN_8BIT;
    uint8_t               protocolVersion=COM_PROTOCOL_VERSION;
    COM_FEATURE_EN        featureId;        // Indicates the payload struct used. i.e. how many more bytes to read.
    COM_PACKETTYPE_EN     packetType;       // Indicates the payload struct used. i.e. how many more bytes to read.
    uint8_t               checksum;         // Over everthing including header and payload.
} COM_HEADER_ST;

