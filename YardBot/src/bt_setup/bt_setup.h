#ifndef __BT_SETUP__
#define __BT_SETUP__

/*
This is an Arduino HC-06 bluetooth setup library.

Requirements:
 - Developed using 4 pin HC-06 modules. Not tested with 6 pin modules.
 - Arduino UNO with SoftwareSerial btSerial device created.

Featuers:
 - Baud rate scanning to ensure connection is made. Verified using AT commands.
 - Once connected, ensures that the Name, PIN and BAUD rates have been programmed correctly.
 - Handles bluetooth connections that occur during setup to sure reliable communication.
 - Allows for an arbitrary app "keep alive" string to be spcified for connection detected.
 - AT command handling for when \r\n required and when no \r\n required.
 - TBD fast setup doesn't wait longer than needed.
 - AT command retry.

*/


// Return Codes:
typedef enum {
    BT_RC_FAILED,       // No proper HC06 communication detected.
    BT_RC_COMPLETED,    // HC06 found and configured as specified.
    BT_RC_PARTIALCONFIG // HC06 found but may not be configured as specified due to active connection.
} BT_RC_EN;


// ----------------------------------------------------------------------
// bt_setup()
//
// PARAMETERS:
//     name     - must be under 20 chars.
//     pin      - must be 4 numerical digits.
//     baudrate - i.e. recomended 57600. Existing baud will be detected and reconfigured to use this.
//     keepAlivePattern - Used to detect active bluetooth connection to avoid failing AT->OK attempts.
//
// RETURNS: BT_RC_EN - see above codes.
//
// EXAMPLE:
//     #define BLUETOOTH_NAME  "hc06test"
//     #define BLUETOOTH_PIN   "1234"
//     #define BLUETOOTH_SPEED 57600
//     BT_RC_EN bt_rc = bt_setup(BLUETOOTH_NAME, BLUETOOTH_PIN, BLUETOOTH_SPEED, "ka");
// ----------------------------------------------------------------------
BT_RC_EN bt_setup(char * name, char * pin, uint32_t baudrate, char * keepAlivePattern);



#endif // __BT_SETUP__
