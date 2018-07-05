#include <SoftwareSerial.h>
#include "bt_setup.h"

#define BLUETOOTH_NAME  "hc06test"
#define BLUETOOTH_PIN   "1234"
#define BLUETOOTH_SPEED 57600

// Swap RX/TX connections on bluetooth chip
//   Pin 10 --> Bluetooth TX
//   Pin 11 --> Bluetooth RX
SoftwareSerial btSerial(10, 9); // RX, TX


//////////////////////////////////////////////////////////////////////
// Setup:
//////////////////////////////////////////////////////////////////////
void setup() {
    Serial.begin(9600); // begin serial communitication  
    Serial.println("");
    Serial.println("Application Start!");
    Serial.println("");

    BT_RC_EN bt_rc = bt_setup(BLUETOOTH_NAME, BLUETOOTH_PIN, BLUETOOTH_SPEED, "ka");
    // if (!btrc)
}

//////////////////////////////////////////////////////////////////////
// Run-Time
//////////////////////////////////////////////////////////////////////
void loop() {
    static uint8_t numChars=0;

    if (Serial.available()) {
        btSerial.write(Serial.read());
    }
    if (btSerial.available()) {
        Serial.write(btSerial.read());

        if (numChars++ > 60) {
            Serial.write("\r\n    ");
            numChars=0;
        }
    }

    return;
}

