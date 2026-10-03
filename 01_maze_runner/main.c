/////////////////////////////////////////////////////////////
// 과제명: Maze Runner
// 과제개요: 시작위치(0,0)에 있는 'Mouse'(파란 정사각형)를 조이스틱(JS)을 이용하여
// 목표위치(4,4)에 이동시키는 게임
// 사용한 하드웨어(기능): GPIO, Joy-stick, LCD
// 제출일: 2025. 5. 14
// 이름: 백주원
///////////////////////////////////////////////////////////////

#include "stm32f4xx.h"
#include "GLCD.h"

void _GPIO_Init(void);
uint16_t KEY_Scan(void);

void BEEP(void);                                    // 부저를 울리게 하는 함수
void DisplayInitScreen(void);                       // Display를 초기화 하는 함수
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint16_t JOY_Scan(void);
void ClearMouse(uint8_t x, uint8_t y);             // 마우스를 지우는 함수
void DrawMouse(uint8_t x, uint8_t y);              // 마우스를 그리는 함수
void DrawMaze(void);                                // 미로를 그리는 함수

#define NAVI_PUSH   0x03C0  //PI5 0000 0011 1100 0000 
#define NAVI_UP     0x03A0  //PI6 0000 0011 1010 0000 
#define NAVI_DOWN   0x0360  //PI7 0000 0011 0110 0000 
#define NAVI_RIGHT  0x02E0  //PI8 0000 0010 1110 0000 
#define NAVI_LEFT   0x01E0  //PI9 0000 0001 1110 0000

uint8_t Ucount, Dcount, Lcount, Rcount;             // 상, 하, 좌, 우를 count하기 위한 변수
uint8_t Mx, My, flag;                               // mouse 좌표 변경을 위한 변수와 SW0 RESET, SW7 START를 위한 flag 변수

int main(void)
{
    _GPIO_Init();                                   // GPIO (LED, SW, Buzzer) 초기화
    LCD_Init();                                     // LCD 모듈 초기화
    DelayMS(100);
    BEEP();

    GPIOG->ODR &= ~0x00FF;                          // LED 초기값: LED0~7 Off
    GPIOG->BSRRL = 0x0001;                          // LED0 ON
    DisplayInitScreen();                            // LCD 초기화면

    while (1)
    {
        switch (KEY_Scan())                          // 입력된 Switch 정보 분류 
        {
        case 0xFE00:                            //SW0를 누르면 LCD 초기화면으로 돌아간다.
            DisplayInitScreen();                // LCD 초기화면
            GPIOG->BSRRH = 0x00FE;              // LED 초기값: LED1~7 Off
            break;
        case 0x7F00:                            // SW7를 누르면 MazeRunner를 시작할 수 있다.
            flag = 1;                           // flag = 1로 초기화
            BEEP();                             // 부저가 한 번 울린다.
            GPIOG->BSRRL = 0x0080;              // LED7 ON
            LCD_DisplayChar(5, 22, ((Mx - 1) / 17) + '0');  // x좌표를 0으로 표시
            LCD_DisplayChar(5, 24, ((My - 21) / 17) + '0'); // y좌표를 0으로 표시
            break;
        }  // switch(KEY_Scan()) 

        if (flag)                                   // SW7를 눌러 flag = 1이 되면 실행하기 위한 조건문
        {
            switch (JOY_Scan())                      // 입력된 JOYSTICK 정보 분류
            {
            case NAVI_UP:                       // Joystick UP
                if ((My - 21) / 17 == 0)        // y = 0인 상태에서 UP을 하면 부저가 두 번 울린다.
                {
                    BEEP();
                    DelayMS(500);
                    BEEP();
                    break;
                }
                else                            // 이동 시 부저가 한 번 울린다.
                    BEEP();

                ClearMouse(Mx, My);             // 기존 마우스 위치를 지운다.
                DrawMaze();                     // Maze를 새로 그린다.
                My = My - 17;                   // 마우스 위치를 한칸 위로 올린다.
                DrawMouse(Mx, My);              // 마우스를 그린다.

                Ucount++;                       // Up할 때마다 count 증가
                if (Ucount > 4)                 // count가 4보다 크면 0으로 초기화한다.
                    Ucount = 0;
                LCD_DisplayChar(10, 3, Ucount + '0');             // Ucount 횟수를 화면에 출력한다.
                LCD_DisplayChar(5, 24, ((My - 21) / 17) + '0'); // 마우스 y좌표를 출력한다.

                GPIOG->BSRRH = 0x007E;          // LED0,7 빼고 전부 OFF
                GPIOG->BSRRL = 0x0002;          // LED1 ON
                break;

            case NAVI_DOWN:                     // Joystick DOWN
                if ((My - 21) / 17 == 4)        // y = 4인 상태에서 DOWN을 하면 부저가 두 번 울린다.
                {
                    BEEP();
                    DelayMS(500);
                    BEEP();
                    break;
                }
                else
                    BEEP();                     // 이동 시 부저가 한 번 울린다.

                ClearMouse(Mx, My);             // 기존 마우스 위치를 지운다.
                DrawMaze();                     // Maze를 새로 그린다.
                My = My + 17;                   // 마우스 위치를 한칸 아래로 내린다.
                DrawMouse(Mx, My);              // 마우스를 그린다.

                Dcount++;                       // Down할 때마다 count 증가
                if (Dcount > 4)                 // count가 4보다 크면 0으로 초기화한다.
                    Dcount = 0;
                LCD_DisplayChar(10, 7, Dcount + '0');             // Dcount 횟수를 화면에 출력한다.     
                LCD_DisplayChar(5, 24, ((My - 21) / 17) + '0'); // 마우스 y좌표를 출력한다.

                GPIOG->BSRRH = 0x007E;          // LED0, 7 빼고 전부 OFF
                GPIOG->BSRRL = 0x0004;          // LED2 ON
                break;

            case NAVI_LEFT:                     // Joystick LEFT
                if ((Mx - 1) / 17 == 0)         // x= 0인 상태에서 LEFT을 하면 부저가 두 번 울린다.
                {
                    BEEP();
                    DelayMS(500);
                    BEEP();
                    break;
                }
                else
                    BEEP();                     // 이동 시 부저가 한 번 울린다.

                ClearMouse(Mx, My);             // 기존 마우스 위치를 지운다.
                DrawMaze();                     // Maze를 새로 그린다.
                Mx = Mx - 17;                   // 마우스 위치를 한칸 왼쪽으로 이동한다.
                DrawMouse(Mx, My);              // 마우스를 그린다.

                Lcount++;                       // LEFT할 때마다 count 증가
                if (Lcount > 4)                 //  count가 4보다 크면 0으로 초기화한다.
                    Lcount = 0;
                LCD_DisplayChar(10, 11, Lcount + '0');            //  Lcount 횟수를 화면에 출력한다.
                LCD_DisplayChar(5, 22, ((Mx - 1) / 17) + '0');  //  마우스 x좌표를 출력한다.

                GPIOG->BSRRH = 0x007E;          // LED0, 7 뺴고 전부 OFF
                GPIOG->BSRRL = 0x0008;          // LED3 ON
                break;

            case NAVI_RIGHT:                    // Joystick RIGHT	
                if ((Mx - 1) / 17 == 4)         // x = 4인 상태에서 RIGHT을 하면 부저가 두 번 울린다.
                {
                    BEEP();
                    DelayMS(500);
                    BEEP();
                    break;
                }
                else
                    BEEP();                     // 이동 시 부저가 한 번 울린다

                ClearMouse(Mx, My);             // 기존 마우스 위치를 지운다.
                DrawMaze();                     // Maze를 새로 그린다.
                Mx = Mx + 17;                   // 마우스 위치를 한칸 오른쪽으로 이동한다.
                DrawMouse(Mx, My);              // 마우스를 그린다.

                Rcount++;                       // RIGHT할 때마다 count 증가
                if (Rcount > 4)                 //  count가 4보다 크면 0으로 초기화한다.
                    Rcount = 0;
                LCD_DisplayChar(10, 15, Rcount + '0');            //  Rcount 횟수를 화면에 출력한다.
                LCD_DisplayChar(5, 22, ((Mx - 1) / 17) + '0');  //  마우스 x좌표를 출력한다.

                GPIOG->BSRRH = 0x007E;          // LED0, 7 뺴고 전부 OFF
                GPIOG->BSRRL = 0x0010;          // LED4 ON
                break;
            }  // switch(JOY_Scan())

            if ((Mx - 1) / 17 == 4 && (My - 21) / 17 == 4)     // 도착 지점은 (4, 4)에 도달하면 부저 1 + 2번 울린다.
            {
                DelayMS(500);
                BEEP();
                DelayMS(500);
                BEEP();
                DelayMS(500);
                BEEP();
                while (1)                           // SW1을 누르면 반복문을 탈출한다.
                {
                    if (KEY_Scan() == 0xFE00)
                    {
                        DisplayInitScreen();        // 처음 화면으로 돌아간다.
                        GPIOG->BSRRH = 0x00FE;      // LED0을 제외한 모든 LED OFF
                        break;
                    }
                }
            }
        }
    }  // while(1)
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer)) 초기 설정	*/
void _GPIO_Init(void)
{
    // LED (GPIO G) 설정
    RCC->AHB1ENR |= 0x00000040;                 // RCC_AHB1ENR : GPIOG(bit#6) Enable							
    GPIOG->MODER &= 0xFFFF0000;                  // GPIOG 0~7 : Clear (0b00)			
    GPIOG->MODER |= 0x00005555;                 // GPIOG 0~7 : Output mode (0b01)						
    GPIOG->OTYPER &= ~0x00FF;                     // GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
    GPIOG->OSPEEDR &= ~0x0000FFFF;                 // GPIOG 0~7 : Clear (0b00)		 	
    GPIOG->OSPEEDR |= 0x00005555;                 // GPIOG 0~7 : Output speed 25MHZ Medium speed 

    // SW (GPIO H) 설정 
    RCC->AHB1ENR |= 0x00000080;                 // RCC_AHB1ENR : GPIOH(bit#7) Enable							
    GPIOH->MODER &= ~0xFFFF0000;                 // GPIOH 8~15 : Input mode (reset state)				
    GPIOH->PUPDR &= ~0xFFFF0000;                 // GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

    // Buzzer (GPIO F) 설정 
    RCC->AHB1ENR |= 0x00000020;                 // RCC_AHB1ENR : GPIOF(bit#5) Enable							
    GPIOF->MODER &= ~0x000C0000;                 // GPIOF 9 : Clear (0b00)
    GPIOF->MODER |= 0x00040000;                 // GPIOF 9 : Output mode (0b01)						
    GPIOF->OTYPER &= ~0x0200;                     // GPIOF 9 : Push-pull  	
    GPIOF->OSPEEDR &= ~0x000C0000;                 // GPIOF 9 : Clear (0b00)
    GPIOF->OSPEEDR |= 0x00040000;                 // GPIOF 9 : Output speed 25MHZ Medium speed 

    // Joystick (GPIO I) 설정
    RCC->AHB1ENR |= 0x00000100;                  // RCC_AHB1ENR GPIOI Enable
    GPIOI->MODER &= ~0x000FFC00;                 // GPIOI 5~9 : Input mode (reset state)
    GPIOI->PUPDR &= ~0x000FFC00;                 // GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}

/* GLCD 초기화면 설정 함수 */
void DisplayInitScreen(void)
{
    Ucount = 0, Dcount = 0, Lcount = 0, Rcount = 0;
    Mx = 1, My = 21;
    flag = 0;

    LCD_Clear(RGB_WHITE);                           // 화면 클리어
    LCD_SetFont(&Gulim7);                           // 폰트 : 굴림 7
    LCD_SetBackColor(RGB_YELLOW);                   // 글자배경색 : Yellow
    LCD_SetTextColor(RGB_BLUE);                     // 글자색 : Blue
    LCD_DisplayText(0, 0, "Maze Runner (BJW)");       // Title

    LCD_SetBackColor(RGB_WHITE);                    // 글자배경색 : White
    LCD_SetTextColor(RGB_BLACK);                    // 글자색 : Black
    LCD_DisplayText(1, 1, "(0,0)");
}