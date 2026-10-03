///////////////////////////////////////////////////////////////
// 과제명: 이진연산기 제작
// 과제개요: 이진(2비트) 연산을 하는 계산기 프로그램 제작
// 사용한 하드웨어(기능): EXTI, GPIO, ...
// 제출일: 2025. 06. 07
// 제출자 클래스: 목요일반
// 이름: 백주원
///////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

// NO Joy-Button : 0x03E0 : 0000 0011 1110 0000 
//Bit No                            FEDC BA98 7654 3210
#define NAVI_PUSH		0x03C0				//PI5 0000 0011 1100 0000 
#define NAVI_UP		    0x03A0				//PI6 0000 0011 1010 0000 
#define NAVI_DOWN	    0x0360				//PI7 0000 0011 0110 0000 
#define NAVI_RIGHT	    0x02E0				//PI8 0000 0010 1110 0000 
#define NAVI_LEFT		0x01E0				//PI9 0000 0001 1110 0000 
#define RGB_PINK GET_RGB(255, 192, 203)

void _GPIO_Init(void);
void _EXTI_Init(void);

void DisplayInitScreen(void);
uint16_t KEY_Scan(void);
void BEEP(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);
void DrawYellowRectangle(uint8_t x, uint8_t y);
void DrawBlueHorLine(uint8_t x, uint8_t y, uint8_t d);
void DecToBin(void);

uint8_t AMSB, ALSB, BMSB, BLSB, Aopnd, Bopnd, flag;         // A의 MSB, A의 LSB, B의 MSB, B의 LSB, A 피연산자, B 피연산자, 연속 증가 모드에 사용될 플래그
int result;                                                                                                                                // 연산 결과를 저장하는 변수
uint8_t Result[4] = { 0, 0, 0, 0 };                                                                                              // 결과를 2진수로 저장하기 위한 배열
char Operator = '+', ResultSign = '+';                                                                              // 연산자, 결과에 대한 연산자

int main(void)
{
	LCD_Init();		// LCD 모듈 초기화
	DelayMS(10);
	_GPIO_Init();	                // GPIO 초기화
	_EXTI_Init();	                // EXTI 초기화

	Fram_Init();                      // FRAM 초기화 H/W 초기화
	Fram_Status_Config();     // FRAM 초기화 S/W 초기화

	DisplayInitScreen();	// LCD 초기화면

	Operator = Fram_Read(527);
	result = (int8_t)Fram_Read(528);                       // write할 때 uint8_t로 저장되므로 음수를 저장하기 위해서 read할 때는 int8_t로 형변환을 시켜야한다.

	DecToBin();                             // 10진수를 2진수로 변환하는 함수

	GPIOG->ODR &= ~0x00FF;	// 초기값: LED0~7 Off

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

	AMSB = ALSB = BMSB = BLSB = 0;                  // 변수를 0으로 초기화한다.

	while (1)
	{
		LCD_SetFont(&Gulim8);                           // 굴림8
		LCD_SetBackColor(RGB_PINK);              // 글자 배경색 : 핑크
		LCD_DisplayChar(4, 9, Operator);           // 연산자 출력
		LCD_SetFont(&Gulim7);                           // 굴림7
		LCD_SetBackColor(RGB_YELLOW);        // 글자 배경색 : 노랑

		switch (KEY_Scan())
		{
		case 0xFB00:	// SW2
			BEEP();
			if (AMSB == 0)                                                      // 0이면 1로 변경
			{
				AMSB = 1;
				LCD_DisplayChar(2, 6, AMSB + '0');
			}
			else if (AMSB == 1)                                             // 1이면 0으로 변경
			{
				AMSB = 0;
				LCD_DisplayChar(2, 6, AMSB + '0');
			}
			break;
		case 0xF700:	// SW3
			BEEP();
			if (ALSB == 0)                                                      // 0이면 1로 변경
			{
				ALSB = 1;
				LCD_DisplayChar(4, 6, ALSB + '0');
			}
			else if (ALSB == 1)                                             // 1이면 0으로 변경
			{
				ALSB = 0;
				LCD_DisplayChar(4, 6, ALSB + '0');
			}
			break;
		case 0xEF00:	// SW4
			BEEP();
			if (BMSB == 0)                                                      // 0이면 1로 변경
			{
				BMSB = 1;
				LCD_DisplayChar(7, 6, BMSB + '0');
			}
			else if (BMSB == 1)                                             // 1이면 0으로 변경
			{
				BMSB = 0;
				LCD_DisplayChar(7, 6, BMSB + '0');
			}
			break;
		case 0xDF00:	// SW5
			BEEP();
			if (BLSB == 0)                                                      // 0이면 1로 변경
			{
				BLSB = 1;
				LCD_DisplayChar(9, 6, BLSB + '0');
			}
			else if (BLSB == 1)                                             // 1이면 0으로 변경
			{
				BLSB = 0;
				LCD_DisplayChar(9, 6, BLSB + '0');
			}
			break;
		}

		Aopnd = AMSB * 2 + ALSB * 1;                                    // Aopnd를 10진수로 저장한다.
		Bopnd = BMSB * 2 + BLSB * 1;                                   // Bopnd를 10진수로 저장한다.
		Fram_Write(527, Operator);   // FRAM(0~8191) 527번지에 0 저장 
		DecToBin();                                                                // 10진수를 2진수로 변환하는 함수
	}
}

/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
	// 가운데  큰 사각형 테두리 :  초록색, 배경색 : 흰색
	LCD_Clear(RGB_WHITE);							// 화면 클리어
	LCD_SetFont(&Gulim10);							// 폰트 : 굴림 8
	LCD_SetPenColor(RGB_GREEN);
	LCD_DrawRectangle(55, 5, 45, 120);

	// A 사각형 테두리 : 초록색, 배경색 : 흰색
	LCD_DrawRectangle(9, 15, 20, 20);
	LCD_SetBackColor(RGB_WHITE);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(1, 2, 'A');

	// B 사각형 테두리 : 초록색, 배경색 : 흰색
	LCD_DrawRectangle(9, 67, 20, 20);
	LCD_SetBackColor(RGB_WHITE);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(4, 2, 'B');

	// R 사각형 테두리 : 초록색, 배경색 : 흰색
	LCD_DrawRectangle(130, 32, 20, 20);
	LCD_SetBackColor(RGB_WHITE);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(2, 17, 'R');

	// A MSB 테두리 : 검정색, 배경색 : 노란색
	LCD_SetFont(&Gulim7);                                                   // 폰트 : 굴림7
	DrawYellowRectangle(32, 20);
	LCD_SetBackColor(RGB_YELLOW);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(2, 6, '0');
	DrawBlueHorLine(32, 20, 0);

	// A LSB 테두리 : 검정색, 배경색 : 노란색
	DrawYellowRectangle(32, 40);
	LCD_SetBackColor(RGB_YELLOW);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(4, 6, '0');
	DrawBlueHorLine(32, 40, 0);

	// B MSB 테두리 : 검정색, 배경색 : 노란색
	DrawYellowRectangle(32, 75);
	LCD_SetBackColor(RGB_YELLOW);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(7, 6, '0');
	DrawBlueHorLine(32, 75, 0);

	// B LSB 테두리 : 검정색, 배경색 : 노란색
	DrawYellowRectangle(32, 95);
	LCD_SetBackColor(RGB_YELLOW);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(9, 6, '0');
	DrawBlueHorLine(32, 95, 0);

	// 연산자를 저장하는 사각형 테두리 : 검정색, 배경색 : 핑크색
	LCD_SetFont(&Gulim8);                           // 굴림 8
	LCD_SetBrushColor(RGB_PINK);
	LCD_DrawFillRect(67, 50, 17, 17);
	LCD_SetPenColor(RGB_BLACK);
	LCD_DrawRectangle(67, 50, 17, 17);
	LCD_SetBackColor(RGB_PINK);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(4, 9, Operator);

	// 결과 부호를 저장하는 사각형 테두리 : 검정색, 배경색 : 노란색
	LCD_SetFont(&Gulim7);                           // 굴림 7
	LCD_SetBackColor(RGB_YELLOW);
	DrawYellowRectangle(110, 9);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(1, 19, ResultSign);
	DrawBlueHorLine(110, 10, 1);

	// 결과를 저장하는 사각형 테두리 : 검정색, 배경색 : 노란색
	DrawYellowRectangle(110, 30);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(3, 19, Result[0] + '0');
	DrawBlueHorLine(110, 30, 1);

	DrawYellowRectangle(110, 51);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(5, 19, Result[1] + '0');
	DrawBlueHorLine(110, 51, 1);

	DrawYellowRectangle(110, 73);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(7, 19, Result[2] + '0');
	DrawBlueHorLine(110, 73, 1);

	DrawYellowRectangle(110, 95);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayChar(9, 19, Result[3] + '0');
	DrawBlueHorLine(110, 95, 1);

	LCD_SetBrushColor(RGB_YELLOW);
	LCD_DrawFillRect(67, 97, 20, 15);
	LCD_SetPenColor(RGB_BLACK);
	LCD_DrawRectangle(67, 97, 20, 15);
	LCD_SetBackColor(RGB_YELLOW);
	LCD_SetTextColor(RGB_BLACK);
	LCD_DisplayText(9, 12, "+0");
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

	//Joy Stick SW(PORT I) 설정
	RCC->AHB1ENR |= 0x00000100;	// RCC_AHB1ENR GPIOI Enable
	GPIOI->MODER &= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)
	GPIOI->PUPDR &= ~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI9(GPIOH.9, SW1)) 초기 설정  */
void _EXTI_Init(void)
{
	// SW (GPIO H) EXTI 설정
	RCC->AHB1ENR |= 0x0080;		// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR |= 0x4000;		// Enable System Configuration Controller Clock

	GPIOH->MODER &= ~0xFFFF0000;	// GPIOH PIN8~PIN15 Input mode (reset state)				 

	SYSCFG->EXTICR[2] &= ~0x00F0;           // EXTI9 CLEAR
	SYSCFG->EXTICR[2] |= 0x0070;             //  EXTI9에 대한 소스 입력은 GPIOH로 설정

	SYSCFG->EXTICR[3] &= ~0x0F00;          // EXTI14 CLEAR
	SYSCFG->EXTICR[3] |= 0x0700;	      // EXTI14에 대한 소스 입력은 GPIOH로 설정
	// EXTI9 <- PH9, EXTI14 <- PH14 
	// EXTICR3(EXTICR[2]), EXTICR4(EXTICR[3])를 이용 
	// reset value: 0x0000	

	EXTI->FTSR |= 0x00004200;		// EXTI9,14: Falling Trigger Enable 
	EXTI->IMR |= 0x00004200;		// EXTI9,14 인터럽트 mask (Interrupt Enable) 설정

	NVIC->ISER[0] |= (1 << 23);                     // Enable 'Global Interrupt EXTI9'
	NVIC->ISER[1] |= (1 << (40 - 32));		// Enable 'Global Interrupt EXTI14'
	// Vector table Position 참조

		// Joy Stick SW(PORT I) EXTI 설정
	RCC->AHB1ENR |= 0x0100;
	RCC->APB2ENR |= 0x4000;

	GPIOI->MODER &= ~0x00018000;

	SYSCFG->EXTICR[1] &= ~0xF000;
	SYSCFG->EXTICR[1] |= 0x8000;

	SYSCFG->EXTICR[2] &= ~0x000F;
	SYSCFG->EXTICR[2] |= 0x0008;

	EXTI->FTSR |= 0x00000180;
	EXTI->IMR |= 0x00000180;

	NVIC->IP[23] = 0xF0;  	// High Priority 
	NVIC->IP[40] = 0xE0;  	// Low Priority 
}

/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{
	if (EXTI->PR & 0x0080) // EXTI7  연속 증가 모드  JoyStick Down
	{
		BEEP();
		EXTI->PR |= 0x0080;
		flag = 1;                 // 플래그를 1로 한다.

		if (flag)                 // 플래그가 1이면
		{
			while (flag)      // 플래그가 1일 때 반복
			{
				LCD_SetFont(&Gulim7);
				LCD_SetBackColor(RGB_YELLOW);
				// +1로 화면에 출력한다.
				LCD_DisplayText(9, 12, "+1");
				result++;
				DecToBin();
				// LED 7이 켜진다.
				GPIOG->BSRRL = 0x01 << 7;
				DelayMS(500);
				Fram_Write(528, result);
				if (!flag)                // EXTI14번이 발동하면 플래그 0으로 변환
				{
					BEEP();                         // 1초 딜레이
					DelayMS(1000);
					// 다시 +0으로 화면에 출력한다.
					LCD_DisplayText(9, 12, "+0");
					// 세 번 부조 울린다. (0.5초 딜레이)
					BEEP();
					DelayMS(500);
					BEEP();
					DelayMS(500);
					BEEP();
					// LED 7이 꺼진다.
					GPIOG->BSRRH = 0x01 << 7;
				}
			}
		}
	}

	if (EXTI->PR & 0x0100) // EXTI8 - 연산자를 정하는 인터럽트 JoyStick Right
	{
		EXTI->PR |= 0x0100;
		BEEP();

		// 읽어온 operator를 올바르게 다음으로 이동시키기 위한 조건문
		if (Operator == '+')                        // '+'이면 '-'
			Operator = '-';
		else if (Operator == '-')                   // '-'이면 'x'
			Operator = 'x';
		else if (Operator == 'x')                   // 'x'이면 '&'
			Operator = '&';
		else if (Operator == '&')                   // '&'이면 '|'
			Operator = '|';
		else if (Operator == '|')                   // '|'이면 '^'
			Operator = '^';
		else if (Operator == '^')                   // '^'이면 '+'
			Operator = '+';

		Fram_Write(527, Operator);   // FRAM(0~8191) 527번지에 0 저장 
	}

	if (EXTI->PR & 0x0200) // EXTI9 - 계산하는 인터럽트 SW1
	{
		EXTI->PR |= 0x0200;
		BEEP();

		if (Operator == '+')
		{
			result = Aopnd + Bopnd;
			ResultSign = '+';
		}
		else if (Operator == '-')
		{
			result = Aopnd - Bopnd;
			if (result >= 0)
			{
				ResultSign = '+';
			}
			else
			{
				ResultSign = '-';
			}
		}
		else if (Operator == 'x')
		{
			result = Aopnd * Bopnd;
			ResultSign = '+';
		}
		else if (Operator == '&')
		{
			result = Aopnd & Bopnd;
			ResultSign = '+';
		}
		else if (Operator == '|')
		{
			result = Aopnd | Bopnd;
			ResultSign = '+';
		}
		else if (Operator == '^')
		{
			result = Aopnd ^ Bopnd;
			ResultSign = '+';
		}

		DecToBin();
	}
}

/* EXTI10~15 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)
{
	if (EXTI->PR & 0x4000) // EXTI14 탈출하는 인터럽트 SW6
	{
		EXTI->PR |= 0x4000;
		flag = 0;
	}
}

/* Joystick switch가 입력되었는지 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */
uint8_t joy_flag = 0;
uint16_t JOY_Scan(void)	// input joy stick NAVI_* 
{
	uint16_t key;
	key = GPIOI->IDR & 0x03E0;	// any key pressed ?
	if (key == 0x03E0)		// if no key, check key off
	{
		if (joy_flag == 0)
			return key;
		else
		{
			DelayMS(10);
			joy_flag = 0;
			return key;
		}
	}
	else				// if key input, check continuous key
	{
		if (joy_flag != 0)	// if continuous key, treat as no key input
			return 0x03E0;
		else			// if new key,delay for debounce
		{
			joy_flag = 1;
			DelayMS(10);
			return key;
		}
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

void DrawYellowRectangle(uint8_t x, uint8_t y)                  // 노란색 사각형을 만드는 함수
{
	LCD_SetBrushColor(RGB_YELLOW);
	LCD_DrawFillRect(x, y, 15, 15);
	LCD_SetPenColor(RGB_BLACK);
	LCD_DrawRectangle(x, y, 15, 15);
}

void DrawBlueHorLine(uint8_t x, uint8_t y, uint8_t d)           // 파란선을 그리는 함수
{
	LCD_SetPenColor(RGB_BLUE);
	if (d == 0)
		LCD_DrawHorLine(15 + x, 7 + y, 8);
	else if (d == 1)
		LCD_DrawHorLine(x - 9, 7 + y, 10);
}

void DecToBin(void)                                                             // 10진수를 2진수로 변환하는 함수
{
	uint8_t x;
	if (result >= 0)                                                                // 결과가 양수이면 '+'
	{
		ResultSign = '+';
		x = result;
	}
	else if (result < 0)                                                         // 결과가 음수이면 '-'
	{
		ResultSign = '-';
		x = -result;
	}

	// 2진수를 저장한다.
	Result[3] = x % 2;
	x /= 2;
	Result[2] = x % 2;
	x /= 2;
	Result[1] = x % 2;
	x /= 2;
	Result[0] = x % 2;

	// 결과를 출력
	LCD_DisplayChar(1, 19, ResultSign);
	LCD_DisplayChar(3, 19, Result[0] + '0');
	LCD_DisplayChar(5, 19, Result[1] + '0');
	LCD_DisplayChar(7, 19, Result[2] + '0');
	LCD_DisplayChar(9, 19, Result[3] + '0');

	Fram_Write(528, result);        // FRAM(0~8191) 528번지에 0 저장 
}