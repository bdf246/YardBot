// COM_PROTOCOL_VERSION
// The protocol version is used on every packet sent to verify both sides speek the same language.
// Devices should attempt to work with lower protocol versions where possible.
#define COM_PROTOCOL_VERSION 0


// Interface Defines;
#define COM_STATE_KEEP_ALIVE_TIME_IN_MS  100
#define COM_MAX_SIGNAL_LOST_TIME_IN_MS   (COM_STATE_KEEP_ALIVE_TIME_IN_MS*10)

// Sync Codes:
// #define 32BIT_COM_SYNCPATTERN 0xD42E0F75
#define COM_SYNCPATTERN_8BIT  0xD3


// Packet type defines:
typedef enum {
    COM_PACKETTYPE_STATE,      // Absolute target state.
    COM_PACKETTYPE_RELATIVE,   // Relative parameters like up/down, etc.
    COM_PACKETTYPE_REL_ACK,    // Acknowledgement for relative parameters. Sender retries if not received.
    COM_PACKETTYPE_STATUS      // Status info sent from robot to controllers.
} COM_PACKETTYPE_EN;

// For relavtive packet types, parameters to control:
typedef enum {
    COM_PACKETTYPE_RELATIVE_PARAM1,
    COM_PACKETTYPE_RELATIVE_PARAM2
} COM_PACKETTYPE_RELATIVE_EN;

// ----------------------------------------------------------------------
// Control Struct:
// ----------------------------------------------------------------------

// Control State;
// Used packet type: COM_PACKETTYPE_STATE
// NOTE: This control structure fully represnents the current state.
//       To clarify further, it represents the entire current state that CAN be represented.
//       Parameters for which a robot is not able to take an absolute value for are not included in this structure.
//       Because this structure represents the current state, it is sent continuously by controllers as part of the
//       keep alive mechanism. i.e. it is ok to loose a packet and never process it as we know another one will come along
//       shortly after. If the stream of this packet stops, then the robot will reset to safe values until such time
//       that this structure is received again. Like default values, those safe values are not defined in this interface.
//       The keep alive rate is defined in this interface as: COM_STATE_KEEP_ALIVE_TIME_IN_MS
typedef struct {
    int8_t      driveSpeed;       // -127 to +127
    int8_t      turnPosition;     // -127 to +127
    uint8_t     armPosition;      // 0 to 255 ; Uses 2 relays with sensor to achive position.
    uint8_t     headRotation;     // 0 to 255 ; Uses 2 relays with sensor to achive position.
    uint8_t     relayBitmask;     // 0-off 1-on ; Robot specific relays. Includes relays controlled by internal sensor 
                                 // (i.e. arm). however these values are ignored by robot for the purpose of control.
                                 // They are however useful as status back to the controller for debug.
} COM_CONTROLPARMS_ST;

// ----------------------------------------------------------------------
// Relative Control Structs:
// ----------------------------------------------------------------------

// Relative control header and acknowledgement struct;
// Used in relative control struct and for packet type: COM_PACKETTYPE_REL_ACK
typedef struct {
    uint8_t                      sequenceNumber;  // Number in the sequence - used for controller acknowledgement.
                                                 // Loops to 0 after 255. Global accross all parameters.
    COM_PACKETTYPE_RELATIVE_EN  parmToControl;   // Which parameter is being controlled.
} COM_RELATIVE_HEADER_ST;

// Relative control struct;
// Used packet type: COM_PACKETTYPE_RELATIVE
typedef struct {
    COM_RELATIVE_HEADER_ST  header;
    uint8_t                  value;   // Parameter specific value. TODO: Consider more structure here...
} COM_RELATIVE_ST;

// ----------------------------------------------------------------------
// Status Structs:
// ----------------------------------------------------------------------

// Ultrasonic sensors placed on each corner of the robot:
typedef struct {
    uint8_t     frontRight;
    uint8_t     frontLeft;
    uint8_t     backRight;
    uint8_t     backLeft;
} COM_SENSORS_DIST_ST;

// Power usage. i.e. Sabertooth 24V, INA219 values read for 12V actuator circuit.
// TODO - refine this struct!
typedef struct {
    uint8_t     current;
    uint8_t     voltage;
    uint8_t     batteryLevel_24V;
} COM_SENSORS_POWER_ST;

// Status:
// Used packet type: COM_PACKETTYPE_STATUS
typedef struct {
    COM_CONTROLPARMS_ST   ctlState;        
    // ctlState; This is exactly as last sent by any controller. Required for controller startup and when multiple controllers 
    //           are connected. Note that relay values for sensor controlled actuators are not modified from what the controller 
    //           sent. For actual relay status, see the ctlStatus paramater below.
    COM_CONTROLPARMS_ST   ctlStatus;       
    // ctlStatus; This is not necessarily the same values sent to the robot, rather, it is the actual current state the robot has 
    //            been able to implement. For example, speed may be set to 100 but the robot doesn't immediately try to go 100; 
    //            rather it starts slower and gradually increases to the requested speed.
    COM_SENSORS_DIST_ST   distSensors;
    COM_SENSORS_POWER_ST  powerSensors;
} COM_STATUSPARMS_ST;



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
    COM_PACKETTYPE_EN     packetType;       // Indicates the payload struct used. i.e. how many more bytes to read.
    uint8_t               checksum;         // Over everthing including header and payload.
} COM_HEADER_ST;

