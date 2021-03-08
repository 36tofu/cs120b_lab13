
/*	Author: lab
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: PORTB = tmpBT1;Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif


void transmit_data(unsigned char data, unsigned char cs) {
    int i;
    for (i = 0; i < 8 ; ++i) {
   	 // Sets SRCLR to 1 allowing data to be set
   	 // Also clears SRCLK in preparation of sending data
	 if(cs == 0) PORTC = 0x08;
	 else if (cs == 1) PORTC = 0x20;
   	 // set SER = next bit of data to be sent.
   	 PORTC |= ((data >> i) & 0x01);
   	 // set SRCLK = 1. Rising edge shifts next bit of data into the shift register
   	 PORTC |= 0x02;  
    }
    // set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
    if(cs == 0) PORTC |= 0x04;
    else if(cs == 1) PORTC |= 0x10;
    // clears all lines in preparation of a new transmission
    PORTC = 0x00;
}

unsigned long _avr_timer_M = 1; //start count from here, down to 0. Dft 1ms
unsigned long _avr_timer_cntcurr = 0; //Current internal count of 1ms ticks

void A2D_init() {
      ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: Enables analog-to-digital conversion
	// ADSC: Starts analog-to-digital conversion
	// ADATE: Enables auto-triggering, allowing for constant
	//	    analog to digital conversions.
}


void TimerOn(){
	//AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B; //bit 3 = 0: CTC mode (clear timer on compare)
	//AVR output compare register OCR1A
	OCR1A = 125; // Timer interrupt will be generated when TCNT1 == OCR1A
	//AVR timer interrupt mask register
	TIMSK1 = 0x02; //bit1: OCIE1A -- enables compare match interrupt
	//Init avr counter
	TCNT1 = 0;

	_avr_timer_cntcurr = _avr_timer_M;
	//TimerISR will be called every _avr_timer_cntcurr ms
	
	//Enable global interrupts 
	SREG |= 0x80; //0x80: 1000000

}

void TimerOff(){
	TCCR1B = 0x00; //bit3bit1bit0 = 000: timer off
}


ISR(TIMER1_COMPA_vect){
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
			TimerISR();
			_avr_timer_cntcurr = _avr_timer_M;
			}
}

void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}




typedef struct task {
  int state; // Current state of the task
  unsigned long period; // Rate at which the task should tick
  unsigned long elapsedTime; // Time since task's previous tick
  int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[1];

const unsigned char tasksNum = 1;


const unsigned long tasksPeriodGCD = 1;
const unsigned long periodKP = 50;
const unsigned long periodSQ = 50;
const unsigned long periodIS = 100;
const unsigned long periodSample = 200;
const unsigned long demoSample = 1;



/*
const unsigned long periodThreeLEDs = 300;
unsigned long periodSpeaker = 2;
const unsigned long periodCombined = 1;
const unsigned long periodSample = 200;
*/

enum KP_States { KP_SMStart, KP_wait, KP_inc, KP_dec, KP_reset };
int TickFct_KP(int state);

enum Demo_States {wait, bp};
int Demo_Tick(int state);

/*
enum SQ_States { SQ_SMStart, SQ_init, SQ_begin, SQ_wait, SQ_check, SQ_match };
int TickFct_detectSQ(int state);


enum IS_States { IS_SMStart, IS_unlock, IS_lock };
int TickFct_IS(int state);


enum OnOff_States { OnOff_SMStart, s_Off, s_On};
int TickFct_OnOff(int state);

enum SP_States { SP_SMStart, SP_s0, SP_s1};
int TickFct_Speaker(int state);

enum FRQ_States { FRQ_SMStart, FRQ_s0, FRQ_inc, FRQ_dec};
int TickFct_FRQ(int state);

*/

void TimerISR() {
  unsigned char i;
  for (i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
     if ( tasks[i].elapsedTime >= tasks[i].period ) { // Ready
        tasks[i].state = tasks[i].TickFct(tasks[i].state);
        tasks[i].elapsedTime = 0;
     }
     tasks[i].elapsedTime += tasksPeriodGCD;
  }
}


unsigned char tmpBUL;
unsigned char t2unlock; //0 = locked    1 = unlocked
unsigned char keyPressed;


int main() {
 
  //DDRA = 0x00; PORTA = 0x0F;
  DDRB = 0x07; PORTB = 0x00;
  DDRD = 0xFF; PORTD = 0x00;
  DDRC = 0xFF; PORTC = 0x00;
  unsigned char i=0;
  A2D_init();
  tasks[i].state = wait;
  tasks[i].period = demoSample;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &Demo_Tick;
  /*
  ++i;
  tasks[i].state = SQ_SMStart;
  tasks[i].period = periodSQ;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_detectSQ;
  ++i;
  tasks[i].state = IS_SMStart;
  tasks[i].period = periodIS;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_IS;
  ++i
  tasks[i].state = OnOff_SMStart;
  tasks[i].period = periodSample;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &TickFct_OnOff;
  ++i;
  //TimerSet(tasksPeriodGCD);
  */

  TimerOn();

  
  
  while(1) {
  }
  return 0;
}

int Demo_Tick(int state) {
	//unsigned char template[5] = {0, 0x3C, 0x24, 0x3C, 0};  	// Row(s) displaying pattern. 
	unsigned char template[5] = {0, 0, 0, 0, 0};  	// Row(s) displaying pattern. 
	//unsigned  char tmpA = ~PINA & 0x0F;
	unsigned  char tmpA = 0;
	// Local Variables
	static unsigned char pattern = 0x80;	// LED pattern - 0: LED off; 1: LED on
	static unsigned char row = 0x01;  	// Row(s) displaying pattern. 
							// 0: display pattern on row
							// 1: do NOT display pattern on row
	unsigned char j = 0;
	static unsigned char i = 0;
	static unsigned char vert = 1; //0: hit bottom of matrix.   2: hit top of matrix
	static unsigned char hori = 2; //0: hit left of matrix.    4: hit right of matrix
	//adc 
	unsigned short input = ADC;
	template[0] = input & 0x00FF;
	template[2] = (input & 0x0300) >> 2;
	// Transitions
	switch (state) {
		case wait:
			if(tmpA == 1){//right
				if(hori < 4){
				  	hori++;	
					state = bp;
				}
			}
			else if(tmpA == 2){//up
				if(vert < 2){
				       vert++;	
					state = bp;
				}
			}
			else if(tmpA == 4){//down
				if(vert > 0){ 
					vert--;
					state = bp;
				}
			}
			else if(tmpA == 8){//left
				if(hori > 0){ 
					hori--;
					state = bp;
				}
			}
			else 
				state = wait;
			break;
		case bp:
			if(tmpA == 0)
				state = wait;
			else state = bp;
			break;
		default:	
			state = wait;
			break;
	}	
	// Actions
	switch (state) {
		case wait:	
			if(vert == 1)j=i;
			else if(vert == 0){
				if(i == 0) j = 0;
				else j = i-1;
			}
			else if(vert == 2){
				if(i == 4) j = 4;
				else j= i+1;
			}

			if(hori <= 2) pattern = template[j] << (2-hori);
			else pattern = template[j] >> (hori - 2);
			

			row = (0x01) << i;
			if(i != 0x04) i++;
			else i = 0x00;
			break;
		case bp:
			break;
		default:
		break;
	}
	transmit_data(pattern, 0);
	transmit_data(~row, 1);

	//PORTC = pattern;	// Pattern to display
	//PORTD = ~row;		// Row(s) displaying pattern	
	//PORTB = hori;		// Row(s) displaying pattern	
	return state;
}

