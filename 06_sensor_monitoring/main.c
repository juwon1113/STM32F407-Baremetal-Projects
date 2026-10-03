/////////////////////////////////////////////////////////////
// HW3: USART 통신을 이용한 센서 모니터링
// 제출자: 백주원
// 주요 내용 및 구현 방법
// - ADC1과 DMA2_Stream0를 이용하여 3개 센서값 취득 (SCAN 모드)
// - TIM1_CH2(OC mode) CC2 event로 400ms마다 ADC 시작
// - LCD에 온도/전압 표시 및 온도값 막대 그래프로 그리기
// - USART1으로 PC와 통신 (38400bps, 9bit, Odd parity)
// - USART1 RX: Interrupt, TX: Polling
/////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"

void DisplayInitScreen(void);		// Display 초기화
void _ADC_Init(void);				// ADC1 초기화
void DMAInit(void);					// DMA2_Stream0 초기화
void TIMER1_Init(void);				// TIMER1_CH2 초기화
void USART1_Init(void);				// USART1 초기화
void USART_BRR_Configuration(uint32_t USART_BaudRate);		// USART1 Boud rate를 계산
void DisplaySensorData(void);								// 온도 및 전압값을 LCD에 출력
void Draw_Bar(void);										// 온도 크기를 나타내는 막대 그래프

void SerialSendChar(uint8_t c);								// 1문자 보내기 함수
void SerialSendString(char* s);								// 여러문자 보내기 함수

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint16_t ADC_result[3];					// ADC 변환 결과 저장 배열
char str[20];							// LCD, USART_TX 출력용 문자열 버퍼
float Vsense;							// 센서 출력 전압(V)
uint16_t Temp_1, Temp_2, Temp_3;		// 각각 가변저항, 외부센서, 내부센서의 온도값(C)
char ch = '0';								// USART_RX 수신 문자

int main(void)
{
	LCD_Init();
	DisplayInitScreen();			// Display 초기화
	_ADC_Init();					// ADC1 초기화
	DMAInit();						// DMA2_Stream0 초기화
	TIMER1_Init();					// TIMER1_CH2 초기화
	USART1_Init();					// USART1 초기화 

	while (1)
	{
		DisplaySensorData();			// 온도 및 전압값을 LCD에 업데이트
		Draw_Bar();						// 온도 크기를 나타내는 막대 그래프

		// Tx: Polling
		if (ch == '1')						// USART_RX 수신 문자가 '1'이면 가변저항 온도값 전송
		{
			LCD_DisplayChar(1, 18, ch);
			sprintf(str, "%d%d ", Temp_1 / 10, Temp_1 % 10);
			SerialSendString(str);
			ch = '0';						// USART_RX 수신 문자 초기화
		}
		else if (ch == '2')					// USART_RX 수신 문자가 '2'이면 외부센서 온도값 전송
		{
			LCD_DisplayChar(1, 18, ch);
			sprintf(str, "%d%d ", Temp_2 / 10, Temp_2 % 10);
			SerialSendString(str);
			ch = '0';						// USART_RX 수신 문자 초기화
		}
		else if (ch == '3')					// USART_RX 수신 문자가 '3'이면 내부센서 온도값 전송
		{
			LCD_DisplayChar(1, 18, ch);
			sprintf(str, "%d%d ", Temp_3 / 10, Temp_3 % 10);
			SerialSendString(str);
			ch = '0';						// USART_RX 수신 문자 초기화
		}
	}
}

void DisplayInitScreen(void)
{
	LCD_Clear(RGB_WHITE);						// 화면 클리어
	LCD_SetFont(&Gulim8);						// 폰트 : 굴림 8
	LCD_SetBrushColor(RGB_BLUE);
	LCD_DrawFillRect(0, 0, 115, 25);

	LCD_SetBackColor(RGB_BLUE);					// 글자배경색 : BLUE
	LCD_SetTextColor(RGB_YELLOW);				// 글자색 : YELLOW
	LCD_DisplayText(0, 0, "TMP monitor");		// Title
	LCD_DisplayText(1, 0, "BJW");	// Name

	// 온도 센서1의 출력전압과 온도
	LCD_SetBackColor(RGB_WHITE);				// 글자배경색 : WHITE
	LCD_SetTextColor(RGB_BLACK);				// 글자색 : BLACK
	LCD_DisplayText(2, 0, " S1: ");
	LCD_DisplayText(2, 7, "C(");
	LCD_DisplayText(2, 12, "V)");
	
	// 온도 센서2의 출력전압과 온도
	LCD_SetBackColor(RGB_WHITE);				// 글자배경색 : WHITE
	LCD_SetTextColor(RGB_BLACK);				// 글자색 : BLACK
	LCD_DisplayText(4, 0, " S2: ");
	LCD_DisplayText(4, 7, "C(");
	LCD_DisplayText(4, 12, "V)");

	// 온도 센서3의 출력전압과 온도
	LCD_SetBackColor(RGB_WHITE);				// 글자배경색 : WHITE
	LCD_SetTextColor(RGB_BLACK);				// 글자색 : BLACK
	LCD_DisplayText(6, 0, " S3: ");
	LCD_DisplayText(6, 7, "C(");
	LCD_DisplayText(6, 12, "V)");

	// Hercules 명령값
	LCD_SetTextColor(RGB_RED);
	LCD_DisplayChar(1, 18, ch);
}

void _ADC_Init(void)
{
	// 온도센서1: 키트 상의 가변저항(ADC1_IN1(PA1))
	RCC->AHB1ENR |= (1 << 0);  		// RCC_AHB1ENR GPIOA Enable
	GPIOA->MODER |= (3 << 2);		// GPIOA PIN1(PA1) 가변저항

	// 온도센서2: 외부센서(ADC1_IN8(PB0))
	RCC->AHB1ENR |= (1 << 1);  		// RCC_AHB1ENR GPIOB Enable
	GPIOB->MODER |= (3 << 0);		// GPIOB PIN0(PB0) 외부센서

	// 온도센서3: MCU 내부 온도센서(ADC1_IN16) (GPIO 설정 불필요)

	RCC->APB2ENR |= (1 << 8);		// RCC_APB2ENR ADC1 Enable

	ADC->CCR &= ~(0X1F<<0);			// ADC_Mode_Independent
	ADC->CCR |= (1 << 16);			// ADC_Prescaler_Div4 (ADC MAX Clock 36Mhz, 84Mhz(APB2)/4 = 21Mhz
	ADC->CCR |= (1 << 23);			// Temperature sensor and VREFINT channel enabled

	ADC1->CR1 &= ~(3 << 24);		// RES[1:0]=0b00 : 12bit Resolution
	ADC1->CR1 |= 0x00000100;		// ADC_ScanCovMode Enable (SCAN=1)
	ADC1->CR1 &= ~(1 << 5);			// EOCIE=0: Interrupt disable for EOC

	ADC1->CR2 &= ~(1 << 1);			// CONT=0: ADC_Continuous ConvMode Disable
	ADC1->CR2 |= (3 << 28);			// EXTEN[1:0]: ADC_ExternalTrigConvEdge_Enable(Both Edge)
	ADC1->CR2 |= (0x01 << 24);		// EXTSEL:TIM1_CC2
	ADC1->CR2 &= ~(1 << 11);		// ALIGN=0: ADC_DataAlign_Right
	ADC1->CR2 &= ~(1 << 10);		// EOCS=0: The EOC bit is set at the end of each sequence of regular conversions

	ADC1->SQR1 |= (2 << 20);		// ADC Regular channel sequece length = 3 conversion

	ADC1->SMPR2 |= 0x07 << 3;					// ADC1_CH1 Sample TIme_480Cycles (3*Channel_1)
	ADC1->SQR3 |= (0x01 << (5 * (1 - 1)));		// ADC1_CH1 << (5*(Rank-1)), Rank = 1 (1순위로 변환: 가변저항)

	ADC1->SMPR2 |= 0x07 << 24;					// ADC1_CH8 Sample Time_480Cycles (3*Channel_8)
	ADC1->SQR3 |= (0x08 << (5 * (2 - 1)));		// ADC1_CH8 << (5*(Rank-1)), Rank = 2 (2순위로 변환: 외부센서)

	ADC1->SMPR1 |= 0x07 << 18;					// ADC1_CH16 Sample Time_480Cycles (3*Channel_16)
	ADC1->SQR3 |= (0x10 << (5 * (3 - 1)));		// ADC1_CH16 << (5*(Rank-1)), Rank = 3 (3순위로 변환: 내부센서)

	ADC1->CR2 |= (1 << 9);			// DMA requests are issued as long as data are converted and DMA=1
	ADC1->CR2 |= (1 << 8);			// DMA mode enabled  (DMA=1)
	ADC1->CR2 |= (1 << 0);			// Enable ADC1:  ADON=1
}

void DMAInit(void)
{
	RCC->AHB1ENR |= (1 << 22);						// DMA2 clock enable
	DMA2_Stream0->CR &= ~(7 << 25);					// DMA2 Stream0 channel 0 selected

	// ADC1->DR(Peripheral) ==> ADC_vlaue(Memory)
	DMA2_Stream0->PAR |= (uint32_t)&ADC1->DR;		// Peripheral address - ADC1->DR(Regular data) Address
	DMA2_Stream0->M0AR |= (uint32_t)&ADC_result;		// Memory address - ADC_Value address 
	DMA2_Stream0->CR &= ~(3 << 6);					// Data transfer direction : Peripheral-to-memory (P=>M)
	DMA2_Stream0->NDTR = 3;							// DMA_BufferSize = 3 (ADC_Value[3])

	DMA2_Stream0->CR &= ~(1 << 9); 					// Peripheral increment mode  - Peripheral address pointer is fixed
	DMA2_Stream0->CR |= (1 << 10);					// Memory increment mode - Memory address pointer is incremented after each data transferd 
	DMA2_Stream0->CR |= (1 << 11);					// Peripheral data size - halfword(16bit)
	DMA2_Stream0->CR |= (1 << 13);					// Memory data size - halfword(16bit)   
	DMA2_Stream0->CR |= (1 << 8);					// Circular mode enabled   
	DMA2_Stream0->CR |= (2 << 16);					// Priority level - High

	DMA2_Stream0->FCR &= ~(1 << 2);					// DMA_FIFO_direct mode enabled
	
	DMA2_Stream0->CR &= ~(3 << 21);					// Peripheral burst transfer configuration - single transfer
	DMA2_Stream0->CR &= ~(3 << 23);					// Memory burst transfer configuration - single transfer  
	DMA2_Stream0->CR |= (1 << 0);					// DMA2_Stream0 enabled
}

void TIMER1_Init(void)
{					
	// TIM1_CH2: 400ms 이벤트 발생
	RCC->APB2ENR |= (1 << 0);		// RCC_APB2ENR TIM1 clock enabled

	// CR1 : Up counting
	TIM1->CR1 &= ~(1 << 4);			// DIR=0(Up counter)(reset state)
	TIM1->CR1 &= ~(1 << 1);			// UDIS=0(Update event Enabled): By one of following events
									//	- Counter Overflow/Underflow, 
									// 	- Setting the UG bit Set,
									//	- Update Generation through the slave mode controller 
	TIM1->CR1 &= ~(1 << 2);			// URS=0(Update event source Selection): one of following events
									//	- Counter Overflow/Underflow, 
									// 	- Setting the UG bit Set,
									//	- Update Generation through the slave mode controller 
	TIM1->CR1 &= ~(1 << 3);			// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM1->CR1 &= ~(1 << 7);			// ARPE=0(ARR is NOT buffered) (reset state)
	TIM1->CR1 &= ~(3 << 8); 		// CKD(Clock division)=00(reset state)
	TIM1->CR1 &= ~(3 << 5); 		// CMS(Center-aligned mode Sel)=00 (Edge-aligned mode) (reset state)
									// Center-aligned mode: The counter counts Up and DOWN alternatively
	
	// Assign 'Interrupt Period' and 'Output Pulse Period'
	TIM1->PSC = 8400 - 1;			// Prescaler 84,000,000/8400 = 10KHz (0.1ms)
	TIM1->ARR = 4000 - 1;			// Auto reload  : 0.1ms * 4000 = 400ms(period)

	// Event & Interrup Enable : UI  
	TIM1->EGR |= (1 << 0);			// UG: Update generation

	TIM1->CCER |= (1 << 4);			// CC2E=1: CC2 channel Output Enable

	// 'Mode' Selection : Output mode, toggle  
	TIM1->CCMR1 &= ~(3 << 8);		// CC2S(CC2 channel) : Output 
	TIM1->CCMR1 &= ~(1 << 11);		// OC2P=0: Output Compare 2 preload disable
	TIM1->CCMR1 |= (3 << 12);		// Output Compare 2 Mode : toggle

	TIM1->CCR2 = 3999;				// TIM1_CH2 Compare value (400ms period)

	TIM1->BDTR |= (1 << 15);		// main output enable
	TIM1->CR1 |= (1 << 0);			// CEN: Enable the TIM1 Counter
}

void USART1_Init(void)
{
	RCC->AHB1ENR |= (1 << 0);			// RCC_AHB1ENR GPIOA Enable

	// USART1 : TX(PA9)
	GPIOA->MODER |= (2 << 2 * 9);		// GPIOA PIN9 Alternate function mode					
	GPIOA->OSPEEDR |= (3 << 2 * 9);		// GPIOA PIN9 Output speed (100MHz Very High speed)
	GPIOA->AFR[1] |= (7 << 4);			// Connect GPIOA pin9 to AF7(USART1)

	// USART1 : RX(PA10)
	GPIOA->MODER |= (2 << 2 * 10);		// GPIOA PIN10 Alternate function mode
	GPIOA->AFR[1] |= (7 << 8);			// Connect GPIOA pin10 to AF7(USART1)

	RCC->APB2ENR |= (1 << 4);			// RCC_APB2ENR USART1 Enable

	USART_BRR_Configuration(38400);		// USART Baud rate Configuration

	USART1->CR1 |= (1 << 12);			// USART_WordLength 9 Data bit
	USART1->CR1 |= (1 << 10);			// Parity control enabled
	USART1->CR1 |= (1 << 9);			// Odd parity
	USART1->CR1 |= (1 << 2);			// 0x0004, USART_Mode_RX Enable
	USART1->CR1 |= (1 << 3);			// 0x0008, USART_Mode_Tx Enable
	USART1->CR2 &= ~(3 << 12);			// 0b00, USART_StopBits_1
	USART1->CR3 = 0x0000;				// No HardwareFlowControl, No DMA

	USART1->CR1 |= (1 << 5);			// 0x0020, RXNE interrupt Enable
	USART1->CR1 &= ~(1 << 7);			// 0x0080, TXE interrupt Disable 

	NVIC->ISER[1] |= (1 << (37 - 32));	// Enable Interrupt USART1 (NVIC 37번)
	USART1->CR1 |= (1 << 13);			//  0x2000, USART1 Enable
}

void USART1_IRQHandler(void)
{
	// RX Buffer Full interrupt
	if ((USART1->SR & USART_SR_RXNE))		// USART_SR_RXNE=(1<<5) 
	{
		ch = (uint16_t)(USART1->DR & (uint16_t)0x01FF);	// 수신된 문자 저장
		// DR 을 읽으면 SR.RXNE bit(flag bit)는 clear 된다. 즉 clear 할 필요없음 
	}
}

void SerialSendChar(uint8_t Ch) // 1문자 보내기 함수
{
	while ((USART1->SR & USART_SR_TXE) == RESET); // USART_SR_TXE=(1<<7), 송신 가능한 상태까지 대기

	USART1->DR = (Ch & 0x01FF);	// 전송 (최대 9bit 이므로 0x01FF과 masking)
}

void SerialSendString(char* str) // 여러문자 보내기 함수
{
	while (*str != '\0') // 종결문자가 나오기 전까지 구동, 종결문자가 나온후에도 구동시 메모리 오류 발생가능성 있음.
	{
		SerialSendChar(*str);	// 포인터가 가르키는 곳의 데이터를 송신
		str++; 			// 포인터 수치 증가
	}
}

// Baud rate  
void USART_BRR_Configuration(uint32_t USART_BaudRate)
{
	uint32_t tmpreg = 0x00;
	uint32_t APB2clock = 84000000;	//PCLK2_Frequency
	uint32_t integerdivider = 0x00;
	uint32_t fractionaldivider = 0x00;

	// Determine the integer part 
	if ((USART1->CR1 & USART_CR1_OVER8) != 0) // USART_CR1_OVER8=(1<<15)
	{                                         // USART1->CR1.OVER8 = 1 (8 oversampling)
		// Computing 'Integer part' when the oversampling mode is 8 Samples 
		integerdivider = ((25 * APB2clock) / (2 * USART_BaudRate));
	}
	else  // USART1->CR1.OVER8 = 0 (16 oversampling)
	{	// Computing 'Integer part' when the oversampling mode is 16 Samples 
		integerdivider = ((25 * APB2clock) / (4 * USART_BaudRate));
	}
	tmpreg = (integerdivider / 100) << 4;

	// Determine the fractional part 
	fractionaldivider = integerdivider - (100 * (tmpreg >> 4));

	// Implement the fractional part in the register 
	if ((USART1->CR1 & USART_CR1_OVER8) != 0)	// 8 oversampling
	{
		tmpreg |= (((fractionaldivider * 8) + 50) / 100) & (0x07);
	}
	else 			// 16 oversampling
	{
		tmpreg |= (((fractionaldivider * 16) + 50) / 100) & (0x0F);
	}

	// Write to USART BRR register
	USART1->BRR = (uint16_t)tmpreg;
}

void DelayMS(unsigned short wMS)
{
	register unsigned short i;
	for (i = 0; i < wMS; i++)
		DelayUS(1000);  // 1000us => 1ms
}
void DelayUS(unsigned short wUS)
{
	volatile int Dly = (int)wUS * 17;
	for (; Dly; Dly--);
}

void DisplaySensorData(void)
{
	// STM32F407IGT6 Datasheet 확인
	// Voltage at 25C: 0.76V
	// Average slope: 2.5mV/C
	float V25 = 0.76;
	float AVG_SLOPE = 0.0025;

	// ADC1_CH1 가변저항 출력전압과 온도값 표시
	Vsense = ADC_result[0] * 3.3 / 4095;
	Temp_1 = 3.5 * Vsense * Vsense + 1;
	if (Temp_1 / 10 == 0)
		LCD_DisplayChar(2, 5, ' ');
	else
		LCD_DisplayChar(2, 5, Temp_1 / 10 + '0');
	LCD_DisplayChar(2, 6, Temp_1 % 10 + '0');
	sprintf(str, "%2.1f", Vsense);
	LCD_DisplayText(2, 9, str);

	// ADC1_CH8 외부센서 출력전압과 온도값 표시
	Vsense = ADC_result[1] * 3.3 / 4095;
	Temp_2 = 3.5 * Vsense * Vsense + 1;
	if (Temp_2 / 10 == 0)
		LCD_DisplayChar(4, 5, ' ');
	else
		LCD_DisplayChar(4, 5, Temp_2 / 10 + '0');
	LCD_DisplayChar(4, 6, Temp_2 % 10 + '0');
	sprintf(str, "%2.1f", Vsense);
	LCD_DisplayText(4, 9, str);

	// ADC1_CH16 내부센서 출력전압과 온도값 표시
	Vsense = ADC_result[2] * 3.3 / 4095;
	Temp_3 = (Vsense - V25) / AVG_SLOPE + 25;
	if (Temp_3 / 10 == 0)
		LCD_DisplayChar(6, 5, ' ');
	else
		LCD_DisplayChar(6, 5, Temp_3 / 10 + '0');
	LCD_DisplayChar(6, 6, Temp_3 % 10 + '0');
	sprintf(str, "%2.1f", Vsense);
	LCD_DisplayText(6, 9, str);
}

void Draw_Bar(void)
{
	static uint16_t width1 = 0, width2 = 0, width3 = 0;		// 이전 막대 그래프 길이를 저장할 변수
	uint16_t new_width1, new_width2, new_width3;			// 새 막대 그래프 길이를 계산할 변수			

	// S1 바 가변저항(빨간색)
	new_width1 = Temp_1 * 139 / 39;				// 막대 그래프 길이 = 현재 온도 * 최대 막대 길이 / 최대 온도
	if (new_width1 > 139) new_width1 = 139;		// 최대 막대 그래프 길이: 139

	if (new_width1 > width1) {
		// 증가한 부분만 빨간색으로 그리기
		LCD_SetBrushColor(RGB_RED);
		LCD_DrawFillRect(10 + width1, 41, new_width1 - width1, 10);
	}
	else if (new_width1 < width1) {
		// 감소한 부분만 흰색으로 지우기
		LCD_SetBrushColor(RGB_WHITE);
		LCD_DrawFillRect(10 + new_width1, 41, width1 - new_width1, 10);
	}
	width1 = new_width1;		// 새 막대 그래프 길이 저장

	// S2 바 외부센서(초록색)
	new_width2 = Temp_2 * 139 / 39;				// 막대 그래프 길이 = 현재 온도 * 최대 막대 길이 / 최대 온도
	if (new_width2 > 139) new_width2 = 139;		// 최대 막대 그래프 길이: 139

	if (new_width2 > width2) {
		// 증가한 부분만 초록색으로 그리기
		LCD_SetBrushColor(RGB_GREEN);
		LCD_DrawFillRect(10 + width2, 67, new_width2 - width2, 10);
	}
	else if (new_width2 < width2) {
		// 감소한 부분만 흰색으로 지우기
		LCD_SetBrushColor(RGB_WHITE);
		LCD_DrawFillRect(10 + new_width2, 67, width2 - new_width2, 10);
	}
	width2 = new_width2;		// 새 막대 그래프 길이 저장

	// S3 바 내부센서(파란색)
	new_width3 = Temp_3 * 139 / 39;				// 막대 그래프 길이 = 현재 온도 * 최대 막대 길이 / 최대 온도
	if (new_width3 > 139) new_width3 = 139;		// 최대 막대 그래프 길이: 139

	if (new_width3 > width3) {
		// 증가한 부분만 파란색으로 그리기
		LCD_SetBrushColor(RGB_BLUE);
		LCD_DrawFillRect(10 + width3, 92, new_width3 - width3, 10);
	}
	else if (new_width3 < width3) {
		// 감소한 부분만 흰색으로 지우기
		LCD_SetBrushColor(RGB_WHITE);
		LCD_DrawFillRect(10 + new_width3, 92, width3 - new_width3, 10);
	}
	width3 = new_width3;		// 새 막대 그래프 길이 저장
}