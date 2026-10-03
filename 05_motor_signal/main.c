/////////////////////////////////////////////////////////////
// HW1: STEP 및 DC Motor 구동용 펄스 발생
// 제출자: 백주원
// 주요 내용 및 구현 방법
// -스위치 입력으로 step motor 구동용 위치제어 펄스를발생
// -가변저항 전압값 입력으로 DC motor 구동용 torque 제어펄스를 발생
// -Step motor Position LCD로 출력
// -DC motor Torque LCD로 출력
// -TIM1_CH3 -> step motor 구동용 pulse
// -TIM5_CH2 <- SW3 위치명령용 pulse(count)
// -TIM4 Counter Mode
// -TIM14_CH1 -> Buzzer DC motor구동용 PWM pulse
// -ADC3_IN1 <- 가변저항 torque명령용 전압값
/////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

// SW 0~7 Define
#define SW0_PUSH        0xFE00  // PH8
#define SW1_PUSH        0xFD00  // PH9
#define SW2_PUSH        0xFB00  // PH10
#define SW3_PUSH        0xF700  // PH11
#define SW4_PUSH        0xEF00  // PH12
#define SW5_PUSH        0xDF00  // PH13
#define SW6_PUSH        0xBF00  // PH14
#define SW7_PUSH        0x7F00  // PH15

#define STEP_DEG 7.5

void _GPIO_Init(void);			// GPIO 초기화
void DisplayInitScreen(void);	// Display 초기화

void TIMER1_OC_init(void);			// Display 초기화
void TIMER4_init(void);				// ADC3 초기화
void TIMER5_COUNTER_init(void);		// TIMER14 초기화
void TIMER14_PWM_init(void);		// TIMER5 초기화
void _ADC_Init(void);				// TIMER1 초기화

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
uint16_t KEY_Scan(void);			// Switch 입력 함수

uint16_t Pos = 0;									// Position
uint16_t ADC_Value = 0, Torque = 0, Voltage = 0;	// ADC_Value, 토크, 전압

int main(void)
{
	LCD_Init();
	DisplayInitScreen();	// Display 초기화
	_ADC_Init();			// ADC3 초기화
    TIMER14_PWM_init();		// TIMER14 초기화
    TIMER5_COUNTER_init();	// TIMER5 초기화
	TIMER4_init();			// TIMER4 초기화
	TIMER1_OC_init();		// TIMER1 초기화

	ADC3->CR2 |= ADC_CR2_SWSTART; // 0x40000000 (1<<30)
	while (1)
	{
		switch (Torque)
		{
		case 0:
			TIM14->CCR1 = 0;		// DR: 0%
			break;
		case 1:
			TIM14->CCR1 = 24;		// DR: 30%
			break;
		case 2:
			TIM14->CCR1 = 48;		// DR: 60%
			break;
		case 3:
			TIM14->CCR1 = 72;		// DR: 90%
			break;
		}
	}
}

void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);					// 화면 클리어
	LCD_SetFont(&Gulim10);					// 폰트 : 굴림 10

	LCD_SetBackColor(RGB_BLACK);				// 글자배경색 : BLACK
	LCD_SetTextColor(RGB_WHITE);				// 글자색 : WHITE
	LCD_DisplayText(0, 0, "Motor Control");		// Title
	LCD_DisplayText(1, 0, "BJW");	// Name

	LCD_SetBackColor(RGB_YELLOW);			// 글자배경색 : YELLOW
	LCD_SetTextColor(RGB_BLACK);			// 글자색 : BLACK
	LCD_DisplayText(2, 0, ">Step Motor");	// Step Motor
	LCD_DisplayText(3, 0, " Position:");	// Position command
	LCD_DisplayText(4, 0, ">DC Motor");		// Step Motor
	LCD_DisplayText(5, 0, " Torque:");		// Torque command

	LCD_SetTextColor(RGB_RED);				// 글자색 : RED
	LCD_DisplayChar(3, 10, 0 + '0');		// Position value
	LCD_DisplayChar(5, 8, 0 + '0');			// Torque value
}

void TIMER1_OC_init(void)
{
	// PE13: TIM1_CH3
	// PE13을 출력설정하고 Alternate function(TIM1_CH3)으로 사용 선언
	RCC->AHB1ENR |= (1 << 4);		// RCC_AHB1ENR GPIOE Enable
	RCC->APB2ENR |= (1 << 0);		// RCC_APB2ENR TIMER1 Enable

	GPIOE->MODER |= (2 << 26);					// GPIOE PIN13 Output Alternate function mode
	GPIOE->OSPEEDR |= (3 << 26);				// GPIOE PIN13 Output speed(100MHz High speed)
	GPIOE->OTYPER = 0x00000000;					// GPIOE PIN13 Output type push - pull(reset state)
	GPIOE->PUPDR |= (2 << 2 * 13);				// GPIOE PIN13 Pull-Down
	GPIOE->AFR[1] |= (1 << 4 * (13 - 8));		// AFR[1].(23~20)

	TIM1->PSC = 8400 - 1;						// Prescaler 168,000,000Hz/8400= 20KHz (50us)
	TIM1->ARR = 20000 - 1;						// 주기 = 50us * 20000 = 1s

	TIM1->CR1 &= ~(1 << 4);			// DIR: Countermode = Upcounter (reset state)
	TIM1->CR1 &= ~(3 << 8);			// CKD: Clock division = 1 (reset state)
	TIM1->CR1 &= ~(3 << 5);			// CMS(Center-aligned mode Sel): No(reset state)

	TIM1->EGR |= (1 << 0);			// UG: Update generation

	// Output/Compare Mode
	TIM1->CCER &= ~(1 << 8);				 // CC3E: OC3 Active 엑셀 OFF
	TIM1->CCER |= (1 << (4 * (3 - 1) + 1));  // CC3P: OCPolarity_Active Low(반전)

	TIM1->BDTR |= (1 << 15);		// main output enable

	TIM1->CCMR2 &= ~(3 << 0);		// CC3S(CC3channel): Output 
	TIM1->CCMR2 &= ~(1 << 3);		// OC3PE: Output Compare 3 preload disable
	TIM1->CCMR2 |= (3 << 4);		// OC3M: Output Compare 3 Mode : toggle

	TIM1->CR1 &= ~(1 << 7);			// ARPE: Auto reload preload disable
	TIM1->DIER |= (1 << 3);			// CC3IE: Enable the Tim1 CC3 interrupt

	NVIC->ISER[0] |= (1 << 27);		// TIM1_CC
	TIM1->CR1 &= ~(1 << 0);			// CEN: Disable the Timer1 Counter 시동 OFF
}

uint16_t INT_CNT, GOAL_DEG;
void TIM1_CC_IRQHandler(void)      //RESET: 0
{
	if ((TIM1->SR & 0x08) != RESET)	// CC3 interrupt flag 
	{
		TIM1->SR &= ~0x08;			// CC3 Interrupt Clear
		INT_CNT++;
		if (INT_CNT >= 2 * GOAL_DEG / STEP_DEG)	// 출력 펄스수 제어 
		{
			TIM1->CCER &= ~(1 << 8);	// CC3E Disable 엑셀 OFF
			TIM1->CR1 &= ~(1 << 0);		// TIM1 Disable 시동 OFF
			INT_CNT = 0;
		}
	}
}

void TIMER4_init(void)
{
	RCC->APB1ENR |= (1 << 2);		// RCC_APB1ENR TIMER4 Enable

	// Time base 설정
	// Setting CR1 : 0x0000
	TIM4->CR1 &= ~(1 << 4);			// DIR=0(Up counter)(reset state)
	TIM4->CR1 &= ~(1 << 1);			// UDIS=0(Update event Enabled): By one of following events
	TIM4->CR1 &= ~(1 << 2);			// URS=0(Update Request Source  Selection): By one of following events
	TIM4->CR1 &= ~(1 << 3);			// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM4->CR1 |= (1 << 7);			// ARPE=1(ARR is buffered): ARR Preload Enalbe 
	TIM4->CR1 &= ~(3 << 5);			// CKD(Clock division)=00(reset state)
	TIM4->CR1 &= ~(3 << 8);			// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)

	// PSC, ARR
	TIM4->PSC = 8400 - 1;			// Prescaler=84, 84MHz/8400 = 10KHz (0.1ms)
	TIM4->ARR = 1000 - 1;			// Auto reload  : 0.1ms * 1000 = 100ms(period) : 인터럽트주기나 출력신호의 주기 결정

	// Clear the Counter
	TIM4->EGR |= (1 << 0);			// UG(Update generation)=1

	// Setting an UI(UEV) Interrupt 
	NVIC->ISER[0] |= (1 << 30); 	// Enable Timer4 global Interrupt
	TIM4->DIER |= (1 << 0);			// Enable the Timer4 Update interrupt

	TIM4->CR1 |= (1 << 0);			// Enable the Timer4 Counter (clock enable)
}

void TIM4_IRQHandler(void)  		// 100ms Interrupt
{
	TIM4->SR &= ~(1 << 0);						// Interrupt flag Clear
	LCD_DisplayChar(3, 10, TIM5->CNT + '0');	// Position value

	if (TIM5->CNT == 0)							// TIM5->CNT = 0이면 반환
		return;

	if (Pos != TIM5->CNT)
	{
		Pos = TIM5->CNT;						// TIM5 CNT 값(Encoder 펄스 수) 읽음
		GOAL_DEG = STEP_DEG * (2 * TIM5->CNT);

		TIM1->CCER |= (1 << 8);					// CC3E Enable
		TIM1->CR1 |= (1 << 0);					// TIM1_CNT Enable
	}	
}

void TIMER5_COUNTER_init(void)
{
	// Position 입력(Counting) 핀: PH11 (TIM5_CH2)
	// Clock Enable : GPIOH & TIMER5
	RCC->AHB1ENR |= (1 << 7);		// RCC_AHB1ENR : GPIOH(bit#7) Enable
	RCC->APB1ENR |= (1 << 3);		// RCC_APB1ENR TIMER5 Enable

	// PH11: TIM5_CH2
	// PH11을 입력설정하고 Alternate function(TIM5_CH2)으로 사용 선언
    GPIOH->MODER |= (2 << 22);		// GPIOH PIN11 Output Alternate function mode
	GPIOH->PUPDR &= ~(3 << 22); 	// GPIOH PIN11 NO Pull-up
	GPIOH->AFR[1] |= (2 << 12);		// AFR[1].(15~12)

	// Time base 설정
	// Setting CR1 : 0x0000
	TIM5->CR1 &= ~(1 << 4);			// DIR=0(Up counter)(reset state)
	TIM5->CR1 &= ~(1 << 1);			// UDIS=0(Update event Enabled): By one of following events
	TIM5->CR1 &= ~(1 << 2);			// URS=0(Update Request Source  Selection): By one of following events
	TIM5->CR1 &= ~(1 << 3);			// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM5->CR1 &= ~(3 << 5);			// CKD(Clock division)=00(reset state)
	TIM5->CR1 &= ~(3 << 8);			// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)

	// PSC, ARR
	TIM5->PSC = 1 - 1;	// Prescaler=1
	TIM5->ARR = 3;		// Auto reload  :  count값 범위: 0~3

	// Update(Clear) the Counter
	TIM5->EGR |= (1 << 0);    // UG=1, REG's Update (CNT clear) 

	// External Clock Mode 1
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM5->CCMR1 |= (1 << 8); 	// CC2S(CC2 channel) = Input 
	TIM5->CCMR1 &= ~(15 << 12);	// IC2F : No Input Filter 

	// CCER(Capture/Compare Enable Register) : Enable "Channel 2" 
	TIM5->CCER &= ~(1 << 4);	// CC2E=0: Capture Disable
	// TI2FP2 NonInverting / Falling Edge
	TIM5->CCER |= (1 << 5);		// CC2P=1
	TIM5->CCER &= ~(1 << 7);	// CC2NP=0

	// SMCR(Slave Mode Control Reg.) : External Clock Enable
	TIM5->SMCR |= (6 << 4);	// TS(Trigger Selection)=0b101 :TI2FP2(Filtered Timer Input 1 출력신호)
	TIM5->SMCR |= (7 << 0);	// SMS(Slave Mode Selection)=0b111 : External Clock Mode 1

	TIM5->CR1 |= (1 << 0);	// CEN: Enable the Timer5 Counter 
}

void TIMER14_PWM_init(void)
{
	// ADC3: PA1(pin 41)
	RCC->AHB1ENR |= (1 << 5);			// RCC_AHB1ENR : GPIOF(bit#5) Enable
	RCC->APB1ENR |= (1 << 8);			// RCC_APB1ENR TIMER14 Enable

	GPIOF->MODER |= (2 << 18);			// GPIOF PIN9 Output Alternate function mode
	GPIOF->OSPEEDR |= (3 << 18);		// GPIOF PIN9 Output speed(100MHz High speed)
	GPIOF->OTYPER &= ~(1 << 9);			// GPIOF PIN9 Output type push - pull(reset state)
	GPIOF->AFR[1] |= (9 << 4);			// AFR[1].(7~4)

	// Time base 설정
	// Setting CR1 : 0x0000
	TIM14->CR1 &= ~(1 << 4);			// DIR=0(Up counter)(reset state)
	TIM14->CR1 |= (1 << 7);				// ARPE=1(ARR is buffered): ARR Preload Enalbe 
	TIM14->CR1 &= ~(3 << 5);			// CKD(Clock division)=00(reset state)
	TIM14->CR1 &= ~(3 << 8);			// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)

	// TIM14 Channel 1 : PWM 2 mode
	// Assign 'PWM Pulse Period'
	TIM14->PSC = 420 - 1;				// Prescaler 84,000,000Hz/420 = 200KHz(5us)
	TIM14->ARR = 80 - 1;				// Auto reload  (5us * 80 = 400us : PWM Period)

	// Define the corresponding pin by 'Output'  
	// CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
	TIM14->CCER |= (1 << 0);			// CC1E=1: OC1(TIM14_CH1) Active(Capture/Compare 1 output enable)
	// 해당핀(27번)을 통해 신호출력
	TIM14->CCER |= (1 << 1);			// CC1P=0: CC1 output Polarity low (OC1으로 반전 출력)

	// Duty Ratio 
	TIM14->CCR1 = 0;					// CCR1 value

	// 'Mode' Selection : Output mode, PWM 2
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM14->CCMR1 &= ~(3 << 0); 	// CC1S(CC1 channel)='0b00' : Output 
	TIM14->CCMR1 |= (1 << 3); 	// OC1PE=1: Output Compare 1 preload Enable

	TIM14->CCMR1 |= (7 << 4);	// OC1M: Output compare 1 mode: PWM 2 mode
	TIM14->CCMR1 |= (1 << 7);	// OC1CE: Output compare 1 Clear enable

	TIM14->CR1 |= (1 << 0);		// CEN: Enable the Timer14 Counter
}

void _ADC_Init(void)
{
	RCC->AHB1ENR |= (1 << 0);		// ENABLE GPIOA CLK
	GPIOA->MODER |= (3 << 2);		// CONFIG GPIOA PIN1(PA1) TO ANALOG IN MODE

	RCC->APB2ENR |= (1 << 10);		// ENABLE ADC3 CLK
	
	ADC->CCR &= ~(0X1F << 0);		// MULTI[4:0]: ADC_Mode_Independent
	ADC->CCR |= (1 << 16); 			// 0x00010000 ADCPRE:ADC_Prescaler_Div4 (ADC MAX Clock 36MHz, 84Mhz(APB2)/4 = 21MHz)

	ADC3->CR1 |= (1 << 24);			// RES[1:0] : 10bit Resolution
	ADC3->CR1 &= ~(1 << 8);			// SCAN=0 : ADC_ScanCovMode Disable
	ADC3->CR1 |= (1 << 5);			// EOCIE=1: Interrupt enable for EOC

	ADC3->CR2 &= ~(1 << 1);			// CONT=0: ADC_Continuous ConvMode Disable
	ADC3->CR2 &= ~(3 << 28);		// EXTEN[1:0]=0b00:
									// ADC_ExternalTrigConvEdge_None
	ADC3->CR2 &= ~(1 << 11);		// ALIGN=0: ADC_DataAlign_Right
	ADC3->CR2 &=  ~(1 << 10);		// EOCS=1: The EOC bit is set at the end of each 

	ADC3->SQR1 &= ~(0xF << 20);			// L[3:0]=0b0000: ADC Regular channel sequece length 0b0000:1 conversion)

	ADC3->SMPR2 |= (0x7 << (3 * 1));	// ADC3_CH1 Sample Time_480Cycles (3*Channel_1)
	//Channel selection, The Conversion Sequence of PIN1(ADC3_CH1)

	ADC3->SQR3 |= (1 << 0);		// SQ1[4:0]=0b0001 : CH1

	NVIC->ISER[0] |= (1 << 18);	// Enable ADC global Interrupt

	ADC3->CR2 |= (1 << 0);		// ADON=1: ADC ON
}

void ADC_IRQHandler(void)
{
	ADC3->SR &= ~(1 << 1);						// EOC flag clear

	ADC_Value = ADC3->DR;						// Reading ADC result
	Voltage = ADC_Value * (3.3 * 100) / 1023;   // 3.3 : 1023 =  Volatge : ADC_Value

	if (Voltage <= 79)			// 0.00V~0.79V : 0
		Torque = 0;
	else if (Voltage <= 159)	// 0.80V~1.59V : 1
		Torque = 1;
	else if (Voltage <= 239)	// 1.60V~2.39V : 2
		Torque = 2;
	else if (Voltage <= 330)	// 2.40V~3.30V : 3
		Torque = 3;

	LCD_DisplayChar(5, 8, Torque + '0');		// Torque value

	ADC3->CR2 |= ADC_CR2_SWSTART;				// 변환 가능
}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i = 0; i < wMS; i++)
		DelayUS(1000);   // 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS * 17;
	for (; Dly; Dly--);
}

uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if (key == 0xFF00)		// if no key, check key off
	{
		if (key_flag == 0)
			return key;
		else
		{
			DelayMS(10);
			key_flag = 0;
			return key;
		}
	}
	else				// if key input, check continuous key
	{
		if (key_flag != 0)	// if continuous key, treat as no key input
			return 0xFF00;
		else			// if new key,delay for debounce
		{
			key_flag = 1;
			DelayMS(10);
			return key;
		}
	}
}