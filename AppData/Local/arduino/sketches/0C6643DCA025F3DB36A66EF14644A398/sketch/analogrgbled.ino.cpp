#include <Arduino.h>
#line 1 "C:\\Users\\Ebrar Celik\\Documents\\Arduino\\analogrgbled\\analogrgbled.ino"
#include <SoftwareSerial.h>

SoftwareSerial BT(10, 11); // RX, TX

#line 5 "C:\\Users\\Ebrar Celik\\Documents\\Arduino\\analogrgbled\\analogrgbled.ino"
void setup();
#line 11 "C:\\Users\\Ebrar Celik\\Documents\\Arduino\\analogrgbled\\analogrgbled.ino"
void loop();
#line 5 "C:\\Users\\Ebrar Celik\\Documents\\Arduino\\analogrgbled\\analogrgbled.ino"
void setup() {
  BT.begin(9600);
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP); // Buton
}

void loop() {
  if (digitalRead(2) == LOW) {
    BT.println("B");       // "B" harfi gönder
    Serial.println("Gönderildi: B");
    delay(500);            // Gecikme
  }
}

