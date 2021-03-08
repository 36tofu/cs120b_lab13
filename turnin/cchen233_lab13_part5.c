/* Author:Christopher Chen
 * Partner(s) Name (if applicable):  
 * Lab Section:21
 * Assignment: Lab #13  Exercise #5
 * Exercise Description: [optional - include for your own benefit]
 *
 * I acknowledge all content contained herein, excluding template or example
 * code, is my own original work.
 *
 *  Demo Link: https://youtu.be/lZB8xbPdG7w
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

void Set_A2D_Pin(unsigned char pinNum) {
	ADMUX = (pinNum <= 0x07) ? pinNum : ADMUX;
	// Allow channel to stabilize
	static unsigned char i = 0;
	for ( i=0; i<15; i++ ) { asm("nop"); } 
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

task tasks[3];

const unsigned char tasksNum = 3;

const unsigned long tasksPeriodGCD = 1;
const unsigned long periodSample = 250;
const unsigned long periodDisplay = 1;

enum DSPLY_States {dis};
int DSPLY_Tick(int state);

enum JS_States {sample};
int JS_Tick(int state);

int JSV_Tick(int state);

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

unsigned char g_pattern;
unsigned char g_row;


int main() {
 
  DDRB = 0x07; PORTB = 0x00;
  DDRD = 0xFF; PORTD = 0x00;
  DDRC = 0xFF; PORTC = 0x00;
  unsigned char i=0;
  A2D_init();
  tasks[i].state = dis;
  tasks[i].period = periodDisplay;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &DSPLY_Tick;
  ++i;
  tasks[i].state = sample;
  tasks[i].period = periodSample;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &JS_Tick;
  ++i;
  tasks[i].state = sample;
  tasks[i].period = periodSample;
  tasks[i].elapsedTime = tasks[i].period;
  tasks[i].TickFct = &JSV_Tick;
  //TimerSet(tasksPeriodGCD);

  TimerOn();
  
  while(1) {
  }
  return 0;
}

int DSPLY_Tick(int state) {
	transmit_data(g_pattern, 0);
	transmit_data(~g_row, 1);
	return state;
}

int JS_Tick(int state) {
	static unsigned char pattern = 0x10;	// LED pattern - 0: LED off; 1: LED on
	short deviation;   	                // Joy stick position deviation from neutral
	//adc 
	Set_A2D_Pin(0);
	unsigned short input = ADC;
	
	// Actions
	switch (state) {
		case sample:
			// update dot in horizental direction
			if(input > 562){
				if(pattern > 0x01) pattern = pattern >> 1;
				// else pattern = 0x80; 
			}
			else if(input < 462){
				if(pattern < 0x80) pattern = pattern << 1;
				// else pattern = 0x01;
			}	
			// update update rate
			deviation = input - 512;
			if (deviation < 0) deviation = -deviation;
			if (deviation > 450)
				tasks[1].period = 100;
			else if (deviation > 300)
				tasks[1].period = 250;
			else if (deviation > 150)
				tasks[1].period = 500;
			else
				tasks[1].period = 1000;
			
			break;
		default:
			break;
	}
	g_pattern = pattern;
	return state;
}

int JSV_Tick(int state) {
	static unsigned char row = 0x04;  	// Row(s) displaying pattern. 
	short deviation;   	                // Joy stick position deviation from neutral
	//adc 
	Set_A2D_Pin(1);
	unsigned short input = ADC;
	
	// Actions
	switch (state) {
		case sample:
			// update dot in horizental direction
			if(input > 562){
				if(row > 0x01) row = row >> 1;
				// else row = 0x80; 
			}
			else if(input < 462){
				if(row < 0x10) row = row << 1;
				// else row = 0x01;
			}	
			// update update rate
			deviation = input - 512;
			if (deviation < 0) deviation = -deviation;
			if (deviation > 450)
				tasks[2].period = 100;
			else if (deviation > 300)
				tasks[2].period = 250;
			else if (deviation > 150)
				tasks[2].period = 500;
			else
				tasks[2].period = 1000;
			
			break;
		default:
			break;
	}
	g_row = row;
	return state;
}

