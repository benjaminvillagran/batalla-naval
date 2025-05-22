#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <RH_ASK.h>
#include <SPI.h>
#include "Arduino.h"
// Colores
#define NEGRO    0x0000
#define AZUL     0x001F
#define ROJO     0xF800
#define VERDE    0x07E0
#define BLANCO   0xFFFF
#define CIAN     0x07FF
#define AMARILLO 0xFFE0

// Pines y botones
const int EJE_X    = A0;
const int EJE_Y    = A1;
const int BTN_ROTAR  = 8;
const int BTN_SELEC  = 3;
const int VELOCIDAD  = 200;

// Comunicación RF
enum Placeholder{}; // para compatibilidad de redefinición
RH_ASK rf_driver(1000, 11, 12);

// Matriz NeoPixel
Adafruit_NeoMatrix matriz = Adafruit_NeoMatrix(
  8, 8, 6,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ800
);

// Estado de juego
bool miTurno        = false;
bool barcosEnviados = false;

// Estructura de barco
typedef struct { uint8_t x, y, w, h; } Barco;
Barco misBarcos[10];
Barco barcosOponente[10];
uint8_t numBarcos = 0;

// Disparos
bool disparos[8][8]         = {false};
bool disparosOponente[8][8] = {false};

// Cursor y rotación
uint8_t cursorX = 0, cursorY = 0;

// Función para actualizar pantalla sin interrupciones
void mostrarSeguro() {
  noInterrupts();
  matriz.show();
  interrupts();
}

// Colisión de barcos
bool colision(uint8_t nx, uint8_t ny, uint8_t nw, uint8_t nh) {
  for (uint8_t i = 0; i < numBarcos; i++) {
    Barco &b = misBarcos[i];
    if (!(nx + nw - 1 < b.x || nx > b.x + b.w - 1 ||
          ny + nh - 1 < b.y || ny > b.y + b.h - 1)) return true;
  }
  return false;
}

//pegaste?
bool impacto(uint8_t x, uint8_t y, Barco *flota) {
  for (uint8_t i = 0; i < 10; i++) {
    Barco &b = flota[i];
    if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h)
      return true;
  }
  return false;
}

void setup() {
  Serial.begin(9600);
  matriz.begin();
  matriz.clear();
  matriz.setBrightness(10);

  rf_driver.init();
  pinMode(BTN_SELEC, INPUT);
  pinMode(BTN_ROTAR, INPUT_PULLUP);

  // Tamaños predeterminados de barcos
  uint8_t dims[10][2] = {{4,1},{3,1},{3,1},{2,1},{2,1},{2,1},{1,1},{1,1},{1,1},{1,1}};
  for (uint8_t i = 0; i < 10; i++) {
    misBarcos[i].w = dims[i][0];
    misBarcos[i].h = dims[i][1];
  }
}

void loop() {
  // Leer joystick
  if (analogRead(EJE_X) > 900 && cursorX < 8 - misBarcos[numBarcos].w) { cursorX++; delay(VELOCIDAD); }
  if (analogRead(EJE_X) < 100 && cursorX > 0)                         { cursorX--; delay(VELOCIDAD); }
  if (analogRead(EJE_Y) > 900 && cursorY < 8 - misBarcos[numBarcos].h) { cursorY++; delay(VELOCIDAD); }
  if (analogRead(EJE_Y) < 100 && cursorY > 0)                         { cursorY--; delay(VELOCIDAD); }
  
  // Rotar barco actual
  if (digitalRead(BTN_ROTAR) == LOW) {
    Barco &b = misBarcos[numBarcos];
    uint8_t t = b.w; b.w = b.h; b.h = t;
    delay(300);
  }

  // Limites cursor
  if (cursorX > 8 - misBarcos[numBarcos].w) cursorX = 8 - misBarcos[numBarcos].w;
  if (cursorY > 8 - misBarcos[numBarcos].h) cursorY = 8 - misBarcos[numBarcos].h;
  if (cursorX < 0) cursorX = 0;
  if (cursorY < 0) cursorY = 0;

  // Colocar barcos la primera fase
  if (!barcosEnviados) {
    // Dibujar barcos actuales + cursor
    matriz.clear();
    for (uint8_t i = 0; i < numBarcos; i++)
      matriz.drawRect(misBarcos[i].x, misBarcos[i].y, misBarcos[i].w, misBarcos[i].h, AZUL);
    matriz.drawRect(cursorX, cursorY, misBarcos[numBarcos].w, misBarcos[numBarcos].h, BLANCO);
    mostrarSeguro();

    // Seleccionar posición
    if (digitalRead(BTN_SELEC) == HIGH) {
      if (!colision(cursorX, cursorY, misBarcos[numBarcos].w, misBarcos[numBarcos].h)) {
        misBarcos[numBarcos].x = cursorX;
        misBarcos[numBarcos].y = cursorY;
        numBarcos++;
        delay(200);
        if (numBarcos == 10) {
          // Enviar barcos
          uint8_t buf[40];
          memcpy(buf, misBarcos, sizeof(buf));
          rf_driver.send(buf, sizeof(buf));
          rf_driver.waitPacketSent();
          barcosEnviados = true;
        }
      } else {
        matriz.drawRect(cursorX, cursorY, misBarcos[numBarcos].w, misBarcos[numBarcos].h, ROJO);
        mostrarSeguro(); delay(500);
      }
    }
    return;
  }

  // Recibir bardos de oponente una sola vez
  if (barcosEnviados && !miTurno) {
    uint8_t buf[40], len=40;
    if (rf_driver.recv(buf, &len)) {
      memcpy(barcosOponente, buf, sizeof(buf));
      miTurno = true; // empieza quien recibe
    }
  }

  // Mi turno disparar
  matriz.clear();
  if (miTurno) {
    // Dibujar mis disparos
    for (uint8_t i = 0; i < 8; i++) for (uint8_t j = 0; j < 8; j++)
      if (disparos[i][j])
        matriz.drawPixel(i, j, impacto(i,j, barcosOponente)? ROJO: BLANCO);
    matriz.drawPixel(cursorX, cursorY, AMARILLO);
    mostrarSeguro();

    if (digitalRead(BTN_SELEC)==HIGH && !disparos[cursorX][cursorY]) {
      disparos[cursorX][cursorY] = true;
      if (!impacto(cursorX,cursorY,barcosOponente)) miTurno = false;
      uint8_t c[2] = {cursorX, cursorY};
      rf_driver.send(c, 2);
      rf_driver.waitPacketSent();
      delay(200);
    }
  } else {
    //Dibujar mis barcos y disparos oponente
    for (uint8_t i=0; i<numBarcos; i++)
      matriz.drawRect(misBarcos[i].x, misBarcos[i].y, misBarcos[i].w, misBarcos[i].h, CIAN);
    for (uint8_t i=0; i<8; i++) for (uint8_t j=0; j<8; j++)
      if (disparosOponente[i][j])
        matriz.drawPixel(i, j, impacto(i,j, misBarcos)? ROJO: BLANCO);
    mostrarSeguro();

    // Recibir disparo
    uint8_t c[2], l=2;
    if (rf_driver.recv(c,&l)) {
      disparosOponente[c[0]][c[1]] = true;
      if (!impacto(c[0],c[1], misBarcos)) miTurno = true;
    }
  }
}
