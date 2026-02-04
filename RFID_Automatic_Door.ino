#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* ===== PIN DEFINITIONS ===== */
#define SS_PIN   15   // D8
#define RST_PIN  16   // D0

#define SERVO_PIN 0   // D3

#define RED_LED   5   // D1
#define GREEN_LED 4   // D2

// OLED on RX / TX
#define OLED_SDA 3    // RX (GPIO3)
#define OLED_SCL 1    // TX (GPIO1)

/* ===== OBJECTS ===== */
MFRC522 mfrc522(SS_PIN, RST_PIN);
Servo doorServo;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

/* ===== ALLOWED RFID CARDS ===== */
byte allowedCards[][4] = {
  {0xCF, 0x72, 0x4A, 0xEE},
  {0x8F, 0x59, 0x35, 0xEE}
};
const byte TOTAL_CARDS = 2;

/* ===== OLED TWO-LINE BIG FONT FUNCTION ===== */
void oledMsg2(const char* line1, const char* line2) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(0, 10);
  display.println(line1);

  display.setCursor(0, 34);
  display.println(line2);

  display.display();
}

/* ===== RFID CHECK ===== */
bool isAuthorized() {
  for (byte i = 0; i < TOTAL_CARDS; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (mfrc522.uid.uidByte[j] != allowedCards[i][j]) {
        match = false;
        break;
      }
    }
    if (match) return true;
  }
  return false;
}

/* ===== DOOR CONTROL ===== */
void openDoor() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(GREEN_LED, HIGH);

  oledMsg2("UNLOCKING", "WAIT....");

  doorServo.attach(SERVO_PIN);
  for (int pos = 0; pos <= 180; pos++) {
    doorServo.write(pos);
    delay(15);
  }

  oledMsg2("PROCEED", "NOW >>>");
  delay(8000);

  // 👇 NEW MESSAGE WHILE CLOSING
  oledMsg2("GATE", "IS LOCKING");

  for (int pos = 180; pos >= 0; pos--) {
    doorServo.write(pos);
    delay(15);
  }
  doorServo.detach();

  digitalWrite(GREEN_LED, LOW);
  digitalWrite(RED_LED, HIGH);

  oledMsg2("GATE", "LOCKED");
  delay(2000);
  oledMsg2("SCAN RFID", "CARD");
}

/* ===== SETUP ===== */
void setup() {
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);

  Serial.begin(9600);

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();

  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  oledMsg2("SCAN RFID", "CARD");

  Serial.println("RFID SYSTEM READY");
}

/* ===== LOOP ===== */
void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  if (isAuthorized()) {
    Serial.println("AUTH SUCCESS");
    oledMsg2("AUTH", "SUCCESS");
    openDoor();
  } else {
    Serial.println("AUTH FAILED");
    oledMsg2("ACCESS", "DENIED");
    delay(2000);
    oledMsg2("SCAN RFID", "CARD");
  }

  mfrc522.PICC_HaltA();
}
