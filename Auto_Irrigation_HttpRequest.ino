#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
LiquidCrystal_I2C lcd(0x3F,16,2);
#include "DS3231.h"
#include <OneWire.h>
#include "DallasTemperature.h"
#include <SoftwareSerial.h>
#include "WiFiEsp.h"

SoftwareSerial Serial1(10, 11); // RX, TX

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
boolean PUMP1_STA = 0;
boolean PUMP2_STA = 0;
#define PUMP1_BUTT 2
#define PUMP2_BUTT 3
#define PUMP1_CTR 4
#define PUMP2_CTR 5

#define WaterLOW 60
#define WaterHIGH 30
#define Alarm 12

// Biến đọc chế độ AUTO hay MANUAl
boolean MODE;
boolean wifiControl;
boolean pump1c,pump2c;
// Biến lưu giá trị đọc được
float airt ;
float h;
int p;
int distance;
int watert;
//int waterf;
char ssid1[] = "vldt";            
char pass1[] = "vldt38300595";   
char ssid2[] = "Cube V";            
char pass2[] = "anhquan2211";           
int status = WL_IDLE_STATUS;     

char server[] = "share102.com";

unsigned long lastConnectionTime_1 = 0;
unsigned long lastConnectionTime_2 = 0;           
#define postingInterval_1  30000
#define postingInterval_2  1000
WiFiEspClient client;

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
  Serial.begin(115200);
  Serial1.begin(9600);
  WiFi.init(&Serial1);
  dht.begin();
  tempsensor.begin();
  // kiem tra xem có wifi shield ko

if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Khong co wifi shield");
  }

  // ket noi den wifi
  while ( status != WL_CONNECTED) {

    status = WiFi.begin(ssid1, pass1);
    status = WiFi.begin(ssid2, pass2);
  }

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
      
      modeselect(digitalRead(MODE_PIN)); 
      LCDshowMode();
      getData();
       if (millis() - lastConnectionTime_1 > postingInterval_1) {
        String data= "";
             data += String("?pump1s=")+PUMP1_STA+String("&pump2s=")+PUMP2_STA;
             data += String("&mode=")+MODE+String("&airt=")+airt+String("&hum=")+h;
             data += String("&pts=")+p+String("&dis=")+distance+String("&watert=")+watert+String("&waterf=")+2;
      senddata(data);
      }
        askcmd();
}

void getData(){
      tempsensor.requestTemperatures(); 
      dt = clock.getDateTime();   
      airt = dht.readTemperature();
      watert= tempsensor.getTempCByIndex(0); 
      h = dht.readHumidity();
      p = getAVRvalue(PHOTOSENSOR1);
      distance = GetDistance();
  }
  
void modeselect(int val){
    if (val == HIGH){
    if (wifiControl == 0){
        autoPump();
        MODE = 0;
 //   Serial.println("Auto Mode ");
    }
    else { if ( wifiControl == 1){
      ManualCmdMode();
      }
    }
    }
  else { if (val == LOW){
      MODE = 1;
      ManualCmdMode();
 //     Serial.println("Manual Mode ");
      }
  }
  }

void senddata(String data)
{ 

     client.stop();
  if (client.connect(server, 3000)) {
    Serial.println("OK");
  client.print(String("GET /sensor/create"));
  client.print(data); client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.println(server);
  client.println(F("Connection: close"));
  client.println();
  lastConnectionTime_1 = millis();
  }
  
  else {
    Serial.println("Connection failed");
  }
  client.stop();
}

void askcmd()
{
  if (client.connect(server, 3000)) {
  client.print(String("GET /status/get"));
  client.println(F(" HTTP/1.1"));
  client.print(F("Host: ")); client.println(server);
  client.println(F("Connection: close"));
  client.println();
  
  String response = "";
    while (client.available()) {
      char c = client.read();
      response += c;
      }
  if( response.indexOf("\"sth\":true")> 0 ){
 
      wifiControl = 1;
        if( response.indexOf("\"pump1c\":true")> 0 ){
  
        pump1c = 1;
      }
        else { 
            if ( response.indexOf("\"pump1c\":false")> 0){
              pump1c = 0;
                  }
              }       
        if ( response.indexOf("\"pump2c\":true")>0){
  
              pump2c=1;
              }
        else{ 
              if ( response.indexOf("\"pump2c\":false")> 0){
  
               pump2c = 0;
              }
            }
  }
   else{ 
        if (response.indexOf("\"sth\":false")> 0){
        wifiControl = 0;
           }
        }
}
//lastConnectionTime_2 = millis();
client.stop();
}

void autoPump(){
  if ( distance > WaterLOW  || distance < WaterHIGH ){
    delay(100);
    if (distance > WaterLOW  || distance < WaterHIGH ){
    digitalWrite(PUMP1_CTR,LOW);// Tắt bơm 1
    PUMP1_STA = 0;
    delay(1000);
    digitalWrite(PUMP2_CTR,LOW);// Tắt bơm 2
    PUMP2_STA = 0;
    delay(1000);
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

  if ( wifiControl == 1){
  if ( pump1c == 1 ){
    digitalWrite(PUMP1_CTR,HIGH);// Bật bơm 1
    PUMP1_STA = 1; // Update tình trạng bơm 1
  }
  else { 
    if ( pump1c == 0){
    digitalWrite(PUMP1_CTR,LOW);// Tắt bơm 1
    PUMP1_STA = 0;
  }
    }
  if ( pump2c == 1){
    digitalWrite(PUMP2_CTR,HIGH);// Bật bơm 2
    PUMP2_STA = 1;// Update tình trạng bơm 2
    }
  else{ if ( pump2c == 0 ){
    digitalWrite(PUMP2_CTR,LOW);
    PUMP2_STA = 0;
    }
  }
  }
  else {
    if ( wifiControl == 0 ){
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
      } 
  }
  
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


int getAVRvalue(byte anaPin)
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
  if (debounce(LCDBUTTON) == LOW){
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
 
  
boolean debounce(byte pinIN){
  boolean state;
  boolean previousState;
  const int debounceDelay = 5;

  previousState = digitalRead(pinIN);
  for (int counter = 0; counter < debounceDelay; counter++)
  {
    delay(5);
    state = digitalRead(pinIN);
    if (state != previousState)
    {
      counter = 0;
      previousState = state;
    }
  }
  return state;
}  


