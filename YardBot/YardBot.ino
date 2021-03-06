#include <SabertoothSimplified.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#include "Interface.h"



#define ROBOT_NAME "BradsYardBot"

// If you haven't configured your device before use this
//#define BLUETOOTH_SPEED 38400 //This is the default baudrate that HC-05 uses
// If you are modifying your existing configuration, use this:
// #define BLUETOOTH_SPEED 57600
#define BLUETOOTH_SPEED 9600

#include <SoftwareSerial.h>


// Swap RX/TX connections on bluetooth chip
//   Pin 10 --> Bluetooth TX
//   Pin 11 --> Bluetooth RX
SoftwareSerial mySerial(10, 11); // RX, TX

#define CONTROL_TYPE 1 // 0 for hardware controller, 1 for bluetooth.

#if CONTROL_TYPE == 0
    #define CONTROL_SERIAL Serial2
#elif CONTROL_TYPE == 1
    #define CONTROL_SERIAL mySerial
#else
    #error
#endif



// Find your address from I2C Scanner function and add it here:
#define LCD_I2C 0x27
#define LCD_BACKLIGHT_PIN 3
#define LCD_En_pin  2
#define LCD_Rw_pin  1
#define LCD_Rs_pin  0
#define LCD_D4_pin  4
#define LCD_D5_pin  5
#define LCD_D6_pin  6
#define LCD_D7_pin  7

LiquidCrystal_I2C  lcd(LCD_I2C,LCD_En_pin,LCD_Rw_pin,LCD_Rs_pin,LCD_D4_pin,LCD_D5_pin,LCD_D6_pin,LCD_D7_pin);

unsigned long currentTime,timeOfLastGoodPacket = 0,timeOfLastTTL = 0,timeOfLastStatus=0;
unsigned long prevDispTime = 0;

typedef struct {
    bool                   connectionLost;
    COM_FEATURE_DRIVE_ST   ctlParms;
} CONTROLCONTEXT_ST;

    
static CONTROLCONTEXT_ST controlContext = {true, { {0, 0 }, 0}};


static unsigned long previousTime = 0;

typedef struct {
    unsigned long currentTime = 0;
    unsigned long previousTime = millis();
    int currentDrive = 0;
    int currentTurn = 0;
} MOTORSTATE_ST;
 
// MOTORSTATE_ST Motor = {0, 0, 0, 0};
MOTORSTATE_ST Motor;

//----------------------------------------------------------------------
// Serial Port Usage
//----------------------------------------------------------------------
// Serial(TX0, RX0) - Adruino Serial Monitor.
//----------------------------------------------------------------------
// Serial1(TX1, RX1) - ?
//----------------------------------------------------------------------
// Serial2(TX2, RX2) - XBees
//----------------------------------------------------------------------
// Serial3(TX3, RX3) - Sabertooth H-Bridge
// Connections to make:
//   Arduino TX->13  ->  Sabertooth S1
//   Arduino GND     ->  Sabertooth 0V
//   Arduino VIN     ->  Sabertooth 5V (OPTIONAL, if you want the Sabertooth to power the Arduino)
// For how to configure the Sabertooth, see the DIP Switch Wizard for
//   http://www.dimensionengineering.com/datasheets/SabertoothDIPWizard/start.htm
//   http://www.dimensionengineering.com/datasheets/SabertoothDIPWizard/nonlithium/serial/simple/single.htm
//----------------------------------------------------------------------
SabertoothSimplified ST(Serial3); 


void waitForResponse() {
    delay(1000);
    while (mySerial.available()) {
      Serial.write(mySerial.read());
    }
    Serial.write("\n");
}



void setup() {
    // H-Bridge
    Serial3.begin(9600);

    // XBee:
    #if CONTROL_TYPE == 0
        Serial2.begin(115200);
    #endif

    // Debug:
    Serial.begin(115200);
    Serial.println("Starting!!!");

    for (int i=0; i<8; i++) {
        pinMode(i+22,OUTPUT);
    }
    lcd.begin (20,4);
    lcd.setBacklightPin(LCD_BACKLIGHT_PIN,POSITIVE);
    lcd.setBacklight(HIGH);
    lcd.home (); // go home
    lcd.print("Robo Plow     ver4.4+");
    lcd.setCursor(0,1);
    lcd.print("Initializing..."); 
    delay(3000);
    allStop(); 
    lcdReset();
    Serial.println("LCD Reset!!!");

    previousTime = millis();

    #if CONTROL_TYPE == 1
        setup_bluetooth();
    #endif
}

#if CONTROL_TYPE == 1
void setup_bluetooth() {

    Serial.println("Starting config");
    mySerial.begin(BLUETOOTH_SPEED);
    delay(1000);

    // Should respond with OK
    mySerial.print("AT\r\n");
    waitForResponse();

    // Should respond with its version
    mySerial.print("AT+VERSION\r\n");
    waitForResponse();

    // Set pin to 0000
    mySerial.print("AT+PSWD=0000\r\n");
    waitForResponse();

    // Set the name to ROBOT_NAME
    String rnc = String("AT+NAME=") + String(ROBOT_NAME) + String("\r\n"); 
    mySerial.print(rnc);
    waitForResponse();

    // Set baudrate to 57600
    mySerial.print("AT+UART=57600,0,0\r\n");
    waitForResponse();

    Serial.println("Done!");
}
#endif



void lcdReset() {
    lcd.clear();
    lcd.home();
    // lcd.print("(X:");lcd.print(PacketsRX[1]);lcd.print(",Y:");lcd.print(PacketsRX[2]);lcd.print(") sum=");lcd.print(PacketsRX[0]);
}

// The Sabertooth won't act on mixed mode until
// it has received power levels for BOTH throttle and turning, since it
// mixes the two together to get diff-drive power levels for both motors.
// Stop robot if no good PacketsRX received within 200th of a seconds
void allStop() {
    // So, we set both to zero initially.
    // ST.drive(0);
    // ST.turn(0);
    // Serial.print('\n'); Serial.write("All STOP +++++");
    lcd.setCursor(12,3);
    lcd.print("All Stop");
    // PacketsRX[1]=0;
    // PacketsRX[2]=0;
    // PacketsRX[3]=0;
    // PacketsRX[4]=0;
}

// void timeout() {
    // if (currentTime > (timeOfLastGoodPacket + 500)) {
        // allStop();
        // timeOfLastGoodPacket = currentTime;
    // }
// 
    // // Send stay alive command to receiver every COM_MAX_SIGNAL_LOST_TIME_IN_MS (200ms):
    // if (currentTime > (timeOfLastTTL + COM_MAX_SIGNAL_LOST_TIME_IN_MS)) {
        // lcdReset();
        // timeOfLastTTL=currentTime;
    // }
// }

void UpdateDisplay(CONTROLCONTEXT_ST & controlContext, MOTORSTATE_ST & motor) {
    // lcd.setCursor(0,3);
    // TODO:: ensure not too much com. to LCD (update every 1 second)
    // lcd.print(controlContext.ctlParms.);
    //
    char line1[41];
    sprintf(line1, "Target:D:%4d,S:%4d Motor:D:%4d,S:%4d", 
        controlContext.ctlParms.driveParms.driveSpeed,
        controlContext.ctlParms.driveParms.turnPosition,
        motor.currentDrive,
        motor.currentTurn);
    
    lcd.clear();
    lcd.home();
    lcd.print(line1);
}


bool updateControlContext(CONTROLCONTEXT_ST * pControlContext) {
    bool anyChangeToStateData=false;
    bool anyKeepAliveStateData=false;

    // Save a copy of the current control parameters:
    COM_FEATURE_DRIVE_ST absBodyCompleted = pControlContext->ctlParms;

    static bool syncRecv = false;
    static bool hdrReceived = false;
    static COM_HEADER_ST       header;
    static COM_FEATURE_DRIVE_ST absBody;
    // static COM_RELATIVE_ST     relBody;
    static char * buf = (char *) &header;
    static int idx=0;
    static int bodySize=0;

    int hdrSize = sizeof(header);
    int absBodySize = sizeof(absBody);
    // int relBodySize = sizeof(relBody);

    // Read a packet:
    while (CONTROL_SERIAL.available()) {
        unsigned char ch = CONTROL_SERIAL.read();

        char chHexStr[10];
        sprintf(chHexStr, "%02x ", (unsigned char) ch);
        Serial.write(chHexStr);
        // delay(1);

        if (!syncRecv) {
            // Check for sync byte:
            if (ch == COM_SYNCPATTERN_8BIT)  {
                syncRecv = true;
                buf[idx++] = ch;
            }
            else {
                // Still waiting for the header byte. Ignore this byte and try read again:
                Serial.write("\n");
                continue;
            }
        }
        else {
            if (!hdrReceived) {
                // Receiving Header:
                if (idx < hdrSize) {
                    buf[idx++] = ch;
                    if (idx == hdrSize) {
                        // Serial.write("\n");
                        // Move on to body;
                        idx = 0;
                        if ((header.featureId == COM_FEATURE_DRIVE) 
                         && (header.packetType == COM_PACKETTYPE_STATE)) {
                            hdrReceived = true;
                            buf = (char *) &absBody;
                            bodySize = absBodySize;
                        }
                        // else if (header.packetType == COM_PACKETTYPE_RELATIVE) {
                            // buf = (char *) &relBody;
                            // bodySize = relBodySize;
                        // }
                        else {
                            // Invalid body type. Resume looking for sync.
                            // TODO: First see if sync re-occurd already in header and shift if so.
                            //       Do same for body later if checksum fails? Probably need better way.
                            buf = (char *) &header;
                            syncRecv = false;
                            Serial.write("\n");
                            continue;
                        }
                    }
                }
            }
            else {
                // Receiving Body:
                if (idx < bodySize) {
                    buf[idx++] = ch;
                    if (idx == bodySize) {
                        Serial.write("\n");
                        // Whole packet recieved! Verify Checksum/CRC:
                        // TODO:
                        
                        // 
                        if (header.packetType == COM_PACKETTYPE_STATE) {
                            // - Update context:
                            if (memcmp((char *)&absBodyCompleted, (char *)&absBody, absBodySize) != 0) {
                                absBodyCompleted = absBody;
                                // Serial.print("Saving iterim body\n");
                                anyChangeToStateData=true;
                            }
                            anyKeepAliveStateData = true;
                        }
                        // else if (header.packetType == COM_PACKETTYPE_RELATIVE) {
                            // // Record change for later exectuion...
                            // Send ack:
                        // }

                        // Continue reading as there may be a subsequent state packet that overwrites.
                        // Reset packet state vars:
                        hdrReceived = false;
                        buf = (char *) &header;
                        idx = 0;
                        syncRecv = false;
                    }
                }
            }
        }
    }

    unsigned long curTime = millis();

    if (anyKeepAliveStateData) {
        // Serial.write("HERE\n");
        pControlContext->connectionLost = false;
        previousTime = curTime;

        if (memcmp((char *)&(pControlContext->ctlParms), (char *)&absBodyCompleted, absBodySize) != 0) {
            pControlContext->ctlParms = absBodyCompleted;
            anyChangeToStateData=true;
            // Serial.print("Saving body");
        }
        else {
            anyChangeToStateData=false;
        }
    }
    else {
        // Check for keepalive:

        // If connection was not prevsiously noted as lost:
        if (!(pControlContext->connectionLost)) {
            // If its been too long since a control packet was received:
            if ((curTime - previousTime) > COM_MAX_SIGNAL_LOST_TIME_IN_MS) {
                // char buffer[300];
                // sprintf(buffer, "prevTime:%lu, curTime:%lu\n", previousTime, curTime);
                // Serial.println(buffer);

                Serial.println("Connection Lost!");
                // ALL STOP
                pControlContext->connectionLost = true;

                // Reset Parameters!
                pControlContext->ctlParms.driveParms.turnPosition = 0;
                pControlContext->ctlParms.driveParms.driveSpeed   = 0;
    
                // Set that data was changed!
                anyChangeToStateData=true;

                syncRecv = false;
                Serial.write("\n");
            }
        }
    }

    return (anyChangeToStateData);
}

 

bool adjustSpeedAndDirection(int8_t targetDrive, int8_t targetTurn, int8_t halt, bool connectionLost) 
{
    bool rv = false;
    long interval=50;

    if (connectionLost) halt = 2;

    if (halt > 0) {
        targetDrive = 0;
        targetTurn = 0;
        if (halt == 1) interval=1;
        if (halt == 2) interval=5;
    }

    Motor.currentTime = millis();
    if ((Motor.currentTime - Motor.previousTime) >= interval) {
        if (targetDrive > Motor.currentDrive) Motor.currentDrive += 1;
        if (targetDrive < Motor.currentDrive) Motor.currentDrive -= 1;  
        if (targetTurn > Motor.currentTurn)  Motor.currentTurn += 1;
        if (targetTurn < Motor.currentTurn)  Motor.currentTurn -= 1;   

        Motor.previousTime = Motor.currentTime;
    }

    // Serial.print('\n');Serial.print(PacketsRX[1]);Serial.print(',');Serial.print(PacketsRX[2]);Serial.print(',');Serial.print(PacketsRX[3]);Serial.print('*');

    ST.drive(Motor.currentDrive);
    ST.turn(Motor.currentTurn);

    return (rv);
}

void loop()
{
    static bool pendingDisplay=true; // start with an update to the display...
    currentTime = millis();

    // ----------------------------------------------------------------------
    // Determine target state:
    // ----------------------------------------------------------------------
    bool controlContextUpdated = updateControlContext(&controlContext);
    // bool controlContextUpdated = updateSensorControl(&controlContext);

    // if (controlContextUpdated || sensorContextUpdated)
    //
    if (controlContextUpdated) pendingDisplay = true;

    if ((currentTime - prevDispTime) > 400) {
        if (pendingDisplay) {
            // Debug info...
            char buffer[200];
            sprintf(buffer, "Steering:%d, Throatle:%d\n", controlContext.ctlParms.driveParms.turnPosition, controlContext.ctlParms.driveParms.driveSpeed);
            Serial.print(buffer);
            pendingDisplay = false;
        }

        UpdateDisplay(controlContext, Motor);
        prevDispTime = currentTime;
    }


    // ----------------------------------------------------------------------
    // Make target and actual line up:
    // NOTE: These functions do not complete action in one call!
    //       They must be called continuously to get to the target.
    //       Also note that the target may change between calls!
    //       It is ok to call them even when no change will ocurr.
    // ----------------------------------------------------------------------
    // if (controlContextUpdated) {
        // Serial.write("Updating ST\n");
        bool speedChanged = adjustSpeedAndDirection(controlContext.ctlParms.driveParms.driveSpeed, 
                                                    controlContext.ctlParms.driveParms.turnPosition,
                                                    controlContext.ctlParms.halt,
                                                    controlContext.connectionLost);
    // }


    // If any change, need to wait for it to take effect:
    // TODO: This is not final. Want to service specific changes when required and not wait for delay 
    //       required by other changes.
    // if (speedChanged || directionChanged) {
        // delay(20);
    // }

    // delay(100);
}

