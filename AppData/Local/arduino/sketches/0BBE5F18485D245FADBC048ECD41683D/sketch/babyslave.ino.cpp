#include <Arduino.h>
#line 1 "C:\\Users\\Ebrar Celik\\Documents\\Arduino\\babyslave\\babyslave.ino"
#include <SoftwareSerial.h>

SoftwareSerial BT(10, 11); // RX, TX

#line 5 "C:\\Users\\Ebrar Celik\\Documents\\Arduino\\babyslave\\babyslave.ino"
void setup();
#line 12 "C:\\Users\\Ebrar Celik\\Documents\\Arduino\\babyslave\\babyslave.ino"
void loop();
#line 5 "C:\\Users\\Ebrar Celik\\Documents\\Arduino\\babyslave\\babyslave.ino"
void setup() {
  Serial.begin(9600);       // Arduino Seri Monitor
  BT.begin(9600);          // HC-05 AT modu varsayılan baud rate

  Serial.println("HC-05 AT Komut Terminali Hazır");
}

void loop() {
  // Bilgisayardan gelen veriyi HC-05'e gönder
  if (Serial.available()) {
    BT.write(Serial.read());
  }

  // HC-05'ten gelen veriyi bilgisayara gönder
  if (BT.available()) {
    Serial.write(BT.read());
  }
}

