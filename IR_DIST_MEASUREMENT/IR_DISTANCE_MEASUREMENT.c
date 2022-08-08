// Documentation IR_DISTANCE_MEASUREMENT.c
// PROJECT CECS347 Lab 4: Measurment of Distance
// Description: Sampling Analog Signals, data conversion and calibrations.
// Name: TY-CO
// YOUTUBE VIDEO: https://www.youtube.com/watch?v=Gb1LwpQNqhM&ab_channel=b




/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2013

 Copyright 2013 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// input signal connected to PE2/AIN1

#include "ADCSWTrigger.h"
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "Nokia5110.h"


void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

void PortF_Init(void);
void SysTick_Init(void);
unsigned int lookupTable(unsigned long AVG);
void LCDlayout(void);

unsigned int ADCvalue;
volatile unsigned long AVG=0;
int SAMPLE = 1000;
int sFlag= 0;
double A = -0.50209529;
double B = 35960.0893;

unsigned int distV[13] ={3380, 2362, 1767, 1443, 1186, 1009, 882, 781, 703, 647, 576, 546, 532};
unsigned int distCM[13] ={10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70};


int main(void){
	unsigned long volatile delay;
	unsigned int tCalc, fCalc;
  PLL_Init();                           // 80 MHz
  ADC0_InitSWTriggerSeq3_Ch1();         // ADC initialization PE2/AIN1
	PortF_Init();
	SysTick_Init();
	Nokia5110_Init();
  

	LCDlayout(); // updates the LCD screen to proper layout
	
	
	

  while(1){
    GPIO_PORTF_DATA_R |= 0x04;          // profile
		// add a loop to collect 5 to 10 samples and apply software filtering for the data collected
		if(sFlag ==1){
			
			LCDlayout();
			ADCvalue = AVG;
			tCalc = lookupTable(AVG);
			fCalc = A+B/AVG;
			
	
			Nokia5110_SetCursor(4,3);
			Nokia5110_OutUDec(ADCvalue);
			Nokia5110_SetCursor(4,4);
			Nokia5110_OutUDec(tCalc);
			Nokia5110_SetCursor(4,5);
			Nokia5110_OutUDec(fCalc);
			fCalc = 0;
			tCalc = 0;
			AVG = 0;
			sFlag = 0;
		}
    
    GPIO_PORTF_DATA_R &= ~0x04;
    for(delay=0; delay<100000; delay++){};
			
		WaitForInterrupt();	
  }
}


void LCDlayout(void){
	Nokia5110_Clear();
	Nokia5110_SetCursor(0,0);
	Nokia5110_OutString("************CECS347 LAB4************");
	Nokia5110_SetCursor(0,3);
	Nokia5110_OutString("ADC");//ADC Value
	Nokia5110_SetCursor(0,4);
	Nokia5110_OutString("TAB");//Table DistanceValue
	Nokia5110_SetCursor(0,5);
	Nokia5110_OutString("CAL");//Calibrated Distance

}

unsigned int lookupTable(unsigned long avg){

  int i= 0;
	int value1,value2,value3,cm,dist;
	
	if(avg>distV[0] | avg<distV[12] | avg==0)
		dist = 0;
	else{
		
		while(avg<distV[i]){
			i++;
		}
		value1 =distV[i-1] - distV[i];
		value2 = value1/5; // adcValue per cm
		value3 = avg - distV[i];
		cm = value3/value2; // number of cm between avg sample and the adcValue
		dist = distCM[i]- cm;
	}
	
		return dist;
	
}

void PortF_Init(void){
	unsigned long volatile delay;
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // activate port F
  delay = SYSCTL_RCGC2_R;
  GPIO_PORTF_DIR_R |= 0x04;             // make PF2 out (built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x04;          // disable alt funct on PF2
  GPIO_PORTF_DEN_R |= 0x04;             // enable digital I/O on PF2
                                        // configure PF2 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFF0FF)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;               // disable analog functionality on PF
}


void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;         // disable SysTick during setup
  NVIC_ST_RELOAD_R = 4000000-1;// reload value 20HZ

  NVIC_ST_CURRENT_R = 0;      // any write to current clears it
  NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R&0x00FFFFFF)|0x40000000; // priority 2
                              // enable SysTick with core clock and interrupts
  NVIC_ST_CTRL_R = 0x07;

}


void SysTick_Handler(void){
	sFlag = 1;
	for(int i = 0;i<SAMPLE; i++)
		AVG = AVG + ADC0_InSeq3();
	
	AVG = AVG/SAMPLE;
}

