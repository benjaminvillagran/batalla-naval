//Librerías matriz y RF
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>

#include <RH_ASK.h>
#include <SPI.h>

//Colores
#define BLACK    0x0000
#define BLUE     0x001F
#define RED      0xF800
#define GREEN    0x07E0
#define WHITE    0xFFFF
#define CYAN     0x07FF
#define YELLOW   0xFFE0

//Joystick y botones
const int x_axis = A0;
const int y_axis = A1;
const int rot = 8;
const int selec = 3;
const int velocidad = 200;

//Turno y Estado de barcos enviados.
bool turno = false; //si es true, es mi turno. si es false, es el turno del oponente.
bool barcos_enviados = false; 

//------------------CARACTERISTICAS BARCOS------------------------
//Modelo (estructura) de barcos
struct Barcos{
  uint8_t x;
  uint8_t y;
  uint8_t w;
  uint8_t h;
};
//Array barcos propios
Barcos barco[10];   // Array para guardar todos los barcos colocados
//Array barcos recibidos 
Barcos barco_rec[10];

uint8_t indice = 0;    // Cantidad de barcos colocados

//-------------VARIABLES----------------
//Posición y tamaño
int x = 0;
int y = 0;
uint8_t w = 0;
uint8_t h = 0;

//Ataques realizados [2da Parte del Juego]
uint8_t disparos_ex = 0;
const uint8_t pixeles_barcos = 20;
bool disparo[8][8] = {false};
bool disparo_rec[8][8] = {false};



//-----------------DECLARACION MATRIZ Y RF---------------
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, 6,
NEO_MATRIX_TOP + NEO_MATRIX_LEFT +
NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
NEO_GRB + NEO_KHZ800);

RH_ASK rf_driver;

void setup() {
  Serial.begin(9600);
  matrix.begin();
  matrix.clear();
  matrix.setBrightness(10);

  rf_driver.init();
  delay(100);

  pinMode(selec, INPUT);
  pinMode(rot, INPUT_PULLUP);
  //A0 y A1 ya definidos como input

  // Tamaños de los barcos predefinidos
  barco[0].w = 4; barco[0].h = 1; //Barco 0
  barco[1].w = 3; barco[1].h = 1; //Barco 1
  barco[2].w = 3; barco[2].h = 1; //Barco 2
  barco[3].w = 2; barco[3].h = 1; //Barco 3
  barco[4].w = 2; barco[4].h = 1; //Barco 4
  barco[5].w = 2; barco[5].h = 1; //Barco 5
  barco[6].w = 1; barco[6].h = 1; //Barco 6
  barco[7].w = 1; barco[7].h = 1; //Barco 7
  barco[8].w = 1; barco[8].h = 1; //Barco 8
  barco[9].w = 1; barco[9].h = 1; //Barco 9
  
}
//-----------------EVITAR SUPERPOSICION DE BARCOS-------------------------
bool colision(int nuevox, int nuevoy, int nuevow, int nuevoh){
  //Iteración para comparar con todos los barcos
  for(int i = 0; i < indice; i++){
    uint8_t refx = barco[i].x;
    uint8_t refy = barco[i].y; 
    uint8_t refw = barco[i].w;
    uint8_t refh = barco[i].h; 
    
  //Condiciones, comparación de lados
  //Si no se cumple alguna de las condiciones, está colisionando o en el lado contrario al dispuesto
  
    if(!(nuevox + nuevow - 1 < refx || //Lado derecho del barco nuevo con el izquierdo de los demás (true = b_nuevo izquierda)
       nuevox > refx + refw - 1 ||     //Lado izquierdo de barco nuevo con el derecho del los demás (true = b_nuevo derecha)
       nuevoy + nuevoh - 1 < refy ||   //Lado inferior de barco nuevo con el superior de los demás (true = b_nuevo arriba)
       nuevoy > refy + refh - 1)){     //Lado superior de barco nuevo con el inferior de los demás (true = b_nuevo abajo)
      return true;
    }
  }
  return false;
}
//------------------DETECAR ATAQUE EXITOSO (CURSOR)------------------------------
  bool ataque_exitoso(int cursorx, int cursory, Barcos* barco_ref) {
    for (int i = 0; i < 10; i++) { // revisar los 10 barcos
      uint8_t refx = barco_ref[i].x;
      uint8_t refy = barco_ref[i].y;
      uint8_t refw = barco_ref[i].w;
      uint8_t refh = barco_ref[i].h;
  
      if (cursorx >= refx &&        //colision con extremo izquierdo de barco
          cursorx < refx + refw &&  //colision con extremo derecho de barco
          cursory >= refy &&        //colision con extremo superior de barco
          cursory < refy + refh) {  //colisión con extremo inferior de barco
          return true;
      }
    }
    return false;
  }
  
//--------------------VOID LOOP--------------------------------
void loop() {
  //NUEVOS CONTROLES
  if(analogRead(x_axis) > 900){
    x++;
    delay(velocidad);
  }
  else if(analogRead(x_axis) < 100){
    x--;
    delay(velocidad);
  }
  if(analogRead(y_axis) > 900){
    y++;
    delay(velocidad);
  }
  else if(analogRead(y_axis) < 100){
    y--;
    delay(velocidad);
  }
  else if(digitalRead(rot) == LOW){
    rotar();
    delay(300);
  }
  
  //-----------LIMITES-----------------------------------
  if (x > 8 - barco[indice].w) x = 8 - barco[indice].w;
  if (x < 0) x = 0;
  if (y > 8 - barco[indice].h) y = 8 - barco[indice].h;
  if (y < 0) y = 0;

  //-------------------ESTABLECER BARCOS-------------------
  if(barcos_enviados == false){
    if(digitalRead(selec) == HIGH){ //Cuando se presione el boton Selec, se creará un barco con las caracteristicas
      if(!colision(x, y, barco[indice].w, barco[indice].h)){
        barco[indice].x = x;
        barco[indice].y = y;
        indice++;
        Serial.println(indice);
        delay(200);
      }
      else{
        matrix.drawRect(x, y, barco[indice].w, barco[indice].h, RED);
        matrix.show();
        delay(500);
      } 
    } 
  }

  //---------------DIBUJADO DE BARCOS----------------------
  matrix.clear();
  
  // Dibujar todos los barcos azules
  for (int i = 0; i < indice; i++) {
    matrix.drawRect(barco[i].x, barco[i].y, barco[i].w, barco[i].h, BLUE);
  }
  
  //Dibujar el cursor
  if(barcos_enviados == false){
    matrix.drawRect(x, y, barco[indice].w, barco[indice].h, WHITE);
    matrix.show();
    delay(10);
  }
 

  //-----------------ENVIO DE DATOS---------------------
  if(barcos_enviados == false && indice == 10) { //si los barcos no han sido enviados y el indice llega a 10
    barcos_enviados = true;  //los barcos han sido enviados una sola vez
    uint8_t datos[40];  // 10 barcos × 4 bytes cada uno = 40 bytes
    memcpy(datos, barco, sizeof(barco));  // Copia los datos binarios de la estructura

    rf_driver.send(datos, sizeof(datos));  // Envía los datos por RF
    rf_driver.waitPacketSent();
    Serial.println("Datos de barcos enviados");
  }

  //---------------RECEPCION DE DATOS----------------------
  uint8_t buf[40];
  uint8_t buflen = sizeof(buf);
  if(rf_driver.recv(buf, &buflen)){
    Serial.println("Datos recibidos");
    if(buflen == 40){
      memcpy(barco_rec, buf, sizeof(barco_rec));

      if(barcos_enviados == false){ //Si recibo mis barcos y no envie los mios, no es mi turno.
        turno = false;
      }
      else{ //Si recibo barcos y envie los mios, es mi turno.
        turno = true; 
      }
      
    }
    else{
      Serial.print("Tamaño inesperado: ");
      Serial.println(buflen);
    }
  }
  //--------------------------SEGUNDA PARTE DEL JUEGO-------------------------
  if(barcos_enviados == true){
    //----------------------CASO: MI TURNO-------------------------
    if(turno == true){
      //------------------------ESTABLECER DISPAROS------------------------
      if(digitalRead(selec) == HIGH && !disparo[x][y]){ //Si se presiona selec y no hay un disparo en esa coordenada...
        disparo[x][y] = true; //Se registra tal cordenada como disparada
        if(ataque_exitoso(x, y, barco_rec)){
          disparos_ex++;        
        }
        else{
          turno = false; 
        }
        //Enviar coordenadas del disparo realizado al oponente
        uint8_t coord[2] = {(uint8_t)x, (uint8_t)y};
        rf_driver.send(coord, sizeof(coord));
        rf_driver.waitPacketSent();
        
        Serial.println("Disparo enviado en"); Serial.print(coord[0]); Serial.print(","); Serial.print(coord[1]);
      }
      //---------------------DIBUJAR DISPAROS---------------------
      matrix.clear();
      //Dibujar disparos realizados
      for(int i = 0; i < 8; i++){
        for(int j = 0; j <8; j++){
          if(disparo[i][j]){
            if(ataque_exitoso(i, j, barco_rec)) matrix.drawPixel(i, j, RED);
            else matrix.drawPixel(i, j, WHITE);
          }
        }
      }
      //Mostrar cursor
      matrix.drawPixel(x, y, YELLOW);
      
      matrix.show();    
      delay(10);
    }
    //-----------------------CASO: TURNO DEL OPONENTE------------------------------------
    if(turno == false){
      for (int i = 0; i < indice; i++) {
        matrix.drawRect(barco[i].x, barco[i].y, barco[i].w, barco[i].h, CYAN);
      }
  
      //Recibir coordenadas de disparo del oponente
      uint8_t coord_rec[2];
      uint8_t coord_reclen = sizeof(coord_rec);
      
      if(rf_driver.recv(coord_rec, &coord_reclen)){
        if(coord_reclen == 2){
          
        //Coordenadas x, y recibidas
        uint8_t x_rec = coord_rec[0];
        uint8_t y_rec = coord_rec[1];
        
        disparo_rec[x_rec][y_rec] = true; //Disparo en coordenada x, y = true
        
        if(ataque_exitoso(x_rec, y_rec, barco)) matrix.drawPixel(x_rec, y_rec, RED), turno = false;
        else matrix.drawPixel(x_rec, y_rec, WHITE), turno = true;
  
        Serial.println("Disparo recibido en: ");
        Serial.print(x_rec); Serial.print(", "); Serial.print(y_rec);
        }
      }
    }
  }
}
//ROTAR BARCOS
void rotar(){
  uint8_t temp = barco[indice].w;
  barco[indice].w = barco[indice].h;
  barco[indice].h = temp;
}
