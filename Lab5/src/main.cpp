#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

// reset & slave select
#define RST_PIN D3
#define SS_PIN D8

// object
MFRC522 mfrc522(SS_PIN, RST_PIN);

String rfid_in = ""; 
// function
String dump_byte_array(byte *buffer, byte bufferSize);

const int CARD_WAIT = 0;
const int CARD_TOUCH = 1;
int state;

void setup() {
  state = CARD_WAIT;
  Serial.begin(115200);
  SPI.begin();
  mfrc522.PCD_Init();
  pinMode(D1, OUTPUT);
  // for showing on serial
  mfrc522.PCD_DumpVersionToSerial();
}

void loop() {
  if (state == CARD_WAIT){
    // check if card touching
    if(mfrc522.PICC_IsNewCardPresent()&&mfrc522.PICC_ReadCardSerial()){
      state = CARD_TOUCH;
    }
    delay(1000);
  }else if (state == CARD_TOUCH){
    // read RFID id
    rfid_in = dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);

    if (rfid_in == " D0 48 0A 32") {
      Serial.println("Welcome :" + rfid_in);
      digitalWrite(D1, HIGH);
      delay(1000);
      digitalWrite(D1, LOW);
    } else {
      Serial.println("Access Denied");
    }
    state = CARD_WAIT;
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