#include <SPI.h>
#include <LoRa.h>

//define the pins used by the transceiver module
#define ss 6
#define rst 5
#define dio0 2

uint32_t counter = 0;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Sender");
  LoRa.setPins(ss, rst, dio0);
  

  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }

  LoRa.setSyncWord(0xC4);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  counter++;  
  Serial.println("Sending");
  uint8_t temp = LoRa.readTemperature();
  LoRa.beginPacket();
  LoRa.print( "1" );
  LoRa.print( temp );
  LoRa.endPacket();
  delay(10000);
  LoRa.beginPacket();
  LoRa.print( "2" );
  LoRa.print( counter );
  LoRa.endPacket();
  delay(60000);
}
