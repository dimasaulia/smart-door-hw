#include <Arduino.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Key.h>
#include <Keypad.h>
#include <Keypad_I2C.h>
#include <Wire.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

#define I2CADDR 0x20 // Set the Address of the PCF8574
#define SS_PIN  15   
#define RST_PIN 4 
#define SSID "vivo 1718"
#define PASSWORD "12345678"
#define MOTOR_L 12
#define MOTOR_R 13
#define BUZZ_PIN 17
#define LED 16
#define LCD_RST 14
#define LCD_DC 27
#define LCD_CS 18

const byte ROWS = 4; // Set the number of Rows
const byte COLS = 4; // Set the number of Columns

String pinContainer;
String cardIdContainer;
String RUID = "gsg205";
String BASE_URL = "https://smart-door-pnj.herokuapp.com";
String CHECKIN_URL = "/api/v1/room/check-in/" + RUID;
boolean is_checkin = true;
boolean isCardExist = false;

// Set the Key at Use (4x4)
char keys [ROWS] [COLS] = {
  {'D', '#', '0', '*'},
  {'C', '9', '8', '7'},
  {'B', '6', '5', '4'},
  {'A', '3', '2', '1'},
};

// define active Pin (4x4)
byte rowPins [ROWS] = {0, 1, 2, 3}; // Connect to Keyboard Row Pin
byte colPins [COLS] = {4, 5, 6, 7}; // Connect to Pin column of keypad.

Keypad_I2C keypad (makeKeymap (keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574);
MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);


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

void BUZZER_ON(){
  digitalWrite(BUZZ_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZ_PIN, LOW);
}

void BUZZER_SUCCESS(){
  digitalWrite(BUZZ_PIN, HIGH);
  delay(1250);
  digitalWrite(BUZZ_PIN, LOW);
}

void BUZZER_FAILED(){
  digitalWrite(BUZZ_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZ_PIN, LOW);
  delay(100);
  digitalWrite(BUZZ_PIN, HIGH);
  delay(100);
  digitalWrite(BUZZ_PIN, LOW);
  delay(100);
  digitalWrite(BUZZ_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZ_PIN, LOW);
}

void setUpText() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(30, 30);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.println("SMART DOOR");
  tft.setCursor(80, 60);
  tft.println("LOCK!!");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(20, 90);
  tft.setTextSize(2);
  tft.println("Tap Your Card and");
  tft.setCursor(35, 110);
  tft.println("Enter Your Pin");
}

void writePin(String pin) {
  setUpText();
  tft.setCursor(75, 140);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setTextWrap(false);
  if(pin.length() == 0){
    tft.println("------");
  }else{
    String text = "";
    for (size_t i = 0; i < pin.length(); i++) {
      text+="*";
    }

    for (size_t i = 0; i < (6 - (pin.length())); i++) {
      text+="-";
    }
    tft.println(text);
  }
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  keypad.begin (makeKeymap (keys));
  tft.init(240, 240, SPI_MODE2);    // Init ST7789 display 240x240 pixel
  tft.setRotation(2);
  delay(100);
  rfid.PCD_Init(); // init MFRC522
  WiFi.begin(SSID, PASSWORD);
  pinMode(LED, OUTPUT);
  pinMode(BUZZ_PIN, OUTPUT);
  pinMode(MOTOR_L, OUTPUT);
  pinMode(MOTOR_R, OUTPUT);

  digitalWrite(LED, HIGH);
  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Setup Room 
  String payload = "{\"ruid\":\"" + RUID +  "\"}";

  int httpCode = sendDataToServer("/api/v1/room/get-or-create", payload);
  Serial.print("Room Setup Status : ");
  Serial.println(httpCode);
  Serial.println("Connected to the WiFi network");
  Serial.println("Start listening key press : ");
  Serial.println("Tap an RFID/NFC tag on the RFID-RC522 reader");
  BUZZER_SUCCESS();
}


void loop() {
  char key = keypad.getKey(); 

  if(key == '#'){
    digitalWrite(MOTOR_L, LOW);
    digitalWrite(MOTOR_R, HIGH);
    BUZZER_ON();
    delay(350);
  }


  if(key == '*'){
    BUZZER_ON();
    pinContainer = "";
    cardIdContainer = "";
    isCardExist = false;
    Serial.println("Clear all data & change mode");

    if(is_checkin == true){
     is_checkin = false;
      digitalWrite(LED, LOW);
    }else{
      is_checkin = true;
      digitalWrite(LED, HIGH);
    }
  }

  if(key == 'C'){
    BUZZER_ON();
    pinContainer = "";
    cardIdContainer = "";
    isCardExist = false;
    Serial.println("Clear all data");
  }

  if(key == 'D'){
      BUZZER_ON();
      if(isCardExist){
        if(pinContainer.length() == 6){
          Serial.print("Data send to server : ");
          String payload = "{\"cardNumber\":\""+cardIdContainer+"\""+","+"\"pin\":\""+pinContainer+"\"}";
          Serial.println(payload);
          if(is_checkin == true){
            int httpCode = sendDataToServer(CHECKIN_URL, payload);
            if(httpCode == 200){
                Serial.println("RUANGAN BERHASIL TERBUKA");
                digitalWrite(MOTOR_L, HIGH);
                digitalWrite(MOTOR_R, LOW);
                BUZZER_SUCCESS();
                delay(350);
            }
            if(httpCode == 400){
              BUZZER_FAILED();
              Serial.println("RUANGAN GAGAL TERBUKA");
            }
          }

          if(is_checkin == false){
            int httpCode = sendDataToServer("/api/v1/card/register", payload);
            if(httpCode == 201){
              BUZZER_SUCCESS();
              Serial.println("CARD SUCCESSFULLY REGISTER");
            }
            if(httpCode == 500){
              BUZZER_FAILED();
              Serial.println("CARD ALREADY REGISTER");
            }
          }
          pinContainer = "";
          cardIdContainer = "";
          isCardExist = false;
        }else{
          BUZZER_FAILED();
          Serial.println("Min 6 pin char");
        }
      }else{
        BUZZER_FAILED();
        Serial.println("Tap your card first");
      }
  }

  if(key){
    if(key != 'A' && key != 'B' && key != 'C' && key != 'D' && key != '*' && key != '#'){
      BUZZER_ON();
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
              BUZZER_ON();
              for(byte i = 0; i < rfid.uid.size; i++){
                cardIdContainer.concat(String(rfid.uid.uidByte[i] < 0x10 ? " 0" : " "));
                cardIdContainer.concat(String(rfid.uid.uidByte[i], HEX));
              }

              Serial.print("CARD ID : ");
              Serial.println(cardIdContainer);
        
              isCardExist = true;
            }
        }
      }
  writePin(pinContainer);
}
