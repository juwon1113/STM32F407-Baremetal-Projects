#include "stm32f4xx.h"
#include "GLCD.h"
#include "ACC.h"

void DisplayInit(void);				// 초기 화면 설정
void _EXTI_Init(void);				// EXTI12 초기화
void SPI1_Init(void);				// SPI1 초기화
void TIMER6_Init(void);				// TIMER6 Counter 초기화
void TIMER11_OC_Init(void);			// TIMER11 OC mode 초기화

void DelayMS(unsigned short wMS);	// MS
void DelayUS(unsigned short wUS);	// US

void SPI1_Process(int16* pBuf);		// ACC.c (ACC.h) 
void ACC_Init(void);				// ACC.c (ACC.h)
void LCD_Init(void);				// GLCD.c (GLCD.h)

void Update_Axis(int16* pBuf);		// 가속도 센서 읽어오는 함수
void Erase_Ball(void);				// Golf Ball 지우는 함수
void Draw_Ball(void);				// Golf Ball 그리는 함수
void Draw_Hole(void);				// Hole 그리는 함수
void Move_Ball(void);				// Golf Ball 움직이는 함수

char str[20], ch;					// sprintf를 사용하기 위한 문자열, 가속도 부호를 저장할 문자
double Ax, Ay, Az;					// x, y, z 가속도를 저장할 변수
unsigned char s100m = '0', s1 = '0', s10 = '0';		// Timer에 사용할 변수
uint8_t SW4_flag, bControl, move_flag, hole_flag;	// EXTI12에 사용할 변수, 가속도 센서 측정하기 위한 변수, Move_Ball에 쓸 flag 변수, 홀인원에 쓸 flag 변수
uint8_t x = 52, y = 60, MS;							// Golf Ball 좌표에 사용할 변수, Delay를 사용할 변수

int main(void)
{
	int16 buffer[3];

	LCD_Init();			// LCD 구동 함수
	DelayMS(10);		// LCD구동 딜레이
	DisplayInit();		// LCD 초기화면구동 함수
	DelayMS(10);		// LCD구동 딜레이
	
	_EXTI_Init();
	SPI1_Init();        // SPI1 초기화
	ACC_Init();			// 가속도센서 초기화
	TIMER6_Init();
	TIMER11_OC_Init();

	while (1)
	{
		if (bControl)
		{
			bControl = FALSE;
			SPI1_Process(&buffer[0]);	// SPI통신을 이용하여 가속도센서 측정
			Update_Axis(&buffer[0]);	// 측정값을 LCD에 표시
		}

		Move_Ball();					// 볼 굴리기
	}
}

void DisplayInit(void)
{
	LCD_Clear(RGB_WHITE);			// 화면 클리어
	LCD_SetFont(&Gulim7);			// 글자 크기: 7

	LCD_SetBackColor(RGB_WHITE);    // 글자배경색: WHITE
	LCD_SetTextColor(RGB_BLACK);    // 글자색: BLACK

	LCD_DisplayText(0, 0, "Golf game: BJW");  // Title
	LCD_DisplayText(2, 19, "Ax:");		// Ax
	LCD_DisplayText(3, 19, "Ay:");		// Ay
	LCD_DisplayText(4, 19, "Az:");		// Az
	LCD_DisplayText(7, 19, "T: ");		// Timer

	// Golf 경기장
	LCD_SetPenColor(RGB_BLUE);			// 펜색: BLUE
	LCD_DrawRectangle(1, 15, 108, 96);

	LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
	LCD_SetBrushColor(RGB_YELLOW);		// 브러쉬색: YELLOW
	LCD_SetBackColor(RGB_YELLOW);		// 글자배경색: YELLOW

	// Golf Hole
	// 1번 홀
	LCD_DrawFillRect(50 + 1, 68 + 15, 9, 9);
	LCD_DrawChar(51 + 1, 68 + 15, '1');
	LCD_DrawRectangle(49 + 1, 67 + 15, 10, 10);

	// 2번 홀
	LCD_DrawFillRect(77 + 1, 44 + 15, 9, 9);
	LCD_DrawChar(78 + 1, 44 + 15, '2');
	LCD_DrawRectangle(76 + 1, 43 + 15, 10, 10);

	// 3번 홀
	LCD_DrawFillRect(23 + 1, 20 + 15, 9, 9);
	LCD_DrawChar(24 + 1, 20 + 15, '3');
	LCD_DrawRectangle(22 + 1, 19 + 15, 10, 10);

	LCD_SetBackColor(RGB_WHITE);    // 글자배경색: WHITE
	LCD_SetTextColor(RGB_RED);		// 글자색: RED

	LCD_DisplayText(2, 23, "0.0");		// Ax 초기값
	LCD_DisplayText(3, 23, "0.0");		// Ay 초기값
	LCD_DisplayText(4, 23, "0.0");		// Az 초기값
	LCD_DisplayText(7, 22, "00:0");		// Timer 초기값

	// Golf Ball
	LCD_SetPenColor(RGB_RED);					// 펜색: RED
	LCD_DrawRectangle(x, y, 6, 6);
}

void SPI1_Init(void)
{
	/*!< Clock Enable  *********************************************************/
	RCC->APB2ENR |= (1 << 12);	// 0x1000, SPI1 Clock EN
	RCC->AHB1ENR |= (1 << 0);	// 0x0001, GPIOA Clock EN		

	/*!< SPI1 pins configuration ************************************************/
	/*!< SPI1 SCK pin(PA5) configuration : SPI1_SCK */
	GPIOA->MODER |= (2 << (2 * 5)); 	// 0x00000800, PA5 Alternate function mode
	GPIOA->OTYPER &= ~(1 << 5); 		// 0020, PA5 Output type push-pull (reset state)
	GPIOA->OSPEEDR |= (3 << (2 * 5));	// 0x00000C00, PA5 Output speed (100MHz)
	GPIOA->PUPDR |= (2 << (2 * 5)); 	// 0x00000800, PA5 Pull-down
	GPIOA->AFR[0] |= (5 << (4 * 5));	// 0x00500000, Connect PA5 to AF5(SPI1)

	/*!< SPI1 MOSI pin(PA7) configuration : SPI1_MOSI */
	GPIOA->MODER |= (2 << (2 * 7));	// 0x00008000, PA7 Alternate function mode
	GPIOA->OTYPER &= ~(1 << 7);	// 0x0080, PA7 Output type push-pull (reset state)
	GPIOA->OSPEEDR |= (3 << (2 * 7));	// 0x0000C000, PA7 Output speed (100MHz)
	GPIOA->PUPDR |= (2 << (2 * 7)); 	// 0x00008000, PA7 Pull-down
	GPIOA->AFR[0] |= (5 << (4 * 7));	// 0x50000000, Connect PA7 to AF5(SPI1)

	/*!< SPI1 MISO pin(PA6) configuration : SPI1_MISO */
	GPIOA->MODER |= (2 << (2 * 6));	// 0x00002000, PA6 Alternate function mode
	GPIOA->OTYPER &= ~(1 << 6);	// 0x0040, PA6 Output type push-pull (reset state)
	GPIOA->OSPEEDR |= (3 << (2 * 6));	// 0x00003000, PA6 Output speed (100MHz)
	GPIOA->PUPDR |= (2 << (2 * 6));	// 0x00002000, PA6 Pull-down
	GPIOA->AFR[0] |= (5 << (4 * 6));	// 0x05000000, Connect PA6 to AF5(SPI1)

	/*!< SPI1 NSS pin(PA8) configuration : GPIO 핀  */
	GPIOA->MODER |= (1 << (2 * 8));		// 0x00010000, PA8 Output mode
	GPIOA->OTYPER &= ~(1 << 8); 		// 0x0100, push-pull(reset state)
	GPIOA->OSPEEDR |= (3 << (2 * 8));	// 0x00030000, PA8 Output speed (100MHZ) 
	GPIOA->PUPDR &= ~(3 << (2 * 8));	// 0x00030000, NO Pullup Pulldown(reset state)

	// Init SPI1 Registers 
	SPI1->CR1 |= (1 << 2);	// MSTR(Master selection)=1, Master mode
	SPI1->CR1 &= ~(1 << 15);	// SPI_Direction_2 Lines_FullDuplex
	SPI1->CR1 &= ~(1 << 11);	// SPI_DataSize_8bit
	SPI1->CR1 |= (1 << 9);  	// SSM(Software slave management)=1, 
	// NSS 핀 상태가 코딩에 의해 결정
	SPI1->CR1 |= (1 << 8);	// SSI(Internal_slave_select)=1,
	// 현재 MCU가 Master이므로 NSS 상태는 'High' 
	SPI1->CR1 &= ~(1 << 7);	// LSBFirst=0, MSB transmitted first    
	SPI1->CR1 |= (4 << 3);	// BR(BaudRate)=0b100, fPCLK/32 (84MHz/32 = 2.625MHz)
	SPI1->CR1 |= (1 << 1);	// CPOL(Clock polarity)=1, CK is 'High' when idle
	SPI1->CR1 |= (1 << 0);	// CPHA(Clock phase)=1, 두 번째 edge 에서 데이터가 샘플링

	SPI1->CR1 |= (1 << 6);	// SPE=1, SPI1 Enable 
}

void TIMER11_OC_Init(void)
{
	// Time base 설정
	RCC->APB2ENR |= (1 << 18);	// 0x04, RCC_APB1ENR TIMER11 Enable

	// Setting CR1 : 0x0000 
	TIM11->CR1 &= ~(1 << 4);	// DIR=0(Up counter)(reset state)
	TIM11->CR1 &= ~(1 << 1);	// UDIS=0(Update event Enabled): By one of following events
	TIM11->CR1 &= ~(1 << 2);	// URS=0(Update Request Source  Selection): By one of following events
	TIM11->CR1 &= ~(1 << 3);	// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM11->CR1 &= ~(1 << 7);		// ARPE=1(ARR is buffered): ARR Preload Enalbe 
	TIM11->CR1 &= ~(3 << 8); 	// CKD(Clock division)=00(reset state)
	TIM11->CR1 &= ~(3 << 5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)

	// Setting the Period
	TIM11->PSC = 8400 - 1;		// Prescaler=8400, 84MHz/8400 = 10KHz (0.1ms)
	TIM11->ARR = 2000 - 1;		// Auto reload  : 0.1ms * 2K = 200ms(period) : 인터럽트주기나 출력신호의 주기 결정

	// Update(Clear) the Counter
	TIM11->EGR |= (1 << 0);	// UG: Update generation    

	// Output Compare 설정
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM11->CCMR1 &= ~(3 << 0);	// CC1S(CC1 channel) = '0b00' : Output  
	TIM11->CCMR1 |= (3 << 4);	// OC1M=0b011 (Output Compare 1 Mode : toggle)
	// OC1REF toggles when CNT = CCR1

	// CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
	TIM11->CCER |= (1 << 0);	// CC1E=1: CC1 channel Output Enable
	TIM11->CCER &= ~(1 << 1);	// CC1P=0: CC1 channel Output Polarity (OCPolarity_High : OC1으로 반전없이 출력)  

	// CC1I(CC 인터럽트) 인터럽트 발생시각 또는 신호변화(토글)시기 결정: 신호의 위상(phase) 결정
	TIM11->CCR1 = 1000;	// Tim11 CCR1 TIM4_Pulse

	TIM11->DIER |= (1 << 1);	// CC1IE: Enable the Tim11 CC1 interrupt

	NVIC->ISER[0] |= (1 << 26);	// Enable Tim11 global Interrupt on NVIC

	// TIM11->CR1 |= (1 << 0);	// CEN: Enable the Tim11 Counter  					
}

void TIM1_TRG_COM_TIM11_IRQHandler(void)      //RESET: 0
{
	if ((TIM11->SR & 0x02) != RESET)	// Capture/Compare 1 interrupt flag
	{
		TIM11->SR &= ~(1 << 1);	// CC 1 Interrupt Claer
		bControl = TRUE;		// 200ms마다 센서 측정
	}
}

void TIMER6_Init(void)
{
	// Enable Timer CLK 
	RCC->APB1ENR |= (1 << 4);	// RCC_APB1ENR TIMER6 Enable

	TIM3->CR1 &= ~(1<<4);		// DIR=0(Up counter)(reset state)

	TIM6->CR1 &= ~(1 << 1);		// UDIS=0(Update event Enabled): By one of following events,
	TIM6->CR1 &= ~(1 << 2);		// URS=0(Update Request Source  Selection): By one of following events
	TIM6->CR1 &= ~(1 << 3);		// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM6->CR1 &= ~(1 << 7);		// ARPE=0(ARR is NOT buffered) (reset state)
	TIM6->CR1 &= ~(3 << 8); 	// CKD(Clock division)=00(reset state)
	TIM6->CR1 &= ~(3 << 5); 	// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)

	// Deciding the Period
	TIM6->PSC = 8400 - 1;	    // Prescaler 84,000,000Hz/8400 = 10000 Hz (0.1ms)  (1~65536)
	TIM6->ARR = 1000 - 1;		// Auto reload  0.1ms * 1000 = 0.1s

	// Clear the Counter
	TIM6->EGR |= (1 << 0);		// UG(Update generation)=1 

	// Setting an UI(UEV) Interrupt 
	NVIC->ISER[1] |= (1 << (54 - 32)); 	// Enable Tim6 global Interrupt
	TIM6->DIER |= (1 << 0);				// Enable the Tim6 Update interrupt

	// TIM6->CR1 |= (1 << 0);			// Enable the Tim6 Counter (clock enable)   
}

void TIM6_DAC_IRQHandler(void)
{
	TIM6->SR &= ~(1 << 0);	// Interrupt flag Clear

	if(SW4_flag)			// SW4를 누르면 카운터를 시작
	{
		s100m++;
	}

	if (s100m > '9')		// s100m이 '9'보다 커지면 s1 증가시킨다.
	{
		s1++;
		s100m = '0';
	}

	if (s1 > '9')			// s1이 '9'보다 커지면 s10 증가시킨다.
	{
		s10++;
		s1 = '0';
	}

	if (s10 > '5')			// s10이 '5'보다 커지면 00:0으로 초기화한다.
	{
		s100m = '0';
		s1 = '0';
		s10 = '0';
	}

	// 타이머 출력
	LCD_DisplayChar(7, 22, s10);
	LCD_DisplayChar(7, 23, s1);
	LCD_DisplayChar(7, 25, s100m);
}

void _EXTI_Init(void)
{
	RCC->AHB1ENR |= 0x00000080;	// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR |= 0x00004000;	// Enable System Configuration Controller Clock

	GPIOH->MODER &= ~0xFFFF0000;	// GPIOH PIN8~PIN15 Input mode (reset state)				 

	SYSCFG->EXTICR[3] &= ~0x00FF; 	// EXTICR[3] reset
	SYSCFG->EXTICR[3] |= 0x0007; 	// EXTI12에 대한 소스 입력은 GPIOH로 설정 (EXTICR3) 	

	EXTI->RTSR |= (1 << 12);		// Rising Trigger  Enable  (EXTI12:PH12) 
	EXTI->IMR |= (1 << 12);  		// EXTI12 인터럽트 mask (Interrupt Enable)

	NVIC->ISER[1] |= (1 << (40 - 32));   // Enable Interrupt EXTI12 Vector table Position 참조
}

void EXTI15_10_IRQHandler(void)
{
	if (EXTI->PR & (1 << 12))	// EXTI8 Interrupt Pending(발생) 여부?
	{
		EXTI->PR |= (1 << 12);	// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
		DisplayInit();
		Erase_Ball();
		s100m = '0';
		s1 = '0';
		s10 = '0';
		TIM6->CR1 |= (1 << 0);		// CEN: Enable the Tim6 Counter
		TIM11->CR1 |= (1 << 0);		// CEN: Enable the Tim11 Counter
		x = 52;
		y = 60;
		SW4_flag = 1;
		move_flag = 1;
		hole_flag = 0;
	}
}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;

	for (i = 0; i < wMS; i++)
		DelayUS(1000);		//1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS * 17;
	for (; Dly; Dly--);
}

void Update_Axis(int16* pBuf)
{
	UINT16 G_VALUE;
	// X 축 가속도 표시		
	if (pBuf[0] < 0)  //음수
	{
		G_VALUE = abs(pBuf[0]);
		ch = '-';
		LCD_DisplayChar(2, 22, ch); // g 부호 표시
	}
	else			// 양수
	{
		G_VALUE = pBuf[0];
		ch = '+';
		LCD_DisplayChar(2, 22, ch); // g 부호 표시
	}

	G_VALUE = 100 * G_VALUE / 0x4009; // 가속도 --> g 변환
	if (ch == '-') Ax = -(G_VALUE / 100 * 10 + G_VALUE % 100 / 10);
	else Ax = G_VALUE / 100 * 10 + G_VALUE % 100 / 10;
	LCD_DisplayChar(2, 23, G_VALUE / 100 + 0x30);
	LCD_DisplayChar(2, 24, '.');
	LCD_DisplayChar(2, 25, G_VALUE % 100 / 10 + 0x30);

	// Y 축 가속도 표시	
	if (pBuf[1] < 0)  //음수
	{
		G_VALUE = abs(pBuf[1]);
		ch = '-';
		LCD_DisplayChar(3, 22, ch); // g 부호 표시
	}
	else			// 양수
	{
		G_VALUE = pBuf[1];
		ch = '+';
		LCD_DisplayChar(3, 22, ch); // g 부호 표시
	}

	G_VALUE = 100 * G_VALUE / 0x4009;
	if (ch == '-') Ay = -(G_VALUE / 100 * 10 + G_VALUE % 100 / 10);
	else Ay = G_VALUE / 100 * 10 + G_VALUE % 100 / 10;
	LCD_DisplayChar(3, 23, G_VALUE / 100 + 0x30);
	LCD_DisplayChar(3, 24, '.');
	LCD_DisplayChar(3, 25, G_VALUE % 100 / 10 + 0x30);

	// Z 축 가속도 표시	
	if (pBuf[2] < 0)  //음수
	{
		G_VALUE = abs(pBuf[2]);
		ch = '-';
		LCD_DisplayChar(4, 22, ch); // g 부호 표시
	}
	else				// 양수
	{
		G_VALUE = pBuf[2];
		ch = '+';
		LCD_DisplayChar(4, 22, ch); // g 부호 표시
	}

	G_VALUE = 100 * G_VALUE / 0x2AB3;
	if (ch == '-') Az = -(G_VALUE / 100 * 10 + G_VALUE % 100 / 10);
	else Az = G_VALUE / 100 * 10 + G_VALUE % 100 / 10;
	LCD_DisplayChar(4, 23, G_VALUE / 100 + 0x30);
	LCD_DisplayChar(4, 24, '.');
	LCD_DisplayChar(4, 25, G_VALUE % 100 / 10 + 0x30);
}

void Erase_Ball(void)
{
	LCD_SetPenColor(RGB_WHITE);			// 펜색: WHITE
	LCD_DrawRectangle(x, y, 6, 6);
}

void Draw_Ball(void)
{
	LCD_SetPenColor(RGB_RED);			// 펜색: RED
	LCD_DrawRectangle(x, y, 6, 6);
}

void Draw_Hole(void)
{
	if (x >= 52 && x <= 57 && y >= 84 && y <= 89)					// 홀인원을 하면 hole_flag를 1 대입하고 파란색으로 변경한다.
	{
		if (hole_flag == 0)
		{
			hole_flag = 1;
			LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
			LCD_SetTextColor(RGB_BLACK);		// 글자색: BLACK
			LCD_SetBrushColor(RGB_BLUE);		// 브러쉬색: BLUE
			LCD_SetBackColor(RGB_BLUE);			// 글자배경색: BLUE

			LCD_DrawFillRect(50 + 1, 68 + 15, 9, 9);
			LCD_DrawChar(51 + 1, 68 + 15, '1');
			LCD_DrawRectangle(49 + 1, 67 + 15, 10, 10);
		}
	}
	else if (x >= 44 && x <= 65 && y >= 76 && y <= 97)				// Golf Ball이 움직이면서 홀을 지우면 다시 채워주기 위한 과정
	{
		if (hole_flag <= 0)											// 홀인원이 안됐다면 노란색으로 채움
		{
			LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
			LCD_SetTextColor(RGB_BLACK);		// 글자색: BLACK
			LCD_SetBrushColor(RGB_YELLOW);		// 브러쉬색: YELLOW
			LCD_SetBackColor(RGB_YELLOW);		// 글자배경색: YELLOW

			LCD_DrawFillRect(50 + 1, 68 + 15, 9, 9);
			LCD_DrawChar(51 + 1, 68 + 15, '1');
			LCD_DrawRectangle(49 + 1, 67 + 15, 10, 10);
		}
		else														// 홀인원이 안됐다면 파란색으로 채움
		{
			LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
			LCD_SetTextColor(RGB_BLACK);		// 글자색: BLACK
			LCD_SetBrushColor(RGB_BLUE);		// 브러쉬색: BLUE
			LCD_SetBackColor(RGB_BLUE);			// 글자배경색: BLUE

			LCD_DrawFillRect(50 + 1, 68 + 15, 9, 9);
			LCD_DrawChar(51 + 1, 68 + 15, '1');
			LCD_DrawRectangle(49 + 1, 67 + 15, 10, 10);
		}
	}

	if (x >= 79 && x <= 84 && y >= 60 && y <= 65)					// 홀인원을 하면 hole_flag를 1 대입하고 파란색으로 변경한다.
	{
		if (hole_flag == 1)
		{
			hole_flag = 2;
			LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
			LCD_SetTextColor(RGB_BLACK);		// 글자색: BLACK
			LCD_SetBrushColor(RGB_BLUE);		// 브러쉬색: BLUE
			LCD_SetBackColor(RGB_BLUE);			// 글자배경색: BLUE

			LCD_DrawFillRect(77 + 1, 44 + 15, 9, 9);
			LCD_DrawChar(78 + 1, 44 + 15, '2');
			LCD_DrawRectangle(76 + 1, 43 + 15, 10, 10);
		}
	}
	else if (x >= 71 && x <= 92 && y >= 52 && y <= 73)				// Golf Ball이 움직이면서 홀을 지우면 다시 채워주기 위한 과정
	{
		if (hole_flag <= 1)											// 홀인원이 안됐다면 노란색으로 채움
		{
			LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
			LCD_SetTextColor(RGB_BLACK);		// 글자색: BLACK
			LCD_SetBrushColor(RGB_YELLOW);		// 브러쉬색: YELLOW
			LCD_SetBackColor(RGB_YELLOW);		// 글자배경색: YELLOW

			LCD_DrawFillRect(77 + 1, 44 + 15, 9, 9);
			LCD_DrawChar(78 + 1, 44 + 15, '2');
			LCD_DrawRectangle(76 + 1, 43 + 15, 10, 10);
		}
		else														// 홀인원이 안됐다면 파란색으로 채움
		{
			LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
			LCD_SetTextColor(RGB_BLACK);		// 글자색: BLACK
			LCD_SetBrushColor(RGB_BLUE);		// 브러쉬색: BLUE
			LCD_SetBackColor(RGB_BLUE);			// 글자배경색: BLUE

			LCD_DrawFillRect(77 + 1, 44 + 15, 9, 9);
			LCD_DrawChar(78 + 1, 44 + 15, '2');
			LCD_DrawRectangle(76 + 1, 43 + 15, 10, 10);
		}
	}

	if (x >= 25 && x <= 30 && y >= 36 && y <= 41)					// 홀인원을 하면 hole_flag를 1 대입하고 파란색으로 변경한다.
	{
		if (hole_flag == 2)
		{
			hole_flag = 3;
			LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
			LCD_SetTextColor(RGB_BLACK);		// 글자색: BLACK
			LCD_SetBrushColor(RGB_BLUE);		// 브러쉬색: BLUE
			LCD_SetBackColor(RGB_BLUE);			// 글자배경색: BLUE

			LCD_DrawFillRect(23 + 1, 20 + 15, 9, 9);
			LCD_DrawChar(24 + 1, 20 + 15, '3');
			LCD_DrawRectangle(22 + 1, 19 + 15, 10, 10);
			
			move_flag = 0;
			TIM6->CR1 &= ~(1 << 0);			// CEN: disable the Tim6 Counter
			TIM11->CR1 &= ~(1 << 0);		// CEN: disable the Tim11 Counter

		}
	}
	else if (x >= 17 && x <= 38 && y >= 28 && y <= 49)				// Golf Ball이 움직이면서 홀을 지우면 다시 채워주기 위한 과정
	{
		if (hole_flag <= 2)											// 홀인원이 안됐다면 노란색으로 채움
		{
			LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
			LCD_SetTextColor(RGB_BLACK);		// 글자색: BLACK
			LCD_SetBrushColor(RGB_YELLOW);		// 브러쉬색: YELLOW
			LCD_SetBackColor(RGB_YELLOW);		// 글자배경색: YELLOW

			LCD_DrawFillRect(23 + 1, 20 + 15, 9, 9);
			LCD_DrawChar(24 + 1, 20 + 15, '3');
			LCD_DrawRectangle(22 + 1, 19 + 15, 10, 10);
		}
		else														// 홀인원이 안됐다면 파란색으로 채움
		{
			LCD_SetPenColor(RGB_BLACK);			// 펜색: BLACK
			LCD_SetTextColor(RGB_BLACK);		// 글자색: BLACK
			LCD_SetBrushColor(RGB_BLUE);		// 브러쉬색: BLUE
			LCD_SetBackColor(RGB_BLUE);			// 글자배경색: BLUE

			LCD_DrawFillRect(23 + 1, 20 + 15, 9, 9);
			LCD_DrawChar(24 + 1, 20 + 15, '3');
			LCD_DrawRectangle(22 + 1, 19 + 15, 10, 10);
		}
	}

	LCD_SetBackColor(RGB_WHITE);		// 글자배경색: WHITE
	LCD_SetTextColor(RGB_RED);			// 글자색: RED
}

void Move_Ball(void)
{
	Erase_Ball();			// Golf Ball을 이동할 때마다 지워준다.

	float new_Ax, new_Ay;	// new_Ax, new_Ay 부호 변환하고 저장한다.
	if (move_flag)			// move_flag가 1이면
	{
		if (Ax == 0.0)
		{
			new_Ax = Ax;
			x += 0.0;
		}
		else if (Ax > 0.0)		// 양수면 왼쪽으로 이동
		{
			new_Ax = Ax;
			x--;
			if (x <= 2) x = 2;		// 벽을 못넘어가게 해주는 위한 조건문
		}
		else if (Ax < 0.0)		// 음수면 오른쪽으로 이동
		{
			new_Ax = -Ax;
			x++;
			if (x >= 102) x = 102;	// 벽을 못넘어가게 해주는 위한 조건문
		}

		if (Ay == 0.0)
		{
			new_Ay = Ay;
			y += 0.0;
		}
		else if (Ay > 0.0)		// 양수면 왼쪽으로 이동
		{
			new_Ay = Ay;
			y--;
			if (y <= 16) y = 16;	// 벽을 못넘어가게 해주는 위한 조건문
		}
		else if (Ay < 0.0)		// 음수면 오른쪽으로 이동
		{
			new_Ay = -Ay;
			y++;
			if (y >= 104) y = 104;	// 벽을 못넘어가게 해주는 위한 조건문
		}
	}

	Draw_Hole();				// Golf Ball이 지나가면서 지우면 채워준다.
	Draw_Ball();				// 변한 좌표에 맞게 이동한다.
	
	// Ax, Ay 두 값 중 큰 값으로 MS를 구한다.
	if (new_Ax > new_Ay) MS = (uint8_t)(20 - new_Ax * 2);
	else MS = (uint8_t)(20 - new_Ay * 2);

	// 벽면에서 이동할 때는 작은 값으로 MS를 구한다.
	if (x == 2 || x == 102) MS = (uint8_t)(20 - new_Ay * 2);
	if (y == 16 || y == 104) MS = (uint8_t)(20 - new_Ax * 2);

	DelayMS(MS);
}