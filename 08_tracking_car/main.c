/////////////////////////////////////////////////////////////
// PR: Car Tracking system
// 제출자: 백주원
// 주요 내용
// - 거리센서1(D1,선도차와의 거리측정): PA1(ADC3_IN1) 이용
// - 거리센서2(D2,인도와의 거리측정): PF3(ADC3_IN9) 이용
// - 추종차 속도제어기(엔진): PB7(TIM4_CH2(PWM mode)), LED*로 PWM 변화 확인
// - 추종차 방향제어기(핸들): PF9(TIM14_CH1(PWM mode)), Buzzer(소리)로 PWM 변화확인
// - Off-line 추종차 시동: Move-key(SW4(EXTI12)), Stop-key(SW6(EXTI14))
//   시동상태 표시등(LED4(PG4), LED6(PG6))
// - 선도차와의 거리값 표시: GLCD(D1(m), D2(m)), PC(D1(m))
// - 추종차 속도 표시: GLCD(SP(DR%))
// - 추종차 방향 표시: GLCD(DIR(DR):‘L’, ‘R’, ‘F’)
/////////////////////////////////////////////////////////////
#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

void DisplayInitScreen(void);		// 디스플레이 초기화
void _GPIO_Init(void);				// GPIO(LED) 초기화
void _EXTI_Init(void);				// EXTI 12, 14 초기화
void USART1_Init(void);				// USART1 초기화
void _ADC_Init(void);				// ADC3 초기화
void DMAInit(void);					// DMA2_Stream0 초기화
void TIMER1_Init(void);				// TIMER1_CH3 초기화
void TIMER4_PWM_Init(void);			// TIM4_CH2 초기화
void TIMER14_PWM_Init(void);		// TIM14_CH1 초기화

void Draw_Distance(void);			// 거리 막대 그래프 그리는 함수
void Speed_Control(void);			// 속도 제어 함수
void Direction_Control(void);		// 방향 제어 함수

void USART_BRR_Configuration(uint32_t USART_BaudRate);		// USART1 Boud rate를 계산
void SerialSendChar(uint8_t c);								// 1문자 보내기 함수
void SerialSendString(char* s);								// 여러문자 보내기 함수
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint16_t ADC_result[2];										// 가변저항1, 가변저항2 ADC값을 저장할 배열
char str[20];												// USART1 Tx에 쓰는 배열
char ch, State, Direction;									// 추종차 운행상태, 추종차 방향제어값
float Voltage_1, Voltage_2;									// 가변저항 1 전압, 가변저항 2 전압
uint8_t Distance_1, Distance_2, Speed, ADC_flag;			// 추종차와 선도차 사이 거리, 추종차와 인도 사이 거리, 추종차 속도제어값, 변환 완료 flag

int main(void)
{
	LCD_Init();
	USART1_Init();
	DisplayInitScreen();
	_GPIO_Init();
	_EXTI_Init();
	TIMER1_Init();
	_ADC_Init();
	DMAInit();
	TIMER4_PWM_Init();
	TIMER14_PWM_Init();

	Fram_Init();                    // FRAM 초기화 H/W 초기화
	Fram_Status_Config();			// FRAM 초기화 S/W 초기화
	
	State = Fram_Read(1201);		// 1201번지에 저장된 값을 읽어온다.
	while (1)
	{
		if (State != 'M' && State != 'S')	// 상태가 정해져있지않으면 'M'으로 저장
			State = 'M';

		// 변환 후 속도, 방향 제어를 한다.
		// 변환 전에 제어하면 Speed, Direction값이 0으로 적용되는 걸 방지
		if (ADC_flag)						
		{
			if (State == 'M')
			{
				// LED 4 ON, LED 6 OFF
				GPIOG->ODR |= (1 << 4);
				GPIOG->ODR &= ~(1 << 6);

				LCD_DisplayChar(1, 18, State);
				Draw_Distance();
				Speed_Control();
				Direction_Control();
			}
			else if (State == 'S')
			{
				// LED 4 OFF, LED 6 ON
				GPIOG->ODR |= (1 << 6);
				GPIOG->ODR &= ~(1 << 4);

				LCD_DisplayChar(1, 18, State);
				Draw_Distance();
				Speed_Control();
				Direction_Control();
			}
		}

		// Tx: Polling
		if (TIM1->SR & (1 << 3))							// 400ms마다 전송
		{

			TIM1->SR &= ~(1 << 3);							// TIM1 CC3IF clear
			
			if (Distance_1 / 10 == 0)						// 10의 자리가 0이면
				sprintf(str, "%dm ", Distance_1 % 10);
			else											// 10의 자리가 0이 아니면
				sprintf(str, "%d%dm ", Distance_1 / 10, Distance_1 % 10);
			SerialSendString(str);
		}
	}
}

void DisplayInitScreen(void)
{
	LCD_Clear(RGB_WHITE);						// 화면 클리어
	LCD_SetFont(&Gulim8);						// 폰트 : 굴림 8

	LCD_SetBackColor(RGB_WHITE);				// 글자배경색: WHITE
	LCD_SetTextColor(RGB_BLACK);				// 글자색: BLACK

	LCD_DisplayText(0, 0, "BJW");	// Name
	LCD_DisplayText(1, 0, "Tracking Car");		// Title

	LCD_DisplayText(2, 0, "D1: ");					// 추종차와 선도차 사이 거리
	LCD_DisplayText(3, 0, "D2: ");					// 추종차와 인도 사이 거리

	LCD_DisplayText(4, 0, "SP(DR):  %DIR(DR):");	// ‘SP(DR)’는 추종차 속도제어값으로 PWM 듀티비 표시,
													// ‘DIR(DR)’은 추종차 방향제어값으로 PWM을 방향으로 변환 표시

	LCD_SetTextColor(RGB_BLUE);					// 글자색: BLUE
}

void _GPIO_Init(void)
{
	// LED (GPIO G) 설정 : Output mode
	RCC->AHB1ENR |= 0x00000040;		// RCC_AHB1ENR : GPIOG(bit#6) Enable							

	GPIOG->MODER &= ~0x0000FFFF;	// GPIOG 0~7 : Clear(0b00)						
	GPIOG->MODER |= 0x00005555;		// GPIOG 0~7 : Output mode (0b01)						

	GPIOG->OTYPER &= ~0x00FF;		// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	

	GPIOG->OSPEEDR &= ~0x0000FFFF;	// GPIOG 0~7 : Clear(0b00)
	GPIOG->OSPEEDR |= 0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed
}

void _EXTI_Init(void)
{
	RCC->AHB1ENR |= 0x00000080;			// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR |= 0x00004000;			// Enable System Configuration Controller Clock

	GPIOH->MODER &= ~0xFFFF0000;		// GPIOH PIN8~PIN15 Input mode (reset state)

	// EXTI 12, 14 설정
	SYSCFG->EXTICR[3] |= 0x0707; 		// EXTI 12, 14에 대한 소스 입력은 GPIOH로 설정	

	EXTI->RTSR |= 0x005000;				// EXTI 12, 14: Rising Trigger Enable
	EXTI->IMR |= 0x005000;				// EXTI 12, 14 인터럽트 mask (Interrupt Enable) 설정

	NVIC->ISER[1] |= (1 << (40 - 32));
}

void EXTI15_10_IRQHandler(void)
{
	if (EXTI->PR & 0x1000)				// EXTI12 Interrupt Pending(발생) 여부?
	{
		EXTI->PR |= 0x1000;				// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
		State = 'M';
		Fram_Write(1201, State);		// FRAM(0~8191) 1201번지에 State 저장
	}

	if (EXTI->PR & 0x4000)				// EXTI14 Interrupt Pending(발생) 여부?
	{
		EXTI->PR |= 0x4000;				// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
		State = 'S';
		Fram_Write(1201, State);		// FRAM(0~8191) 1201번지에 State 저장
	}
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

	USART_BRR_Configuration(19200);		// USART Baud rate Configuration

	USART1->CR1 &= ~(1 << 12);			// USART_WordLength 8 Data bit
	USART1->CR1 &= ~(1 << 10);			// Parity control disabled
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
		ch = USART1->DR;
		if (ch == 'M')
		{
			State = 'M';
			Fram_Write(1201, State);		// FRAM(0~8191) 1201번지에 State 저장
		}
		else if (ch == 'S')
		{
			State = 'S';
			Fram_Write(1201, State);		// FRAM(0~8191) 1201번지에 State 저장
		}
	}
}

void _ADC_Init(void)
{
	// 내부 가변저항(ADC3_IN1(PA1))
	RCC->AHB1ENR |= (1 << 0);		// RCC_AHB1ENR GPIOA Enable
	GPIOA->MODER |= (3 << 2);		// GPIOA PIN1(PA1) 가변저항1

	// 외부 가변저항(ADC3_IN9(PF3))
	RCC->AHB1ENR |= (1 << 5);		// RCC_AHB1ENR GPIOB Enable
	GPIOF->MODER |= (3 << 6);		// GPIOB PIN0(PB0) 가변저항2

	RCC->APB2ENR |= (1 << 10);		// RCC_APB2ENR ADC3 Enable

	ADC->CCR &= ~(0x1F << 0);		// ADC_Mode_Independent
	ADC->CCR |= (1 << 16);			// ADC_Prescaler_Div4 (ADC MAX Clock 36Mhz, 84Mhz(APB2)/4 = 21Mhz

	ADC3->CR1 |= (1 << 24);			// RES[1:0]=0b00 : 10bit Resolution
	ADC3->CR1 |= (1 << 8);			// ADC_ScanCovMode Enable (SCAN=1)
	ADC3->CR1 |= (1 << 5);			// EOCIE=1: Interrupt enable for EOC

	ADC3->CR2 &= ~(1 << 1);			// CONT=0: ADC_Continuous ConvMode Disable
	ADC3->CR2 |= (3 << 28);			// EXTEN[1:0]: ADC_ExternalTrigConvEdge_Enable(Both Edge)
	ADC3->CR2 |= (2 << 24);			// EXTSEL:TIM1_CC3
	ADC3->CR2 &= ~(1 << 11);		// ALIGN=0: ADC_DataAlign_Right
	ADC3->CR2 &= ~(1 << 10);		// EOCS=0: The EOC bit is set at the end of each sequence of regular conversions

	ADC3->SQR1 |= (1 << 20);		// ADC Regular channel sequece length = 2 conversion

	ADC3->SMPR2 |= 0x07 << 3;					// ADC3_CH1 Sample TIme_480Cycles (3*Channel_1)
	ADC3->SQR3 |= (0x01 << (5 * (1 - 1)));		// ADC3_CH1 << (5*(Rank-1)), Rank = 1 (1순위로 변환: 가변저항)

	ADC3->SMPR2 |= 0x07 << 27;					// ADC3_CH9 Sample Time_480Cycles (3*Channel_9)
	ADC3->SQR3 |= (0x09 << (5 * (2 - 1)));		// ADC3_CH9 << (5*(Rank-1)), Rank = 2 (2순위로 변환: 외부센서)

	ADC3->CR2 |= (1 << 9);			// DMA requests are issued as long as data are converted and DMA=1
	ADC3->CR2 |= (1 << 8);			// DMA mode enabled  (DMA=1)
	ADC3->CR2 &= ~(1 << 10);		// The EOC bit is set at the end of each sequence of regular conversions. Overrun detection is enabled only if DMA = 1

	NVIC->ISER[0] |= (1 << 18);		// Enable ADC global Interrupt

	ADC3->CR2 |= (1 << 0);			// Enable ADC3:  ADON=1
}

void ADC_IRQHandler(void)
{
	ADC3->SR &= ~(1 << 1);							// EOC flag clear
	
	// 400ms마다 거리 계산 후 출력
	uint8_t New_Distance_1, New_Distance_2;
	if (State == 'M')
	{
		Voltage_1 = ADC_result[0] * 3.3 / 1023;				// 전압 계산
		New_Distance_1 = (uint8_t)(Voltage_1 * 5 + 3);		// 거리 계산

		Voltage_2 = ADC_result[1] * 3.3 / 1023;				// 전압 계산
		New_Distance_2 = (uint8_t)Voltage_2;				// 거리 계산
	}
	else if (State == 'S')
	{
		New_Distance_1 = 0;
		New_Distance_2 = 1;
	}

	// 거리가 변할 때만 값을 출력하고 저장한다.
	// LCD에 잘못된 값이 출력되는걸 방지
	if (Distance_1 != New_Distance_1)
	{
		if (New_Distance_1 / 10 == 0)
			LCD_DisplayChar(2, 17, ' ');
		else
			LCD_DisplayChar(2, 17, New_Distance_1 / 10 + '0');
		LCD_DisplayChar(2, 18, New_Distance_1 % 10 + '0');

		Distance_1 = New_Distance_1;
	}

	if (Distance_2 != New_Distance_2)
	{
		LCD_DisplayChar(3, 18, New_Distance_2 + '0');
		Distance_2 = New_Distance_2;
	}

	// 거리가 0일 때 부팅 시 출력 안되는 현상을 방지
	if (Distance_1 == 0)
		LCD_DisplayChar(2, 18, '0');
	if (Distance_2 == 0)
		LCD_DisplayChar(3, 18, '0');

	ADC_flag = 1;
}

void DMAInit(void)
{
	// DMA2_Stream0_Channel2
	RCC->AHB1ENR |= (1 << 22);						// DMA2 clock enable
	DMA2_Stream0->CR |= (2 << 25);					// DMA2 Stream0 channel 2 selected

	// ADC1->DR(Peripheral) ==> ADC_vlaue(Memory)
	DMA2_Stream0->PAR |= (uint32_t)&ADC3->DR;		// Peripheral address - ADC3->DR(Regular data) Address
	DMA2_Stream0->M0AR |= (uint32_t)&ADC_result;	// Memory address - ADC_Value address 
	DMA2_Stream0->CR &= ~(3 << 6);					// Data transfer direction : Peripheral-to-memory (P=>M)
	DMA2_Stream0->NDTR = 2;							// DMA_BufferSize = 2 (ADC_Value[2])

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
	// TIM1_CH3: 400ms 이벤트 발생
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
	TIM1->PSC = 8400 - 1;			// Prescaler 168,000,000/8400 = 20KHz (0.05ms)
	TIM1->ARR = 8000 - 1;			// Auto reload  : 0.05ms * 8000 = 400ms(period)

	// Event & Interrup Enable : UI  
	TIM1->EGR |= (1 << 0);			// UG: Update generation

	TIM1->CCER |= (1 << 8);			// CC3E=1: CC3 channel Output Enable

	// 'Mode' Selection : Output mode, toggle  
	TIM1->CCMR2 &= ~(3 << 0);			// CC3S(CC3 channel) : Output 
	TIM1->CCMR2 |= (3 << 4);			// Output Compare 3 Mode : toggle Duty

	TIM1->CCR3 = 8000 - 1;				// TIM1_CH3 Compare value

	TIM1->BDTR |= (1 << 15);		// main output enable
	TIM1->CR1 |= (1 << 0);			// CEN: Enable the TIM1 Counter
}

void TIMER4_PWM_Init(void)
{
	// TIM4_CH2 : PB7
	RCC->AHB1ENR |= (1 << 1);
	RCC->APB1ENR |= (1 << 2);	// TIMER4 CLOCK Enable

	GPIOB->MODER |= (2 << 14);
	GPIOB->OSPEEDR |= (3 << 14);
	GPIOB->OTYPER &= ~(1 << 7);
	GPIOB->PUPDR |= (1 << 14);
	GPIOB->AFR[0] |= (2 << 28);	// AF2

	// Setting CR1 : 0x0000 (Up counting)
	TIM4->CR1 &= ~(1 << 4);		// DIR=0(Up counter)(reset state)
	TIM4->CR1 &= ~(1 << 1);		// UDIS=0(Update event Enabled)
	TIM4->CR1 &= ~(1 << 2);		// URS=0(Update event source Selection)g events
	TIM4->CR1 &= ~(1 << 3);		// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM4->CR1 |= (1 << 7);		// ARPE=1(ARR is buffered): ARR Preload Enable 
	TIM4->CR1 &= ~(3 << 8); 	// CKD(Clock division)=00(reset state)
	TIM4->CR1 &= ~(3 << 5); 	// CMS(Center-aligned mode Sel)=00 : Edge-aligned mode(reset state)

	TIM4->PSC = 8400 - 1;		// Prescaler 84,000,000Hz/8400 = 10,000 Hz(0.1ms)  (1~65536)
	TIM4->ARR = 50000 - 1;		// Auto reload  (0.1ms * 50000 = 5s : PWM Period)

	// CCER(Capture/Compare Enable Register) : Enable "Channel 2" 
	TIM4->CCER |= (1 << 4);			// CC2E=1: OC2(TIM4_CH2) Active(Capture/Compare 2 output enable)
	TIM4->CCER &= ~(1 << 5);		// CC2P=0: CC2 Output Polarity (OCPolarity_High : OC2으로 반전없이 출력)

	// Duty Ratio
	TIM4->CCR2 = 0;			// CCR2 value

	// 'Mode' Selection : Output mode, PWM 1
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM4->CCMR1 &= ~(3 << 8);		// CC2S(CC2 channel)= '0b00' : Output 
	TIM4->CCMR1 |= (1 << 11);		// OC2PE=1: Output Compare 2 preload Enable
	TIM4->CCMR1 |= (6 << 12);		// OC2M: Output compare 2 mode: PWM 1 mode
	TIM4->CCMR1 |= (1 << 15);		// OC2CE=1: Output compare 2 Clear enable

	//Counter TIM4 enable
	TIM4->CR1 |= (1 << 0);	// CEN: Counter TIM4 enable
}

void TIMER14_PWM_Init(void)
{
	// TIM14_CH1 : PF9
	RCC->AHB1ENR |= (1 << 5);
	RCC->APB1ENR |= (1 << 8);	// TIMER14 CLOCK Enable

	GPIOF->MODER |= (2 << 18);
	GPIOF->OSPEEDR |= (3 << 18);
	GPIOF->OTYPER &= ~(1 << 9);
	GPIOF->PUPDR |= (1 << 18);
	GPIOF->AFR[1] |= (9 << 4);	// AF9

	// Setting CR1 : 0x0000 (Up counting)
	TIM14->CR1 &= ~(1 << 4);		// DIR=0(Up counter)(reset state)
	TIM14->CR1 &= ~(1 << 1);		// UDIS=0(Update event Enabled)
	TIM14->CR1 &= ~(1 << 2);		// URS=0(Update event source Selection)g events
	TIM14->CR1 &= ~(1 << 3);		// OPM=0(The counter is NOT stopped at update event) (reset state)
	TIM14->CR1 |= (1 << 7);			// ARPE=1(ARR is buffered): ARR Preload Enable 
	TIM14->CR1 &= ~(3 << 8); 		// CKD(Clock division)=00(reset state)
	TIM14->CR1 &= ~(3 << 5); 		// CMS(Center-aligned mode Sel)=00 : Edge-aligned mode(reset state)

	TIM14->PSC = 420 - 1;			// Prescaler 84,000,000Hz/420 = 200000 Hz(5us)  (1~65536)
	TIM14->ARR = 80 - 1;			// Auto reload  (5us * 80 = 0.4ms : PWM Period)

	// CCER(Capture/Compare Enable Register) : Enable "Channel 1" 
	TIM14->CCER |= (1 << 0);		// CC1E=1: OC1(TIM14_CH1) Active(Capture/Compare 1 output enable)
	TIM14->CCER &= ~(1 << 1);		// CC1P=0: CC1 Output Polarity (OCPolarity_High : OC1으로 반전없이 출력)

	// Duty Ratio
	TIM14->CCR1 = 0;				// CCR1 value

	// 'Mode' Selection : Output mode, PWM 1
	// CCMR1(Capture/Compare Mode Register 1) : Setting the MODE of Ch1 or Ch2
	TIM14->CCMR1 &= ~(3 << 0);		// CC1S(CC1 channel)= '0b00' : Output 
	TIM14->CCMR1 |= (1 << 3);		// OC1PE=1: Output Compare 1 preload Enable
	TIM14->CCMR1 |= (6 << 4);		// OC1M: Output compare 1 mode: PWM 1 mode
	TIM14->CCMR1 |= (1 << 7);		// OC1CE=1: Output compare 1 Clear enable

	//Counter TIM14 enable
	TIM14->CR1 |= (1 << 0);	// CEN: Counter TIM14 enable
}

uint8_t width_1, width_2;
void Draw_Distance(void)
{
	uint8_t new_width_1, new_width_2;				// 새 거리 저장 문자

	// D1 바 가변저항1(빨간색)
	new_width_1 = Distance_1 * 100 / 19;
	if (width_1 < new_width_1)
	{
		// 증가한 부분만 빨간색으로 그리기
		LCD_SetBrushColor(RGB_RED);
		LCD_DrawFillRect(27 + width_1, 27, new_width_1 - width_1, 9);
	}
	else
	{
		// 감소한 부분만 흰색으로 지우기
		LCD_SetBrushColor(RGB_WHITE);
		LCD_DrawFillRect(27 + new_width_1, 27, width_1 - new_width_1, 9);
	}
	width_1 = new_width_1;		// 새 막대 그래프 길이 저장

	// D2 바 가변저항2(초록색)
	new_width_2 = Distance_2 * 100 / 3;
	if (width_2 < new_width_2)
	{
		// 증가한 부분만 초록색으로 그리기
		LCD_SetBrushColor(RGB_GREEN);
		LCD_DrawFillRect(27 + width_2, 40, new_width_2 - width_2, 9);
	}
	else
	{
		// 감소한 부분만 흰색으로 지우기
		LCD_SetBrushColor(RGB_WHITE);
		LCD_DrawFillRect(27 + new_width_2, 40, width_2 - new_width_2, 9);
	}
	width_2 = new_width_2;		// 새 막대 그래프 길이 저장
}

void Speed_Control(void)
{
	if (Distance_1 <= 2)			// 거리 0 ~ 2m, Duty = 00%
	{
		TIM4->CCR2 = 0;
		Speed = 0;
	}
	else if (Distance_1 <= 4)		// 거리 3 ~ 4m, Duty = 10%
	{
		TIM4->CCR2 = 5000;
		Speed = 10;
	}
	else if (Distance_1 <= 6)		// 거리 5 ~ 6m, Duty = 20%
	{
		TIM4->CCR2 = 10000;
		Speed = 20;
	}
	else if (Distance_1 <= 8)		// 거리 7 ~ 8m, Duty = 30%
	{
		TIM4->CCR2 = 15000;
		Speed = 30;
	}
	else if (Distance_1 <= 10)		// 거리 9 ~ 10m, Duty = 40%
	{
		TIM4->CCR2 = 20000;
		Speed = 40;
	}
	else if (Distance_1 <= 12)		// 거리 11 ~ 12m, Duty = 50%
	{
		TIM4->CCR2 = 25000;
		Speed = 50;
	}
	else if (Distance_1 <= 14)		// 거리 13 ~ 14m, Duty = 60%
	{
		TIM4->CCR2 = 30000;
		Speed = 60;
	}
	else if (Distance_1 <= 16)		// 거리 15 ~ 16m, Duty = 70%
	{
		TIM4->CCR2 = 35000;
		Speed = 70;
	}
	else if (Distance_1 <= 18)		// 거리 17 ~ 18m, Duty = 80%
	{
		TIM4->CCR2 = 40000;
		Speed = 80;
	}
	else if (Distance_1 == 19)		// 거리 19m, Duty = 90%
	{
		TIM4->CCR2 = 45000;
		Speed = 90;
	}

	LCD_DisplayChar(4, 7, Speed / 10 + '0');	// Duty값 10의 자리
	LCD_DisplayChar(4, 8, Speed % 10 + '0');	// Duty값 1의 자리
}

void Direction_Control(void)
{
	if (Distance_2 == 0)		// 거리 0m, Duty = 30%
	{
		TIM14->CCR1 = 24;
		Direction = 'R';		// 핸들 오른쪽 회전(R)
	}
	else if (Distance_2 == 1)	// 거리 1m, Duty = 0%
	{
		TIM14->CCR1 = 0;
		Direction = 'F';		// 핸들 직진(F)

	}
	else						// 거리 2 ~ 3m, Duty = 90%
	{	
		TIM14->CCR1 = 72;
		Direction = 'L';		// 핸들 왼쪽 회전(L)
	}

	LCD_DisplayChar(4, 18, Direction);
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
	else			// if key input, check continuous key
	{
		if (key_flag != 0)	// if continuous key, treat as no key input
			return 0xFF00;
		else		// if new key,delay for debounce
		{
			key_flag = 1;
			DelayMS(10);
			return key;
		}
	}
}
