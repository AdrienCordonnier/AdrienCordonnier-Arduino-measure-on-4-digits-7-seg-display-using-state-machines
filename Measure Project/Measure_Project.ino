//Adrien Cordonnier
#include "Timer.h"
#include <IRremote.h>

const int A=8, B=9, C=10, D=11, E=12, F=13, G=14, D1=15, D2=16, D3=3, D4=4, IR=2, DP=5, Trig=A4, Echo=A5;
long duration_echo;
float dist;
int i=0, cm=1, ft=0, cnt=0, j=0, s=0, command=0, command_value=0;;
int pins[] = {A,B,C,D,E,F,G,D1,D2,D3,D4,DP,Trig};
#define IR_BUTTON_POWER 69//number for the button to convert the dist between cm and ft
#define true 1
#define TOTAL_TASKS 3//number of tasks
#define PERIOD_gcd 5//period for the 4 digit 7 segments display
#define PERIOD_DISTANCE 200//period for the distance calculation
#define PERIOD_DISP PERIOD_gcd//period for the 4 digit 7 segments display
#define PERIOD_CONVERSION 100//period for the converison of the dist between cm and ft

// task structure
struct task {

	unsigned short period;
	unsigned short timeElapsed;

	void ( *tick ) ( void );
};
static struct task gTaskSet[ TOTAL_TASKS ];

void initializeTasks ( void ) {

	// initialize task[0] to distance measurement
  gTaskSet[0].period = PERIOD_DISTANCE;
  gTaskSet[0].timeElapsed = 0;
  gTaskSet[0].tick = TickFct_DISTANCE;

  // initialize task[1] to IR remote control conversion
  gTaskSet[1].period = PERIOD_CONVERSION;
  gTaskSet[1].timeElapsed = 0;
  gTaskSet[1].tick = TickFct_CONVERSION;

  // initialize task[2] to display
  gTaskSet[2].period = PERIOD_DISP;
  gTaskSet[2].timeElapsed = 0;
  gTaskSet[2].tick = TickFct_DISP;

	return;
}

void setup() {
  Serial.begin(9600);
  IrReceiver.begin(IR);//receiver IR signals
  TimerSet(PERIOD_gcd);
  TimerOn();
  for (i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
    pinMode(pins[i], OUTPUT);//setting all the pins to output mode
  }
  pinMode(Echo, INPUT);
  // initialize tasks;
	initializeTasks();
  }

char gSegPins[] = {A,B,C,D,E,F,G};

void displayNumTo7Seg(unsigned int targetNum, int digitPin) {
    unsigned char encodeInt[10] = {
        // 0     1     2     3     4     5     6     7     8     9
        0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67,
    };
    digitalWrite(digitPin, HIGH);
    for (int k = 0; k < 7; ++k) {
        digitalWrite(gSegPins[k], encodeInt[targetNum] & (1 << k));
    }
    digitalWrite(digitPin, LOW);
}

enum SM3_States { SM3_Init, SM3_Trig, SM3_Echo } state;
void TickFct_DISTANCE() { //SM to have the distance
   switch(state) { // Transitions
      case -1:
         state = SM3_Init;
         break;
      case SM3_Init:
         if (1) {
          digitalWrite(Trig,1);
          state = SM3_Trig;
         }
         break;
      case SM3_Trig:
         if (1) {
            digitalWrite(Trig,0);
            state = SM3_Echo;
         }
         break;
      case SM3_Echo:
         if (1) {
            state = SM3_Init;
         }
         break;
      default:
         state = -1;
      } // Transitions

   switch(state) { // State actions
      case SM3_Init:
         digitalWrite(Trig,0);
         digitalWrite(Echo,0);
         break;
      case SM3_Trig:
         break;
      case SM3_Echo:
         duration_echo=pulseIn(Echo,1);
         Serial.println(duration_echo);//receiving the signal and computing the time between sending and receiving the signal
         if(cm>0){dist=duration_echo*(0.034/2.0);}//computing the distance for the case of the centimeter distance
         else if(ft>0){dist = (duration_echo*(0.034/2.0))/30.48;}// computing the distance for the case of the ft distance
         break;
      default: // ADD default behaviour below
         break;
   } // State actions

}
enum SM1_States { SM1_Init, SM1_Receive, SM1_Conversion } SM1_State;

void TickFct_CONVERSION() {//Converting the distance when pressing the button of the remote
   switch(SM1_State) { // Transitions
      case -1:
         SM1_State = SM1_Init;
         break;
         case SM1_Init:
         if (!(IrReceiver.decode())) {//if a signal is detected
            SM1_State = SM1_Init;
         }
         else if (IrReceiver.decode()) {
            SM1_State = SM1_Receive;
         }
         break;
      case SM1_Receive:
         if ((command_value==69)&&!(IrReceiver.decode())) {//if the value of the button is 69(power button of the remote) and the signal is interrupted, transitionning
            command_value=0;//resetting the command value
            SM1_State = SM1_Conversion;
         }
         else if (IrReceiver.decode()) {
            SM1_State = SM1_Receive;
         }
         else if (!(IrReceiver.decode())) {
            SM1_State = SM1_Init;
         }
         break;
      case SM1_Conversion:
         if (1) {
            SM1_State = SM1_Init;
         }
         break;
      default:
         SM1_State = SM1_Init;
   } // Transitions

   switch(SM1_State) { // State actions
      case SM1_Init:
         command=0;
         break;
      case SM1_Receive:
         command = IrReceiver.decodedIRData.command;//receiving the value of the button pressed
         if(command==69){command_value=69;}//saving that the button has been pressed
         IrReceiver.resume();//continue to receive
         break;
      case SM1_Conversion:
         if (cm > 0){cm=0; ft=1;}//cm is 1 by default, transitionning to the ft distance
         else if (ft>0){cm=1;ft=0;}//if we are in ft, transitionnig to the cm distance
         break;
      default: // ADD default behaviour below
      break;
   } // State actions

}

enum SM2_States { SM2_Init, SM2_FirstNumber, SM2_2ndNumber, SM2_3rdNumber, SM2_4thNumber } SM2_State;
void TickFct_DISP() {
  unsigned char digit1;
  unsigned char digit2;
  unsigned char digit3;
  unsigned char digit4;// the 4 digit that we'll display
  int dist1 = dist;// int of the float distance
  int dist2 = 0;
  if (cm){ // if we are in cm
    digit1 = dist1/1000;// 1st digit from the left
    digit2 = (dist1/100)%10;// 2nd digit from the left
    digit3 = (dist1/10)%10;// 3rd digit from the left
    digit4 = dist1%10;}// 4th digit from the left
  else if (ft){// if we are in ft
    dist2 = (dist - dist1)*100;// taking only the decimals of the ft distance and scaling them by 100
    digit1 = (dist1/10)%10;// 1st digit from the left
    digit2 = dist1%10;// 2nd digit from the left
    digit3 = (dist2/10)%10;//1st digit of the decimals from the left
    digit4 = dist2%10;}//2nd digit of the decimals from the left

  switch(SM2_State) { // Transitions
    case -1:
        SM2_State = SM2_Init;
        break;
      case SM2_Init:
        if (1) {
          SM2_State = SM2_FirstNumber;
        }
        break;
      case SM2_FirstNumber:
         if (1) {
            SM2_State = SM2_2ndNumber;
            digitalWrite(D4, 1);//turning off the first digit from the left
         }
         break;
      case SM2_2ndNumber:
         if (1) {
            SM2_State = SM2_3rdNumber;
            digitalWrite(D3, 1);//turning off the second digit from the left
            digitalWrite(DP,0);//turning off the decimal point when transitionning
         }
         break;
      case SM2_3rdNumber:
         if (1) {
            SM2_State = SM2_4thNumber;
            digitalWrite(D2, 1);//turning off the third digit from the left
         }
         break;
      case SM2_4thNumber:
         if (1) {
            SM2_State = SM2_FirstNumber;
            digitalWrite(D1, 1);//turning off the fourth digit from the left
         }
         break;
      default:
         SM2_State = -1;
      } // Transitions

   switch(SM2_State) { // State actions
      case SM2_Init:
         digitalWrite(D4, 1);// setting all fourth digits off
         digitalWrite(D3, 1);
         digitalWrite(D2, 1);
         digitalWrite(D1, 1);
         break;
      case SM2_FirstNumber:
         displayNumTo7Seg(digit1,D4);// displaying the first number from the left
         break;
      case SM2_2ndNumber:
         displayNumTo7Seg(digit2,D3);// displaying the second number from the left
         if(ft){digitalWrite(5,1);}//if we are in ft, turning on the decimal point
         else{digitalWrite(5,0);}//if we are in cm, turning off the decimal point
         break;
      case SM2_3rdNumber:
         displayNumTo7Seg(digit3,D2);// displaying the third number from the left
         break;
      case SM2_4thNumber:
         displayNumTo7Seg(digit4,D1);// displaying the fourth number from the left
         break;
      default: // ADD default behaviour below
         break;
   } // State actions
}

void loop() {
  for ( int i = 0;  i < TOTAL_TASKS; i++ ) {

		gTaskSet[i].timeElapsed += PERIOD_gcd;

		if( gTaskSet[i].timeElapsed >= gTaskSet[i].period ) {
			gTaskSet[i].tick();
			gTaskSet[i].timeElapsed = 0;
		}
	}

  while (!TimerFlag) {}
  TimerFlag = 0;
}
