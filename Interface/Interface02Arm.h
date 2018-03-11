// COM_PROTOCOL_VERSION
// The protocol version is used on every packet sent to verify both sides speek the same language.
// Devices should attempt to work with lower protocol versions where possible.
#define COM_PROTOCOL_ARM_VERSION 0


// ----------------------------------------------------------------------
// Control Struct:
// ----------------------------------------------------------------------

// Control State;
// Used packet type: COM_PACKETTYPE_STATE
// NOTE: This control structure fully represnents the current state of the driveSpeed and turnPosition.
typedef struct {
    int8_t      armPosition;       // -127 to +127
} COM_FEATURE_ARM_PARMS_ST;

typedef struct {
    COM_FEATURE_ARM_PARMS_ST armParms;
    int8_t                   halt;   // non-zero means STOP !!!! - overrides drive and turn.
                                     // 1 - Immediate E-Stop - may jerk/damage system.
                                     // 2 - Fast stop - Quick but smooth stop. (<500ms) 
                                     //   - THis is equivent to lost communication.
} COM_FEATURE_ARM_ST;

// ----------------------------------------------------------------------
// Status Structs:
// ----------------------------------------------------------------------

// Status:
// Used packet type: COM_PACKETTYPE_STATUS
typedef struct {
    COM_FEATURE_ARM_PARMS_ST ctlStatus;       
    // ctlStatus; This is not necessarily the same values sent to the robot, rather, it is the actual current state the robot has 
    //            been able to implement. For example, speed may be set to 100 but the robot doesn't immediately try to go 100; 
    //            rather it starts slower and gradually increases to the requested speed.
} COM_FEATURE_ARM_STATUS_ST;


