// CECS346 Project 1: Traffic Light Controller
// Student Name: Dylan Dang
// Lab description: Traffic Light Controller using FSM & Systick Timer

// Hardware Design
// 1)	Port B will be used to control 3 West LEDs (RED,YELLOW,GREEN) and 3 South LEDs (RED,YELLOW,GREEN)
// 2)	Port E will be used for the 3 switches: sw1 (PE0), sw2 (PE1), sw3 (PE2)
// 3) Port F will be used to control 2 Pedestrian LEDs (RED,GREEN)

#include <stdint.h>   // for data type alias

// Registers for switches
// Complete the following register definitions
#define SENSORS									(*((volatile unsigned long *)0x4002401C))		// bit addresses for the 3 switches, Port E 2-0
	
#define GPIO_PORTE_DIR_R        (*((volatile unsigned long *)0x40024400))
#define GPIO_PORTE_AFSEL_R      (*((volatile unsigned long *)0x40024420))
#define GPIO_PORTE_DEN_R        (*((volatile unsigned long *)0x4002451C))
#define GPIO_PORTE_AMSEL_R      (*((volatile unsigned long *)0x40024528))
#define GPIO_PORTE_PCTL_R       (*((volatile unsigned long *)0x4002452C))
#define GPIO_PORTA_PDR_R        (*((volatile unsigned long *)0x40004514))

//// Registers for Port B LEDs
#define T_LIGHTS                (*((volatile unsigned long *)0x400050FC)) 		// bit addresses for the 6 LEDs // PORT B 0-5
	
#define GPIO_PORTB_DIR_R        (*((volatile unsigned long *)0x40005400))
#define GPIO_PORTB_AFSEL_R      (*((volatile unsigned long *)0x40005420))
#define GPIO_PORTB_DEN_R        (*((volatile unsigned long *)0x4000551C))
#define GPIO_PORTB_AMSEL_R      (*((volatile unsigned long *)0x40005528))
#define GPIO_PORTB_PCTL_R       (*((volatile unsigned long *)0x4000552C))

//// Registers for Port F LEDs
#define P_LIGHTS                (*((volatile unsigned long *)0x40025028)) 		// bit addresses for the 2 LEDs // PORT F 1&3
	
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
	
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))	

// Constants definitions
#define SYSCTL_RCGC2_GPIOB      0x00000002  // Port B Clock Gating Control
#define SYSCTL_RCGC2_GPIOE      0x00000010  // Port E Clock Gating Control
#define SYSCTL_RCGC2_GPIOF      0x00000020  // Port F Clock Gating Control

#define NVIC_ST_CTRL_R          (*((volatile unsigned long *)0xE000E010))
#define NVIC_ST_RELOAD_R        (*((volatile unsigned long *)0xE000E014))
#define NVIC_ST_CURRENT_R       (*((volatile unsigned long *)0xE000E018))
#define NVIC_ST_CTRL_COUNT      0x00010000  // Count flag
#define NVIC_ST_CTRL_CLK_SRC    0x00000004  // Clock Source
#define NVIC_ST_CTRL_INTEN      0x00000002  // Interrupt enable
#define NVIC_ST_CTRL_ENABLE     0x00000001  // Counter mode
//#define NVIC_ST_RELOAD_M        0x00FFFFFF  // Counter load value

// Function Prototypes
void T_Light_Init(void);
void P_Light_Init(void);
void Sensor_Init(void);
void SysTick_Init(void);
void Wait_QuarterSecond(uint8_t n);

// FSM state data structure
struct State {
  uint32_t Out_T; 
	uint32_t Out_P; 
  uint32_t Time;  
  uint32_t Next[8];
}; 


typedef const struct State STyp;

// Constants definitions
enum my_states {GoS,WaitS,GoW,WaitW,GoP,WaitPOn1,WaitPOff1,WaitPOn2,WaitPOff2};		
#define SW1 			(*((volatile unsigned long *)0x40024004))   // PE0 for Pedestrian Switch																	
#define SW2 			(*((volatile unsigned long *)0x40024008))   // PE1 for West Switch
#define SW3				(*((volatile unsigned long *)0x40024010))   // PE2 for South Switch
	
#define GoS 			0								 
#define WaitS 		1								    
#define GoW				2												 
#define WaitW 		3												
#define GoP  			4 										
#define WaitPOn1	5
#define WaitPOff1	6
#define WaitPOn2	7	
#define WaitPOff2	8
																					

STyp FSM[9] = {																																							
	{0x21,0x02,8,{GoS,WaitS,WaitS,WaitS,GoS,WaitS,WaitS,WaitS}},						//(output, output, delay , states)
	{0x22,0x02,4,{GoW,GoP,GoW,GoW,GoP,GoP,GoW,GoP}},												//(0.25*8 = 2sec)
	{0x0C,0x02,8,{GoW,WaitW,GoW,WaitW,WaitW,WaitW,WaitW,WaitW}},																				
	{0x14,0x02,4,{GoS,GoP,GoP,GoP,GoS,GoP,GoS,GoS}},
	{0x24,0x08,8,{GoP,GoP,WaitPOn1,WaitPOn1,WaitPOn1,WaitPOn1,WaitPOn1,WaitPOn1}},
	{0x24,0x02,1,{WaitPOff1,WaitPOff1,WaitPOff1,WaitPOff1,WaitPOff1,WaitPOff1,WaitPOff1,WaitPOff1}},
	{0x24,0x00,1,{WaitPOn2,WaitPOff1,WaitPOn2,WaitPOn2,WaitPOn2,WaitPOn2,WaitPOn2,WaitPOn2}},
	{0x24,0x02,1,{WaitPOff2,WaitPOff2,WaitPOff2,WaitPOff2,WaitPOff2,WaitPOff2,WaitPOff2,WaitPOff2}},
	{0x24,0x00,1,{GoS,GoP,GoW,GoW,GoS,GoS,GoS,GoW}}
};    

int main(void){ 
  uint32_t S;  									// index to the current state 
  uint32_t Input; 
	
	SysTick_Init(); 
	T_Light_Init();
	P_Light_Init();
	Sensor_Init();
  S = GoS;                   		// FSM start with Green on South
    
  while(1){
		// Put your FSM engine here
		T_LIGHTS = FSM[S].Out_T; 						// Set lights to state output
		P_LIGHTS = FSM[S].Out_P;
		Wait_QuarterSecond(FSM[S].Time);		// Set delay to state time
		Input = SENSORS;
		S = FSM[S].Next[Input];
  }
}

/*********************************************SYSTICK*******************************************************/

void SysTick_Init(void){
  NVIC_ST_CTRL_R = 0;                    // disable SysTick during setup
//NVIC_ST_RELOAD_R = NVIC_ST_RELOAD_M;   // maximum reload value
	NVIC_ST_RELOAD_R = 4000000 - 1;
  NVIC_ST_CURRENT_R = 0;                 // any write to current clears it
                                         // enable SysTick with core clock
  NVIC_ST_CTRL_R = NVIC_ST_CTRL_ENABLE + NVIC_ST_CTRL_CLK_SRC;
}

void Wait_QuarterSecond(uint8_t n){
  unsigned long i;
  for(i = 0; i < n; i++){
		NVIC_ST_CURRENT_R = 0;																// value written to CURRENT is cleared
		while ((NVIC_ST_CTRL_R & NVIC_ST_CTRL_COUNT) == 0) {} // wait for COUNT
  }
}

/*********************************************SYSTICK*******************************************************/

void Sensor_Init(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOE;      			// Activate Port E clocks 
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOE) != SYSCTL_RCGC2_GPIOE){}	// wait for clock to be active
		
	GPIO_PORTE_AMSEL_R &= ~0x07; 				  //disable analog function
	GPIO_PORTE_PCTL_R  &= ~0x00000FFF;    // enable regular GPIO
	GPIO_PORTE_DIR_R 	 &= ~0x07; 					//PE2, PE1, PE0 input
	GPIO_PORTE_AFSEL_R &= ~0x07; 				  //no alternate function
	GPIO_PORTE_DEN_R 	 |= 0x07;						// Enable digital signals on PE2-PE0
//GPIO_PORTA_PDR_R   |= 0x07;     		  // Optional: Enable pull-down resistors for PA3-2
}

void T_Light_Init(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOB;       		// Activate Port B clocks
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOB) != SYSCTL_RCGC2_GPIOB){} // wait for clock to be active
		
	GPIO_PORTB_AMSEL_R &= ~0x3F; 				 //disable analog function
	GPIO_PORTB_PCTL_R  &= ~0x00FFFFFF; 	 //enable regular GPIO
	GPIO_PORTB_DIR_R 	 |= 0x3F; 				 //PB5-PB0 output
	GPIO_PORTB_AFSEL_R &= ~0x3F; 				 //no alternate function
	GPIO_PORTB_DEN_R 	 |= 0x3F;	      	 //enable digital pins PF4-PF0
}

void P_Light_Init(void){
	SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF;       		// Activate Port F clocks
	while ((SYSCTL_RCGC2_R&SYSCTL_RCGC2_GPIOF) != SYSCTL_RCGC2_GPIOF){} // wait for clock to be active
		
	GPIO_PORTF_AMSEL_R &= ~0x0A;         // disable analog function
  GPIO_PORTF_PCTL_R  &= ~0x0000F0F0; 	 // GPIO clear bit PCTL  
  GPIO_PORTF_DIR_R   |= 0x0A;          // PF3,PF1 output   
	GPIO_PORTF_AFSEL_R &= ~0x0A;         // no alternate function 
  GPIO_PORTF_DEN_R   |= 0x0A;          // enable digital pins PF1 & PF3       
}
