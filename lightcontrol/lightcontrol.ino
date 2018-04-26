#include <EGAD_005.h>
#include <SoftwareSerial.h>
SoftwareSerial Serial1(3, 4); // RX, TX

// parameters for user
int cmdWait = 10000;  //進入命令模式後等待手揮的時間, 若超過則需要重新進入命令模式
unsigned int ledLightness = 80;  //指示LED的強度
boolean rtnDistanceType = 1;  // 0:傳出距離值   1:傳出與上次的距離差

// parameters for system
byte pinMUART = 8;  //接到MUART模組的CEB, idle時送出HIGH將MUART關閉以省電
byte pinRed = 11;  //三色LED燈
byte pinBlue = 10;  //三色LED燈
byte pinYellow = 9;   //三色LED燈
boolean modeActived = 0;  //目前狀態是否為命令控制模式
unsigned long timeControlMode;  //存放時間計時使用

unsigned long last_millis_0 = 0;
const unsigned long delay_0 = 400;  //LED閃爍的間隔

void doAction(byte gestID) {
  switch (gestID) {
    case 1:
      Serial.println("left");
      Serial1.print("L");
      break;
    case 2:
      Serial.println("up");
      Serial1.print("U");
      break;
    case 4:
      Serial.println("right");
      Serial1.print("R");
      break;
    case 8:
      Serial.println("down");
      Serial1.print("D");
      break;
    case 0xc0:
      Serial.println("occupy");
      Serial1.println();
      delay(100);
      Serial1.print("O");
      break;
  }
}

void LED(unsigned int command) {
  switch (command) {
    case 0:  //off
      analogWrite(pinRed, 0);
      analogWrite(pinBlue, 0);
      analogWrite(pinYellow, 0);
      break;
    case 1:
      analogWrite(pinRed, 0);
      analogWrite(pinBlue, 0);
      analogWrite(pinYellow, ledLightness);
      break;
    case 2:
      analogWrite(pinRed, ledLightness/2);
      analogWrite(pinBlue, ledLightness/2);
      analogWrite(pinYellow, 0);
      break;
    case 3:
      analogWrite(pinRed, 0);
      analogWrite(pinBlue, ledLightness);
      analogWrite(pinYellow, 0);
      break;
    case 4:
      analogWrite(pinRed, ledLightness);
      analogWrite(pinBlue, 0);
      analogWrite(pinYellow, 0);
      break;
    case 5:
      analogWrite(pinRed, 0);
      analogWrite(pinBlue, 0);
      analogWrite(pinYellow, ledLightness);
      break;
  }
}

void blinkLED(unsigned int ledcmd1, unsigned int ledcmd2) {
  unsigned long now = millis();

  if ((now - last_millis_0) >= delay_0) {
    last_millis_0 = now;
    LED(ledcmd1);
  } else {
    LED(ledcmd2);
  }
}

void setup() {
  EGAD_005.init(2);  // EGAD-005ready pin一定要為 #2, 否則無法工作
  pinMode(pinMUART, OUTPUT);
  digitalWrite(pinMUART, HIGH);
  pinMode(pinRed, OUTPUT);
  digitalWrite(pinRed, LOW);
  pinMode(pinBlue, OUTPUT);
  digitalWrite(pinBlue, LOW);
  pinMode(pinYellow, OUTPUT);
  digitalWrite(pinYellow, LOW);

  Serial.begin(9600);
  Serial1.begin(9600);

}

void loop() {
  byte gest, dist, prev_dist;
  int baseDistance;
  byte last_dist = 0;
  dist = 0;
  
  gest = EGAD_005.get_swipe();
  if (gest == 0) return;

  if (gest == 0xc0) {  //如果為occupy, 則表示準備進入控制模式
    Serial.println("occupy");
    digitalWrite(pinMUART, LOW);
    modeActived = 1;
    //持續等到手部離開, 才進入控制模式
    /*
    while (gest & 0x40) {
      Serial.println("Prepare to enter control mode...");
      gest = EGAD_005.get_swipe(1);
      LED(1);
    }
  */
    //下方若 gest 非 0x40 表示已進入控制模式
    if ((gest & 0x40) == 0) {
      Serial.println("Start control mode.");
      timeControlMode = millis();  //控制模式開始時間
      gest = EGAD_005.get_swipe();

      //如果還在設定的控制模式時間內, 持續偵測手勢
      while ((millis() - timeControlMode < cmdWait)) {
        //如果使用者有作出手勢, 則重設時間
        if (gest != 0) timeControlMode = millis();

        gest = EGAD_005.get_swipe();
        blinkLED(1, 0);

        doAction(gest);
        
        prev_dist = 0;
        if (gest & 0x40) {
          baseDistance = EGAD_005.get_distance();
          last_dist = 0;
          delay(50);
        }
        while (gest & 0x40) {
          timeControlMode = millis();
          last_dist = dist;
          dist = EGAD_005.get_distance();  // 18~63, 63=far, 0=near , but actual is 20~60

          if(rtnDistanceType==0) {
            Serial1.println(dist);
            Serial.println(dist);
          }else{
            int difference;
            if(baseDistance>0) {
              difference = dist - baseDistance + 100;
            }else{
              difference = last_dist - dist + 100;
            }
            Serial1.println(difference);
            Serial.print("last_dist="); Serial.print(last_dist); Serial.print(" dist="); Serial.print(dist); Serial.print(" baseDistance="); Serial.print(baseDistance);
            Serial.print(" --->");
            Serial.println(difference );
            baseDistance = 0;
          }
          
          gest = EGAD_005.get_swipe(1);

          if ((gest & 0x40) == 0) {
            Serial.println("end of occupy");
            LED(0);
          }

          delay(50);
        }
        
        delay(50);  // delay 50 ms
      }

    }
    digitalWrite(pinMUART, HIGH);

  }
  LED(0);

}
