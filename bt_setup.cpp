#include <SoftwareSerial.h>
#include <HardwareSerial.h>
#include <Arduino.h>
#include "bt_setup.h"

extern SoftwareSerial btSerial;
extern HardwareSerial Serial;

#define DEBUG 0

// One datasheet lists max as 20 but with AT+NAME prefix tests showed 14 is limit:
#define BT_MAX_NAME_LEN 14

// ----------------------------------------------------------------------
typedef struct {
    char     * baudCode;
    uint32_t   baudNum;
} BT_BAUD_ST;

static const BT_BAUD_ST baudRates[] = {
    { "BAUD4", 9600 },
    { "BAUD7", 57600 },
    { "BAUD6", 38400 },
    { "BAUD5", 19200 },
    { "BAUD3", 4800 },
    { "BAUD2", 2400 },
    { "BAUD1", 1200 },

    // WARNING WARNING WARNING WARNING WARNING WARNING WARNING 
    // Test at 230400 found that writes work but reads did not.
    // Without reads a scan of these baud rates won't work.
    // They are left active in case reads happen to work but they
    // should probably not be used as there is a chance once set
    // the HC06 won't communicate.
    // WARNING WARNING WARNING WARNING WARNING WARNING WARNING 
    { "BAUD8", 115200 }, // Noisy! - Not recomended.
    { "BAUD9", 230400 }, // DO NOT USE 
    { "BAUDA", 460800 }, // DO NOT USE
    { "BAUDB", 921600 }, // DO NOT USE
    { "BAUDC", 1382400 } // DO NOT USE
    // WARNING WARNING WARNING WARNING WARNING WARNING WARNING 
};

//////////////////////////////////////////////////////////////////////
// Setup:
//////////////////////////////////////////////////////////////////////
static const uint16_t BUFFER_SIZE = 30;
static char buf[BUFFER_SIZE];

// ----------------------------------------------------------------------
// validateBluetoothParms()
// Returns ture if valid, false otherwise.
static bool validateBluetoothParms(char * name, char * pin, uint32_t baudrate)
{
    bool rv = true;

    if (strlen(name) > BT_MAX_NAME_LEN) {
        #if DEBUG
        Serial.println("Name too long.");
        #endif
        rv = false;
    }
    else if (strlen(pin) != 4) {
        #if DEBUG
        Serial.println("PIN must be 4 digits.");
        #endif
        rv = false;
    }
    else {
        rv = false;

        // Don't allow potentially dangerous baudrates that could brick the module:
        if (baudrate <= 57600) {
            for (int idx=0; idx < sizeof(baudRates)/sizeof(BT_BAUD_ST); idx++) {
                if (baudRates[idx].baudNum == baudrate) {
                    rv = true;
                    break;
                }
            }
        }
        if (!rv) {
            #if DEBUG
            Serial.println("Invalid baudrate.");
            #endif
        }
    }

    return (rv);
}

static int readBluetoothBuffer(char * buf, uint16_t bufSize)
{
    int i = 0;

    // In case we don't write ANYTHING:
    buf[0] = NULL;

    // Look for data to write:
    for (i=0; i < (bufSize-1); ) {
        if (btSerial.available()) {
            char ch = btSerial.read(); 
            if (ch != NULL) {
                buf[i] = ch;
                i++;
            }
        }
        else {
            buf[i] = NULL;
            break;
        }
    }
    buf[bufSize-1] = NULL;

    return (i);
}

static void flushBluetoothInput() 
{
    // To get rid of final "OK", etc if present.
    readBluetoothBuffer(buf, BUFFER_SIZE);
}

static uint32_t numPatternInBuffer(char * buf, char * keepAlivePattern)
{
    char * strToSearch = buf;
    char * newStrToSearch = NULL;
    uint32_t count=0;

    // Count occurances:
    do {
        newStrToSearch = strstr(strToSearch, keepAlivePattern);
        if (newStrToSearch) {
            count++;
            strToSearch = newStrToSearch;
        }
        if (count > 5) break;
    } while (newStrToSearch);

    return(count);
}


// ----------------------------------------------------------------------
// tryAT()
// Returns BT_RC_COMPLETED if OK returned.
// Returns BT_RC_PARTIALCONFIG if keepAliveString found instead.
static BT_RC_EN tryAT (bool withCRLR, char * keepAliveString, char * cmd=NULL, char * reqResponse="OK", char * fullResponseNeeded=NULL) 
{
    BT_RC_EN rc = BT_RC_FAILED;
    const uint8_t wbSiz=32;
    char writeBuf[wbSiz];

    // Loop for retries:
    for (int k=0; (k < 2) && (rc == BT_RC_FAILED); k++) {
        // #if DEBUG
        // Serial.println("      - Attemp #...");
        // #endif
        if (cmd) {
            strncpy(writeBuf, cmd, wbSiz);
            if (withCRLR) strncat(writeBuf, "\r\n", wbSiz);
            #if DEBUG
            Serial.write(writeBuf);
            if (!withCRLR) Serial.write("\r\n");
            #endif
            btSerial.write(writeBuf);
        }
        else {
            if (withCRLR) btSerial.write("AT\r\n");
            else          btSerial.write("AT");
        }
        
        int charsRead = 0;
        long prevTime = millis();
        while (rc == BT_RC_FAILED) {
            // Wait 1 second max but allow for faster response detection:
            long curTime = millis();
            if ((curTime - prevTime) > 1000) {
                #if DEBUG
                Serial.print(buf);
                #endif
                break;
            }

            // Short delay...
            delay(20);

            // Check for more response data:
            charsRead += readBluetoothBuffer(&(buf[charsRead]), BUFFER_SIZE-charsRead);
            uint32_t numPattern = numPatternInBuffer(buf, reqResponse);
            if (numPattern > 0) {
                if (fullResponseNeeded) {
                    numPattern = numPatternInBuffer(buf, fullResponseNeeded);
                    if (numPattern > 0) {
                        rc = BT_RC_COMPLETED;
                        break;
                    }
                }
                else {
                    rc = BT_RC_COMPLETED;
                    break;
                }
            }
            else {
                const uint8_t minNumKeepAlives = 2;
                uint32_t numKeepAlives = numPatternInBuffer(buf, keepAliveString);
                if (numKeepAlives > 0) {
                    #if DEBUG
                    Serial.write("Number of keepAlives=");
                    Serial.print(numKeepAlives);
                    Serial.write("\r\n");
                    #endif
                    if (numKeepAlives > minNumKeepAlives) {
                        Serial.println("Active bluetooth connection re-established!");
                        rc = BT_RC_PARTIALCONFIG;
                        break;
                    }
                    else {
                        // Removed to reduced global variable use:
                        // #if DEBUG
                        // Serial.println("Possible active bluetooth connection ignored; not enough keep alives detected.");
                        // #endif 
                        // We didn't find any match but allow loop to try again.
                    }
                }
            }
        } // end while()
    }

    return(rc);
}

static BT_RC_EN tryATOK (bool withCRLR, char * keepAliveString)
{
    return (tryAT(withCRLR, keepAliveString));
}

static BT_RC_EN tryATRead (bool withCRLR, char * cmd, char * reqResponse, char * fullResponseNeeded, char * keepAliveString)
{
    return (tryAT(withCRLR, keepAliveString, cmd, reqResponse, fullResponseNeeded));
}

static BT_RC_EN tryATWrite (bool withCRLR, char * cmd, char * keepAliveString)
{
    return (tryAT(withCRLR, keepAliveString, cmd));
}

// ----------------------------------------------------------------------
// configureBluetooth()
// Its possible a bluetooth connection may occur before the configuration is applied. 
// If so this returns BT_RC_PARTIALCONFIG
static BT_RC_EN configureBluetooth(bool useLFCR, char * name, char * pin, uint32_t baudrate, char * keepAlivePattern) {
    BT_RC_EN rc = BT_RC_FAILED;
    bool okToContinue = true;

    const uint8_t wbSiz=30;
    char writeBuf[wbSiz];
    char responseBuf[wbSiz];
    char fullResponseBuf[wbSiz];

    // General parameter used multiple times:
    bool writeNeeded=false;

    Serial.print(" - Connected! ");
    if (useLFCR) Serial.println("Chip uses AT with \\r\\n");
    else         Serial.println("Chip uses AT without \\r\\n.");
    if (useLFCR) Serial.println("Verifying bluetooth chip programmed with requested config.");
    else         Serial.println("Writing bluetooth config to chip...");

    // PIN
    if (useLFCR) {
        #if DEBUG
        Serial.println("Checking PIN");
        #endif
        strncpy(writeBuf, "AT+PSWD", wbSiz);
        strncpy(responseBuf, "+PIN:", wbSiz);
        strncpy(fullResponseBuf, responseBuf, wbSiz);
        strncpy(fullResponseBuf, "\"", wbSiz);
        strncat(fullResponseBuf, pin, wbSiz);
        strncat(fullResponseBuf, "\"", wbSiz);
        rc = tryATRead(useLFCR, writeBuf, responseBuf, fullResponseBuf, keepAlivePattern);
        if (rc == BT_RC_COMPLETED ) {
            #if DEBUG
            Serial.print("PIN already set!\r\n");
            #endif
            writeNeeded = false;
        }
        else if (rc == BT_RC_PARTIALCONFIG) {
            okToContinue = false;
        }
        else {
            writeNeeded = true;
        }
    }
    else {
        // Removed to reduced global variable use:
        // #if DEBUG
        // Serial.println("Not Checking PIN - version not known to read PIN back.");
        // #endif
        writeNeeded = true;
    }
    if (okToContinue) {
        if (writeNeeded) {
            #if DEBUG
            Serial.print("Writing PIN\r\n");
            #endif
            if (useLFCR) {
                strncpy(writeBuf, "AT+PSWD=\"", wbSiz);
                strncat(writeBuf, pin, wbSiz);
                strncat(writeBuf, "\"", wbSiz);
            }
            else {
                strncpy(writeBuf, "AT+PIN", wbSiz);
                strncat(writeBuf, pin, wbSiz);
            }
            rc = tryATWrite(useLFCR, writeBuf, keepAlivePattern);
            if (rc == BT_RC_FAILED) {
                Serial.println(" - Error writing pin.");
            }
            else if (rc == BT_RC_PARTIALCONFIG) {
                okToContinue = false;
            }
            else if (rc == BT_RC_COMPLETED) {
                if (useLFCR) Serial.print(" - PIN changed to: ");
                else         Serial.print(" - PIN write completed to: ");
                Serial.println(pin);
            }
        }
    }

    // Name
    if (okToContinue) {
        if (useLFCR) {
            #if DEBUG
            Serial.println("Checking Name");
            #endif
            strncpy(writeBuf, "AT+NAME", wbSiz);
            strncpy(responseBuf, "+NAME:", wbSiz);
            strncpy(fullResponseBuf, responseBuf, wbSiz);
            strncat(fullResponseBuf, name, wbSiz);
            strncat(fullResponseBuf, "\r\n", wbSiz);
            rc = tryATRead(useLFCR, writeBuf, responseBuf, fullResponseBuf, keepAlivePattern);
            if (rc == BT_RC_COMPLETED ) {
                #if DEBUG
                Serial.print("Name already set!\r\n");
                #endif
                writeNeeded = false;
            }
            else if (rc == BT_RC_PARTIALCONFIG) {
                okToContinue = false;
            }
            else {
                writeNeeded = true;
            }
        }
        else {
            // Removed to reduced global variable use:
            // #if DEBUG
            // Serial.println("\r\nNot Checking Name - version not known to read name back.");
            // #endif
            writeNeeded = true;
        }
    }
    if (okToContinue) {
        if (writeNeeded) {
            #if DEBUG
            Serial.print("Writing Name\r\n");
            #endif
            strncpy(writeBuf, "AT+NAME", wbSiz);
            if (useLFCR) strncat(writeBuf, "=", wbSiz);
            strncat(writeBuf, name, wbSiz);
            rc = tryATWrite(useLFCR, writeBuf, keepAlivePattern);
            if (rc == BT_RC_FAILED) {
                Serial.println(" - Error writing name");
            }
            else if (rc == BT_RC_PARTIALCONFIG) {
                okToContinue = false;
            }
            else if (rc == BT_RC_COMPLETED) {
                if (useLFCR) Serial.print(" - Name changed to: ");
                else         Serial.print(" - Name write completed to: ");
                Serial.println(name);
            }
        }
    }

    // BAUD
    char convStr[20];
    if (okToContinue) {
        if (useLFCR) {
            #if DEBUG
            Serial.println("Checking Baud");
            #endif
            strncpy(writeBuf, "AT+UART", wbSiz);
            strncpy(responseBuf, "+UART:", wbSiz);
            strncpy(fullResponseBuf, responseBuf, wbSiz);
            strncat(fullResponseBuf, ltoa(baudrate, convStr, 10), wbSiz);
            strncat(fullResponseBuf, ",0,0", wbSiz);
            rc = tryATRead(useLFCR, writeBuf, responseBuf, fullResponseBuf, keepAlivePattern);
            if (rc == BT_RC_COMPLETED ) {
                #if DEBUG
                Serial.print("Baud already set!\r\n");
                #endif
                writeNeeded = false;
            }
            else if (rc == BT_RC_PARTIALCONFIG) {
                okToContinue = false;
            }
            else {
                writeNeeded = true;
            }
        }
        else {
            // Removed to reduced global variable use:
            // #if DEBUG
            // Serial.println("\r\nNot Checking Baud - version not known to read baud back.");
            // #endif 
            writeNeeded = true;
        }
    }
    if (okToContinue) {
        if (writeNeeded) {
            #if DEBUG
            Serial.print("Writing Baud\r\n");
            #endif 
            if (useLFCR) {
                strncpy(writeBuf, "AT+UART=", wbSiz);
                strncat(writeBuf, ltoa(baudrate, convStr, 10), wbSiz);
                strncat(writeBuf, ",0,0", wbSiz);
            }
            else {
                for (int idx=0; idx < sizeof(baudRates)/sizeof(BT_BAUD_ST); idx++) {
                    if (baudRates[idx].baudNum == baudrate) {
                        strncpy(writeBuf, "AT+", wbSiz);
                        strncat(writeBuf, baudRates[idx].baudCode, wbSiz);
                        break;
                    }
                }
            }
            rc = tryATWrite(useLFCR, writeBuf, keepAlivePattern);
            if (rc == BT_RC_FAILED) {
                Serial.println(" - Error writing baud");
            }
            else if (rc == BT_RC_PARTIALCONFIG) {
                okToContinue = false;
            }
            else if (rc == BT_RC_COMPLETED) {
                #if DEBUG
                Serial.println("Restarting Bluetooth Serial");
                #endif
                btSerial.begin(baudrate);

                if (useLFCR) Serial.print(" - BAUD changed to: ");
                else         Serial.print(" - BAUD write completed to: ");
                Serial.println(baudrate);
            }
        }
    }
    
    if (okToContinue) {
        delay(20);
        flushBluetoothInput();
        Serial.println("Done! Bluetooth configuration completed!");
    }

    #if DEBUG
    if (okToContinue) {
        if (useLFCR) {
            Serial.println("AT Commands working - Make sure \"Both NL & CR\" selected in serial monitor.");
        }
        else {
            Serial.println("AT Commands working - Make sure \"No line ending\" selected in serial monitor.");
        }
    }
    else {
        Serial.println("AT Commands not working due to active bluetooth connection.");
    }
    #endif

    return(rc);
}

// ----------------------------------------------------------------------
// bt_setup()
// Its possible a bluetooth connection may occur before the baud rate is 
// determined. If the keep alive pattern is seen then no configuration is applied.
// Otherwise the connecting app will need to detect there is no bi-direction 
// communication, and disconnect.
// TODO - have the Arudino code keep trying while it waits for app disconnection
// so that baud rates can be scaned using AT commands.
//
// Returns: true - Keep alive seen or baud rate configuration applied.
//          false - Unable to connect or no valid data observed.
BT_RC_EN bt_setup(char * name, char * pin, uint32_t baudrate, char * keepAlivePattern) {
    BT_RC_EN rc=BT_RC_FAILED;
    bool okToContinue=true;
    bool useLFCR = false;

    // Ensure buffer is all zeros. Other wise keep alive pattern may still be found in it.
    for (int i=0; i < BUFFER_SIZE; i++) buf[i] = NULL;

    okToContinue = validateBluetoothParms(name, pin, baudrate);
    if (!okToContinue ) Serial.println("ERROR: Invalid Bluetooth Parameters.");

    if (okToContinue ) {
        // Try specified speed.
        // Check for keep alive. If found then skip further setup.
        // Verify AT->OK works with or without \r\n.
        // If not, then cycle through list until AT->OK works with or without \r\n.
        // If nothing found, light up a red error light, but continue without BT.
        // If speed found, reconfigure. Reboot if continuous cycling can be prevented.
    
        // TODO: 
        //  - Handle case where no OK ever comes back..
        //    - Inform user but stick with requested baud.
        //       - Look for keep alive, etc.
        //  - Handle mid configuration connections.
        
        Serial.print("Starting bluetooth config using requested baudrate: ");
        Serial.print(baudrate);
        Serial.println("");

        // Try connection:
        btSerial.begin(baudrate);

        // Check if AT->OK works on requested baudrate:
        #if DEBUG
        Serial.println("Trying AT with \\r\\n");
        #endif
        rc = tryATOK(true, keepAlivePattern);
        if (rc == BT_RC_COMPLETED) {
            useLFCR = true;
        }
        else if (rc == BT_RC_PARTIALCONFIG) {
            // Active connection detected.
            okToContinue = false;
        }
        else {
            #if DEBUG
            Serial.println("Trying AT without \\r\\n");
            #endif
            rc = tryATOK(false, keepAlivePattern);
            if (rc == BT_RC_COMPLETED) {
                useLFCR = false;
            }
            else if (rc == BT_RC_PARTIALCONFIG) {
                // Active connection detected.
                okToContinue = false;
            }
        }
    }

    if (okToContinue ) {
        if (rc == BT_RC_COMPLETED) {
            rc = configureBluetooth(useLFCR, name, pin, baudrate, keepAlivePattern);
        }
        else {
            Serial.println("Connection failed. Trying other baudrates...");
            // Continue and try other baud rates:
            for (int idx=0; idx < sizeof(baudRates)/sizeof(BT_BAUD_ST); idx++) {
                // The requested baud was already tried. Don't check it again.
                if (baudRates[idx].baudNum == baudrate) continue;
    
                #if DEBUG
                Serial.print(" - Trying ");
                #endif 
                Serial.print(baudRates[idx].baudNum);
                Serial.print("\r\n");
                btSerial.begin(baudRates[idx].baudNum);
                delay(100);
    
                // Some devices need \r\n and some only work with out it. Try both ways:
                #if DEBUG
                Serial.print("   - With \\r\\n\r\n");
                #endif 
                rc = tryATOK(true, keepAlivePattern);
                if (rc == BT_RC_COMPLETED) {
                    useLFCR = true;
                }
                else if (rc == BT_RC_PARTIALCONFIG) {
                    // Active connection detected.
                    okToContinue = false;
                    break;
                }
                else {
                    #if DEBUG
                    Serial.print("   - Without \\r\\n\r\n");
                    #endif 
                    rc = tryATOK(false, keepAlivePattern);
                    if (rc == BT_RC_COMPLETED) {
                        useLFCR = false;
                    }
                    else if (rc == BT_RC_PARTIALCONFIG) {
                        // Active connection detected.
                        okToContinue = false;
                        break;
                    }
                }
                if (rc == BT_RC_COMPLETED) {
                    rc = configureBluetooth(useLFCR, name, pin, baudrate, keepAlivePattern);
                    break;
                }
            }
        }
    }

    return(rc);
}

