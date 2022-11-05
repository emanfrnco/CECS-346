/***
CECS346 Project 2: Drag Race
Student Name: Dylan Dang
Lab description: Using a FSM, SysTick, Edge Triggered, & Level Triggered interrupts to create a Drag Race 
where two people compete to see who releases the button faster. 

Hardware Design
1)	Port A will be used to control right & left sensors
2)	Port B will be used for LED out
3) 	Port C will be used for LED out
4) 	Port E will be used for reset button
***/

/***
RR - PB0
RL - PB1
GR - PB2
GL - PB3
Y2R - PC4
Y2L - PC5
Y1R - PC6
Y1L - PC7

RB - PA2
LB - PA3
Reset - PE3
***/

#include <stdint.h>   // for data type alias
#include <tm4c123gh6pm.h>

// PORT A
#define SENSOR_PORTA 						(*((volatile unsigned long *)0x40004030))

// PORT B
#define LIGHT_PORTB							(*((volatile unsigned long *)0x4000503C))


// PORT C
#define LIGHT_PORTC							(*((volatile unsigned long *)0x400063C0))

// Function Prototypes
void DisableInterrupts(void); 	// disables interrupts
void EnableInterrupts(void);		// enables interrupts
void WaitForInterrupt(void);		// go to low power mode while waiting for the next interrupt

void PortA_Init(void);					// initialize Port A buttons
void PortB_LEDInit(void);				// initialize Port B LEDs
void PortC_LEDInit(void);				// initialize Port C LEDs
void PortE_Init(void);					// initialize Port E buttons
void SysTick_Init(void);				// initialize SysTick timer
void SysTick_Wait500ms(unsigned long delay);	// generates 0.5s delay
void GPIOPortA_Handler(void);		// handle GPIO Port A interrupts
void GPIOPortE_Handler(void);		// handle GPIO Port E interrupts
void SysTick_Handler(void);			// handle SysTick generated interrupts
void Debounce(void);						// debounce module for button

// FSM state data structure
struct State {
  unsigned long Out;
	unsigned long Time;
  unsigned long Next[4];
}; 

typedef const struct State STyp;
	
#define Init 			0								 
#define Wait 			1								    
#define Y1				2												 
#define Y2				3												
#define Go  			4 										
#define FalseSL		5
#define FalseSR		6
#define FalseB		7	
#define WinLeft		8
#define WinRight	9
#define WinBoth		10
																					

STyp FSM[11] = {	// 00, 01, 10, 11																																				
	{0xFF, 2, {Wait, Wait, Wait, Wait}},	          // initialize state, all LEDs on, SysTick Timer restarts					
	{0x00, 1, {Wait, Wait, Wait, Y1}},							// waiting state, all LEDs off											
	{0xC0, 1, {FalseB, FalseSL, FalseSR, Y2}},			// countdown Y1 state, Y1L & Y1R LEDs on																			
	{0x30, 1, {FalseB, FalseSL, FalseSR, Go}},			// countdown Y2 state, Y2L & Y2R LEDs on
	{0x0C, 1, {WinBoth, WinLeft, WinRight, Go}},		// go state, GL & GR LEDs on
	{0x02, 2, {Wait, Wait, Wait, Wait}},						// false start left, RL LED on
	{0x01, 2, {Wait, Wait, Wait, Wait}},						// false start right, RR LED on
	{0x03, 2, {Wait, Wait, Wait, Wait}},						// false start both, RL & RR LEDs on
	{0x08, 2, {Wait, Wait, Wait, Wait}},						// win left, GL LED on
	{0x04, 2, {Wait, Wait, Wait, Wait}},						// win right, GR LED on
	{0x0C, 2, {Wait, Wait, Wait, Wait}}							// win both, GL & GR LED on
};    

unsigned long S; 			// index to current state
unsigned long Input;	

int main(void){ 
	PortA_Init();
	PortE_Init();
	PortB_LEDInit();
	PortC_LEDInit();
	SysTick_Init();
  S = Init;
  while(1){
		WaitForInterrupt();
		
  }
}

void PortA_Init(void) { volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x00000001;														// activate clock for Port A
	GPIO_PORTA_CR_R |= 0x0C;
	GPIO_PORTA_DIR_R &= ~0x0C;															// make PA3 - PA2 inputs
	GPIO_PORTA_DEN_R |= 0x0C;																// enable digital I/O on PA3 - PA2
	GPIO_PORTA_AMSEL_R = 0;																	// disable analog function for PA3 - PA2
	GPIO_PORTA_PCTL_R &= ~0x0000FF00;												// GPIO clear bit PCTL for PA3 - PA2
	GPIO_PORTA_AFSEL_R &= ~0x0C;														// no alternate function for PA3 - PA2
	
	// Port A Interrupt
	GPIO_PORTA_IS_R &= ~0x0C;																// PA3 - PA2 is edge sensitive
	GPIO_PORTA_IBE_R |= 0x0C;																// PA3 - PA2 is both edges
	GPIO_PORTA_ICR_R |= 0x0C;																// clear flag
	GPIO_PORTA_IM_R |= 0x0C;																// arm interrupt on PA3 - PA2
	NVIC_PRI0_R = (NVIC_PRI0_R & 0xFFFFFF00) | 0x00000040;	// priority 2
	NVIC_EN0_R = 0x00000001;																// enable interrupt 0 in NVIC
	EnableInterrupts();																			// enable global Interrupt flag 
}

void PortE_Init(void) { volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x00000010;														// activate clock for Port E
	GPIO_PORTE_CR_R |= 0x08;
	GPIO_PORTE_DIR_R &= ~0x08;															// make PE3 inputs
	GPIO_PORTE_DEN_R |= 0x08;																// enable digital I/O on PE3
	GPIO_PORTE_AMSEL_R = 0;																	// disable analog function for PE3
	GPIO_PORTE_PCTL_R &= ~0x0000F000;												// GPIO clear bit PCTL for PE3
	GPIO_PORTE_AFSEL_R &= ~0x08;														// no alternate function for PE3
	
	// Port E Interrupt
	GPIO_PORTE_IS_R |= 0x08;																// PE3 is level sensitive
	GPIO_PORTE_IEV_R |= 0x08;
	GPIO_PORTE_ICR_R |= 0x08;																// clear flag
	GPIO_PORTE_IM_R |= 0x08;																// arm interrupt on PE3
	NVIC_PRI1_R = (NVIC_PRI1_R & 0xFFFFFF00) | 0x00000020;	// priority 1
	NVIC_EN0_R = 0x00000010;																// enable interrupt 4 in NVIC
	EnableInterrupts();																			// enable global Interrupt flag
}

void PortC_LEDInit(void) { volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x00000004;														// activate clock for Port C
	delay = SYSCTL_RCGC2_R;																	// delay
	GPIO_PORTC_AMSEL_R &= ~0xF0;														// disable analog function for PC7 - PC4
	GPIO_PORTC_PCTL_R &= ~0xFFFF0000;												// GPIO clear bit PCTL for PC7 - PC4
	GPIO_PORTC_DIR_R |= 0xF0;																// PC7 - PC4 output
	GPIO_PORTC_AFSEL_R &= ~0xF0;														// no alternate function for PC7 - PC4
	GPIO_PORTC_DEN_R |= 0xF0;																// enable digital pins PC7 - PC4
}

void PortB_LEDInit(void) { volatile unsigned long delay;
	SYSCTL_RCGC2_R |= 0x00000002;														// activate clock for Port B
	delay = SYSCTL_RCGC2_R;																	// delay
	GPIO_PORTB_AMSEL_R &= ~0x0F;														// disable analog function for PB3 - PB0
	GPIO_PORTB_PCTL_R &= ~0x0000FFFF;												// GPIO clear bit PCTL for PB3 - PB0
	GPIO_PORTB_DIR_R |= 0x0F;																// PB3 - PB0 output
	GPIO_PORTB_AFSEL_R &= ~0x0F;														// no alternate function for PB3 - PB0
	GPIO_PORTB_DEN_R |= 0x0F;																// enable digital pins PB3 - PB0
}

void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;                    													// disable SysTick during setup
	NVIC_ST_RELOAD_R = 16000000 - 1;				 												// reload value
  NVIC_ST_CURRENT_R = 0;                 													// any write to current clears it
	NVIC_SYS_PRI3_R = (NVIC_SYS_PRI3_R & 0x00FFFFFF) | 0x60000000;	// priority 3
  NVIC_ST_CTRL_R = 0x07;																					// enable SysTick with core clock and interrupts
	EnableInterrupts();
}

void GPIOPortA_Handler(void) {
	Debounce();
	if (GPIO_PORTA_RIS_R & 0x04) { // PA2
		if (LIGHT_PORTB == 0xC0 || LIGHT_PORTB == 0x30) {
			S = FalseSR;
		GPIO_PORTA_ICR_R = 0x04;  // clear flag
		}
		else {
			GPIO_PORTA_ICR_R = 0x04; // clear flag
		}
	}
	Debounce();
	if (GPIO_PORTA_RIS_R & 0x08) { // PA3
		if (LIGHT_PORTB == 0xC0 || LIGHT_PORTB == 0x30) {
			S = FalseSL;
			GPIO_PORTA_ICR_R = 0x08; // clear flag
		}
		else {
			GPIO_PORTA_ICR_R = 0x08; // clear flag
		}
	}
	Debounce();
	if (GPIO_PORTA_RIS_R & 0x0C) { // PA2 & PA3
		if (LIGHT_PORTB == 0xC0 || LIGHT_PORTB == 0x30) {
			S = FalseB;
			GPIO_PORTA_ICR_R = 0x0C; // clear flag
		}
		else {
			GPIO_PORTA_ICR_R = 0x0C; // clear flag
		}
	}
}

void GPIOPortE_Handler(void) {
	if (GPIO_PORTE_RIS_R & 0x08) { // PE3
		S = Init;
		GPIO_PORTE_ICR_R |= 0x08; // clear flag
		GPIO_PORTE_IM_R |= 0x08;
	}
	else {
		GPIO_PORTE_ICR_R = 0x08;  // clear flag
	}
}

void SysTick_Handler(void) {
	LIGHT_PORTB = FSM[S].Out;
	LIGHT_PORTC = FSM[S].Out;
	SysTick_Wait500ms(FSM[S].Time);
	Input = SENSOR_PORTA >> 2;			// read sensors
	S = FSM[S].Next[Input];
}

void SysTick_Wait(unsigned long delay) {
	volatile unsigned long elapsedTime;
	unsigned long startTime = NVIC_ST_CURRENT_R;
	do {
		elapsedTime = (startTime - NVIC_ST_CURRENT_R) & 0x00FFFFFF;
	}
	 while (elapsedTime <= delay);
}

void SysTick_Wait500ms(unsigned long delay) {
	unsigned long i;
	unsigned long j;
	for (j = 0; j < 50; j++) {
		for (i = 0; i < delay; i++) {
			SysTick_Wait(160000);			// wait 500ms (assumes 80 MHz clock)
		}
	}
}

void Debounce(void) {
	unsigned long j;
	for (j = 0; j < 199999; j++) {
	}
}
