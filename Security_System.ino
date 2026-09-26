#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <LiquidCrystal.h>

// Pin Definitions
#define SS_PIN 10
#define RST_PIN 9
#define SERVO_PIN 6
#define BUZZER_PIN 3
#define GREEN_LED 4
#define RED_LED 5

// Direct 16-Pin LCD Setup (RS, E, D4, D5, D6, D7)
LiquidCrystal lcd(7, 8, A0, A1, A2, A3);

MFRC522 rfid(SS_PIN, RST_PIN);
Servo lockServo;

byte authorizedUID[4] = {0x01, 0x1B, 0x85, 0x6E}; 

enum SystemState {
  STATE_IDLE,
  STATE_VERIFYING,
  STATE_GRANTED,
  STATE_DENIED
};

SystemState currentState = STATE_IDLE;
unsigned long stateTimer = 0;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  
  lockServo.attach(SERVO_PIN);
  lockServo.write(0); // Locked position (0 degrees)
  
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  
  lcd.begin(16, 2);
  resetDisplay();
}

void loop() {
  unsigned long currentMillis = millis();

  switch (currentState) {
    case STATE_IDLE:
      digitalWrite(GREEN_LED, LOW);
      digitalWrite(RED_LED, LOW);
      digitalWrite(BUZZER_PIN, LOW);

      // Non-blocking card check
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        currentState = STATE_VERIFYING;
      }
      break;

    case STATE_VERIFYING:
      lcd.clear();
      
      // Compare scanned UID array against your authorized array
      if (memcmp(rfid.uid.uidByte, authorizedUID, 4) == 0) {
        lockServo.write(90); // Unlock servo (90 degrees)
        digitalWrite(GREEN_LED, HIGH);
        
        // Success audio beep
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100); 
        digitalWrite(BUZZER_PIN, LOW);

        lcd.setCursor(0, 0);
        lcd.print("ACCESS GRANTED!");
        lcd.setCursor(0, 1);
        lcd.print(" Door Unlocked ");

        stateTimer = currentMillis;
        currentState = STATE_GRANTED;
      } else {
        digitalWrite(RED_LED, HIGH);
        
        // Denied warning beep
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);

        lcd.setCursor(0, 0);
        lcd.print(" ACCESS DENIED ");
        lcd.setCursor(0, 1);
        lcd.print(" Invalid Card  ");

        stateTimer = currentMillis;
        currentState = STATE_DENIED;
      }
      rfid.PICC_HaltA(); // Re-arm reader for next scan
      break;

    case STATE_GRANTED:
      // Hold door open for 3 seconds using millis() timer
      if (currentMillis - stateTimer >= 3000) {
        lockServo.write(0); // Relock
        resetDisplay();
        currentState = STATE_IDLE;
      }
      break;

    case STATE_DENIED:
      // Hold warning message for 1.5 seconds using millis() timer
      if (currentMillis - stateTimer >= 1500) {
        resetDisplay();
        currentState = STATE_IDLE;
      }
      break;
  }
}

void resetDisplay() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  SECURITY HUB  ");
  lcd.setCursor(0, 1);
  lcd.print(" Scan Key Card ");
}