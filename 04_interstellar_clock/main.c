/////////////////////////////////////////////////////////////
// HW1: 인터스텔라 시계 제작
// 제출자: 백주원
// 주요 내용 및 구현 방법
// -지구와 밀러행성의 시계를 표시
// -중력이 다른 밀러행성의 시계를 변경하는 기능
// -밀러행성의 주기를 변경하는 기능
// -지구와 밀러행성의 시간을 초기화하는 기능
// -지구 시간: TIM5의 Up-Counting mode(100msec) 이용
// -밀러 시간: TIM4의 Up-Counting mode(100msec) 이용 
// -밀러 시계 증가 속도 변경: ARR을 변경하여 속도 변경 
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

void _GPIO_Init(void);			// GPIO 초기화
void DisplayInitScreen(void);	// Display 초기화
void _EXTI_Init(void);			// EXTI 초기화
void TIMER4_OC_Init(void);		// TIM4 OC Mode 초기화
void TIMER5_Init(void);			// TIM5 초기화

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint16_t KEY_Scan(void);			// Switch 입력 함수

void BEEP(void);
void DisplayTimeUpdate(void);		// 시간 업데이트
void DisplayPeriodUpdate(void);		// MILLER 행성 주기 업데이트

// 지구 시, 분, 밀러 시, 분을 저장하기 위한 변수
uint8_t E_hour = 23, E_minute = 50, M_hour = 23, M_minute = 50;
// Interrupt flag 변수
uint8_t SW6_flag, SW7_flag;
// EXTI14 발생 횟수, TIM4 주기를 저장하기 위한 변수
uint16_t SW6_cnt, TIM4_period = 100;

int main(void)
{
	LCD_Init();
	_GPIO_Init();
	_EXTI_Init();
	TIMER4_OC_Init();
	TIMER5_Init();
	DisplayInitScreen();
	DelayMS(10);

	BEEP();
	GPIOG->ODR &= 0xFF00;// 초기값: LED0~7 Off

	while (1)
	{
		DisplayTimeUpdate();				// 시간 업데이트

		if (SW6_flag)						// 인터럽트 실행
		{
			SW6_flag = 0;					// 인터럽트 초기화
			if (SW6_cnt % 3 == 0)			// 100ms
			{
				TIM4->CR1 &= ~(1 << 0);		// TIM4->CR1.CEN = 0
				TIM4->ARR = 1000 - 1;		// TIM4->ARR 값 변경 0.1ms * 1000 = 100ms
				TIM4->EGR |= (1 << 0);		// TIM4->CNT = 0
				TIM4->CR1 |= (1 << 0);		// TIM4->CR1.CEN = 1
				TIM4_period = 100;
			}
			else if (SW6_cnt % 3 == 1)		// 200ms
			{
				TIM4->CR1 &= ~(1 << 0);		// TIM4->CR1.CEN = 0
				TIM4->ARR = 2000 - 1;		// TIM4->ARR 값 변경 0.1ms * 2000 = 200ms
				TIM4->EGR |= (1 << 0);		// TIM4->CNT = 0
				TIM4->CR1 |= (1 << 0);		// TIM4->CR1.CEN = 1
				TIM4_period = 200;
			}
			else if (SW6_cnt % 3 == 2)		// 500ms
			{
				TIM4->CR1 &= ~(1 << 0);		// TIM4->CR1.CEN = 0
				TIM4->ARR = 5000 - 1;		// TIM4->ARR 값 변경 0.1ms * 5000 = 500ms
				TIM4->EGR |= (1 << 0);		// TIM4->CNT = 0
				TIM4->CR1 |= (1 << 0);		// TIM4->CR1.CEN = 1
				TIM4_period = 500;
			}

			DisplayPeriodUpdate();			// MILLER 행성 주기 업데이트
		}

		if (SW7_flag)										// 인터럽트 실행
		{
			SW7_flag = 0;									// 인터럽트 초기화
			E_hour = E_minute = M_hour = M_minute = 0;		// 시간 초기화

			TIM4->CR1 &= ~(1 << 0);							// TIM4->CR1.CEN = 0
			TIM5->CR1 &= ~(1 << 0);							// TIM5->CR1.CEN = 0

			GPIOG->BSRRL = 0x80;							// LED7 ON
			BEEP();											// BUZZER 1회

			DelayMS(3000);									// 3초 delay
			GPIOG->BSRRH = 0x80;							// LED7 OFF

			TIM4->EGR |= (1 << 0);							// TIM4->CNT = 0
			TIM5->EGR |= (1 << 0);							// TIM5->CNT = 0

			DisplayTimeUpdate();							// 시간 업데이트

			TIM4->CR1 |= (1 << 0);							// TIM4->CR1.CEN = 1
			TIM5->CR1 |= (1 << 0);							// TIM5->CR1.CEN = 1
		}
	}
}

void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
	RCC->AHB1ENR |= 0x00000040;		// RCC_AHB1ENR : GPIOG(bit#6) Enable
	GPIOG->MODER &= ~0x0000FFFF;	// GPIOG 0~7 : Clear
	GPIOG->MODER |= 0x00005555;		// GPIOG 0~7 : General purpose output mode
	GPIOG->OSPEEDR &= ~0x0000FFFF;	// GPIOG 0~7 : Clear
	GPIOG->OSPEEDR |= 0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed
	GPIOG->OTYPER &= ~0x00FF;		// GPIOG 0~7 : Push-pull

	// Buzzer (GPIO F) 설정
	RCC->AHB1ENR |= 0x00000020;		// RCC_AHB1ENR : GPIOF(bit#5) Enable
	GPIOF->MODER &= ~0x000C0000;	// GPIOF 9 : Clear
	GPIOF->MODER |= 0x00040000;		// GPIOF 9 : General purpose output mode
	GPIOF->OSPEEDR &= ~0x000C0000;	// GPIOF 9 : Clear
	GPIOF->OSPEEDR |= 0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed

	// SW (GPIO H) 설정
	RCC->AHB1ENR |= 0x00000080;		// RCC_AHB1ENR : GPIOH(bit#7) Enable
	GPIOH->MODER &= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)
	GPIOH->PUPDR &= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state
}

void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);						// 화면 클리어(YELLOW)
	LCD_SetFont(&Gulim10);						// 폰트: 굴림 10
	LCD_SetBackColor(RGB_YELLOW);				// 글자배경색: YELLOW
	LCD_SetTextColor(RGB_BLACK);				// 글자색: BLACK
	LCD_DisplayText(0, 0, "BJW");
	LCD_DisplayText(1, 0, ">EARTH");
	LCD_DisplayText(2, 0, ">MILLER");
	LCD_DisplayText(3, 0, " Int period");

	DisplayTimeUpdate();						// 시간 업데이트
	DisplayPeriodUpdate();						// MILLER 행성 주기 업데이트

	LCD_SetTextColor(RGB_BLACK);				// 글자색 : BLACK
	LCD_DisplayText(3, 15, "ms");
}

void _EXTI_Init(void)
{
	RCC->AHB1ENR |= 0x00000080;		// RCC_AHB1ENR : GPIOH(bit#7) Enable
	RCC->APB2ENR |= 0x00004000;		// Enable System Configuration Controller Clock

	GPIOH->MODER &= ~0xFFFF0000;	// GPIOH PIN8~PIN15 Input mode (reset state)

	SYSCFG->EXTICR[3] &= ~0xFF00;	// EXTI14, 15 : Falling Trigger Enable
	SYSCFG->EXTICR[3] |= 0x7700;	// EXTI14, 15 인터럽트 mask (Interrupt Enable) 설정

	EXTI->FTSR |= 0xC000;			// EXTI14, 15 : Falling Trigger Enable
	EXTI->IMR |= 0xC000;			// EXTI14, 15 인터럽트 mask (Interrupt Enable) 설정

	NVIC->ISER[1] = 1 << (40 - 32);	// Enable 'Global Interrupt EXTI14, 15'
}

void EXTI15_10_IRQHandler(void)
{
	if (EXTI->PR & 0x4000)			// EXTI14 Interrupt Pending(발생) 여부
	{
		EXTI->PR |= 0x4000;			// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
		SW6_flag++;					// SW6_flag: EXTI14이 발생되었음을 알리기 위해 만든 변수(main문의 mission에 사용)			
		SW6_cnt++;					// SW6_cnt: EXTI14 발생 횟수를 기록하기 위해 만든 변수(main문의 mission에 사용)
	}

	if (EXTI->PR & 0x8000)			// EXTI14 Interrupt Pending(발생) 여부
	{			
		EXTI->PR |= 0x8000;			// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
		SW7_flag++;					// SW6_flag: EXTI14이 발생되었음을 알리기 위해 만든 변수(main문의 mission에 사용)
	}
}

void TIMER4_OC_Init(void)
{
	// PB7: TIM4_CH2
	// PB7을 출력 설정하고 Alternate function(TIM4_CH2)으로 사용 선언
	RCC->AHB1ENR |= (1 << 1);

	GPIOB->MODER |= (2 << 14);			// (1 << 14), (MODER.(15,14) = 0b10), GPIOD PIN13 Output Alternate function mode
	GPIOB->OSPEEDR |= (3 << 14);		// (3 << 14), (OSPEEDER.(15,14) = 0b11), GPIOD PIN13 Output speed (100MHz High speed)
	GPIOB->OTYPER &= ~(1 << 7);			// ~(1 << 7), GPIOB PIN7 Output type push-pull (reset state)
	GPIOB->PUPDR |= (1 << 14);			// (1 << 14), GPIOB PIN14 Pull-up
	GPIOB->AFR[0] |= 0x20000000;		// (AFR[0]): Connect TIM4 pins(PB7) to AF2(TIM3..5)

	// Time base 설정
	RCC->APB1ENR |= (1 << 2);			// 0x04, RCC_APB1ENR TIMER4 Enable

	TIM4->CR1 &= ~(1 << 4);				// DIR=0(Up counter)(reset state)
	TIM4->CR1 &= ~(1 << 1);				// UDIS=0(Update event Enabled): By one of following events
	TIM4->CR1 &= ~(1 << 2);				// URS=0(Update Request Source  Selection): By one of following events
	TIM4->CR1 &= ~(1 << 3);				// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM4->CR1 |= (1 << 7);				// ARPE=1(ARR is buffered): ARR Preload Enalbe 
	TIM4->CR1 &= ~(3 << 8);				// CKD(Clock division)=00(reset state)
	TIM4->CR1 &= ~(3 << 5);				// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
										// Center-aligned mode: The counter counts UP and DOWN alternatively
	// Setting the Period
	TIM4->PSC = 8400 - 1;				// Prescaler = 8400, 84,000,000/8400 = 10KHz(0.1ms)
	TIM4->ARR = 1000 - 1;				// Auto reload: 0.1ms * 1000 = 100ms(period) : 인터럽트 주기나 출력신호의 주기 결정

	// Update(Clear) the Counter
	TIM4->EGR |= (1 << 0);				// UG: Update generation

	// Output Compare 설정
	// CCMR1(Capture/Compare Mode Register 1): Setting the MODE of Ch1 or Ch2
	TIM4->CCMR1 &= ~(3 << 8);			// CC2S(CC2 channel): Output 
	TIM4->CCMR1 &= ~(1 << 11);			// OC2FE=0: Output Compare 2 Fast disable 
	TIM4->CCMR1 &= ~(1 << 10);			// OC2PE=0: Output Compare 2 preload disable(CCR2에 언제든지 새로운 값을 loading 가능) 
	TIM4->CCMR1 |= (3 << 12);			// OC2M=0b011 (Output Compare 2 Mode : toggle)

	// CCER(Capture/Compare Enable Register) : Enable "Channel 2" 
	TIM4->CCER |= (1 << 4);				// CC2E=1: CC2 channel Output Enable
	TIM4->CCER &= ~(1 << 5);			// CC2P=0: CC2 channel Output Polarity (OCPolarity_High : OC2으로 반전없이 출력)  

	TIM4->CCR2 = 100;					// TIM4 CCR2 TIM4_Pulse

	TIM4->DIER |= (1 << 0);				// UIE: Enable Tim4 Update interrupt
	TIM4->DIER |= (1 << 2);				// CC2IE: Enable the Tim4 CC2 interrupt

	NVIC->ISER[0] |= (1 << 30);			// Enable Timer4 global Interrupt on NVIC

	TIM4->CR1 |= (1 << 0);				// CEN: Enable the Tim4 Counter
}

void TIM4_IRQHandler(void)      //RESET: 0
{
	if ((TIM4->SR & 0x01) != RESET)	// Update interrupt flag
	{
		TIM4->SR &= ~(1 << 0);	// Update Interrupt Clear
	}

	if ((TIM4->SR & 0x04) != RESET)	// Capture/Compare 2 interrupt flag
	{
		TIM4->SR &= ~(1 << 2);	// CC2 Interrupt Clear

		M_minute++;

		if (M_minute >= 60)
		{
			M_minute = 0;		// minute 초기화
			M_hour++;			// hour +1 증가
		}

		if (M_hour >= 24)
		{
			M_hour = 0;			// hour 초기화
		}
	}
}

void TIMER5_Init(void)
{
	RCC->APB1ENR |= 0x08;		// RCC_APB1ENR TIMER5 Enable

	TIM5->CR1 &= ~(1 << 4);		// DIR=0(Up counter)(reset state)
	TIM5->CR1 &= ~(1 << 1);		// UDIS=0(Update event Enabled): By one of following events
	TIM5->CR1 &= ~(1 << 2);		// URS=0(Update Request Source  Selection): By one of following events
	TIM5->CR1 &= ~(1 << 3);		// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM5->CR1 &= ~(1 << 7);		// ARPE=0(ARR is NOT buffered) (reset state)
	TIM5->CR1 &= ~(3 << 8); 	// CKD(Clock division)=00(reset state)
	TIM5->CR1 &= ~(3 << 5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)

	TIM5->PSC = 42 - 1;				// 84,000,000Hz/42 = 2,000,000Hz (0.5us) 32bit
	TIM5->ARR = 200000 - 1;			// Auto reload 0.5us * 200000 = 100ms

	TIM5->EGR |= (1 << 0);	// UG(Update generation)=1 
	// Re-initialize the counter(CNT=0) & generates an update of registers

	NVIC->ISER[1] |= (1 << (50 - 32)); 		// Enable Timer5 global Interrupt
	TIM5->DIER |= (1 << 0);					// Enable the Tim5 Update interrupt
	TIM5->CR1 |= (1 << 0);					// Enable the Tim5 Counter (clock enable)
}

void TIM5_IRQHandler(void)
{
	TIM5->SR &= ~(1 << 0);

	E_minute++;
	if (E_minute >= 60)	
	{
		E_minute = 0;		// minute 초기화
		E_hour++;			// hour +1 증가
	}

	if (E_hour >= 24)
	{
		E_hour = 0;			// hour 초기화
	}
}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i = 0; i < wMS; i++)
		DelayUS(1000);				// 1000us => 1ms
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
	key = GPIOH->IDR & 0xFF00;
	if (key == 0xFF00)
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
	else
	{
		if (key_flag != 0)
			return 0xFF00;
		else
		{
			DelayMS(10);
			key_flag = 1;
			return key;
		}
	}
}

void BEEP(void)					// Beep for 20 ms 
{
	GPIOF->ODR |= (1 << 9);		// PF9 'H' Buzzer on
	DelayMS(20);				// Delay 20 ms
	GPIOF->ODR &= ~(1 << 9);	// PF9 'L' Buzzer off
}

void DisplayTimeUpdate(void)
{
	LCD_SetTextColor(RGB_BLUE);						// 글자색: BLUE
	LCD_DisplayChar(1, 8, E_hour / 10 + '0');
	LCD_DisplayChar(1, 9, E_hour % 10 + '0');
	LCD_DisplayChar(1, 10, ':');
	LCD_DisplayChar(1, 11, E_minute / 10 + '0');
	LCD_DisplayChar(1, 12, E_minute % 10 + '0');
	
	LCD_SetTextColor(RGB_RED);						// 글자색: RED
	LCD_DisplayChar(2, 8, M_hour / 10 + '0');
	LCD_DisplayChar(2, 9, M_hour % 10 + '0');
	LCD_DisplayChar(2, 10, ':');
	LCD_DisplayChar(2, 11, M_minute / 10 + '0');
	LCD_DisplayChar(2, 12, M_minute % 10 + '0');
}

void DisplayPeriodUpdate(void)
{
	if (TIM4_period == 100)				// 100ms
	{
		LCD_SetTextColor(RGB_RED);
		LCD_DisplayText(3, 12, "100");
	}
	else if (TIM4_period == 200)		// 200ms
	{
		LCD_SetTextColor(RGB_RED);
		LCD_DisplayText(3, 12, "200");
	}
	else if (TIM4_period == 500)		// 500ms
	{
		LCD_SetTextColor(RGB_RED);
		LCD_DisplayText(3, 12, "500");
	}
}