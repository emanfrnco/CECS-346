// CECS346 Lab 3: FSM
// Student Name: Dylan Dang
// Lab description: Finite State Machine 

// Hardware Design
// 1)	Port E will be used to control 4 LEDs: white(PE3), red (PE2), yellow (PE1), green (PE0).
// 2)	Port A will be used for the two switches: sw1 (PA2), sw2 (PA3)

#include <stdint.h>   // for data type alias

// Registers for switches
// Complete the following register definitions
#define SENSOR					(*((volatile unsigned long *)0x40004030))		// bit addresses for the two switches/Sensors: bits 2&3 // PORT A
#define GPIO_PORTA_DATA_R       (*((volatile unsigned long *)0x400043FC))
#define GPIO_PORTA_DIR_R        (*((volatile unsigned long *)0x40004400))
#define GPIO_PORTA_AFSEL_R      (*((volatile unsigned long *)0x40004420))
#define GPIO_PORTA_DEN_R        (*((volatile unsigned long *)0x4000451C))
#define GPIO_PORTA_AMSEL_R      (*((volatile unsigned long *)0x40004528))
#define GPIO_PORTA_PCTL_R       (*((volatile unsigned long *)0x4000452C))
//#define GPIO_PORTA_PDR_R        (*((volatile unsigned long *)0x40004514))

//// Registers for LEDs
#define LIGHT                   (*((volatile unsigned long *)0x4002403C)) 		// bit addresses for the four LEDs // PORT E
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
	
// Constants definitions
#define SYSCTL_RCGC2_GPIOE      0x00000010  // port E Clock Gating Control
#define SYSCTL_RCGC2_GPIOA      0x00000001  // port A Clock Gating Control

#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value

void Light_Init(void);
void Sensor_Init(void);
void SysTick_Init(void);
void Wait_HalfSecond(uint8_t n);

// FSM state data structure
struct State {
  uint32_t Out; 
  uint32_t Time;  
  uint32_t Next[5];
}; 

typedef const struct State STyp;

// Constants definitions
enum my_states {GREEN,YELLOW,RED,RW,RW1};
//#define SW 			(*((volatile unsigned long *)0x40004030))   // PORTA, bits 2,3				
#define SW1 			(*((volatile unsigned long *)0x40004010))   // PA2 for switch 1																	
#define SW2 			(*((volatile unsigned long *)0x40004020))   // PA3 for switch 2			
#define GREEN 		0								// PE0 for green	 
#define YELLOW 		1								// PE1 for yellow     
#define RW				2												 
#define RED 			3								// PE2 for red				
#define RW1  			4 							// PE3 for white			
																						
// Output pins are:3(white), 2(red), 1(yellow), 0(green)
// Input pins are: 1:sw2, 0:sw1 

STyp FSM[5] = {																																							
	{0x01,4,{GREEN,GREEN,YELLOW,YELLOW}},						// (output, delay , state)
	{0x02,2,{RW,RW,RW,RW}},													//(0.5 delay * 4 = 2 sec)
	{0x0C,4,{RW,RED,RW,RED}},												// (00, 01, 10, 11)
	{0x04,1,{RW1,RW1,RW1,RW1}},
	{0x0C,1,{GREEN,GREEN,GREEN,GREEN}}
};    

int main(void){ 
  uint32_t S;  									// index to the current state 
  uint32_t Input; 
	
	SysTick_Init(); 
	Light_Init();
	Sensor_Init();
  S = GREEN;                   // FSM start with green  
    
  while(1){
		// Put your FSM engine here
		LIGHT = FSM[S].Out; 								// Set lights to state output
		Wait_HalfSecond(FSM[S].Time);				// Set delay to state time
		Input = SENSOR >> 2;
		S = FSM[S].Next[Input];
  }
}

/*********************************************SYSTICK*******************************************************/

void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;                   // disable SysTick during setup
  //NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M;  // maximum reload value
	NVIC_ST_RELOAD_R = 8000000 - 1;
  NVIC_ST_CURRENT_R = 0;                // any write to current clears it
                                        // enable SysTick with core clock
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE + NVIC_ST_CTRL_CLK_SRC;
}

void Wait_HalfSecond(uint8_t n){
  unsigned long i;
  for(i = 0; i < n; i++){
		NVIC_ST_CURRENT_R = 0;																// value written to CURRENT is cleared
		while ((NVIC_ST_CTRL_R & NVIC_ST_CTRL_COUNT) == 0) {} // wait for COUNT
  }
}

/*********************************************SYSTICK*******************************************************/

void Sensor_Init(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOA;      				// Activate Port A clocks 
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOA) != SYSCTL_RCGC2_GPIOA){}	// wait for clock to be active
		
	GPIO_PORTA_AMSEL_R &= ~0x0C;  										// Disable analog function on PA3-2
  GPIO_PORTA_PCTL_R &= ~0x0000FF00;  								// Enable regular GPIO
  GPIO_PORTA_DIR_R &= ~0x0C;    										// Inputs on PA3-2
  GPIO_PORTA_AFSEL_R &= ~0x0C;  										// Regular function on PA3-2
  GPIO_PORTA_DEN_R |= 0x0C;     										// Enable digital signals on PA3-2
  //GPIO_PORTA_PDR_R |= 0x0C;     									// Optional: Enable pull-down resistors for PA3-2
}

void Light_Init(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE;       		// Activate Port E clocks
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOE) != SYSCTL_RCGC2_GPIOE){} // wait for clock to be active
		
	GPIO_PORTE_AMSEL_R &= ~0x0F;  										// Disable analog function on PE3-0
  GPIO_PORTE_PCTL_R &= ~0x000000FF;  								// Enable regular GPIO
  GPIO_PORTE_DIR_R |= 0x0F;     										// Outputs on PE3-0
  GPIO_PORTE_AFSEL_R &= ~0x0F;  										// Regular function on PE3-0
  GPIO_PORTE_DEN_R |= 0x0F;     										// Enable digital on PE3-0
}
