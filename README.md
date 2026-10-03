# STM32F407 베어메탈 프로젝트 모음

> HAL 없이 레지스터를 직접 설정해 GPIO, EXTI, NVIC, 타이머, ADC, DMA, USART를 다룬 마이크로컴퓨터 수업 프로젝트 8개

| 항목 | 내용 |
| --- | --- |
| 보드 | STM32F407 기반 학교 제공 전용 실습 보드 (LCD, 스위치 8개, LED 8개, 조이스틱, 부저, 가변저항, FRAM) |
| 방식 | CMSIS 레지스터 정의(`stm32f4xx.h`)만 사용, HAL 미사용 |
| 개발 환경 | IAR Embedded Workbench for ARM |
| 수업 | 2025년 3학년 1학기 마이크로컴퓨터 구조, 2학기 마이크로컴퓨터 응용 |
| 참고 문서 | RM0090 (STM32F405/407 레퍼런스 매뉴얼), STM32F407 데이터시트, PM0214 (Cortex-M4 프로그래밍 매뉴얼) |

## 프로젝트 목록

| 폴더 | 프로젝트 | 핵심 주변장치 |
| --- | --- | --- |
| [01_maze_runner](01_maze_runner) | 조이스틱으로 LCD 미로의 마우스 이동 | GPIO 입출력, BSRR, 디바운싱 |
| [02_elevator](02_elevator) | 0~5층 엘리베이터 제어 | EXTI, RCC PLL 클럭 변경 |
| [03_binary_calculator](03_binary_calculator) | 2비트 이진 연산기 | EXTI 5개, **NVIC 우선순위와 인터럽트 선점**, FRAM 저장 |
| [04_interstellar_clock](04_interstellar_clock) | 속도가 다른 두 행성 시계 | 타이머 업데이트 인터럽트, OC 토글, 실행 중 주기 변경 |
| [05_motor_signal](05_motor_signal) | 스텝·DC 모터 구동 신호 발생 | **타이머 외부 클럭 모드(슬레이브)**, OC 펄스 개수 출력, PWM, ADC |
| [06_sensor_monitoring](06_sensor_monitoring) | 온도센서 3개 모니터링과 PC 전송 | **ADC 스캔 + 타이머 트리거 + DMA**, USART 인터럽트 |
| [07_golf_game](07_golf_game) | 키트를 기울여 공을 굴리는 골프 게임 | **SPI로 가속도센서 읽기**, 타이머 2개 |
| [08_tracking_car](08_tracking_car) | 앞차와 인도 거리로 속도·방향을 제어하는 추종차 (텀 프로젝트) | ADC + DMA, **PWM 2개**, USART, EXTI, FRAM 종합 |

## 보드 주요 핀

보드 회로도를 보고 프로젝트에서 사용한 핀만 정리했습니다.

| 장치 | 핀 | 비고 |
| --- | --- | --- |
| LED0~7 | PG0~7 | |
| 스위치 SW0~7 | PH8~15 | 누르면 Low |
| 조이스틱 | PI5~9 | PUSH, UP, DOWN, RIGHT, LEFT 순 |
| 부저 | PF9 | TIM14_CH1로 PWM 출력 가능 |
| 가변저항 | PA1 | ADC_IN1 |
| 가속도센서 | PA5 (SCK), PA6 (MISO), PA7 (MOSI), PA8 (CS) | SPI1 |
| 외부 아날로그 입력 | PB0, PF3 | ADC12_IN8, ADC3_IN9, 확장 헤더로 연결 |
| USART1 | PA9 (TX), PA10 (RX) | PC 통신 |
| FRAM (MB85RS64, 8 KB) | PI0 (CS), PI1 (SCK), PI2 (MISO), PI3 (MOSI) | SPI2 |
| 타이머 출력 확인 | PB7 (TIM4_CH2), PE13 (TIM1_CH3) | 오실로스코프로 펄스 확인 |

## 주변장치별로 보기

| 주변장치 | 다룬 프로젝트 |
| --- | --- |
| GPIO | 전체 |
| EXTI, SYSCFG | 02, 03, 04, 07, 08 |
| NVIC 우선순위 | 03 |
| RCC (PLL) | 02 |
| 타이머 (업데이트, OC, PWM, 외부 클럭) | 04, 05, 06, 07, 08 |
| ADC (단일, 스캔, 외부 트리거, 내부 온도센서) | 05, 06, 08 |
| DMA | 06, 08 |
| USART | 06, 08 |
| SPI | 07 (가속도센서), 03·08 (FRAM, 드라이버 제공) |

## 구현 범위

LCD(`GLCD`), FRAM, 가속도센서(`ACC`) 드라이버, 그리고 각 과제의 기본 프로젝트 틀은 수업에서 제공되었습니다. 이 레포에는 직접 작성한 `main.c`만 포함했으며, 제공된 드라이버 파일은 넣지 않았습니다. 그래서 이 레포만으로는 빌드되지 않습니다.

## 이후 프로젝트와의 연결

여기서 익힌 기능을 이후 [졸업작품 AGV](https://github.com/juwon1113/AGV-Logistics-Firmware)에서 응용했습니다.

| 이 레포 | 졸업작품 AGV |
| --- | --- |
| 05: 타이머 외부 클럭 모드로 스위치 펄스 개수 세기 | 리프트: TIM5 슬레이브가 TIM2의 스텝 펄스 개수를 하드웨어로 세서 정확한 스텝 수만큼 구동 |
| 06: ADC 결과를 DMA로 메모리에 자동 저장 | 라즈베리파이 UART 프레임을 IDLE 감지 DMA로 수신 |
| 04: 인터럽트에서는 플래그만 세우고 처리는 main에서 | 10 ms 타이머 인터럽트가 플래그를 세우고 메인 루프에서 제어 연산 |
