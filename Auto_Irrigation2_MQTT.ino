#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
LiquidCrystal_I2C lcd(0x3F,16,2);
#include "DS3231.h"
#include <OneWire.h>
#include "DallasTemperature.h"
#include <SoftwareSerial.h>
#include "WiFiEsp.h"
#include <PubSubClient.h>
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

//Chân chọn chế độ điều khiển
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

#define WaterLOW 90
#define WaterHIGH 30
#define Arlam 12

// Biến đọc chế độ AUTO hay MANUAl
boolean MODE;
boolean wifiControl = 0;
boolean pump1c,pump2c;
// Biến lưu giá trị đọc được
float airt ;
float h;
int p;
long distance;
int watert;
//int waterf;
char ssid[] = "Cube V";            
char pass[] = "anhquan2211";       
int status = WL_IDLE_STATUS;     

const char* server = "demo-arisite.net";
const char* mqtt_server = "io.adafruit.com";
#define IO_USERNAME  "nguyenduongquan26"
#define IO_KEY       "f550821d71a3440ab5256d694b721f93"
String data;
unsigned long lastConnectionTime_1 = 0;         
const unsigned long postingInterval_1 = 20000L;

WiFiEspClient espClient;
PubSubClient client(espClient);
void setup() 
{
  Serial.begin(115200);
  Serial1.begin(9600);
  WiFi.init(&Serial1);
  setupWifi();
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);
  dht.begin();
  tempsensor.begin();
  lcd.init();  
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("AUTO IRRIGATION");
  pinMode (PHOTOSENSOR1, INPUT);
  pinMode(LCDBUTTON, INPUT_PULLUP);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PUMP1_BUTT,INPUT_PULLUP);
  pinMode(PUMP2_BUTT,INPUT_PULLUP);
  pinMode(PUMP1_CTR,OUTPUT);
  pinMode(PUMP2_CTR,OUTPUT);
  pinMode(MODE_PIN,INPUT_PULLUP);
  pinMode(Arlam,OUTPUT);
  clock.begin();
 // clock.setDateTime(__DATE__, __TIME__);
  lcd.createChar(1,degree);
  delay(1000);
}
 
void loop() 
{
      if (!client.connected()) {
        reconnect();
      }
      client.loop();
      modeselect(digitalRead(MODE_PIN)); 
      getData();
      LCDshowMode();

//       if (millis() - lastConnectionTime_1 > postingInterval_1) {
//        client.disconnect();
//       data += String("?pump1s=")+PUMP1_STA+String("&pump2s=")+PUMP2_STA;
//       data += String("&mode=")+MODE+String("&airt=")+airt+String("&hum=")+h;
//       data += String("&pts=")+p+String("&dis=")+distance+String("&watert=")+watert+String("&waterf=")+2;
//       senddata();
//       data.remove(0);
//      }

}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    wifiControl = 1;
    Serial.println(wifiControl);
  } else {
    wifiControl = 0; 
    Serial.println(wifiControl);
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client",IO_USERNAME,IO_KEY)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.subscribe("controlTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(3000);
    }
  }
}
void setupWifi(){
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("Khong tim thay WiFi ESP8266");
    while (true);
  }

  // ket noi den wifi
  while ( status != WL_CONNECTED) {
    Serial.print("Dang connect toi WPA SSID: ");
    Serial.println(ssid);
  
    status = WiFi.begin(ssid, pass);
  }
}

void getData(){
      tempsensor.requestTemperatures(); 
      dt = clock.getDateTime();   
      airt = getCondition(dht.readTemperature());
      watert= getCondition(tempsensor.getTempCByIndex(0)); 
      h = getCondition(dht.readHumidity());
      p = getAVRvalue(PHOTOSENSOR1);
      distance = getCondition(GetDistance());
  }
  
void modeselect(int val){
  if ( wifiControl == 0 ){
    if (val == HIGH){
    MODE = 0;
    autoPump();
    }
  else { if (val == LOW){
      MODE = 1;
      ManualCmdMode();
      }
  }
  }
  else { if ( wifiControl == 1 ){
    ManualCmdMode();
    }
    }
  }

void senddata()
{
  espClient.stop();
  
  if (espClient.connect(server, 3000)) {
    Serial.println("OK");
  espClient.print(String("GET /sensor/create"));
  espClient.print(data); espClient.println(F(" HTTP/1.1"));
  espClient.print(F("Host: ")); espClient.println(server);
  espClient.println(F("Connection: close"));
  espClient.println();
  lastConnectionTime_1 = millis();
  
  }
  
  else {
    Serial.println("Connection failed");
  }
}

//void printWifiStatus()
//{
//  // print the SSID of the network you're attached to
//  Serial.print("SSID: ");
//  Serial.println(WiFi.SSID());
//
//  // print your WiFi shield's IP address
//  IPAddress ip = WiFi.localIP();
//  Serial.print("IP Address: ");
//  Serial.println(ip);
//
//  // print the received signal strength
//  long rssi = WiFi.RSSI();
//  Serial.print("Signal strength (RSSI):");
//  Serial.print(rssi);
//  Serial.println(" dBm");
//}


void autoPump(){
  if ( distance > WaterLOW  || distance < WaterHIGH ){
    digitalWrite(PUMP1_CTR,LOW);// Tắt bơm 1
    PUMP1_STA = 0;
    digitalWrite(PUMP2_CTR,LOW);// Tắt bơm 2
    PUMP2_STA = 0;
    digitalWrite(Arlam,HIGH);// Bật chuông báo động
    
  }
  else{
  digitalWrite(PUMP1_CTR,HIGH);// Bật bơm 1
  PUMP1_STA = 1; // Update tình trạng bơm 1
  digitalWrite(PUMP2_CTR,HIGH);// Bật bơm 2
  PUMP2_STA = 1;// Update tình trạng bơm 2
    
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
      if (debounce(PUMP1_BUTT) == 0){
    digitalWrite(PUMP1_CTR,HIGH);// Bật bơm 1
    PUMP1_STA = 1; // Update tình trạng bơm 1
  }
  else { 
    if (debounce(PUMP1_BUTT) == 1 ){
    digitalWrite(PUMP1_CTR,LOW);// Tắt bơm 1
    PUMP1_STA = 0;
  }
    }
  if ( debounce(PUMP2_BUTT) == 0 ){
    digitalWrite(PUMP2_CTR,HIGH);// Bật bơm 2
    PUMP2_STA = 1;// Update tình trạng bơm 2
    }
  else{ if ( debounce(PUMP2_BUTT) == 1 ){
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
  for (int i = 0; i < 10; i++) // Đọc giá trị cảm biến 10 lần và lấy giá trị trung bình
  {
    anaValue = anaValue + analogRead(anaPin);
    delay(5);
  }

  anaValue = anaValue / 10;
  anaValue = map(anaValue, 1023, 0, 0, 100); //Tối:0  ==> Sáng 100%

  return anaValue;
}

int getCondition(float temp)
{
  int i = 0;
  int anaValue = 0;
  for (i = 0; i < 10; i++)  //
  {
    anaValue += temp; //
    delay(5);   
  }
  anaValue = anaValue / 10;
  return anaValue;
}

void showTempHumid(int temp, int humid){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Nhiet do:");
    lcd.setCursor(10,0);
    lcd.print(temp+2);
    lcd.setCursor(12,0);
    lcd.write(1);
    lcd.print("C");
    lcd.setCursor(0,1);
    lcd.print("Do am:");
    lcd.setCursor(7,1);
    lcd.print(humid-3);
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
  lcd.print(dt.hour);
  lcd.print(":");
  lcd.print(dt.minute);
  lcd.print(":");
  lcd.print(dt.second);
  lcd.print(" ");
  lcd.setCursor(10,0);
  if ( wifiControl == 0 )
  {
  if (MODE == 0){lcd.print("AUTO");}
  else {lcd.print("MANUAL");}
  }
  else if (wifiControl == 1){
    lcd.print("WIFI");
    }
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
  const int debounceDelay = 10;
  previousState = digitalRead(pinIN);
  for (int counter = 0; counter < debounceDelay; counter++)
  {
    delay(1);
    state = digitalRead(pinIN);
    if (state != previousState)
    {
      counter = 0;
      previousState = state;
    }
  }
  return state;
}  


