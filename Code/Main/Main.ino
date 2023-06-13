#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <string.h>

#define SS_PIN D8
#define RST_PIN D0

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MFRC522::MIFARE_Key key;

// Init array that will store new NUID
byte nuidPICC[4];
String UidArray[] = {" 61 AE 95 A9"};
int nUSER = sizeof(UidArray) / sizeof(UidArray[0]);
int BUZZER = D4;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER, OUTPUT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  blank();  
  pixel(10, 10);
  
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522
  Serial.println();
  Serial.print(F("Reader :"));
  rfid.PCD_DumpVersionToSerial();
  pixel(10, 20);

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  Serial.println();
  Serial.println(F("This code scan the MIFARE Classic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
  pixel(10, 30);
  delay(500);
  blank();
}

void loop() {
  digitalWrite (BUZZER, LOW);
  OLEDprint("RFID Access Control", 10, 10);
  OLEDprint("System", 20, 50);
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  String uid = getHex(rfid.uid.uidByte, rfid.uid.size);
  bool found = validUid(uid, UidArray, nUSER);

  blank();
  OLEDprint("User ID:", 10, 10);
  OLEDprint(uid, 20, 10);
  
  if (found) {
    OLEDprint("ACCESS GRANTED", 40, 20);
    digitalWrite (BUZZER, HIGH);
    delay(1000);
    digitalWrite (BUZZER, LOW);
  } else {
    OLEDprint("ACCESS DENIED", 40, 20);
    digitalWrite (BUZZER, HIGH);
    delay(300);
    digitalWrite (BUZZER, LOW);
    delay(100);
    digitalWrite (BUZZER, HIGH);
    delay(500);
    digitalWrite (BUZZER, LOW);
  }
  delay(3000);
  blank();

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}


/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
   Helper routine to dump a byte array as dec values to Serial.
*/
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}

void pixel(int row, int column){
  display.drawPixel(column, row, WHITE);
  display.display();
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

String getHex(byte *buffer, byte bufferSize) {
  String hexString;

  for (byte i = 0; i < bufferSize; i++) {
    hexString += (buffer[i] < 0x10 ? " 0" : " ");
    hexString += String(buffer[i], HEX);
  }

  hexString.toUpperCase();  // Convert all letters to uppercase

  return hexString;
}

bool validUid(String searchString, String array[], int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    if (searchString.equals(array[i])) {
      return true;  // String found in array
    }
  }
  return false;  // String not found in array
}
