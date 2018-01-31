#include <SabertoothSimplified.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#include "Common.h"


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
    bool                  connectionLost;
    COM_CONTROLPARMS_ST   ctlParms;
    COM_SENSORS_DIST_ST   distSensors;
} CONTROLCONTEXT_ST;

    
static CONTROLCONTEXT_ST controlContext = {true, {0, 0}};


static unsigned long previousTime = 0;


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


void setup() {
    // H-Bridge
    Serial3.begin(9600);

    // XBee:
    Serial2.begin(115200);

    // Debug:
    Serial.begin(9600);
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

}

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

void UpdateDisplay(CONTROLCONTEXT_ST & controlContext) {
    // lcd.setCursor(0,3);
    // TODO:: ensure not too much com. to LCD (update every 1 second)
    // lcd.print(controlContext.ctlParms.);
    //
    char line1[41];
    sprintf(line1, "(X:%4d,Y;%4d) sum=%d", 
        controlContext.ctlParms.driveSpeed,
        controlContext.ctlParms.turnPosition,
        0);
    
    lcd.clear();
    lcd.home();
    lcd.print(line1);
    // lcd.print("(X:");
    // lcd.print(controlContext.ctlParms.driveSpeed);
    // lcd.print(",Y:");
    // lcd.print(controlContext.ctlParms.turnPosition);
    // lcd.print(") sum=");
    // lcd.print(PacketsRX[0]);
}


bool updateControlContext(CONTROLCONTEXT_ST * pControlContext) {
    bool anyChangeToStateData=false;
    bool anyKeepAliveStateData=false;

    // Save a copy of the current control parameters:
    COM_CONTROLPARMS_ST absBodyCompleted = pControlContext->ctlParms;

    static bool syncRecv = false;
    static bool hdrReceived = false;
    static COM_HEADER_ST       header;
    static COM_CONTROLPARMS_ST absBody;
    static COM_RELATIVE_ST     relBody;
    static char * buf = (char *) &header;
    static int idx=0;
    static int bodySize=0;

    int hdrSize = sizeof(header);
    int absBodySize = sizeof(absBody);
    int relBodySize = sizeof(relBody);

    // Read a packet:
    while (Serial2.available()) {
        unsigned char ch = Serial2.read();

        char chHexStr[10];
        sprintf(chHexStr, "%02x ", (unsigned char) ch);
        Serial.write(chHexStr);
        delay(1);

        if (!syncRecv) {
            // Check for sync byte:
            if (ch == COM_SYNCPATTERN_8BIT)  {
                syncRecv = true;
                buf[idx++] = ch;
            }
            else {
                // Still waiting for the header byte. Ignore this byte and try read again:
                continue;
            }
        }
        else {
            if (!hdrReceived) {
                // Receiving Header:
                if (idx < hdrSize) {
                    buf[idx++] = ch;
                    if (idx == hdrSize) {
                        Serial.write("\n");
                        // Move on to body;
                        idx = 0;
                        if (header.packetType == COM_PACKETTYPE_STATE) {
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
                pControlContext->ctlParms.turnPosition = 0;
                pControlContext->ctlParms.driveSpeed   = 0;
    
                // Set that data was changed!
                anyChangeToStateData=true;
            }
        }
    }

    return (anyChangeToStateData);
}

bool adjustSpeedAndDirection(INT_8 drive, INT_8 turn) 
{
    bool rv = false;

    // This code doesn't do anything;
    #if 0
    const long interval=100;
    Motor.currentTime = millis();
    Motor.currentDrive = PacketsRX[2];
    Motor.currentTurn = PacketsRX[1];
    if ((Motor.currentTime - Motor.previousTime) >= interval) {
        Motor.previousTime = Motor.currentTime;
        if (PacketsRX[2] > Motor.currentDrive) Motor.currentDrive += 1;
        if (PacketsRX[2] < Motor.currentDrive) Motor.currentDrive -= 1;  
        if (PacketsRX[1] > Motor.currentTurn)  Motor.currentTurn += 1;
        if (PacketsRX[1] < Motor.currentTurn)  Motor.currentTurn -= 1;   
    }
    if (PacketsRX[2] == 0 && Motor.currentDrive != 0) {
        if (Motor.currentDrive > 0) Motor.currentDrive -= 1;
        if (Motor.currentDrive < 0) Motor.currentDrive += 1;
    }
    if (PacketsRX[1] == 0 && Motor.currentTurn != 0) {
        if (Motor.currentTurn > 0) Motor.currentTurn -= 1;
        if (Motor.currentTurn < 0) Motor.currentTurn += 1;
    }
    #endif
    // How it was;
    // ST.drive(Motor.currentDrive);
    // ST.turn(Motor.currentTurn);
    // Serial.print('\n');Serial.print(PacketsRX[1]);Serial.print(',');Serial.print(PacketsRX[2]);Serial.print(',');Serial.print(PacketsRX[3]);Serial.print('*');

    ST.drive(drive);
    ST.turn(turn);

    return (rv);
}

void loop()
{
    static bool pendingDisplay=false;
    currentTime = millis();

    // ----------------------------------------------------------------------
    // Determine target state:
    // ----------------------------------------------------------------------
    bool controlContextUpdated = updateControlContext(&controlContext);
    // bool controlContextUpdated = updateSensorControl(&controlContext);

    // if (controlContextUpdated || sensorContextUpdated)
    //
    if (controlContextUpdated) pendingDisplay = true;

    if (pendingDisplay) {
        if ((currentTime - prevDispTime) > 500) {
            // Debug info...
            char buffer[200];
            sprintf(buffer, "Steering:%d, Throatle:%d\n", controlContext.ctlParms.turnPosition, controlContext.ctlParms.driveSpeed);
            Serial.print(buffer);

            UpdateDisplay(controlContext);
            prevDispTime = currentTime;
            pendingDisplay = false;
        }
    }


    // ----------------------------------------------------------------------
    // Make target and actual line up:
    // NOTE: These functions do not complete action in one call!
    //       They must be called continuously to get to the target.
    //       Also note that the target may change between calls!
    //       It is ok to call them even when no change will ocurr.
    // ----------------------------------------------------------------------
    if (controlContextUpdated) {
        Serial.write("Updating ST\n");
        bool speedChanged = adjustSpeedAndDirection(controlContext.ctlParms.driveSpeed, 
                                                    controlContext.ctlParms.turnPosition);
    }


    // If any change, need to wait for it to take effect:
    // TODO: This is not final. Want to service specific changes when required and not wait for delay 
    //       required by other changes.
    // if (speedChanged || directionChanged) {
        // delay(20);
    // }

    // delay(100);
}

