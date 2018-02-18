
#include "Interface.h"


boolean XbeeData = false;
int packets[5]={0,0,0,0,0},speedKnobPin=3,joyPin0=14,joyPin1=15;
int relay[8][2]={{1,22},{2,23},{4,24},{8,25},{16,26},{32,27},{64,28},{128,29}};

void setup() {
  for (int i=0;i<8;i++)
    pinMode(i+22,INPUT_PULLUP); 
  Serial.begin(115200);
  Serial3.begin(115200);
  Serial.println("Remote Control v1");
  Serial.print("Initializing...");
  pinMode(53, OUTPUT);
  digitalWrite(53, HIGH);

  delay(2000);
}

void loop() {  
  ReadControls();
  SendCommand();
  delay (COM_STATE_KEEP_ALIVE_TIME_IN_MS);
} 

void SendCommand() {
    COM_HEADER_ST       header;
    header.sync = COM_SYNCPATTERN_8BIT;
    header.protocolVersion = COM_PROTOCOL_VERSION;
    header.featureId = COM_FEATURE_DRIVE;
    header.packetType = COM_PACKETTYPE_STATE;
    header.checksum = 0;

    COM_FEATURE_DRIVE_ST body;
    body.driveParms.driveSpeed = (char) packets[1];
    body.driveParms.turnPosition = (char) packets[2];
    body.halt = 0;

    unsigned char * buf = (unsigned char *) &header;
    for (int i=0; i< sizeof(header); i++) {
        char chHexStr[10];
        sprintf(chHexStr, "%02x ", (unsigned char) buf[i]);
        Serial.write(chHexStr);  // d3 00 00 00 3c 3d 00 00 00

        Serial3.write(buf[i]);
        // delay(1);
    }

    buf = (char *) &body;
    for (int i=0; i< sizeof(body); i++) {
        char chHexStr[10];
        sprintf(chHexStr, "%02x ", (unsigned char) buf[i]);
        Serial.write(chHexStr);

        Serial3.write(buf[i]);
        // delay(1);
    }

    Serial.write("\n");
}

void ReadControls() {
  int speedKnob;
  packets[3]=0;
  speedKnob=map(analogRead(speedKnobPin),1024,0,0,60);
  speedKnob=60;
  packets[2]=map(analogRead(joyPin0),1024,0,60-speedKnob,60+speedKnob);
  packets[1]=map(analogRead(joyPin1),1024,0,60-speedKnob,60+speedKnob);  
  for (int i=0;i<8;i++){
    if (digitalRead(i+22)==LOW){      
      packets[3]+=pow(2,i);
    }
  } 
}

