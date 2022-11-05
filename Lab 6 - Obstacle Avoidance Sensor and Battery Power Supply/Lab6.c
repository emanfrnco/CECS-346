// Name: Dylan Dang
// Lab6.c

#include <stdint.h> // C99 data types
#include "tm4c123gh6pm.h"

// 1. Pre-processor Directives Section
// Constant declarations to access port registers using 
// symbolic names instead of addresses
#define LED       (*((volatile unsigned long *)0x40025018))
// Color    LED(s) PortF
// red      R--    0x02
// blue     --B    0x04
// green    -G-    0x08
#define RED  0x02
#define GREEN 0x08

// Function Prototypes (external functions from startup.s)
extern void DisableInterrupts(void); // Disable interrupts
extern void EnableInterrupts(void);  // Enable interrupts
extern void WaitForInterrupt(void);  // Go to low power mode while waiting for the next interrupt

// Function Prototypes
void EdgeCounter_Init(void);  // Initialize edge trigger interrupt for PF0 (SW2) rising edge
void PortF_LEDInit(void);     // Initialize Port F LEDs

int main(void){
	DisableInterrupts();
  PortF_LEDInit();
	EdgeCounter_Init();           // initialize GPIO Port F interrupt
	EnableInterrupts();
	
	if ((GPIO_PORTF_DATA_R&0x01) == 0x01) {
		//LED = GREEN;
		GPIO_PORTF_DATA_R = 0x08;
		} else {
			//LED = RED;
			GPIO_PORTF_DATA_R = 0x02;
		}
		
  while(1){
		WaitForInterrupt();
  }
}

// Initialize Port F LEDs
void PortF_LEDInit(void) {
	
	if ((SYSCTL_RCGC2_R & 0x00000020) != 0x00000020) {
		SYSCTL_RCGC2_R |= 0x00000020;    // 1) F clock
	}
	while ((SYSCTL_RCGC2_R & 0x00000020) != 0x00000020) {}	
  GPIO_PORTF_AMSEL_R &= ~0x0A;       // 3) disable analog function 
  GPIO_PORTF_PCTL_R &= ~0x0000F0F0;  // 4) GPIO clear bit PCTL  
  GPIO_PORTF_DIR_R |= 0x0A;          // 5) PF0 input & PF3,PF1 output  
  GPIO_PORTF_AFSEL_R &= ~0x0A;       // 6) no alternate function 
  GPIO_PORTF_DEN_R |= 0x0A;          // 7) enable digital pins PF3,PF1 
}

// Initialize edge trigger interrupt for PF0 rising edge
void EdgeCounter_Init(void) {  
	
	if ((SYSCTL_RCGC2_R & 0x00000020) != 0x00000020) {  
		SYSCTL_RCGC2_R |= 0x00000020; 	// activate clock for port F
	}
	while ((SYSCTL_RCGC2_R & 0x00000020) != 0x00000020) {}
	GPIO_PORTF_LOCK_R = 0x4C4F434B;   // unlock PortF PF0
  GPIO_PORTF_CR_R |= 0x01;          // allow changes to PF0
  GPIO_PORTF_DIR_R &= ~0x01;    		//  make PF0 in (built-in button)
  GPIO_PORTF_AFSEL_R &= ~0x01;  		//  disable alt funct on PF0
  GPIO_PORTF_DEN_R |= 0x01;     		//  enable digital I/O on PF0   
  GPIO_PORTF_PCTL_R &= ~0x0000000F; //  configure PF0 as GPIO
  GPIO_PORTF_AMSEL_R = 0;      			//  disable analog functionality on PF
  GPIO_PORTF_PUR_R |= 0x01;     		//  enable weak pull-up on PF0
	
	GPIO_PORTF_IS_R &= ~0x01;     		//  PF0 is edge-sensitive, 0 = edge, 1 = level
  GPIO_PORTF_IBE_R |= 0x01;    			//  PF0 is both edges
//GPIO_PORTF_IEV_R |= 0x01;    			//  PF0 rising edge event, 0 = falling, 1 = rising
  GPIO_PORTF_ICR_R |= 0x01;     		//  clear flag
  GPIO_PORTF_IM_R |= 0x01;      		//  arm interrupt on PF0
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF1FFFFF)|0x00A00000; // priority 5
  NVIC_EN0_R |= 0x40000000;    		 	//  enable interrupt 30 in NVIC  
}

// Handle GPIO Port F interrupts. 
void GPIOPortF_Handler(void) {
	GPIO_PORTF_ICR_R |= 0x01; 		 		// acknowledge flag0 
	
	if ((GPIO_PORTF_DATA_R&0x01) == 0x01) {
		GPIO_PORTF_DATA_R = 0x08;
		} else {
			GPIO_PORTF_DATA_R = 0x02;
		}
	
}
