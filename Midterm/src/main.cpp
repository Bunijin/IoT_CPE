#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN D0
#define SS_PIN D8
#define BUTTON_PIN D4

String dump_byte_array(byte *buffer, byte bufferSize);
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int IDLE = 0;
const int DEDUCT_BALANCE = 1;
const int INSUFFICIENT_FUNDS = 2;
const int ADD_BALANCE = 3;
const int CANCEL_OPERATION = 4;

int state;
unsigned long stateStartTime = 0;
const unsigned long DISPLAY_DURATION = 10000; // 10 seconds

String rfid_in = "";
int balance = 100; // Initial balance of 100 baht
bool cardPresent = false;
bool buttonPressed = false;

void setup()
{
  Serial.begin(115200);
  // RFID setup
  SPI.begin();
  mfrc522.PCD_Init();
  mfrc522.PCD_DumpVersionToSerial();
  // LCD setup
  lcd.init();
  lcd.backlight();
  lcd.clear();
  // pin setup
  pinMode(BUTTON_PIN, INPUT);
  state = IDLE;
}

void loop()
{
  if (state == IDLE)
  {
    if (digitalRead(BUTTON_PIN) == HIGH)
    {
      state = ADD_BALANCE;
    }
    if (mfrc522.PICC_IsNewCardPresent()&&mfrc522.PICC_ReadCardSerial())
    {
      rfid_in = dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
      
  Serial.println(rfid_in);
      state = DEDUCT_BALANCE;
    }
    if (rfid_in == " D0 48 0A 32")
    {
      if (balance >= 35)
      {
        state = DEDUCT_BALANCE;
      }
      else
      {
        state = INSUFFICIENT_FUNDS;
      }
      stateStartTime = millis();
    }
    else if (digitalRead(BUTTON_PIN) == HIGH)
    {
      state = ADD_BALANCE;
      stateStartTime = millis();
    }
    lcd.clear();
  }
  else if (state == DEDUCT_BALANCE)
  {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Deducted 35 Baht");
      lcd.setCursor(0, 1);
      lcd.print("New balance: ");
      lcd.print(balance - 35);
      lcd.print(" Baht");
      delay(DISPLAY_DURATION);
      balance -= 35;
      state = IDLE;
    }
    
  
  else if (state == INSUFFICIENT_FUNDS)
  {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Insufficient");
      lcd.setCursor(0, 1);
      lcd.print("funds");
      delay(DISPLAY_DURATION);
      state = IDLE;
    
  }
  else if (state == ADD_BALANCE)
  {
    if (millis() - stateStartTime < DISPLAY_DURATION)
    {
      if (rfid_in == " D0 48 0A 32")
      {
        lcd.setCursor(0, 0);
        lcd.print("bal: ");
        lcd.print(balance + 100);
        lcd.print(" Baht");
      }
      else if (digitalRead(BUTTON_PIN) == HIGH)
      {
        state = CANCEL_OPERATION; // Changed from CANCEL to CANCEL_OPERATION
        stateStartTime = millis();
      }
    }
    else
    {
      balance += 100;
      state = IDLE;
    }
  }
  else if (state == CANCEL_OPERATION)
  {
    
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cancel");
    delay(DISPLAY_DURATION);
      state = IDLE;
    
  }
}

String dump_byte_array(byte *buffer, byte bufferSize) {
  String content = "";
  // เรียง RFID id
  for (byte i = 0; i < bufferSize; i++) {
    content.concat(String(buffer[i] < 0x10 ? " 0" : " "));
    content.concat(String(buffer[i], HEX));
  }
  content.toUpperCase();
  return content;
}