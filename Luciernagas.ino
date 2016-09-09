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
#define PIN_RUIDO (19)
//#define LUC_MAX (3)
#define MS_MIN_NUEVA_LUC (100)
#define MS_MAXS_NUEVA_LUC (15000)
#define MS_ENCENDIDO (200)
#define MS_APAGADO (700)
#define MS_CLIMAX (400)
#define LIMITE_PWM (255)
#define VALOR_CTC (4000)

// Registro de control de luminosidad de las luciernagas
byte PWM_Luc[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Registro de control del tiempo para encender de nuevo
unsigned int newLuc[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Registro de control del tiempo actual para encender de nuevo
unsigned int interLuc[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Fases en la que se encuentra cada pin
unsigned int Fases[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Contador que controla el numero de interrupciones que se mantiene en el maximo encendido por pin
unsigned int contClimax[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Contador que controla el numero de interrupciones que sellevara encender al maximo el led
unsigned int contArranque[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Contador que controla el numero de interrupciones que sellevara apagar el led
unsigned int contApagado[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Interrupciones transcurridas desde que aparecio la ultima luciernaga
byte iTranscNuevaLuc = 0;
// Interrupciones que tienen que transcurrir para que aparezca una nueva luciernaga
byte iMetaNuevaLuc = 0;
// Numero de luciernagas encendidas
byte lucEncendidas = 0;
// Semilla inicial de la funcion RANDOM
long Randseed = 2589;
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
  
  for(int pin=INICIO_PINES;pin<=FIN_PINES;pin++){
    newLuc[pin]=rand(MS_MAXS_NUEVA_LUC)+MS_MIN_NUEVA_LUC;
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
  for(int k=INICIO_PINES;k<=FIN_PINES;k++){
    switch(Fases[k]){
      case 0:
        if(interLuc[k] >= newLuc[k]){
          Fases[k]=1;
          lucEncendidas++;
          newLuc[k]=rand(MS_MAXS_NUEVA_LUC)+MS_MIN_NUEVA_LUC;
        }
        interLuc[k]+=1;
        break;
      case 1:
        contArranque[k]+=1;
        PWM_Luc[k]=map(contArranque[k],0,MS_ENCENDIDO,0,LIMITE_PWM);
        if(contArranque[k] >= MS_ENCENDIDO) {Fases[k]=2;PWM_Luc[k]=LIMITE_PWM;contArranque[k]=0;}
        break;
      case 2:
        contClimax[k]+=1;
        if(contClimax[k] == MS_CLIMAX) {Fases[k] = 3; contClimax[k] = 0;}
        break;
      case 3:
        contApagado[k]+=1;
        PWM_Luc[k]=map(contApagado[k],0,MS_APAGADO,LIMITE_PWM,0);
        if(contApagado[k]==MS_APAGADO){
          lucEncendidas--;
          PWM_Luc[k]=0;
          Fases[k]=0;
          interLuc[k]=0;
          contApagado[k]=0;
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
  /*
  for(int pin=INICIO_PINES;pin<=FIN_PINES;pin++){
    if(PWM_Luc[pin] > contadorPWM){
      digitalWrite(pin,HIGH);
     }else{
      digitalWrite(pin,LOW);
     }
  }
  */
  
  if(PWM_Luc[2] > contadorPWM){
    digitalWrite(2,HIGH);
  }else{
    digitalWrite(2,LOW);
  }
  if(PWM_Luc[3] > contadorPWM){
    digitalWrite(3,HIGH);
  }else{
    digitalWrite(3,LOW);
  }
  if(PWM_Luc[4] > contadorPWM){
    digitalWrite(4,HIGH);
  }else{
    digitalWrite(4,LOW);
  }
  if(PWM_Luc[5] > contadorPWM){
    digitalWrite(5,HIGH);
  }else{
    digitalWrite(5,LOW);
  }
  if(PWM_Luc[6] > contadorPWM){
    digitalWrite(6,HIGH);
  }else{
    digitalWrite(6,LOW);
  }
  if(PWM_Luc[7] > contadorPWM){
    digitalWrite(7,HIGH);
  }else{
    digitalWrite(7,LOW);
  }
  if(PWM_Luc[8] > contadorPWM){
    digitalWrite(8,HIGH);
  }else{
    digitalWrite(8,LOW);
  }
  if(PWM_Luc[9] > contadorPWM){
    digitalWrite(9,HIGH);
  }else{
    digitalWrite(9,LOW);
  }
  if(PWM_Luc[10] > contadorPWM){
    digitalWrite(10,HIGH);
  }else{
    digitalWrite(10,LOW);
  }
  if(PWM_Luc[11] > contadorPWM){
    digitalWrite(11,HIGH);
  }else{
    digitalWrite(11,LOW);
  }
  if(PWM_Luc[12] > contadorPWM){
    digitalWrite(12,HIGH);
  }else{
    digitalWrite(12,LOW);
  }
  if(PWM_Luc[13] > contadorPWM){
    digitalWrite(13,HIGH);
  }else{
    digitalWrite(13,LOW);
  }
  if(PWM_Luc[14] > contadorPWM){
    digitalWrite(14,HIGH);
  }else{
    digitalWrite(14,LOW);
  }
  if(PWM_Luc[15] > contadorPWM){
    digitalWrite(15,HIGH);
  }else{
    digitalWrite(15,LOW);
  }
  if(PWM_Luc[16] > contadorPWM){
    digitalWrite(16,HIGH);
  }else{
    digitalWrite(16,LOW);
  }
  if(PWM_Luc[17] > contadorPWM){
    digitalWrite(17,HIGH);
  }else{
    digitalWrite(17,LOW);
  }  
  contadorPWM+=8;
}

unsigned int rand(unsigned int randMax){
  Randseed = Randseed * 1103515245 + 12345;
  return ((unsigned int)(Randseed >> 16) % randMax);
}
