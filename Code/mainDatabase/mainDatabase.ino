#include <ESP8266WiFi.h>
#include <Wire.h> 
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>
#include <SPI.h> 
#include <RFID.h>
#include "FirebaseESP8266.h"  // Install Firebase ESP8266 library
#include <NTPClient.h>
#include <WiFiUdp.h>

#define FIREBASE_HOST "rfid-access-control-system-default-rtdb.firebaseio.com" //Without http:// or https:// schemes
#define FIREBASE_AUTH "LYSF6ajLypDTuY7zyUuTPM5ah4XU3z4sX3Do76z3"

RFID rfid(D8, D0);       //D10:pin of tag reader SDA. D9:pin of tag reader RST 
unsigned char str[MAX_LEN]; //MAX_LEN is 16: size of the array 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiUDP ntpUDP;
const long utcOffsetInSeconds = 19800; //(UTC+5:30)
NTPClient timeClient(ntpUDP, "pool.ntp.org");

const char ssid[] = "Avishka Redmi";
const char pass[] = "avishkacherath";

int BUZZER = D3;

String uidPath= "/";
FirebaseJson json;

FirebaseData firebaseData;  //Define FirebaseESP8266 data object

unsigned long lastMillis = 0;
String alertMsg;
String device_id="mainScanner";
boolean checkIn = true;

void connect() {
  Serial.print("Checking WiFi ...");
  OLEDprint("Checking WiFi ...", 30, 10);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\n Connected!");
  blank();
  OLEDprint("Connected!", 30, 35);
  delay(1000);
  blank();
}

void blank(){
  display.clearDisplay();
  display.display();
 }

 void OLEDprint(String text,int row,int column){
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(column, row);
  display.println(text);
  display.display();
 }

void setup()
{

  Serial.begin(115200);
  WiFi.begin(ssid, pass);
  
  pinMode(BUZZER, OUTPUT);

   // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  blank();
  
  SPI.begin();
  rfid.init();
  
  timeClient.begin();
  timeClient.setTimeOffset(utcOffsetInSeconds);
  connect();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}
void checkAccess (String temp)    //Function to check if an identified tag is registered to allow access
{
    if(Firebase.getInt(firebaseData, uidPath+"/Users/"+temp)){
      
      if (firebaseData.intData() == 0)         //If firebaseData.intData() == checkIn
      {  
          alertMsg="CHECKING IN";
          blank();
          OLEDprint("User ID :", 10, 10);
          OLEDprint(temp, 25, 30);       
          OLEDprint("ACCESS GRANTED", 45, 20);
          
          digitalWrite (BUZZER, HIGH);
          delay(1000);
          digitalWrite (BUZZER, LOW);

          blank();
          OLEDprint("Checking IN", 30, 30);
          
          json.add("time", String(timeClient.getFormattedDate()));
          json.add("id", device_id);
          json.add("uid", temp);
          json.add("status",1);                

          Firebase.setInt(firebaseData, uidPath+"/Users/"+temp,1);
          
          if (Firebase.pushJSON(firebaseData, uidPath+ "/Attendence", json)) {
            Serial.println(firebaseData.dataPath() + firebaseData.pushName()); 
          } else {
            Serial.println(firebaseData.errorReason());
          }
          blank();
      }      
      else if (firebaseData.intData() == 1)   //If the lock is open then close it
      { 
          alertMsg="CHECKING OUT";
          blank();
          OLEDprint("User ID :", 10, 10);
          OLEDprint(temp, 25, 30);
          OLEDprint("ACCESS GRANTED", 45, 20);
          
          digitalWrite (BUZZER, HIGH);
          delay(1000);
          digitalWrite (BUZZER, LOW);

          blank();
          OLEDprint("Checking OUT", 30, 30);

          Firebase.setInt(firebaseData, uidPath+"/Users/"+temp,0);
          
          json.add("time", String(timeClient.getFormattedDate()));
          json.add("id", device_id);
          json.add("uid", temp);
          json.add("status",0); 
          
          if (Firebase.pushJSON(firebaseData, uidPath+ "/Attendence", json)) {
            Serial.println(firebaseData.dataPath() + firebaseData.pushName()); 
          } else {
            Serial.println(firebaseData.errorReason());
          }
          blank();
      } 
    }
    else
    {
      Serial.println("FAILED");

      blank();
      OLEDprint("User ID :", 10, 10);
      OLEDprint(temp, 25, 30);
      OLEDprint("ACCESS DENIED", 45, 20);
      
      digitalWrite (BUZZER, HIGH);
      delay(300);
      digitalWrite (BUZZER, LOW);
      delay(100);
      digitalWrite (BUZZER, HIGH);
      delay(500);
      digitalWrite (BUZZER, LOW);  
      
      Serial.println("REASON: " + firebaseData.errorReason());
      blank();
    }
}
void loop() {
  digitalWrite (BUZZER, LOW);
  
  OLEDprint("RFID Access Control", 10, 10);
  OLEDprint("System", 25, 50);
  OLEDprint("- Place ID to scan -", 50, 5);
  
  timeClient.update();
  if (rfid.findCard(PICC_REQIDL, str) == MI_OK)   //Wait for a tag to be placed near the reader
  { 
    Serial.println("Card found");
    blank();
    OLEDprint("Scanning ...", 25, 30);
     
    String temp = "";                             //Temporary variable to store the read RFID number
    if (rfid.anticoll(str) == MI_OK)              //Anti-collision detection, read tag serial number 
    { 
      Serial.print("The card's ID number is : "); 
      for (int i = 0; i < 4; i++)                 //Record and display the tag serial number 
      { 
        temp = temp + (0x0F & (str[i] >> 4)); 
        temp = temp + (0x0F & str[i]); 
      } 
      Serial.println (temp);
      checkAccess (temp);     //Check if the identified tag is an allowed to open tag
    } 
    rfid.selectTag(str); //Lock card to prevent a redundant read, removing the line will make the sketch read cards continually
  }
  rfid.halt();
}
