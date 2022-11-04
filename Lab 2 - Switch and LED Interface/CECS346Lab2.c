// Documentation
// CECS346 Lab 2 - LED & SWITCH INTERFACE
// Description: INTERFACING SWITCHES & LEDs
// Student Name: Dylan Dang

// Input/Output:
//   PE0 - SW 1
//   PE1 - SW 2
//   PB0 - Green LED
//   PB1 - Yellow LED
//   PB2 - Red LED

// Preprocessor Directives
#include <stdint.h>


// 1. Pre-processor Directives Section
// Constant declarations to access port registers using 
// symbolic names instead of addresses
// Replace the ? on the following lines with address for the corresponding registers

// PORT E register definitions
#define GPIO_PORTE_DATA_R       (*((volatile unsigned long *)0x400243FC))
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))
#define GPIO_PORTE_PDR_R        (*((volatile unsigned long *)0x40024514))

// PORT B register definitions
#define GPIO_PORTB_DATA_R       (*((volatile unsigned long *)0x400053FC))
#define GPIO_PORTB_DIR_R        (*((volatile unsigned long *)0x40005400))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_AMSEL_R      (*((volatile unsigned long *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))

// system control register RCGC2 definition
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))

// defining constants
#define LIGHT 		(*((volatile unsigned long *)0x4000501C))   // PORTB, bits 0,1,2
#define SW 			(*((volatile unsigned long *)0x4002400C))   // PORTE, bits 0,1
#define SW1_MASK 	0x01           // PE0 -> SW1
#define SW2_MASK 	0x02           // PE1 -> SW2
#define RED 		0x04	       // PB2 -> Red LED
#define YELLOW 		0x02		   // PB1 -> Yellow LED
#define GREEN 		0x01		   // PB0 -> Green LED

// Function Prototypes - Each subroutine defined
void Delay(uint8_t n_50ms);
void PortB_Init(void);
void PortE_Init(void);

int main(void) {
	uint8_t current_led;
	// Initialize GPIO Ports B, E
  	PortB_Init();
	PortE_Init();
	// Initial state: Green LED on, the other two LEDs off
  	LIGHT = GREEN;

	while(1) {
		Delay(2);

		// If sw1 is pressed, the currently on LED will be turned off, the next LED will be turned on. 
    	// If sw2 is pressed, currently on LED will flash at a speed of 50 ms.
    	// If both sw1 and sw2 are pressed, current led will flash once at a speed of 50 ms 
		// and then move to the next one.	
			
		if (SW&SW2_MASK){
			current_led = (uint8_t)LIGHT;
			LIGHT &= ~0x07;
			Delay(1);
			LIGHT = (uint32_t)current_led;
			Delay(1);
		}
		
		if(SW&SW1_MASK) {
			if(LIGHT==RED)
				LIGHT=GREEN;
			else
				LIGHT <<= 1;
		}
	}
}

// Subroutine to initialize port B pins for output
// PB2,PB2,PB0 are outputs for red, green, and yellow LEDs
// Inputs: None
// Outputs: None

void PortB_Init(void){
	SYSCTL_RCGC2_R |= 0x00000002;
	while ((SYSCTL_RCGC2_R&0x00000002) != 0x00000002){}
    
	GPIO_PORTB_DIR_R |= 0x07;      
	GPIO_PORTB_AFSEL_R &= ~0x07;      
	GPIO_PORTB_DEN_R |= 0x07;        
	GPIO_PORTB_AMSEL_R &= ~0x07;      
	GPIO_PORTB_PCTL_R &= ~0x00000FFF;       
}

// Subroutine to initialize port E pins PE0 & PE1 for input
// Inputs: None
// Outputs: None

void PortE_Init(void){   
	SYSCTL_RCGC2_R |= 0x00000010;
	while ((SYSCTL_RCGC2_R&0x00000010) != 0x00000010){}

	GPIO_PORTE_AMSEL_R &= ~0x03;
	GPIO_PORTE_PCTL_R &= ~0x000000FF;
	GPIO_PORTE_DIR_R &= ~0x03;
	GPIO_PORTE_AFSEL_R &= ~0x03;
	GPIO_PORTE_DEN_R |= 0x03;
	GPIO_PORTE_PDR_R |= 0x03; 		//enable pull-down resistors for PE0, PE1
}

// Subroutine to wait about 0.05 sec
// Inputs: None
// Outputs: None
// Notes: the Keil simulation runs slower than the real board

void Delay(uint8_t n_50ms) {
	volatile uint32_t time;
	time = 727240*100/91; //0.05sec
	while(time){
		time--;
	}
}