#include <SabertoothSimplified.h>
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_LIGHT 0x39
#define I2C_ALTPRES 0x77
#define I2C_TEMP 0x48
#define I2C_LCD 0x27                                                             // <<----- Add your address here.  Find it from I2C Scanner function
#define BACKLIGHT_PIN 3
#define En_pin  2
#define Rw_pin  1
#define Rs_pin  0
#define D4_pin  4
#define D5_pin  5
#define D6_pin  6
#define D7_pin  7

LiquidCrystal_I2C  lcd(I2C_LCD,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);
// Mixed mode is for tank-style diff-drive robots.
// Only Packet Serial actually has mixed mode, so this Simplified Serial library
// emulates it (to allow easy switching between the two libraries).

SabertoothSimplified ST(Serial3); 
int PacketsRX[5], PacketsTX[4]; 
unsigned long currentTime,timeOfLastGoodPacket = 0,timeOfLastTTL = 0,timeOfLastStatus=0;                                                    
boolean XbeeData = false;   
typedef struct {  
  int currentState;
  long timer;
} relay_type;

relay_type relayA[8]={{LOW,0},{LOW,0},{LOW,0},{LOW,0},{LOW,0},{LOW,0},{LOW,0},{LOW,0}};

typedef struct {
  unsigned long currentTime = 0;
  unsigned long previousTime = millis();
  int currentDrive = 0;
  int currentTurn = 0;
} speedControl;

speedControl Motor;

void setup() {
  Serial3.begin(9600);                                                             // Serial3 is the Object which communicate with the Sabertooth H-Bridge
  Serial2.begin(115200);                                                           // Connections to make:
  Serial.begin(9600);                                                              //   Arduino TX->13  ->  Sabertooth S1
  for (int i=0; i<8; i++){                                                         //   Arduino GND     ->  Sabertooth 0V
    pinMode(i+22,OUTPUT);                                                          //   Arduino VIN     ->  Sabertooth 5V (OPTIONAL, if you want the Sabertooth to power the Arduino)                                                                
  }                                                                                // For how to configure the Sabertooth, see the DIP Switch Wizard for
  lcd.begin (20,4);                                                                //   http://www.dimensionengineering.com/datasheets/SabertoothDIPWizard/start.htm
  lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);                                     //   http://www.dimensionengineering.com/datasheets/SabertoothDIPWizard/nonlithium/serial/simple/single.htm
  lcd.setBacklight(HIGH);
  lcd.home (); // go home
  lcd.print("Robo Plow     ver4.4");
  lcd.setCursor(0,1);
  lcd.print("Initializing..."); 
  delay(3000);
  allStop(); 
  lcdReset();                                  
}                                                     

void lcdReset(){
  lcd.clear();
  lcd.home();
  lcd.print("(X:");lcd.print(PacketsRX[1]);lcd.print(",Y:");lcd.print(PacketsRX[2]);lcd.print(") sum=");lcd.print(PacketsRX[0]);
}

void allStop() {                                                                    // The Sabertooth won't act on mixed mode until
  ST.drive(0);                                                                      // it has received power levels for BOTH throttle and turning, since it
  ST.turn(0);                                                                       // mixes the two together to get diff-drive power levels for both motors.
// Serial.print('\n'); Serial.write("All STOP +++++");                               // So, we set both to zero initially.
  lcd.setCursor(12,3);
  lcd.print("All Stop");
  PacketsRX[1]=0;
  PacketsRX[2]=0;
  PacketsRX[3]=0;
  PacketsRX[4]=0;
}

void timeout() {
  if (currentTime > (timeOfLastGoodPacket + 200)) {                                 // Stop robot if no good PacketsRX received within 200th of a seconds
    allStop();
    timeOfLastGoodPacket = currentTime;
  }
  if (currentTime > (timeOfLastTTL +1000)) {                                        // send stay alive command to receiver every 1 second.
    PacketsTX[1]=100;  
    lcdReset();
    timeOfLastTTL=currentTime;
  }
}

void GetCommand() {                                                                 // Data received from Xbee is in byte 0-256. Sending a string requires multiple bytes even for 1 character and requires processing power
  static boolean recvInProgress = false;                                            // One complete packet of data contains 6 byte begining with a startMarker and ends with an endMarker.  The 4 remaining bytes is then stored
  static int ndx = 0;                                                               // in an array called PacketsRX.  
  byte startMarker = 255;                                                           // PacketsRX[0] = Checksum which is the sum of PacketsRX[1] + PacketsRX[2] + PacketsRX[3], this is used to verify if received data is good
  byte endMarker = 254;                                                             // PacketsRX[1] = PacketsRXue range from 0 to 120 which drives the y-axis (Throttle). 1-59 is reverse, 61 to 120 is forward, 60 is stop
  byte rc;                                                                          // PacketsRX[2] = x-axis (Steering) which is the same as y. X and Y is then converted using Map function to give it a range of -127 to 127, 0 is stop

  while (Serial2.available() > 0 && XbeeData == false){                             // PacketsRX[3] = Controls simultaneously 8 channel relay for moving linear actuators

    rc = Serial2.read();                                                            // PacketsRX[4] = Controls simultaneously 8 channel relay for lights and other devices
    if (recvInProgress) {
      if (rc != endMarker){
        PacketsRX[ndx]=rc; 
        ndx++;
      }
      else {        
        XbeeData=true;
        ndx=0;
        recvInProgress = false;
        break;
      }
    }
    else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
}

// A byte has a maximum size of 255, when receiving anything larger, the Arduino takes the difference of the value minus the maximum.
// For example: Since Checksum is the sum of PacketsRX[1]+PacketsRX[2]+PacketsRX[3], the total could exceed 255.  Lets say the total is 258.  
// The Arduino takes 258 - the maximum (255) and assign Checksum the difference which is 3.  The checksum calculation will therefore fail,
// to fix this we continue to subtract 255 from the sum of PacketsRX[1]+PacketsRX[2]+PacketsRX[3] until it is less than 255.
// PacketsRX[3] controls 8 devices simultaneously using just 1 byte of data. Each device are assigned a unique ID {1,2,4,8,16,32,64,128}.
// To turn on each device, you select their corresponding ID.  To turn on multiple devices simutaneously, you take the sum of all the
// corresponding IDs. For example: To turn on device ID 4, you issue a byte 4.  To turn on device 4 and 16, you take their sum which is 20.
// Note: Since we use byte 254 and 255 as an EndMarker.

void ParseData(){
  int sum = PacketsRX[1]+PacketsRX[2]+PacketsRX[3]+PacketsRX[4];
  if (XbeeData) {    
    while (sum > 255){
      sum -= 255; 
    }               
    if (PacketsRX[0]==sum) {                       
      timeOfLastGoodPacket = currentTime;
      PacketsRX[1]=map(PacketsRX[1],0,120,-127,127);
      PacketsRX[2]=map(PacketsRX[2],0,120,127,-127);
      DriveMotor();                                                      
      lcd.setCursor(0,3);                                                           
      lcd.print(PacketsRX[3]);                                                      
      FindID(PacketsRX[3]);
    }
    XbeeData=false;
  }
}

int FindID(int sum) {                                                               // Finds the corresponding ID's from a byte
  lcd.setCursor(0,3); 
  if (sum){
    for (int i=7; i>=0; i--){
      if (sum >= pow(2,i)){
        if (relayA[i].currentState == LOW){
           relayA[i].timer=millis();
        }
        relayA[i].currentState==HIGH;
        sum -= pow(2,i);
      } else {
        if (relayA[i].currentState==HIGH and relayA[i].timer > 100){
          relayA[i].currentState==LOW; 
        }
      }
      digitalWrite(i=22, relayA[i].currentState);
    }
  } else {
    for (int i=7; i>=0; i--){
      if (relayA[i].currentState==HIGH and relayA[i].timer > 100){
        relayA[i].currentState==LOW;
        digitalWrite(i=22, relayA[i].currentState); 
      }
    }
  } 
}

void DriveMotor(){
  const long interval=100;
  Motor.currentTime = millis();
  Motor.currentDrive = PacketsRX[2];
  Motor.currentTurn = PacketsRX[1];
  if (Motor.currentTime - Motor.previousTime >= interval){
    Motor.previousTime = Motor.currentTime;
    if (PacketsRX[2] > Motor.currentDrive)
      Motor.currentDrive += 1;
    if (PacketsRX[2] < Motor.currentDrive)
      Motor.currentDrive -= 1;  
    if (PacketsRX[1] > Motor.currentTurn)
      Motor.currentTurn += 1;
    if (PacketsRX[1] < Motor.currentTurn)
      Motor.currentTurn -= 1;   
  }
  if (PacketsRX[2] == 0 && Motor.currentDrive != 0){
    if (Motor.currentDrive > 0)
      Motor.currentDrive -= 1;
    if (Motor.currentDrive < 0)
      Motor.currentDrive += 1;
  }
  if (PacketsRX[1] == 0 && Motor.currentTurn != 0){
    if (Motor.currentTurn > 0)
      Motor.currentTurn -= 1;
    if (Motor.currentTurn < 0)
      Motor.currentTurn += 1;
  }
  ST.drive(Motor.currentDrive);
  ST.turn(Motor.currentTurn);
  Serial.print('\n');Serial.print(PacketsRX[1]);Serial.print(',');Serial.print(PacketsRX[2]);Serial.print(',');Serial.print(PacketsRX[3]);Serial.print('*');
}

void sendStatus(){
  PacketsTX[0]=0;
  PacketsTX[2]=0;
  PacketsTX[3]=0;
  byte startMarker = 255;                                                           // PacketsRX[0] = Checksum which is the sum of PacketsRX[1] + PacketsRX[2] + PacketsRX[3], this is used to verify if received data is good
  byte endMarker = 254;
  for (int i=0;i<8;i++){
    if (relayA[i].currentState == HIGH) {
      PacketsTX[2] += pow(2,i); 
    }
  }
  PacketsTX[0]=PacketsTX[1] + PacketsTX[2] + PacketsTX[3];
  if (currentTime > (timeOfLastStatus + 2500)){
    Serial2.write(startMarker);
    for (int i=0; i<4; i++) {
      Serial2.write(PacketsTX[i]);
    }
    Serial2.write(endMarker);
    timeOfLastStatus=currentTime; 
  }
}

void getTemp(){
  float convertedtemp, correctedtemp;
  Wire.requestFrom(I2C_TEMP,2);
  while(Wire.available()){
    int8_t msb=Wire.read();
    int8_t lsb=Wire.read();
    lsb=(lsb & 0x80) >> 7;
    float f = msb+0.5*lsb-2;
    lcd.setCursor(0,3);
    lcd.print("Temp ");
    lcd.print(f);
    
  }
}

void i2cScan(){
  byte error,address;
   for (address =1; address < 127; address++){
     Wire.beginTransmission(address);
     error=Wire.endTransmission();
     if (error == 0){
       lcd.setCursor(0,3);
       lcd.print("address 0x");
       if (address<16)
         lcd.print("0");
       lcd.print(address,HEX);
       delay (5000);
     }
   }  
}


void loop()
{
  //i2cScan();
  currentTime = millis();
  GetCommand();
  ParseData();
  timeout();
  sendStatus();
}
