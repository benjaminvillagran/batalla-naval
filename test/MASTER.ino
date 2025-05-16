#include <RH_ASK.h>
#include <RHReliableDatagram.h>
#include <SPI.h>

#define MY_ADDR     1     // Esta placa es Master
#define PEER_ADDR   2     // Dirección del Slave

// Configuro ASK a 1000 bps en pines por defecto (RX=11, TX=12)
RH_ASK driver(1000, 11, 12);
RHReliableDatagram manager(driver, MY_ADDR);

void setup() {
  Serial.begin(9600);
  while (!manager.init()) {
    Serial.println("Master: fallo init, reintentando...");
    delay(500);
  }
  Serial.println("Master listo (addr=1)");
}

void loop() {
  uint8_t outList[] = {10, 20, 30, 40, 50};
  uint8_t outLen = sizeof(outList);

  Serial.println("Master: enviando datos al Slave...");
  if (manager.sendtoWait(outList, outLen, PEER_ADDR)) {
    Serial.println("Master: datos enviados con ACK, esperando respuesta...");

    // Espero respuesta hasta 500 ms
    uint8_t inList[10];
    uint8_t inLen = sizeof(inList);
    uint8_t from;
    if (manager.recvfromAckTimeout(inList, &inLen, 500, &from)) {
      Serial.print("Master: recibido de "); Serial.print(from);
      Serial.print(" ("); Serial.print(inLen); Serial.print(" bytes): ");
      for (uint8_t i = 0; i < inLen; i++) {
        Serial.print(inList[i]); Serial.print(" ");
      }
      Serial.println();
    } else {
      Serial.println("Master: tiempo agotado esperando respuesta.");
    }
  } else {
    Serial.println("Master: NO recibí ACK del Slave.");
  }

  delay(2000); // envío cada 2 segundos
}
