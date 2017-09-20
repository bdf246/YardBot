boolean XbeeData = false;
int packets[5]={0,0,0,0,0},speedKnobPin=3,joyPin0=0,joyPin1=2;
int relay[8][2]={{1,22},{2,23},{4,24},{8,25},{16,26},{32,27},{64,28},{128,29}};

void setup() {
  for (int i=0;i<8;i++)
    pinMode(i+22,INPUT_PULLUP); 
  Serial.begin(9600);
  Serial3.begin(115200);
  Serial.println("Remote Control v1");
  Serial.print("Initializing...");
  delay(2000);
}

void loop() {  
  ReadControls();
  SendCommand();
} 

void SendCommand() {
  packets[0]=packets[1]+packets[2]+packets[3]+packets[4]; //Checksum value
  Serial.print(packets[0]);Serial.print(",");Serial.print(packets[1]);Serial.print(",");Serial.print(packets[2]);Serial.print(",");Serial.print(packets[3]);Serial.print('\n');
  Serial3.write(255); //Send starting marker
  for (int i=0; i<5; i++)
    Serial3.write(packets[i]); 
  Serial3.write(254); //Send ending marker
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

