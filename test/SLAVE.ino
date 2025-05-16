#include <RH_ASK.h>
#include <RHReliableDatagram.h>
#include <SPI.h>

#define MY_ADDR     2    // Esta placa es Slave
#define PEER_ADDR   1    // Dirección del Master

// ASK a 1000 bps en RX=11, TX=12
RH_ASK driver(1000, 11, 12);
RHReliableDatagram manager(driver, MY_ADDR);

void setup() {
  Serial.begin(9600);
  while (!manager.init()) {
    Serial.println("Slave: fallo init, reintentando...");
    delay(500);
  }
  Serial.println("Slave listo (addr=2)");
}

void loop() {
  uint8_t inList[10];
  uint8_t inLen = sizeof(inList);
  uint8_t from;

  // 1) Recibo y envío ACK inmediato
  if (manager.recvfromAckTimeout(inList, &inLen, 1000, &from)) {
    Serial.print("Slave: recibido de "); Serial.print(from);
    Serial.print(" ("); Serial.print(inLen); Serial.print(" bytes): ");
    for (uint8_t i = 0; i < inLen; i++) {
      Serial.print(inList[i]); Serial.print(" ");
    }
    Serial.println();

    // 2) Preparo respuesta
    uint8_t outList[inLen];
    for (uint8_t i = 0; i < inLen; i++) {
      outList[i] = inList[i] * 2;
    }

    delay(20); // breve pausa para que Master esté en modo escucha

    // 3) Envío respuesta y espero ACK del Master
    Serial.println("Slave: enviando respuesta...");
    if (manager.sendtoWait(outList, inLen, PEER_ADDR)) {
      Serial.println("Slave: respuesta enviada con ACK.");
    } else {
      Serial.println("Slave: ERROR al enviar respuesta.");
    }
  }
}
