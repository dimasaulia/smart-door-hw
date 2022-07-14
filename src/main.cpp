#include <Arduino.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Key.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define I2CADDR 0x20 // Set the Address of the PCF8574
#define SS_PIN  15   
#define RST_PIN 4 
#define SSID "vivo 1718"
#define PASSWORD "12345678"
#define MOTOR_L 12
#define MOTOR_R 13

const byte ROWS = 4; // Set the number of Rows
const byte COLS = 4; // Set the number of Columns

String pinContainer;
String cardIdContainer;
String RUID = "gsg205";
String BASE_URL = "http://192.168.43.252:8000";
String CHECKIN_URL = "/api/v1/room/check-in/" + RUID;
String HW_MODE = "CHECKIN";

boolean isCardExist = false;

// Set the Key at Use (4x4)
char keys [ROWS] [COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// define active Pin (4x4)
byte rowPins [ROWS] = {0, 1, 2, 3}; // Connect to Keyboard Row Pin
byte colPins [COLS] = {4, 5, 6, 7}; // Connect to Pin column of keypad.

Keypad_I2C keypad (makeKeymap (keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574);

MFRC522 rfid(SS_PIN, RST_PIN);


int sendDataToServer(String endpoint, String payload){
  String url = BASE_URL + endpoint;
  HTTPClient clinet;
  clinet.begin(url);
  clinet.addHeader("Content-Type", "application/json");
  int httpCode = clinet.POST(payload);
  Serial.print("HTTP STATUS : ");
  Serial.println(httpCode);
  clinet.end();
  return httpCode;
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  SPI.begin(); // init SPI bus

  keypad.begin (makeKeymap (keys));
  rfid.PCD_Init(); // init MFRC522
  WiFi.begin(SSID, PASSWORD);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MOTOR_L, OUTPUT);
  pinMode(MOTOR_R, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println("Start listening key press : ");
  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
}

void loop() {
  char key = keypad.getKey(); 

  if(key == '#'){
    digitalWrite(MOTOR_L, LOW);
    digitalWrite(MOTOR_R, HIGH);
    delay(350);
  }


  if(key == '*'){
    if(HW_MODE == "CHECKIN"){
      HW_MODE = "REGISTER";
      digitalWrite(LED_BUILTIN, LOW);
    }else{
      HW_MODE = "CHECKIN";
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }

  if(key == 'C'){
    pinContainer = "";
    cardIdContainer = "";
    isCardExist = false;
    Serial.println("Clear all data");
  }

  if(key == 'D'){
      if(isCardExist){
        if(pinContainer.length() == 6){
          Serial.print("Data send to server : ");
          String payload = "{\"cardNumber\":\""+cardIdContainer+"\""+","+"\"pin\":\""+pinContainer+"\"}";
          Serial.println(payload);
          if(HW_MODE == "CHECKIN"){
            int httpCode = sendDataToServer(CHECKIN_URL, payload);
            if(httpCode == 200){
              Serial.println("RUANGAN BERHASIL TERBUKA");
                digitalWrite(MOTOR_L, HIGH);
                digitalWrite(MOTOR_R, LOW);
                delay(350);
            }
            if(httpCode == 400){
              Serial.println("RUANGAN GAGAL TERBUKA");
            }
          }

          if(HW_MODE == "REGISTER"){
            int httpCode = sendDataToServer("/api/v1/card/register", payload);
            if(httpCode == 201){
              Serial.println("CARD SUCCESSFULLY REGISTER");
            }
            if(httpCode == 500){
              Serial.println("CARD ALREADY REGISTER");
            }
          }
          pinContainer = "";
          cardIdContainer = "";
          isCardExist = false;
        }else{
          Serial.println("Min 6 pin char");
        }
      }else{
        Serial.println("Tap your card first");
      }
  }

  if(key){
    if(key != 'A' && key != 'B' && key != 'C' && key != 'D' && key != '*' && key != '#'){
      if(pinContainer.length() < 6){
        pinContainer += key;
      }
      Serial.print("Current Pin : ");
      Serial.println (pinContainer);
    }
  }
  
    if (rfid.PICC_IsNewCardPresent()) { // new tag is available
      if (rfid.PICC_ReadCardSerial()) { // NUID has been readed
          if(!isCardExist){
            for(byte i = 0; i < rfid.uid.size; i++){
              cardIdContainer.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
              cardIdContainer.concat(String(rfid.uid.uidByte[i], HEX));
            }

            Serial.print("CARD ID : ");
            Serial.println(cardIdContainer);
          }
      
        isCardExist = true;
      }
    }

}
