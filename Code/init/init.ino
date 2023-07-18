#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h> 
#include <RFID.h>
#include "FirebaseESP8266.h" // Firebase library
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>

#define FIREBASE_HOST "rfid-access-control-system-default-rtdb.firebaseio.com" //Without http:// or https:// schemes
#define FIREBASE_AUTH "LYSF6ajLypDTuY7zyUuTPM5ah4XU3z4sX3Do76z3"
RFID rfid(D8, D0);       //D8:pin of tag reader SDA. D0:pin of tag reader RST 
unsigned char str[MAX_LEN]; //MAX_LEN is 16: size of the array

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char ssid[] = "dusha123";
const char pass[] = "dushyantha";

int BUZZER = D3;

String uidPath= "/";
//Define FirebaseESP8266 data object
FirebaseData firebaseData;

void connect() {
  Serial.print("Checking WiFi...");
  OLEDprint("Checking WiFi...", 20, 10);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\n Connected!");
  blank();
  OLEDprint("Connected!", 20, 10);
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
  
  connect();
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
}
void pushUser (String temp)    //Function to check if an identified tag is registered to allow access
{ 
   Serial.println("PUSHING USER ID: "+temp);
    blank();
    OLEDprint("User ID:", 25, 10);
    OLEDprint(temp, 40, 10);
    digitalWrite (BUZZER, HIGH);
    delay(500);
    digitalWrite (BUZZER, LOW);
    Firebase.setInt(firebaseData, uidPath+"Users/"+temp,0);
    delay(500);    
    blank();
}
void loop() {
  digitalWrite (BUZZER, LOW);
  OLEDprint("RFID Access Control", 25, 10);
  OLEDprint("System", 40, 50);
  if (rfid.findCard(PICC_REQIDL, str) == MI_OK)   //Wait for a tag to be placed near the reader
  { 
    Serial.println("Card Found");
    blank();
    OLEDprint("Card Found", 20, 10); 
    String temp = "";                             //Temporary variable to store the read RFID number
    if (rfid.anticoll(str) == MI_OK)              //Anti-collision detection, read tag serial number 
    { 
      Serial.print("The card's ID number is : "); 
      for (int i = 0; i < 4; i++)                 //Record and display the tag serial number 
      { 
        temp = temp + (0x0F & (str[i] >> 4)); 
        temp = temp + (0x0F & str[i]); 
      }
      delay(200);
      blank(); 
      Serial.println (temp);
      pushUser (temp);     //Check if the identified tag is an allowed to open tag
    } 
    rfid.selectTag(str); //Lock card to prevent a redundant read, removing the line will make the sketch read cards continually
  }
  rfid.halt();
}
