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
#define LUC_MAX (16)
#define INTS_MIN_NUEVA_LUC (80)
#define INTS_MAXS_NUEVA_LUC (900)
#define INCREMENTOS_PWM (1)
#define DECREMENTOS_PWM (1)
#define LIMITE_CLIMAX (100)
#define LIMITE_PWM (100)
#define VALOR_CTC (1200)

// Registro de control de luminosidad de las luciernagas
byte PWM_Luc[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Fases en la que se encuentra cada pin
byte Fases[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
// Contador que controla el numero de interrupciones que se mantiene en el maximo encendido por pin
byte contClimax[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
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
// Valor limite del PWM.
int valorLimitePWM = LIMITE_PWM-INCREMENTOS_PWM+1;

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
  TCCR1A=0;
  TCCR1B=0;
  OCR1A=VALOR_CTC;
  TCCR1B |= (1<<WGM12);
  TCCR1B |= (1<<CS10)|(1<<CS11);
  TIMSK1 |= (1<<OCIE1A);
  sei();
}
/* El loop se encarga de realizar e pwm de las luciernagas, comparando el valor de pwm de
 * cada una de ellas. El contadorPWM controla la velocidad del PWM, para saber los tiempos
 * exactos se tiene que saber cual es el tiempo que le lleva a cada ciclo y los incrementos
 * del contador PWM.
 */
void loop(){
  for(int pin=INICIO_PINES;pin<=FIN_PINES;pin++){
    if(PWM_Luc[pin] > contadorPWM){
      digitalWrite(pin,HIGH);
     }else{
      digitalWrite(pin,LOW);
     }
  }
  contadorPWM+=4;
}

/* Funcion de interrupcion, 
 * La funcionde interrupcion verifica en que fase se encuentra cada luciernaga, si se encuentra
 * en la fase 1 incrementa su valor de PWM hasta llegar al valorLimitePWM donde lo pasa a la
 * fase 2, en la fase 2 se lleva una cuenta del contClimax de cada luciernaga hasta alcanzar
 * el limite de climax donde lo pasa a la fase 3, en la fase 3 decrese el PWM de cada luciernaga
 * hasta llegar a 0, cuando esto ocurre se descuenta uno a las lucEncendidas.
 *
 * En caso de haber transcurrido las interrupciones suficientes y no se hsya excedido el numero
 * maximo de luciernagas encendidas, se selecciona una nueva luciernaga de manera aleatoria, que,
 * si esta en fase 0, se setea en fase 1 y se añade uno a las luciernagas encendidas.
 */
ISR(TIMER1_COMPA_vect){
  byte lucEncender = 0;
  unsigned int newRandValue = 0;
  for(int k=INICIO_PINES;k<=FIN_PINES;k++){
    switch(Fases[k]){
      case 1:
        PWM_Luc[k]+=INCREMENTOS_PWM;
        if(PWM_Luc[k] >= valorLimitePWM) {Fases[k]=2;PWM_Luc[k]=LIMITE_PWM;}
        break;
      case 2:
        contClimax[k]+=1;
        if(contClimax[k] == LIMITE_CLIMAX) {Fases[k] = 3; contClimax[k] = 0;}
        break;
      case 3:
        PWM_Luc[k]-=DECREMENTOS_PWM;
        if(PWM_Luc[k]<DECREMENTOS_PWM){
          lucEncendidas--;
          PWM_Luc[k]=0;
          Fases[k]=0;
        }
        break;
    }
  }
  
  if(iTranscNuevaLuc >= iMetaNuevaLuc){
    newRandValue=rand(LUC_MAX);
    if(lucEncendidas < newRandValue){
      lucEncender = INICIO_PINES+rand(FIN_PINES-INICIO_PINES+1);
      if(Fases[lucEncender] == 0){
        Fases[lucEncender]=1;
        lucEncendidas++;
      }
    }
    iTranscNuevaLuc=0;
    iMetaNuevaLuc=rand(INTS_MAXS_NUEVA_LUC)+INTS_MIN_NUEVA_LUC;
  }
  iTranscNuevaLuc++;  
}

unsigned int rand(unsigned int randMax){
  Randseed = Randseed * 1103515245 + 12345;
  return ((unsigned int)(Randseed >> 16) % randMax);
}
