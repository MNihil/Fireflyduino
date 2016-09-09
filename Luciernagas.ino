/* Codigo para lampara de luciernagas led
 *
 * Copyright (C) Todos los derechos reservados 2014 Maximiliano Rodriguez Porras
 *
 * Este codigo no presenta ninguna garantia de funcionamiento ni de ningun otro tipo.
 * 
 * El presente codigo simula un conjunto de luciernagas que encienden y apagan de manera
 * pseudo aleatoria y controlada de manera paralela.
 *
 * Se controlan mediante un conjunto de arrays, llevando un registro de control de pwm,
 * fases o estados, tiempo de encendido maximo.
 * 
 * PWM_Luc: Este arreglo de registros de tipo byte lleva un registro del pwm de cada
 * luciernaga, con el se controla la intensidad de encendido que nos permite realizar
 * un encendido en rampa o el que nosotros elijamos.
 *
 * Fases: Arreglo de registros de tipo byte, lleva el registro de las fases en las que
 * se encuentra cada luciernaga, existen 4 fases:
 *  0: Apagada
 *  1: Encendiendose
 *  2: Climax
 *  3: Apagandose
 *
 * contClimax:Arreglo de registros de tipo byte, lleva la cuenta del numero de interrupciones
 * que se ha mantenido en climax.
 * 
 * En el loop principal se realiza el encendido y apagado de las luciernagas, controlando el
 * tiempo de encendido por la variable PWM_Luc, que a su vez sera modificada por la interrupcion
 * del timer.
 * 
 * En la fucion de la interrupcion se realizan cambios a las variables de control, se toma en
 * cuenta el limite de luciernagas maximo a generar, el numero de interrupciones para encender una
 * nueva luciernaga, creando un nuevo numero de interrupciones para generar la siguiente de manera
 * pseuoaleatoria.
 */


#include <avr/io.h>
#include <avr/interrupt.h>

// Declaracion de constantes
#define INICIO_PINES (2)
#define FIN_PINES (17)
#define TOTAL_PINES (FIN_PINES-INICIO_PINES+1)
#define PIN_RUIDO (19)
//#define LUC_MAX (3)
#define MS_MIN_NUEVA_LUC (1500)
#define MS_MAXS_NUEVA_LUC (10000)
#define MS_MIN_ENCENDIDO (50)
#define MS_MAX_ENCENDIDO (350)
#define MS_MIN_APAGADO (250)
#define MS_MAX_APAGADO (500)
#define MS_MIN_CLIMAX (300)
#define MS_MAX_CLIMAX (500)
#define LIMITE_PWM (255)
#define VALOR_CTC (4000)

struct datos{
  byte fase=0;
  byte PWM_LED=0;
  byte patron=0;
  unsigned int tiempoNuevaLuc=0;
  unsigned int tiempoTranscurridoApagado=0;
  unsigned int tiempoMsEncendido=0;
  unsigned int tiempoMsClimax=0;
  unsigned int tiempoMsApagado=0;
  unsigned int contadorMsEncendido=0;
  unsigned int contadorMsClimax=0;
  unsigned int contadorMsApagado=0;
} luciernagas[TOTAL_PINES];
// Numero de luciernagas encendidas
byte lucEncendidas = 0;
// Semilla inicial de la funcion RANDOM
long Randseed = 11121983;
// Contador PWM 
byte contadorPWM=0;

unsigned int rand(unsigned int randMax);

void setup(){
  /* Inicializamos los pines como salidas
   * Los pines comienzan desde 2 al 17, pueden ser del 2 al 19 siempre y cuando se conecten
   * los pines A5 y A6, tambien seria bueno dejar un pin analogico para lectura de ruido como
   * generador de semilla aleatorio.
   */
  for(int i=INICIO_PINES;i<=FIN_PINES;i++){
    pinMode(i,OUTPUT);
  }
  // Definimos las interrupciones
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = VALOR_CTC;
  TCCR1B |= (1<<WGM12);
  TCCR1B |= (1<<CS10);
  TIMSK1 |= (1<<OCIE1A);
  sei();
  
  for(int pin=0;pin<TOTAL_PINES;pin++){
    luciernagas[pin].tiempoNuevaLuc=rand(MS_MAXS_NUEVA_LUC)+MS_MIN_NUEVA_LUC;
  }
}

 /*Funcion de loop
 * La funcion verifica en que fase se encuentra cada luciernaga, si se encuentra
 * en la fase 1 incrementa su valor de PWM hasta llegar al valorLimitePWM donde lo pasa a la
 * fase 2, en la fase 2 se lleva una cuenta del contClimax de cada luciernaga hasta alcanzar
 * el limite de climax donde lo pasa a la fase 3, en la fase 3 decrese el PWM de cada luciernaga
 * hasta llegar a 0, cuando esto ocurre se descuenta uno a las lucEncendidas.
 *
 * En caso de haber transcurrido los milisegundos suficientes y no se haya excedido el numero
 * maximo de luciernagas encendidas, se selecciona una nueva luciernaga de manera aleatoria, que,
 * si esta en fase 0, se setea en fase 1 y se añade uno a las luciernagas encendidas.
 */
void loop(){
  unsigned int newRandValue = 0;
  for(int k=0;k<TOTAL_PINES;k++){
    switch(luciernagas[k].fase){
      case 0:
        luciernagas[k].tiempoTranscurridoApagado+=1;
        if(luciernagas[k].tiempoTranscurridoApagado >= luciernagas[k].tiempoNuevaLuc){
          lucEncendidas++;
          luciernagas[k].tiempoNuevaLuc = rand(MS_MAXS_NUEVA_LUC)+MS_MIN_NUEVA_LUC;
          luciernagas[k].tiempoMsEncendido = rand(MS_MAX_ENCENDIDO-MS_MIN_ENCENDIDO)+MS_MIN_ENCENDIDO;
          luciernagas[k].tiempoMsClimax = rand(MS_MAX_CLIMAX-MS_MIN_CLIMAX)+MS_MIN_CLIMAX;
          luciernagas[k].tiempoMsApagado = rand(MS_MAX_APAGADO-MS_MIN_APAGADO)+MS_MIN_APAGADO;
          luciernagas[k].patron=rand(4);
          switch(luciernagas[k].patron){
            case 0:
            case 1:
            case 2:
              luciernagas[k].fase=1;
              break;
            case 3:
              luciernagas[k].fase=4;
              luciernagas[k].tiempoMsEncendido/=3;
              luciernagas[k].tiempoMsApagado/=3;
              luciernagas[k].tiempoMsClimax/=3;
              break;
          }
        }
        break;
      case 1:
        luciernagas[k].contadorMsEncendido+=1;
        luciernagas[k].PWM_LED=map(luciernagas[k].contadorMsEncendido,0,luciernagas[k].tiempoMsEncendido,0,LIMITE_PWM);
        if(luciernagas[k].contadorMsEncendido >= luciernagas[k].tiempoMsEncendido) {
          luciernagas[k].fase = 2;
          luciernagas[k].PWM_LED = LIMITE_PWM;
          luciernagas[k].contadorMsEncendido = 0;
        }
        break;
      case 2:
        luciernagas[k].contadorMsClimax+=1;
        if(luciernagas[k].contadorMsClimax == luciernagas[k].tiempoMsClimax) {
          luciernagas[k].fase = 3; 
          luciernagas[k].contadorMsClimax = 0;}
        break;
      case 3:
        luciernagas[k].contadorMsApagado += 1;
        luciernagas[k].PWM_LED = map(luciernagas[k].contadorMsApagado,0,luciernagas[k].tiempoMsApagado,LIMITE_PWM,0);
        if(luciernagas[k].contadorMsApagado >= luciernagas[k].tiempoMsApagado){
          lucEncendidas--;
          luciernagas[k].PWM_LED = 0;
          luciernagas[k].fase = 0;
          luciernagas[k].tiempoTranscurridoApagado = 0;
          luciernagas[k].contadorMsApagado = 0;
        }
        break;
      case 4:
        luciernagas[k].contadorMsEncendido += 1;
        luciernagas[k].PWM_LED = map(luciernagas[k].contadorMsEncendido,0,luciernagas[k].tiempoMsEncendido,0,LIMITE_PWM);
        if(luciernagas[k].contadorMsEncendido >= luciernagas[k].tiempoMsEncendido) {
          luciernagas[k].fase = 5;
          luciernagas[k].PWM_LED = LIMITE_PWM;
          luciernagas[k].contadorMsEncendido = 0;
        }
        break;
      case 5:
        luciernagas[k].contadorMsClimax += 1;
        if(luciernagas[k].contadorMsClimax == luciernagas[k].tiempoMsClimax) {
          luciernagas[k].fase = 6;
          luciernagas[k].contadorMsClimax = 0;
        }
        break;
      case 6:
        luciernagas[k].contadorMsApagado += 1;
        luciernagas[k].PWM_LED = map(luciernagas[k].contadorMsApagado,0,luciernagas[k].tiempoMsApagado,LIMITE_PWM,0);
        if(luciernagas[k].contadorMsApagado == luciernagas[k].tiempoMsApagado){
          luciernagas[k].PWM_LED = 0;
          luciernagas[k].fase = 7;
          luciernagas[k].contadorMsApagado = 0;
        }
        break;
      case 7:
        luciernagas[k].contadorMsEncendido += 1;
        luciernagas[k].PWM_LED = map(luciernagas[k].contadorMsEncendido,0,luciernagas[k].tiempoMsEncendido,0,LIMITE_PWM);
        if(luciernagas[k].contadorMsEncendido >= luciernagas[k].tiempoMsEncendido) {
          luciernagas[k].fase = 8;
          luciernagas[k].PWM_LED = LIMITE_PWM;
          luciernagas[k].contadorMsEncendido = 0;
        }
        break;
      case 8:
        luciernagas[k].contadorMsClimax += 1;
        if(luciernagas[k].contadorMsClimax == luciernagas[k].tiempoMsClimax) {
          luciernagas[k].fase = 9;
          luciernagas[k].contadorMsClimax = 0;
        }
        break;
      case 9:
        luciernagas[k].contadorMsApagado += 1;
        luciernagas[k].PWM_LED = map(luciernagas[k].contadorMsApagado,0,luciernagas[k].tiempoMsApagado,LIMITE_PWM,0);
        if(luciernagas[k].contadorMsApagado == luciernagas[k].tiempoMsApagado){
          lucEncendidas--;
          luciernagas[k].PWM_LED = 0;
          luciernagas[k].fase = 0;
          luciernagas[k].tiempoTranscurridoApagado = 0;
          luciernagas[k].contadorMsApagado = 0;
        }
        break;
    }
  }
  delay(1);
}

/* Funcion de interrupcion, 
 * La funcion de interrupcion se encarga de realizar el pwm de las luciernagas, comparando
 * el valor de pwm de cada una de ellas. El contadorPWM controla la velocidad del PWM, para
 * saber los tiempos exactos se tiene que saber cual es el tiempo que le lleva a cada ciclo 
 * y los incrementos del contador PWM.
 */
ISR(TIMER1_COMPA_vect){
  for(int pin=INICIO_PINES;pin<=FIN_PINES;pin++){
    if(luciernagas[pin-INICIO_PINES].PWM_LED > contadorPWM){
      digitalWrite(pin,HIGH);
     }else{
      digitalWrite(pin,LOW);
     }
  }
  /*
  if(luciernagas[0].PWM_LED > contadorPWM){
    digitalWrite(2,HIGH);
  }else{
    digitalWrite(2,LOW);
  }
  if(luciernagas[1].PWM_LED > contadorPWM){
    digitalWrite(3,HIGH);
  }else{
    digitalWrite(2,LOW);
  }
  if(luciernagas[2].PWM_LED > contadorPWM){
    digitalWrite(4,HIGH);
  }else{
    digitalWrite(4,LOW);
  }
  if(luciernagas[3].PWM_LED > contadorPWM){
    digitalWrite(5,HIGH);
  }else{
    digitalWrite(5,LOW);
  }
  if(luciernagas[4].PWM_LED > contadorPWM){
    digitalWrite(6,HIGH);
  }else{
    digitalWrite(6,LOW);
  }
  if(luciernagas[5].PWM_LED > contadorPWM){
    digitalWrite(7,HIGH);
  }else{
    digitalWrite(7,LOW);
  }
  if(luciernagas[6].PWM_LED > contadorPWM){
    digitalWrite(8,HIGH);
  }else{
    digitalWrite(8,LOW);
  }
  if(luciernagas[7].PWM_LED > contadorPWM){
    digitalWrite(9,HIGH);
  }else{
    digitalWrite(9,LOW);
  }
  if(luciernagas[8].PWM_LED > contadorPWM){
    digitalWrite(10,HIGH);
  }else{
    digitalWrite(10,LOW);
  }
  if(luciernagas[9].PWM_LED > contadorPWM){
    digitalWrite(11,HIGH);
  }else{
    digitalWrite(11,LOW);
  }
  if(luciernagas[10].PWM_LED > contadorPWM){
    digitalWrite(12,HIGH);
  }else{
    digitalWrite(12,LOW);
  }
  if(luciernagas[11].PWM_LED > contadorPWM){
    digitalWrite(13,HIGH);
  }else{
    digitalWrite(13,LOW);
  }
  if(luciernagas[12].PWM_LED > contadorPWM){
    digitalWrite(14,HIGH);
  }else{
    digitalWrite(14,LOW);
  }
  if(luciernagas[13].PWM_LED > contadorPWM){
    digitalWrite(15,HIGH);
  }else{
    digitalWrite(15,LOW);
  }
  if(luciernagas[14].PWM_LED > contadorPWM){
    digitalWrite(16,HIGH);
  }else{
    digitalWrite(16,LOW);
  }
  if(luciernagas[15].PWM_LED > contadorPWM){
    digitalWrite(17,HIGH);
  }else{
    digitalWrite(17,LOW);
  }  
  */
  contadorPWM+=8;
}

unsigned int rand(unsigned int randMax){
  Randseed = Randseed * 1103515245 + 12345;
  return ((unsigned int)(Randseed >> 16) % randMax);
}
