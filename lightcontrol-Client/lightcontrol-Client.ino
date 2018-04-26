#include <SoftwareSerial.h>
SoftwareSerial Serial1(3, 4); // RX, TX
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
//LiquidCrystal_I2C lcd(0x27, 16, 2);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 


byte pinLED = 9;
byte pinAdjust = A0;
float vLight = 0;
unsigned int g1;
unsigned int g2;
unsigned int degreeValue = 500;
unsigned int degreeValue_last = 500;
unsigned long g1_time = 0;
boolean breathLight = 0;
unsigned long backuplight_time = 0;
float breathLight_degree = 0;
float percent;
String recvString_last = "";  

void displayLCD(int yPosition, String txtMSG) {
  int xPos = (16 - txtMSG.length()) / 2;
  if (xPos < 0) xPos = 0;
  lcd.setCursor(xPos, yPosition);
  lcd.print(txtMSG);
}

void updateDegree(String gesture) {
  String LCD2;
  percent = int(vLight/2.55);
  if(percent>100) percent = 100;
        
  degreeValue_last = degreeValue;
  degreeValue = analogRead(pinAdjust)-20;
  //Serial.print("degreeValue_last="); Serial.print(degreeValue_last); Serial.print("degreeValue="); Serial.println(degreeValue);
  
  if(degreeValue<0) degreeValue=0;
  if(degreeValue>1000) degreeValue=1000;
  if(int(degreeValue_last/100) != int(degreeValue/100)) {
    //Serial.println("update 2");
    backuplight_time=millis();
  }
  
  LCD2 = " " + String(int(degreeValue/100)) + " " + String(int(percent)) + "% " + gesture + " ";
  
  displayLCD(1, LCD2);

  Serial.println(backuplight_time);
  if(millis()-backuplight_time > 3000) {
    lcd.noBacklight();
  }else{
    lcd.backlight();
  }
}

void lightness(byte startDegree, byte endDegree, int spacing, byte speedNum) {
  if(spacing<0) Serial.println("Power down light from " + String(startDegree) + " to " + String(endDegree));
  if(spacing>0) Serial.println("Power up light from " + String(startDegree) + " to " + String(endDegree));
  
  if(startDegree<endDegree) {
    for (int i=startDegree; i<=endDegree; i=i+spacing){
      analogWrite(pinLED, i);
      vLight = i;
      delay(speedNum);
    }     
    
  }else{
    for (int i=startDegree; i>=endDegree; i=i+spacing){
      analogWrite(pinLED, i);
      vLight = i;
      delay(speedNum);
    }            
  }
  if(endDegree==0) analogWrite(pinLED, 0);
}

void lightControl(String stringActions) {
  //Serial.print("stringActions==LR --> "); Serial.print(stringActions=="LR"); Serial.print(" / stringActions==RL --> "); Serial.println(stringActions=="RL");
  //Serial.print("(stringActions==LR || stringActions==RL)"); Serial.println((stringActions=="LR" || stringActions=="RL"));
  //Serial.print("stringActions==DU --> "); Serial.print(stringActions=="DU"); Serial.print(" / stringActions==UD --> "); Serial.println(stringActions=="UD"); 
  
  //水平揮動 --> On/Off
  if(stringActions=="LR" || stringActions=="RL") {
    if(vLight>10) {
      lightness(vLight, 0, -5, 50);  //Poweroff the light
    }else{      
      lightness(vLight, 255, 5, 50);  //Poweron the light
    }       

  }else if(stringActions=="DU" || stringActions=="UD") {
  //垂直揮動 -->    step power on/off    
    if(breathLight == 1) {
      breathLight = 0;
    }else{
      breathLight = 1;     
    }
  }
}

void lightBreath() {
  float degree = float(degreeValue)/500; 
  breathLight_degree = breathLight_degree + degree;
  if(breathLight_degree>360) breathLight_degree = 0;
    
  vLight = (sin(breathLight_degree * 0.0174533) + 1) * 127;
    
  //Serial.println(vLight);
  analogWrite(pinLED, vLight);
}

void setup() {
  pinMode(pinLED, OUTPUT);
  Serial.begin(9600);
  Serial1.begin(9600);
  //lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();
  displayLCD(0, " SunplusIT Maker");
  delay(1500);
  displayLCD(1, "  EGAD-005+MUART0");
  delay(2500);
  lcd.noBacklight();
  delay(500);
  lcd.backlight();
  delay(1000);
  lcd.noBacklight();
  delay(350);
  lcd.backlight();
  delay(600);
  lcd.noBacklight();  
  displayLCD(1, "                 ");
}

void loop() {

  int recv;
  int recvNumbers = 0;
  String recvString = ""; 

  if(breathLight==1) lightBreath();

  if(Serial1.available() > 0) {
    recv = Serial1.peek();
    //Serial.println("update 1");
    backuplight_time = millis();
    
    if(isDigit(recv) || char(recv)=='O') { 
      
      if(char(recv)=='O') Serial1.read();
      Serial.print("Number:");
      recvNumbers = Serial1.parseInt() - 100;

      Serial.print(recvNumbers);
      Serial.print(", ");
      Serial.print(vLight);

      if(abs(recvNumbers)<4) {
        vLight = vLight + recvNumbers*int(degreeValue/100);
        if(vLight>255) vLight=255;
        if(vLight<0) vLight=0;
        
        analogWrite(pinLED, vLight);
      }

      updateDegree(recvString_last);
      
      Serial.print("--> ");
      Serial.println(vLight);
      
      
    }else if(isAlpha(recv)) {
      
      Serial.print("Character: ");
      //recvString = Serial1.readString();
      char peekChar = Serial1.peek();

      if(millis()-g1_time > 3000) {
        g1=0;  //如果第二個動作超過了3秒, 則清掉再從第一次開始
        recvString_last = " / ";
        lightControl(recvString_last); 
      }
      
      if (peekChar=='L' || peekChar=='R' || peekChar=='U' || peekChar=='D') {
        if(g1==0) {
          g1 = Serial1.read();
          g1_time = millis();
          recvString_last = String(char(g1)) + "/ ";
          //updateDegree(String(char(g1)) + "/ ");
          Serial.print("g1="); Serial.println(char(g1));
        }else{
          g2 = Serial1.read();
          recvString = String(char(g1)) + String(char(g2));
          recvString_last = String(char(g1)) + "/" + String(char(g2));
          updateDegree(recvString_last );
          Serial.print("recvString="); Serial.println(recvString);
          recvString_last = String(char(g1)) + "/" + String(char(g2));
          g1 = 0;
          //檢查動作並執行
            
          lightControl(recvString); 
          recvString = "";

        }
      }
      
  
    }else{
      //backuplight_time = millis();
      while (Serial1.available() > 0) {
        Serial1.read();      
      }      
    }

  }else{
    updateDegree(recvString_last);
  }


}
