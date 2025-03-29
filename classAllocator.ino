#include <SPI.h>
#include <MFRC522.h>

#include <WiFi.h>

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <HTTPClient.h>

//iphone hotspot doesn't work
const char* ssid = "wifi_name_android";
const char* password = "xxxx";

#define redPin 32
#define greenPin 12

#define singleClassBtn 26
#define doubleClassBtn 27
#define okBtn 25

constexpr uint8_t RST_PIN = 4;  // Configurable, see typical pin layout above
constexpr uint8_t SS_PIN = 5;   // Configurable, see typical pin layout above

MFRC522 rfid(SS_PIN, RST_PIN);  // Instance of the class
MFRC522::MIFARE_Key key;

String tag;

String server = "https://class-allocator-api.up.railway.app/api/v2/classrooms/hwClass";
String server2 = "https://class-allocator-api.up.railway.app/api/v2/teachers/check?card=";
String serverPath = server;

String res;
int httpResponse;

LiquidCrystal_I2C lcd(0x27, 16, 2);  //lcd address, character count, line count

String arr[] = { "10:00", "10:50", "11:40", "12:45", "01:35", "02:25", "03:30", "04:20", "05:10" };

int count = 0;
int btn1 = 0, btn2 = 0, btn3 = 0;

void setup() {
  Serial.begin(9600);
  lcd.init();       // Initialize the LCD
  lcd.backlight();  // Turn on the backlight
  lcd.clear();
  SPI.begin();  // Init SPI bus

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");

  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(singleClassBtn, INPUT);
  pinMode(doubleClassBtn, INPUT);
  pinMode(okBtn, INPUT);
  rfid.PCD_Init();  // Init MFRC522

  lcd.setCursor(0, 0);
  lcd.print("Connected.");
  delay(1000);

  lcd.setCursor(0, 0);
  lcd.print("Scan your Card");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  HTTPClient http;
  serverPath = server2;
  tag = "";
  btn3 = 0;
  btn2 = 0;
  btn1 = 0;
  if (rfid.PICC_ReadCardSerial()) {
    for (byte i = 0; i < 4; i++) {
      tag += rfid.uid.uidByte[i];
    }

    Serial.println("tag: " + tag + " ");

    serverPath = serverPath + tag;

    http.begin(serverPath.c_str());

    lcd.setCursor(0, 0);
    lcd.print("Authenticating..");

    httpResponse = http.GET();
    Serial.println(httpResponse);

    res = http.getString();

    lcd.setCursor(0, 0);
    lcd.print(res);

    http.end();  //free resources

    if (httpResponse == 200) {
      serverPath = server;
      Serial.println("Access Granted.");

      delay(2000);
      lcd.setCursor(0, 0);

      lcd.print("Start Time ->  ");

      Serial.println(arr[count]);

      while (count >= 0) {
        lcd.setCursor(0, 1);
        lcd.print(arr[count]);
        Serial.println(arr[count]);

        while (btn1 == 0 && btn2 == 0 && btn3 == 0) {
          btn1 = digitalRead(singleClassBtn);
          btn2 = digitalRead(doubleClassBtn);
          btn3 = digitalRead(okBtn);
          // Serial.println(btn3);
        }
        if (btn1 == 1 && count != 0) {
          count = count - 1;
        } else if (btn2 == 1 && count != 8) {
          count = count + 1;
        } else if (btn3 == 1) {
          Serial.println("break");
          break;
        }
        btn1 = 0;
        btn2 = 0;
        delay(1000);
      }
      btn3 = 0;
      lcd.setCursor(0, 0);
      lcd.print("Select Class    ");
      lcd.setCursor(0, 1);
      lcd.print("Single    Double");



      while (btn1 == 0 && btn2 == 0) {
        btn1 = digitalRead(singleClassBtn);
        btn2 = digitalRead(doubleClassBtn);
      }
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 0);

      if (btn1 == 1) {
        int temp = count + 1;

        serverPath = serverPath + "?slot=" + temp + "&classspan=1&room=403&card=" + tag;

        http.begin(serverPath.c_str());
        httpResponse = http.GET();
        Serial.println(httpResponse);
        res = http.getString();
        Serial.println(res);

        lcd.setCursor(0, 0);
        lcd.print(res);

        if (httpResponse == 400) {
          lcd.setCursor(0, 0);
          lcd.print("Scan your Card  ");
          return;
        }
        Serial.println("Single Class Booked.");
        // http.end();
      } else if (btn2 == 1) {
        int temp = count + 1;

        serverPath = serverPath + "?slot=" + temp + "&classspan=2&room=403&card=" + tag;

        http.begin(serverPath.c_str());
        httpResponse = http.GET();
        Serial.println(httpResponse);

        res = http.getString();
        Serial.println(res);

        lcd.print(res);
        if (httpResponse == 400) {
          lcd.setCursor(0, 0);
          lcd.print("Scan your Card  ");
          return;
        }
        // http.end();
        Serial.println("Double Class Booked.");
        // lcd.print("Class Booked.");
      } else {
      }
      btn1 = 0;
      btn2 = 0;
      btn3 = 0;
      digitalWrite(greenPin, HIGH);
      delay(1000);
      digitalWrite(greenPin, LOW);
    } else {

      http.end();
      Serial.println("Access Denied.");
      digitalWrite(redPin, HIGH);
      delay(1000);
      digitalWrite(redPin, LOW);
      // return;
    }
    tag = "";
    count = 0;
  }
  lcd.setCursor(0, 0);
  lcd.print("Scan your Card");
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}