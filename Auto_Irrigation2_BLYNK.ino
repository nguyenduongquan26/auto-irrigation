#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
LiquidCrystal_I2C lcd(0x3F,16,2);
#include "DS3231.h"
#include <OneWire.h>
#include "DallasTemperature.h"
#include <SoftwareSerial.h>
SoftwareSerial EspSerial(10, 11); // RX, TX
#define BLYNK_PRINT Serial
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>
char auth[]= "1ab349adede74dc383a4c47c3c3c0c7d";
#define ESP8266_BAUD 9600

ESP8266 wifi(&EspSerial);
// Chân nối DS18B20 với Arduino
#define ONE_WIRE_BUS 8

//Thiết đặt thư viện onewire
OneWire oneWire(ONE_WIRE_BUS);

//Dùng thư viện DallasTemperature để đọc nhiệt độ

DallasTemperature tempsensor(&oneWire);

// Chân kết nối cảm biến siêu âm
#define TRIG_PIN A2
#define ECHO_PIN A3
#define TIME_OUT 10000

// Đồng hồ thời gian thực DS3231
DS3231 clock;
RTCDateTime dt;

// Chân kết nối cảm biến DHT11
#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//Chân cảm biến ánh sáng photodidode
#define PHOTOSENSOR1 A1

// 
#define MODE_PIN 6

//Nút bấm điều khiển LCD
#define LCDBUTTON A0

// Ký tư nhiệt độ cho LCD
byte degree[8] =
{
  0B01110,
  0B01010,
  0B01110,
  0B00000,
  0B00000,
  0B00000,
  0B00000,
  0B00000
};

// Biến đếm i
byte i=0;

// Chân điều khiển relay máy bơm 1 và máy bơm 2
#define PUMP1_BUTT 2
#define PUMP2_BUTT 3
#define PUMP1_CTR 4
#define PUMP2_CTR 5
boolean PUMP1_STA,PUMP2_STA;
#define WaterLOW 30
#define WaterHIGH 65
#define Alarm 12

// Biến đọc chế độ AUTO hay MANUAl
boolean MODE;
boolean wifiControl;
boolean pump1c,pump2c;
// Biến lưu giá trị đọc được
int airt ;
int h;
int p;
int distance;
int watert;

char ssid[] = "Cube V";            
char pass[] = "anhquan2211";           



BLYNK_WRITE(V1){
  int virtualpin1 = param.asInt();
    if ( virtualpin1 == 1 ) {
      wifiControl = 1;
      }
    else { if ( virtualpin1 == 0 ){
      wifiControl = 0;
      }
    }
}


BLYNK_WRITE(V2){
  int virtualpin2 = param.asInt();
    if ( virtualpin2 == 1 && wifiControl == 1 ) {
//    digitalWrite(PUMP1_CTR,HIGH);
    PUMP1_STA = 1;
    Blynk.virtualWrite(V6,"PUMP1 ON");
    }
   else {
        if ( virtualpin2 == 0 && wifiControl == 1 ) {
  //  digitalWrite(PUMP1_CTR,LOW);
    PUMP1_STA = 0;
    Blynk.virtualWrite(V6,"PUMP1 OFF");
    }   
   }
  }


BLYNK_WRITE(V3){
  int virtualpin3 = param.asInt();
    if ( virtualpin3 == 1 && wifiControl == 1 ) {
    //digitalWrite(PUMP2_CTR,HIGH);
    PUMP2_STA = 1;
    Blynk.virtualWrite(V6,"PUMP2 ON");
    }
   else {
        if ( virtualpin3 == 0 && wifiControl == 1 ) {
    //digitalWrite(PUMP2_CTR,LOW);
    PUMP2_STA = 0;
    Blynk.virtualWrite(V6,"PUMP2 OFF");
    }   
   }
  }


void setup() 
{
 
  pinMode (PHOTOSENSOR1, INPUT);
  pinMode(LCDBUTTON, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PUMP1_BUTT,INPUT_PULLUP);
  pinMode(PUMP2_BUTT,INPUT_PULLUP);
  pinMode(PUMP1_CTR,OUTPUT);
  pinMode(PUMP2_CTR,OUTPUT);
  pinMode(MODE_PIN,INPUT_PULLUP);
  pinMode(Alarm,OUTPUT);
  Serial.begin(9600);
  EspSerial.begin(ESP8266_BAUD);
  Blynk.begin(auth, wifi, ssid, pass);
  dht.begin();
  tempsensor.begin();
  lcd.init();  
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("AUTO IRRIGATION");
  clock.begin();
  //clock.setDateTime(__DATE__, __TIME__);
  lcd.createChar(1,degree);
}
 
void loop() 
{
      Blynk.run();
      if( wifiControl == 0){
      modeselect(digitalRead(MODE_PIN));
       Blynk.virtualWrite(V7,"Button Control");
      } 
      else { 
        if ( wifiControl == 1){
      Blynk.virtualWrite(V7,"Blynk Control");
        if ( PUMP1_STA == 1){
          digitalWrite(PUMP1_CTR,HIGH);

          }
        else {
          if ( PUMP1_STA == 0){
            digitalWrite(PUMP1_CTR,LOW);

            }
          }
          if ( PUMP2_STA == 1){
          digitalWrite(PUMP2_CTR,HIGH);
        
          }
        else {
          if ( PUMP2_STA == 0){
            digitalWrite(PUMP2_CTR,LOW);
         
            }
          }
        }
      }
      tempsensor.requestTemperatures(); 
      dt = clock.getDateTime();   
      airt = getCondition(dht.readTemperature());
      watert= tempsensor.getTempCByIndex(0); 
      h = dht.readHumidity();
      p = getAVRvalue(PHOTOSENSOR1);
      distance =70 - GetDistance();
      
//      if (millis() - lastConnectionTime_1 > postingInterval_1){
//      Blynk.virtualWrite(V12,distance);
//      Blynk.virtualWrite(V11,watert);
//      Blynk.virtualWrite(V10,airt);
//      lastConnectionTime_1 = millis();
//      }
           LCDshowMode();
}

  
void modeselect(boolean val){
    if (val == HIGH){
      MODE = 0;
        autoPump();
    }
  else { if (val == LOW){
      ManualCmdMode();
      MODE = 1;
      }
  }
  }

void autoPump(){
  if ( distance > WaterHIGH  || distance < WaterLOW ){
    delay(1000);
    if (distance > WaterHIGH  || distance < WaterLOW ){
    digitalWrite(PUMP1_CTR,LOW);// Tắt bơm 1
    PUMP1_STA = 0;
    delay(1000);
    digitalWrite(PUMP2_CTR,LOW);// Tắt bơm 2
    PUMP2_STA = 0;
    digitalWrite(Alarm,HIGH);// Bật chuông báo động

  }
  }
  else{
  digitalWrite(PUMP1_CTR,HIGH);// Bật bơm 1
  PUMP1_STA = 1; // Update tình trạng bơm 1
  delay(1000);
  digitalWrite(PUMP2_CTR,HIGH);// Bật bơm 2
  PUMP2_STA = 1;// Update tình trạng bơm 2
  digitalWrite(Alarm,LOW);
  }
}
void ManualCmdMode(void){
      if (digitalRead(PUMP1_BUTT) == 0){
    digitalWrite(PUMP1_CTR,HIGH);// Bật bơm 1
    PUMP1_STA = 1; // Update tình trạng bơm 1
  }
  else { 
    if (digitalRead(PUMP1_BUTT) == 1 ){
    digitalWrite(PUMP1_CTR,LOW);// Tắt bơm 1
    PUMP1_STA = 0;
  }
    }
  if ( digitalRead(PUMP2_BUTT) == 0 ){
    digitalWrite(PUMP2_CTR,HIGH);// Bật bơm 2
    PUMP2_STA = 1;// Update tình trạng bơm 2
    }
  else{ if ( digitalRead(PUMP2_BUTT) == 1 ){
    digitalWrite(PUMP2_CTR,LOW);
    PUMP2_STA = 0;
    }
  }
      digitalWrite(Alarm,LOW);
}
  
int GetDistance()
{
  long duration, distanceCm;
   
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  duration = pulseIn(ECHO_PIN, HIGH, TIME_OUT);

  distanceCm = duration / 29.1 / 2;
  return distanceCm;
}


int getAVRvalue(int anaPin)
{
  int anaValue = 0;
  for (int i = 0; i < 5; i++) // Đọc giá trị cảm biến và lấy giá trị trung bình
  {
    anaValue = anaValue + analogRead(anaPin);
    delay(5);
  }

  anaValue = anaValue / 5;
  anaValue = map(anaValue, 1023, 0, 0, 100); //Tối:0  ==> Sáng 100%

  return anaValue;
}

int getCondition(float temp)
{
  int i = 0;
  int anaValue = 0;
  for (i = 0; i < 5; i++)  //
  {
    anaValue += temp; //
    delay(5);   
  }
  anaValue = anaValue / 5;
  return anaValue;
}

void showTempHumid(int temp, int humid){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Nhiet do:");
    lcd.setCursor(10,0);
    lcd.print(temp);
    lcd.setCursor(12,0);
    lcd.write(1);
    lcd.setCursor(0,1);
    lcd.print("Do am:");
    lcd.setCursor(7,1);
    lcd.print(humid);
    lcd.setCursor(9,1);
    lcd.print("%");
};


void showLumen(int photosens){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Cuog do as: ");
    lcd.setCursor(11,0);
    lcd.print(photosens);
    lcd.setCursor(13,0);
    lcd.print("%");
    
  };

  
  void showDateTime(){
  lcd.clear();
  lcd.setCursor(0,0);
  if (dt.hour < 10){
    lcd.print(0);
    lcd.print(dt.hour);
    }
  else lcd.print(dt.hour);
  lcd.print(":");
   if (dt.minute < 10){
    lcd.print(0);
    lcd.print(dt.minute);
    }
   else lcd.print(dt.minute);
  lcd.print(" ");
  lcd.setCursor(10,0);
  if ( wifiControl == 0){
  if (MODE == 0){lcd.print("AUTO");}
  else {lcd.print("MANUAL");}
  }
  else lcd.print("WIFI");
  lcd.setCursor(0,1);
  lcd.print(dt.day);
  lcd.print("/");
  lcd.print(dt.month);
  lcd.print("/");
  lcd.print(dt.year);
  lcd.print(" ");
};


void showWaterLevel(long dist,long water){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Muc nuoc =");
  lcd.setCursor(11,0);
  lcd.print(dist);
  lcd.setCursor(13,0);
  lcd.print("cm");
  lcd.setCursor(0,1);
  lcd.print("Nhiet do nc: ");
  lcd.print(water);    
  lcd.write(1);  
}


void showPumpState(int a,int b){
lcd.clear();
lcd.setCursor(0,0);
lcd.print("May bom 1 ");
    if ( a == 1)
    {
      lcd.print("ON");
    }
    else { 
      if ( a == 0) {
      lcd.print("OFF");
    }
    }
lcd.setCursor(0,1);
lcd.print("May bom 2 ");
    if ( b == 1)
    {
      lcd.print("ON");
    }
    else { 
      if ( b == 0) {
      lcd.print("OFF");
    }
    }

};

void LCDshowMode(){
  if (digitalRead(LCDBUTTON) == LOW){
     i++;
        if (i == 5){
              i=0;
                   }
  }
  if (i == 0 ){
    showDateTime();
  }
  if (i == 1){
    showTempHumid(airt,h);
  }
  if (i == 2){
    showLumen(p);
  }
  if (i == 3){
    showWaterLevel(distance,watert);
    }
  if (i == 4){
    showPumpState(PUMP1_STA,PUMP2_STA);
    }
};
 


