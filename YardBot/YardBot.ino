#include <SoftwareSerial.h>

#include <SabertoothSimplified.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#include "Interface.h"
#include "src/bt_setup/bt_setup.h"

#define BLUETOOTH_NAME  "YardBot"
#define BLUETOOTH_PIN   "1234"
#define BLUETOOTH_SPEED 57600

// Swap RX/TX connections on bluetooth chip
//   Pin 10 --> Bluetooth TX
//   Pin 11 --> Bluetooth RX
SoftwareSerial btSerial(10, 11); // RX, TX

#define CONTROL_TYPE 1 // 0 for hardware controller, 1 for bluetooth.

#if CONTROL_TYPE == 0
    #define CONTROL_SERIAL Serial2
#elif CONTROL_TYPE == 1
    #define CONTROL_SERIAL btSerial
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

const int MAX_TIME_FOR_KEEPALIVE_IN_MS = 400;

typedef struct {
    int steering; // -100 (left) to 0 (no turn) to 100 (right)
    int throatle; // -100 (reverse) to 0 (stopped) to 100 (forward)
} PARAMS_RC_ST;

typedef struct {
    bool           connectionLost;
    PARAMS_RC_ST   rcParams;
} CONTROLCONTEXT_ST;
// typedef struct {
    // bool                   connectionLost[COM_FEATURE_EN_N];
    // COM_FEATURE_DRIVE_ST   driveParms;
    // COM_FEATURE_ARM_ST     armParms;
// } CONTROLCONTEXT_ST;

static CONTROLCONTEXT_ST controlContext = {true, {0, 0}};
// static CONTROLCONTEXT_ST controlContext = {{true, true}, { {0, 0 }, 0}};

// For keep alive:
unsigned long previousTime = 0;
// static unsigned long previousTime[COM_FEATURE_EN_N] = {0, 0};


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



void setup() {
    // H-Bridge
    Serial3.begin(9600);

    // XBee:
    #if CONTROL_TYPE == 0
        Serial2.begin(115200);
    #endif

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
    DispalyInit();
    Serial.println("LCD Reset!!!");

    previousTime = millis();
    // for (int i=0; i < COM_FEATURE_EN_N; i++) {
        // previousTime[i] = millis();
    // }

    #if CONTROL_TYPE == 1
    BT_RC_EN bt_rc = bt_setup(BLUETOOTH_NAME, BLUETOOTH_PIN, BLUETOOTH_SPEED, "ka");
    #endif
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

void DispalyInit() {
    // lcd.setCursor(0,3);
    // TODO:: ensure not too much com. to LCD (update every 1 second)
    // lcd.print(controlContext.cdriveParms.);
    //
    
    lcd.clear();
    lcd.home();

    char line[100];

    lcd.setCursor(0,0);
    sprintf(line, "Target:D:    ,S:    ");
    lcd.print(line);

    lcd.setCursor(0,1);
    sprintf(line, " Motor:D:    ,S:    ");
    lcd.print(line);

    lcd.setCursor(0,2);
    sprintf(line, "ArmPos:    ");
    lcd.print(line);

}

void UpdateDisplay(CONTROLCONTEXT_ST & controlContext, MOTORSTATE_ST & motor) {
    // lcd.setCursor(0,3);
    // TODO:: ensure not too much com. to LCD (update every 1 second)
    // lcd.print(controlContext.cdriveParms.);
    //

    char item[10];

    lcd.setCursor(9, 0);
    sprintf(item, "%4d", controlContext.rcParams.throatle);
    lcd.print(item);
    lcd.setCursor(16, 0);
    sprintf(item, "%4d", controlContext.rcParams.steering);
    lcd.print(item);

    lcd.setCursor(9, 1);
    sprintf(item, "%4d", convFromByte(motor.currentDrive));
    lcd.print(item);
    lcd.setCursor(16, 1);
    sprintf(item, "%4d", convFromByte(motor.currentTurn));
    lcd.print(item);

    // lcd.setCursor(9, 2);
    // sprintf(item, "%4d", controlContext.armParms.armParms.armPosition);
    // lcd.print(item);

}

#define STR_LEN 10
bool updateControlContext(CONTROLCONTEXT_ST * pControlContext) {
    bool anyData=false;
    char str[STR_LEN];
    char curSteeringStr[STR_LEN] = "";
    char curThroatleStr[STR_LEN] = "";
    char curModeStr[STR_LEN] = "";
    char curMaxspeedStr[STR_LEN] = "";
    char curMinspeedStr[STR_LEN] = "";
    str[0] = '\0';
    int idx = 0;
    // Serial.println("E");
    while (btSerial.available()) {
        // Serial.println("Ei");
        if (idx < (STR_LEN-1)) {
            char ch = btSerial.read();
            Serial.write(ch);
            if (ch == ' ' || (idx == (STR_LEN-1))) {
                idx = 0;
                str[0] = '\0';
            }
            else {
                str[idx++] = ch;
                str[idx] = '\0';
                if (str[0] == 's') {
                    strncpy(curSteeringStr, str, STR_LEN);
                }
                else if (str[0] == 't') {
                    strncpy(curThroatleStr, str, STR_LEN);
                }
                // else if (str[0] == 'm') {
                    // strncpy(curModeStr, str, STR_LEN);
                // }
                // else if (str[0] == 'a') {
                    // strncpy(curMaxspeedStr, str, STR_LEN);
                // }
                // else if (str[0] == 'i') {
                    // strncpy(curMinspeedStr, str, STR_LEN);
                // }
            }
        }
        anyData=true;
    }
    // Serial.println("Eo");

    unsigned long curTime = millis();

    if (anyData) {
        pControlContext->connectionLost = false;
        previousTime = curTime;

        if (pControlContext) {
            if (curSteeringStr[0] != '\0')       pControlContext->rcParams.steering = (atoi(&(curSteeringStr[1])) - 50)*2;
            if (curThroatleStr[0] != '\0')       pControlContext->rcParams.throatle = (atoi(&(curThroatleStr[1])) - 50)*2;
            // if (strcmp(&(curModeStr[1]), "RC") == 0)   pControlContext->mode = CONTROLMODE_RC;
            // if (strcmp(&(curModeStr[1]), "AUTO") == 0) pControlContext->mode = CONTROLMODE_AUTO;
            // if (curMaxspeedStr[0] != '\0')       pControlContext->autoParams.maxSpeed = atoi(&(curMaxspeedStr[1]));
            // if (curMinspeedStr[0] != '\0')       pControlContext->autoParams.minSpeed = atoi(&(curMinspeedStr[1]));
        }
    }
    else {
        // Serial.print(".");
        // Check for keepalive:
        //
        // If connection was not prevsiously noted as lost:
        if (!(pControlContext->connectionLost)) {
            // char buffer[100];
            // sprintf(buffer, "prevTime:%lu, curTime:%lu", previousTime, curTime);
            // Serial.println(buffer);

            // If its been too long since a control packet was received:
            if ((curTime - previousTime) > MAX_TIME_FOR_KEEPALIVE_IN_MS) {
                Serial.println("Connection Lost!");
                // ALL STOP
                pControlContext->connectionLost = true;
                pControlContext->rcParams.steering = 0;
                pControlContext->rcParams.throatle = 0;
    
                // Set that data was changed!
                anyData=true;
            }
        }
    }


    return (anyData);
}

#if 0
bool updateControlContext(CONTROLCONTEXT_ST * pControlContext) {
    bool anyChangeToStateData[COM_FEATURE_EN_N];
    bool anyKeepAliveStateData[COM_FEATURE_EN_N];

    for (int i=0; i < COM_FEATURE_EN_N; i++) {
        anyChangeToStateData[i] = false;
        anyKeepAliveStateData[i] = false;
    }

    // Save a copy of the current control parameters:
    COM_FEATURE_DRIVE_ST updatedDriveParms = pControlContext->driveParms;
    COM_FEATURE_ARM_ST   updatedArmParms   = pControlContext->armParms;


    static bool syncRecv = false;
    static bool hdrReceived = false;
    static COM_HEADER_ST       header;
    static COM_FEATURE_DRIVE_ST driveParmsBody;
    static COM_FEATURE_ARM_ST   armParmsBody;
    static char * bufPtrs[COM_FEATURE_EN_N] = {(char*) &driveParmsBody, (char*) &armParmsBody};
    static int parmStructSizes[COM_FEATURE_EN_N] = {sizeof(driveParmsBody), sizeof(armParmsBody)};
    // static COM_RELATIVE_ST     relBody;
    static char * buf = (char *) &header;
    static int idx=0;
    static int bodySize=0;

    int hdrSize = sizeof(header);
    // int relBodySize = sizeof(relBody);

    char * orgParms[COM_FEATURE_EN_N] = {(char*)&(pControlContext->driveParms), (char*)&(pControlContext->armParms)};
    char * updatedParms[COM_FEATURE_EN_N] = {(char*)&updatedDriveParms, (char*)&updatedArmParms};

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
                        if (header.packetType == COM_PACKETTYPE_STATE) {
                            for (int i=0; i < COM_FEATURE_EN_N; i++) {
                                if (header.featureId == i) {
                                    hdrReceived = true;
                                    buf = bufPtrs[i];
                                    bodySize = parmStructSizes[i];
                                }
                            }
                        }
                        // else if (header.packetType == COM_PACKETTYPE_RELATIVE) {
                            // buf = (char *) &relBody;
                            // bodySize = relBodySize;
                        // }
                        //

                        if (!hdrReceived) {
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
                            for (int i=0; i < COM_FEATURE_EN_N; i++) {
                                if (header.featureId == i) {
                                    // - Update context:
                                    if (memcmp(updatedParms[i], bufPtrs[i], parmStructSizes[i]) != 0) {
                                        memcpy(updatedParms[i], bufPtrs[i], parmStructSizes[i]);
                                        // updatedDriveParms = driveParmsBody;
                                        // Serial.print("Saving iterim body\n");
                                        anyChangeToStateData[i]=true;
                                    }
                                    anyKeepAliveStateData[i] = true;
                                }
                            }
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

    // No more data coming in... use what we got:
    // For each feature, copy its data if received, else check for timeout.

    unsigned long curTime = millis();

    for (int i=0; i < COM_FEATURE_EN_N; i++) {
        if (anyKeepAliveStateData[i]) {
            // Serial.write("HERE\n");
            pControlContext->connectionLost[i] = false;
            previousTime[i] = curTime;

            // TODO carry on from this point...
            if (memcmp(orgParms[i], updatedParms[i], parmStructSizes[i]) != 0) {
                memcpy(orgParms[i], updatedParms[i], parmStructSizes[i]);
                anyChangeToStateData[i]=true;
            }
            else {
                anyChangeToStateData[i]=false;
            }
        }
        else {
            // Check for keepalive:
    
            // If connection was not prevsiously noted as lost:
            if (!(pControlContext->connectionLost[i])) {
                // If its been too long since a control packet was received:
                if ((curTime - previousTime[i]) > COM_MAX_SIGNAL_LOST_TIME_IN_MS) {
                    // char buffer[300];
                    // sprintf(buffer, "prevTime:%lu, curTime:%lu\n", previousTime, curTime);
                    // Serial.println(buffer);
    
                    Serial.println("Connection Lost!");
                    // ALL STOP
                    pControlContext->connectionLost[i] = true;
    
                    // Reset Parameters!
                    // TODO - how to remove this?
                    if (i == COM_FEATURE_DRIVE) {
                        pControlContext->driveParms.driveParms.turnPosition = 0;
                        pControlContext->driveParms.driveParms.driveSpeed   = 0;
                    }
        
                    // Set that data was changed!
                    anyChangeToStateData[i]=true;
    
                    // Removed resetting of sync as we could be in the process of receiving a packet;
                    // syncRecv = false;
                    // Serial.write("\n");
                }
            }
        }
    }

    bool anyChangeToStateData_all = false;
    for (int i=0; i < COM_FEATURE_EN_N; i++) {
        if (anyChangeToStateData[i]) anyChangeToStateData_all = true;
    }

    return (anyChangeToStateData_all);
}
#endif 

 

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

// Takes -127 to 127 range and converts to -100 to +100:
int8_t convFromByte(int val)
{
    int32_t mult = ((int32_t) val)*100;
    int32_t newVal = mult/127;

    // Rounding:
    if      ((mult%127) >  63) newVal++;
    else if ((mult%127) < -63) newVal--;

    // Ensure valid range:
    if      (newVal >  100) newVal =  100;
    else if (newVal < -100) newVal = -100;

    return (newVal);
}

// Takes -100 to 100 range and converts to -127 to +127:
int8_t convToByte(int val)
{
    int32_t mult = ((int32_t) val)*127;
    int32_t newVal = mult/100;

    // Rounding:
    if      ((mult%100) >  49) newVal++;
    else if ((mult%100) < -49) newVal--;

    // Ensure valid range:
    if      (newVal >  127) newVal =  127;
    else if (newVal < -127) newVal = -127;

    return (newVal);
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
            sprintf(buffer, "Steering:%d, Throatle:%d\n", controlContext.rcParams.steering, controlContext.rcParams.throatle);
            // sprintf(buffer, "Steering:%d, Throatle:%d\n", controlContext.driveParms.driveParms.turnPosition, controlContext.driveParms.driveParms.driveSpeed);
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
        bool speedChanged = adjustSpeedAndDirection(convToByte(controlContext.rcParams.throatle), 
                                                    convToByte(controlContext.rcParams.steering),
                                                    0,
                                                    controlContext.connectionLost);
        // bool speedChanged = adjustSpeedAndDirection(controlContext.driveParms.driveParms.driveSpeed, 
                                                    // controlContext.driveParms.driveParms.turnPosition,
                                                    // controlContext.driveParms.halt,
                                                    // controlContext.connectionLost[COM_FEATURE_DRIVE]);
    // }


    // If any change, need to wait for it to take effect:
    // TODO: This is not final. Want to service specific changes when required and not wait for delay 
    //       required by other changes.
    // if (speedChanged || directionChanged) {
        // delay(20);
    // }

    // delay(100);
}

