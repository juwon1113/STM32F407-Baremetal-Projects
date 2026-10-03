/////////////////////////////////////////////////////////////
// 과제명: 엘리베이터 제어
// 과제개요: 빌딩(0~5층)에 설치된 엘리베이터에서, 목표층 스위치를 입력하여
//목표층까지 이동하도록 제어하는 프로그램 작성
// 사용한 하드웨어(기능): EXTI, GPIO, ...
// 제출일: 2025. 05. 30
// 이름: 백주원
///////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"

void _GPIO_Init(void);
void _EXTI_Init(void);

void DisplayInitScreen(void);
uint16_t KEY_Scan(void);
void BEEP(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint8_t Cur, Des;               // 현재층, 도착층
char Hold, Speed;				// Hold, Speed 확인 여부용 변수

int main(void)
{
	LCD_Init();		// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	// GPIO 초기화
	_EXTI_Init();	// EXTI 초기화

	DisplayInitScreen();	// LCD 초기화면

	Cur = Des = 0;
	Hold = 'X';
	Speed = 'H';

	GPIOG->ODR &= ~0x00FF;	// 초기값: LED0~7 Off
	GPIOG->ODR |= 0x0001;	// GPIOG->ODR.0 'H'(LED ON)

	RCC->CR |= (1 << 16);		// HSE ON
	RCC->CFGR &= ~0x0003;		// SYSCLK SW  Clear(HSI) : 
	RCC->CR &= ~0x05000000;		// PLL OFF

	RCC->PLLCFGR &= ~0x3F;		// 기존 PLL_M clear  
	RCC->PLLCFGR |= 8 | (336 << 6) | (((2 >> 1) - 1) << 16);
	// M=8, N=336, P=2  ---> 8M/8/2 * 336= 168Mhz
	// M=16  ---> 16M/8/2 * 336= 84MHz
	RCC->CR |= 0x05000000; // PLL ON

	RCC->CFGR &= ~0x0003;	// SYSCLK SW  Clear(HSI)
	RCC->CFGR |= 0x0002;	// SYSCLK SW : PLLCLK 

	while (1)
	{
		uint16_t key = KEY_Scan();
		if (key != 0xFF00)
		{
			switch (key)
			{
			case 0xFE00:            // SW0
				Des = 0;			// 도착 층 0
				break;
			case 0xFD00:            // SW1
				Des = 1;            // 도착 층 1
				break;
			case 0xFB00:            // SW2
				Des = 2;            // 도착 층 2
				break;
			case 0xF700:            // SW3
				Des = 3;            // 도착 층 3
				break;
			case 0xEF00:            // SW4
				Des = 4;            // 도착 층 4
				break;
			case 0xDF00:            // SW5
				Des = 5;            // 도착 층 5
				break;
			default:
				break;
			}

			BEEP();
			LCD_DisplayChar(2, 9, Des + '0');

			if (Des > Cur)								// 현재 층이 도착하는 층보다 낮으면
			{
				for (int i = Cur; i <= Des; i++) {
					GPIOG->BSRRH = 0xFF;				// LED 0~7 off 한다.
					GPIOG->BSRRL = (1 << i);			// LED Cur번에 on 한다.
					LCD_DisplayChar(1, 9, i + '0');		// 이동하는 층을 출력한다.
					DelayMS(1000);
				}
				Cur = Des;
			}
			else if (Des < Cur)							// 현재 층이 도착하는 층보다 높으면
			{
				for (int i = Cur; i >= Des; i--) {
					GPIOG->BSRRH = 0xFF;				// LED 0~7 off 한다.
					GPIOG->BSRRL = (1 << i);			// LED Cur번에 on 한다.
					LCD_DisplayChar(1, 9, i + '0');		// 이동하는 층을 출력한다.
					DelayMS(1000);
				}
				Cur = Des;
			}

			for (int i = 0; i < 3; i++) {
				BEEP();
				DelayMS(500);
			}
		}
	}
}

/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	LCD_Clear(RGB_YELLOW);							// 화면 클리어
	LCD_SetFont(&Gulim8);							// 폰트 : 굴림 8
	LCD_SetBackColor(RGB_YELLOW);					// 글자배경색 : Yellow
	LCD_SetTextColor(RGB_BLUE);						// 글자색 : Blue
	LCD_DisplayText(0, 0, "MC Elevator(BJW)");  	// Title
	LCD_SetTextColor(RGB_BLACK);					// 글자색 : Black
	LCD_DisplayText(1, 0, "Cur FL: ");				// 현재 층
	LCD_DisplayText(2, 0, "Des FL: ");				// 도착하는 층
	LCD_DisplayText(3, 0, "Hold: ");				// 멈춤 확인
	LCD_DisplayText(4, 0, "Speed: ");				// 속도 확인
	LCD_SetTextColor(RGB_RED);						// 글자색 : Red
	LCD_DisplayText(1, 9, "0");
	LCD_DisplayText(2, 9, "0");
	LCD_DisplayText(3, 6, "X");
	LCD_DisplayText(4, 7, "H");
}
/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) 초기 설정	*/
void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
	RCC->AHB1ENR |= 0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER &= 0xFFFF0000;	// GPIOG 0~7 : Clear (0b00)			
	GPIOG->MODER |= 0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER &= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
	GPIOG->OSPEEDR &= ~0x0000FFFF;	// GPIOG 0~7 : Clear (0b00)		 	
	GPIOG->OSPEEDR |= 0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 

	// SW (GPIO H) 설정 
	RCC->AHB1ENR |= 0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER &= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR &= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
	RCC->AHB1ENR |= 0x00000020; 	// RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER &= ~0x000C0000;	// GPIOF 9 : Clear (0b00)
	GPIOF->MODER |= 0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER &= ~0x0200;	// GPIOF 9 : Push-pull  	
	GPIOF->OSPEEDR &= ~0x000C0000;	// GPIOF 9 : Clear (0b00)
	GPIOF->OSPEEDR |= 0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI9(GPIOH.9, SW1)) 초기 설정  */
void _EXTI_Init(void)
{
	RCC->AHB1ENR |= 0x0080;		// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR |= 0x4000;		// Enable System Configuration Controller Clock

	GPIOH->MODER &= ~0xFFFF0000;	// GPIOH PIN8~PIN15 Input mode (reset state)				 

	SYSCFG->EXTICR[3] &= ~0xFF00;           // EXTI14,15 CLEAR
	SYSCFG->EXTICR[3] |= 0x7700;		// EXTI14,15에 대한 소스 입력은 GPIOH로 설정
	// EXTI14 <- PH14, EXTI15 <- PH15 
	// EXTICR4(EXTICR[3])를 이용 
	// reset value: 0x0000	

	EXTI->FTSR |= 0x0000C000;		// EXTI14,15: Falling Trigger Enable 
	EXTI->IMR |= 0x0000C000;		// EXTI14,15 인터럽트 mask (Interrupt Enable) 설정

	NVIC->ISER[1] |= (1 << (40 - 32));		// Enable 'Global Interrupt EXTI14, 15'
	// Vector table Position 참조
}

/* EXTI10~15 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)
{
	if (EXTI->PR & 0x4000) // EXTI14 - 속도 제어용 인터럽트
	{
		EXTI->PR |= 0x4000; // clear pending
		BEEP();
		if (Speed == 'H') {
			Speed = 'L';

			RCC->CR |= (1 << 16);		// HSE ON
			RCC->CFGR &= ~0x0003;		// SYSCLK SW  Clear(HSI) : 
			RCC->CR &= ~0x05000000;		// PLL OFF

			RCC->PLLCFGR &= ~0x3F;		// 기존 PLL_M clear  
			RCC->PLLCFGR |= 16 | (336 << 6) | (((2 >> 1) - 1) << 16);
			// M=8, N=336, P=2  ---> 8M/8/2 * 336 = 168Mhz
			// M=16  ---> 16M/8/2 * 336 = 84MHz
			RCC->CR |= 0x05000000;		// PLL ON

			RCC->CFGR &= ~0x0003;		// SYSCLK SW  Clear(HSI)
			RCC->CFGR |= 0x0002;		// SYSCLK SW : PLLCLK

			LCD_DisplayChar(4, 7, Speed);
			GPIOG->BSRRL = 0x40;        // LED 6 On
		}
		else {
			Speed = 'H';

			RCC->CR |= (1 << 16);		// HSE ON
			RCC->CFGR &= ~0x0003;		// SYSCLK SW  Clear(HSI) : 
			RCC->CR &= ~0x05000000;		// PLL OFF

			RCC->PLLCFGR &= ~0x3F;		// 기존 PLL_M clear  
			RCC->PLLCFGR |= 8 | (336 << 6) | (((2 >> 1) - 1) << 16);
			// M=8, N=336, P=2  ---> 8M/8/2 * 336= 168Mhz
			// M=16  ---> 16M/8/2 * 336= 84MHz
			RCC->CR |= 0x05000000;		// PLL ON

			RCC->CFGR &= ~0x0003;		// SYSCLK SW  Clear(HSI)
			RCC->CFGR |= 0x0002;		// SYSCLK SW : PLLCLK

			LCD_DisplayChar(4, 7, Speed);
			GPIOG->BSRRH = 0x40;        // LED 6 Off
		}
	}

	if (EXTI->PR & 0x8000) // EXTI15 - 멈추게 하는 인터럽트
	{
		EXTI->PR |= 0x8000;
		Hold = 'O';
		LCD_DisplayChar(3, 6, Hold);
		GPIOG->BSRRL = 0x80;			// LED 7 On
		BEEP();
		DelayMS(4000);					// 4초 정지
		GPIOG->BSRRH = 0x80;			// LED 7 Off
		for (int i = 0; i < 2; i++) {
			BEEP(); DelayMS(500);
		}
		Hold = 'X';
		LCD_DisplayChar(3, 6, Hold);
	}
}

/* Switch가 입력되었는지 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */
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

/* Buzzer: Beep for 30 ms */
void BEEP(void)
{
	GPIOF->ODR |= 0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);		// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i = 0; i < wMS; i++)
		DelayUS(1000);	// 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS * 17;
	for (; Dly; Dly--);
}
