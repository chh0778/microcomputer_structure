#include "stm32f4xx.h"
#include "GLCD.h"

void _GPIO_Init(void);
void _EXTI_Init(void);

uint16_t KEY_Scan(void);
void BEEP(void);

void DisplayInitScreen(void);
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

void SelectMode(void);          // 층선택 모드 실행 함수


uint8_t	SW7_Flag = 0;         // SW7입력 정보 저장 변수 선언 (초기값은 0으로 설정)
uint16_t blue_pointy = 88, red_pointy = 88; // 왼쪽(파랑)과 오른쪽(빨강)의 상태바 y좌표 저장 변수 (초기값은 88로 설정)
uint16_t blue_flag = 1, red_flag = 1;           // 왼쪽(파랑)과 오른쪽(빨강)의 현재 층을 저장하기 위한 정수형변수 (초기값은 1층으로 설정)
// LCD에 사각형을 그릴 때 사각형의 높이를 계산하기 위해 사용
uint16_t count = 0;             // 층 선택 횟수가 홀수인지 짝수인지 저장하는 변수 (0이면 초기상태, 1이면 홀수, 2이면 짝수)
uint16_t status = 0;            // 현재 동작하는 엘리베이터가 왼쪽인지 오른쪽인지 상태를 저장하는 변수 (0이면 파란색, 1이면 빨간색으로 글자색 설정)

char status1 = 'L', status2 = 'S', start = '1', end = '1', left_floor = '1', right_floor = '1';    //  LCD 출력 문자 저장 변수
// status1은 현재 엘리베이터가 왼쪽인지 오른쪽인지 저장 (왼쪽이면 'L', 오른쪽이면 'R')
// status2는 현재 엘리베이터가 어떤 동작을 수행하고 있는지 저장 (정지상태이면 'S', 올라가고 있으면 'U', 내려가고 있으면 'D')
// start는 선택한 출발층 저장, end는 선택한 목표층 저장
// left_floor, right_floor는 왼쪽과 오른쪽의 현재 층을 저장하기 위한 문자형 변수 (start, end와 비교하기 위해 사용)

char *mode = "FL";      // LCD 출력 문자열 저장 변수
// 현재 모드를 저장 (층선택 모드는 "FL", 실행 모드는 "EX", 중단 모드는 "HL" 저장,,, 초기값은 "FL"로 설정)

int main(void)
{
    _GPIO_Init(); // GPIO (LED & SW) 초기화
    _EXTI_Init();   // EXTI(외부 인터럽트) 초기화
    LCD_Init();	// LCD 모듈 초기화
    DelayMS(100);

    GPIOG->ODR |= 0x0001;	// 초기값: LED0ON
    DisplayInitScreen();	// LCD 초기화면
 
   while(1)
    {
        if(SW7_Flag == 0)                       // SW7 입력이 없는 상태라면
              SelectMode();                     //  층선택모드 실행 함수 호출 후 실행 반복
        
        if(SW7_Flag)    // SW7이 입력되면 실행
        {
              GPIOG->ODR &= ~0x0001;    // LED0 OFF
              GPIOG->ODR |= 0x0080;     // LED7 ON
              mode = "EX";      // MODE를 실행모드 "EX"로 변경
              if(abs(left_floor - start) > abs(right_floor - start))              // 오른쪽 엘리베이터가 왼쪽 엘리베이터 보다 출발층에 더 가까운 경우 실행
              {
                      status = 1;       // 글자색을 빨간색으로 설정하기 위해 1 저장
                      status1 = 'R';    // 동작 엘리베이터는 오른쪽'R' 출력
                      if(right_floor < start)   // 엘리베이터의 현재 층이 출발층보다 낮은 경우 실행
                      {
                              status2 = 'U';    // 엘리베이터 동작은 상승 'U' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                              for(; right_floor < start; right_floor++)         // 엘리베이터의 현재 층이 출발층보다 낮은 동안 반복, 같아지면 반복 탈출
                              {                                                         // 층이 올라가는 동작
                                      red_pointy -= 11;        // 왼쪽 엘리베이터의 상태바 1층 상승 (LCD의 좌표는 위로 갈수록 숫자가 줄어든다)
                                      red_flag++;               // 왼쪽 엘리베이터 1층 상승
                                      DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                                      DelayMS(500);     // 0.5초 정지
                              }
                              status2 = 'S';    // 엘리베이터 동작은 정지 'S' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                      }
                      
                      else if(right_floor > start)   // 엘리베이터의 현재 층이 출발층보다 높은 경우 실행
                      {
                              status2 = 'D';    // 엘리베이터 동작은 하강 'D' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                              for(; right_floor > start; right_floor--)         // 엘리베이터의 현재 층이 출발층보다 높은 동안 반복, 같아지면 반복 탈출
                              {                                                         // 층이 내려가는 동작
                                      red_pointy += 11;        // 왼쪽 엘리베이터의 상태바 1층 하강 (LCD의 좌표는 아래로 갈수록 숫자가 늘어난다)
                                      red_flag--;               // 왼쪽 엘리베이터 1층 하강
                                      DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                                      DelayMS(500);     // 0.5초 정지
                              }
                              status2 = 'S';    // 엘리베이터 동작은 정지 'S' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                      }
                      
                      if(right_floor < end)   // 엘리베이터의 현재 층이 목표층보다 낮은 경우 실행
                      {
                              status2 = 'U';    // 엘리베이터 동작은 상승 'U' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                              for(; right_floor < end; right_floor++)         // 엘리베이터의 현재 층이 목표층보다 낮은 동안 반복, 같아지면 반복 탈출
                              {                                                         // 층이 올라가는 동작
                                      red_pointy -= 11;        // 왼쪽 엘리베이터의 상태바 1층 상승 (LCD의 좌표는 위로 갈수록 숫자가 줄어든다)
                                      red_flag++;               // 왼쪽 엘리베이터 1층 상승
                                      DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                                      DelayMS(500);     // 0.5초 정지
                              }
                      }
                      
                      else if(right_floor > end)     // 엘리베이터의 현재 층이 목표층보다 높은 경우 실행
                      {
                              status2 = 'D';    // 엘리베이터 동작은 하강 'D' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                              for(; right_floor > end; right_floor--)           // 엘리베이터의 현재 층이 목표층보다 높은 동안 반복, 같아지면 반복 탈출
                              {                                                         // 층이 내려가는 동작
                                      red_pointy += 11;        // 왼쪽 엘리베이터의 상태바 1층 하강 (LCD의 좌표는 아래로 갈수록 숫자가 늘어난다)
                                      red_flag--;               // 왼쪽 엘리베이터 1층 하강
                                      DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                                      DelayMS(500);     // 0.5초 정지
                              }
                      }
              }
              
              else                            // 그 외 경우 즉, 왼쪽 엘리베이터가 오른쪽 엘리베이터 보다 출발층에 더 가깝거나 두 엘리베이터의 거리가 같은 경우 실행
              {
                      status = 0;       // 글자색을 파란색으로 설정하기 위해 0 저장
                      status1 = 'L'; // 동작 엘리베이터는 오른쪽'R' 출력
                      if(left_floor < start)    // 엘리베이터의 현재 층이 출발층보다 낮은 경우 실행
                      {
                              status2 = 'U';    // 엘리베이터 동작은 상승 'U' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                              for(; left_floor < start; left_floor++)           // 엘리베이터의 현재 층이 출발층보다 낮은 동안 반복, 같아지면 반복 탈출
                              {                                                         // 층이 올라가는 동작
                                      blue_pointy -= 11;        // 오른쪽 엘리베이터의 상태바 1층 상승 (LCD의 좌표는 위로 갈수록 숫자가 줄어든다)
                                      blue_flag++;               // 오른쪽 엘리베이터 1층 상승
                                      DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                                      DelayMS(500);     // 0.5초 정지
                              }
                              status2 = 'S';    // 엘리베이터 동작은 정지 'S' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                      }
                      
                      else if(left_floor > start)   // 엘리베이터의 현재 층이 출발층보다 높은 경우 실행
                      {
                              status2 = 'D';    // 엘리베이터 동작은 하강 'D' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                              for(; left_floor > start; left_floor--)         // 엘리베이터의 현재 층이 출발층보다 높은 동안 반복, 같아지면 반복 탈출
                              {                                                         // 층이 내려가는 동작
                                      blue_pointy += 11;        // 오른쪽 엘리베이터의 상태바 1층 하강 (LCD의 좌표는 아래로 갈수록 숫자가 늘어난다)
                                      blue_flag--;               // 오른쪽 엘리베이터 1층 하강
                                      DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                                      DelayMS(500);     // 0.5초 정지
                              }
                              status2 = 'S';    // 엘리베이터 동작은 정지 'S' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                      }
                      
                      if(left_floor < end)   // 엘리베이터의 현재 층이 목표층보다 낮은 경우 실행
                      {
                              status2 = 'U';    // 엘리베이터 동작은 상승 'U' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                              for(; left_floor < end; left_floor++)           // 엘리베이터의 현재 층이 목표층보다 낮은 동안 반복, 같아지면 반복 탈출
                              {                                                         // 층이 올라가는 동작
                                      blue_pointy -= 11;        // 오른쪽 엘리베이터의 상태바 1층 상승 (LCD의 좌표는 위로 갈수록 숫자가 줄어든다)
                                      blue_flag++;               // 오른쪽 엘리베이터 1층 상승
                                      DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                                      DelayMS(500);     // 0.5초 정지
                              }
                      }
                      
                      else if(left_floor > end)   // 엘리베이터의 현재 층이 목표층보다 높은 경우 실행
                      {
                              status2 = 'D';    // 엘리베이터 동작은 하강 'D' 출력
                              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                              DelayMS(500);     // 0.5초 정지
                              for(; left_floor > end; left_floor--)         // 엘리베이터의 현재 층이 목표층보다 높은 동안 반복, 같아지면 반복 탈출
                              {                                                         // 층이 내려가는 동작
                                      blue_pointy += 11;        // 오른쪽 엘리베이터의 상태바 1층 하강 (LCD의 좌표는 아래로 갈수록 숫자가 늘어난다)
                                      blue_flag--;               // 오른쪽 엘리베이터 1층 하강
                                      DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                                      DelayMS(500);     // 0.5초 정지
                              }
                      }
              }
              
              status2 = 'S';    // 엘리베이터 동작은 정지 'S' 출력
              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
              DelayMS(500);     // 0.5초 정지
              for(int i = 0; i < 3; i++)        // 부저 3번 울리기 위한 반복문
              {
                      BEEP();
                      DelayMS(100);
              }
              
              mode = "FL";      // MODE를 층선택 모드 "FL"로 변경
              DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
              GPIOG->ODR |= 0x0001;     // LED0 ON
              GPIOG->ODR &= ~0x00FE;    // LED0를 제외한 모든 LED OFF
              SW7_Flag = 0;             // SW7 입력 정보 초기화 (0으로 저장)
        }
    }
}

void SelectMode(void)           // 층선택 모드 실행 함수
{
        switch(KEY_Scan())
        {
        	case 0xFD00 : 	//SW1이 눌리면 break까지 실행
                        GPIOG->ODR &= ~0x007E;  // LED0를 제외한 모든 LED OFF
                        GPIOG->ODR |= 0x0002;   // 선택한 층의 LED ON (LED1 ON)
                        
                        if((count == 0) || (count == 2))        // 초기상태 이거나(count == 0) 짝수번째 입력한 상태이면(count == 2) 실행
                        {
                            start = '1';        // 출발층에 '1'저장
                            count = 1;          // 홀수번째 입력 상태
                        }
                        else    // 그 외 모든 경우 (홀수 번째 입력인 경우(count == 1))
                        {
                            end = '1';          // 목표층에 '1'저장
                            if(start == end)    // 이때 만약 출발층과 목표층이 같다면
                                break;          // 스위치문 종료후 스위치 재입력
                            count = 2;          // 짝수번째 입력 상태
                        }
                        
                        DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                        BEEP();     // 부저 1번 울림
                        
          	break;
                
        	case 0xFB00 : 	//SW2이 눌리면 break까지 실행
                        GPIOG->ODR &= ~0x007E;   // LED0를 제외한 모든 LED OFF
                        GPIOG->ODR |= 0x0004;   // 선택한 층의 LED ON (LED2 ON)
                        
                        if((count == 0) || (count == 2))        // 초기상태 이거나(count == 0) 짝수번째 입력한 상태이면(count == 2) 실행
                        {
                            start = '2';        // 출발층에 '2'저장
                            count = 1;          // 홀수번째 입력 상태
                        }
                        else
                        {
                            end = '2';          // 목표층에 '2'저장
                            if(start == end)    // 이때 만약 출발층과 목표층이 같다면
                                break;          // 스위치문 종료후 스위치 재입력
                            count = 2;          // 짝수번째 입력 상태
                        }
                        
                        DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                        BEEP();     // 부저 1번 울림
                        
          	break;
                
        	case 0xF700 : 	//SW3이 눌리면 break까지 실행
                        GPIOG->ODR &= ~0x007E;   // LED0를 제외한 모든 LED OFF
                        GPIOG->ODR |= 0x08;   // 선택한 층의 LED ON (LED3 ON)
                        
                        if((count == 0) || (count == 2))        // 초기상태 이거나(count == 0) 짝수번째 입력한 상태이면(count == 2) 실행
                        {
                            start = '3';        // 출발층에 '3'저장
                            count = 1;          // 홀수번째 입력 상태
                        }
                        else
                        {
                            end = '3';          // 목표층에 '3'저장
                            if(start == end)    // 이때 만약 출발층과 목표층이 같다면
                                break;          // 스위치문 종료후 스위치 재입력
                            count = 2;          // 짝수번째 입력 상태
                        }
                        
                        DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                        BEEP();     // 부저 1번 울림
                        
          	break;
                
        	case 0xEF00 : 	//SW4이 눌리면 break까지 실행
                        GPIOG->ODR &= ~0x007E;   // LED0를 제외한 모든 LED OFF
                        GPIOG->ODR |= 0x10;   // 선택한 층의 LED ON (LED4 ON)
                        
                        if((count == 0) || (count == 2))        // 초기상태 이거나(count == 0) 짝수번째 입력한 상태이면(count == 2) 실행
                        {
                            start = '4';        // 출발층에 '4'저장
                            count = 1;          // 홀수번째 입력 상태
                        }
                        else
                        {
                            end = '4';          // 목표층에 '4'저장
                            if(start == end)    // 이때 만약 출발층과 목표층이 같다면
                                break;          // 스위치문 종료후 스위치 재입력
                            count = 2;          // 짝수번째 입력 상태
                        }
                        
                        DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                        BEEP();     // 부저 1번 울림
                        
          	break;
                
        	case 0xDF00 : 	//SW5이 눌리면 break까지 실행
                        GPIOG->ODR &= ~0x007E;   // LED0를 제외한 모든 LED OFF
                        GPIOG->ODR |= 0x20;   // 선택한 층의 LED ON (LED5 ON)
                        
                        if((count == 0) || (count == 2))        // 초기상태 이거나(count == 0) 짝수번째 입력한 상태이면(count == 2) 실행
                        {
                            start = '5';        // 출발층에 '5'저장
                            count = 1;          // 홀수번째 입력 상태
                        }
                        else
                        {
                            end = '5';          // 목표층에 '5'저장
                            if(start == end)    // 이때 만약 출발층과 목표층이 같다면
                                break;          // 스위치문 종료후 스위치 재입력
                            count = 2;          // 짝수번째 입력 상태
                        }
                        
                        DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                        BEEP();     // 부저 1번 울림
                        
          	break;
                
        	case 0xBF00 : 	//SW6이 눌리면 break까지 실행
                        GPIOG->ODR &= ~0x007E;          // LED0를 제외한 모든 LED OFF   
                        GPIOG->ODR |= 0x40;   // 선택한 층의 LED ON (LED6 ON)
                        
                        if((count == 0) || (count == 2))        // 초기상태 이거나(count == 0) 짝수번째 입력한 상태이면(count == 2) 실행
                        {
                            start = '6';        // 출발층에 '6'저장
                            count = 1;          // 홀수번째 입력 상태
                        }
                        else
                        {
                            end = '6';          // 목표층에 '6'저장
                            if(start == end)    // 이때 만약 출발층과 목표층이 같다면
                                break;          // 스위치문 종료후 스위치 재입력
                            count = 2;          // 짝수번째 입력 상태
                        }
                        
                        DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
                        BEEP();     // 부저 1번 울림
                        
          	break;
        }
}

void _GPIO_Init(void)
{
	// LED (GPIO G) 설정
    	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x000000FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) 설정 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
    	RCC->AHB1ENR	|=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER 	|=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x00000200;	// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}

void _EXTI_Init(void)
{
    	RCC->AHB1ENR 	|= 0x00000080;	// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR 	|= 0x00004000;	// Enable System Configuration Controller Clock
	
	GPIOH->MODER 	&= 0x0000FFFF;	// GPIOH PIN8~PIN15 Input mode (reset state)			
        
	SYSCFG->EXTICR[2] |= 0x0007; 	// EXTI8에 대한 소스 입력은 GPIOH로 설정 (EXTICR3) (reset value: 0x0000)	
        SYSCFG->EXTICR[3] |= 0x7000; 	// EXTI15에 대한 소스 입력은 GPIOH로 설정 (EXTICR4) (reset value: 0x0000)
	
	EXTI->FTSR |= 0x00008100;		// Falling Trigger Enable  (EXTI8:PH8 || EXTI15:PH15)
    	EXTI->IMR  |= 0x00008100;  	// EXTI8,15 인터럽트 mask (Interrupt Enable)
		
	NVIC->ISER[0] |= ( 1 << 23  );   // Enable Interrupt EXTI8 Vector table Position 참조
        NVIC->ISER[1] |= ( 1 << 8  );   // Enable Interrupt EXTI15 Vector table Position 참조
}

void EXTI9_5_IRQHandler(void)		// EXTI 5~9 인터럽트 핸들러
{
    if(SW7_Flag)
    {
        if(EXTI->PR & 0x0100) 		// EXTI8 Interrupt Pending?
        {
            EXTI->PR |= 0x0100; 		// Pending bit Clear
            mode = "HL";      // MODE를 중단 모드 "HL"로 변경
            DisplayInitScreen();      // 변경된 정보로 LCD 초기화 후 재출력
            GPIOG->ODR &= ~0x0081;      // LED0, LED7 OFF, 나머지 LED는 상태 유지
            BEEP();
            
            for(int i = 0; i < 10; i++)          // 5초 유지를 위한 반복문, 0.5초간격으로 부저 울림
            {
                  DelayMS(500);     // 0.5초 정지
                  BEEP();       
            }
            
            DelayMS(500);     // 0.5초 정지
            mode = "EX";      // MODE를 실행 모드 "EX"로 변경
        }
    }
}

void EXTI15_10_IRQHandler(void)		// EXTI 10~15 인터럽트 핸들러
{
        if(EXTI->PR & 0x8000) 		// EXTI15 Interrupt Pending?
        {
            EXTI->PR |= 0x8000; 		// Pending bit Clear            bit 자리에 1을 넣어야 clear 된다.
            SW7_Flag = 1;	                // SW7이 눌리면 SW7_Flag에 1저장 (SW7이 눌렸다는 정보 저장)
            BEEP();     // 부저 1번 울림
        }
}

int y_point = 2;
void DisplayInitScreen(void)    // LCD 화면 출력 함수
{
    LCD_Clear(RGB_YELLOW);	// 화면 클리어 (배경색 : Yellow)
    LCD_SetFont(&Gulim8);	// 폰트 : 굴림 8
    LCD_SetBackColor(RGB_YELLOW);       // 배경색 : Yellow
    
    // 문자열 또는 문자 출력 설정
    LCD_SetTextColor(RGB_GRAY);// 글자색 : Gray
    LCD_DisplayText(0,0," Mecha-Elevator");  // 0번줄 Title : Mecha-Elevator

    //  왼쪽 엘리베이터 층 출력
    LCD_SetTextColor(RGB_BLUE);// 글자색 : Blue
    LCD_DisplayText(0,16, mode);
    y_point = 2;
    for(int i = 6; i >0; i--)
    {
          LCD_DisplayChar(y_point, 4, i + 0x30);
          y_point++;
    }
    /////////////////////////////////
    
    if(status == 0)     // status가 0이면 실행
          LCD_SetTextColor(RGB_BLUE);// 글자색 : Blue
    
    else        // 그 외의 경우 즉, status가 1이면 실행
          LCD_SetTextColor(RGB_RED);// 글자색 : Red
  
    // 현재 엘리베이터 모드 출력
    // 초기값은 L:S
    LCD_DisplayChar(4, 8, status1);
    LCD_DisplayChar(4, 9, ':');
    LCD_DisplayChar(4, 10, status2);
    /////////////////////////////////
    
    // 오른쪽 엘리베이터 층 출력
    LCD_SetTextColor(RGB_RED);// 글자색 : Red
    y_point = 2;
    for(int i = 6; i > 0; i--)
    {
          LCD_DisplayChar(y_point, 14, i + 0x30);
          y_point++;
    }
    /////////////////////////////////
    
    // 현재 입력된 출발층과 목표층 출력
    // 초기값은 1->1
    LCD_SetTextColor(RGB_BLACK);// 글자색 : Black
    LCD_DisplayChar(5, 8, start);
    LCD_DisplayText(5, 9, "->");
    LCD_DisplayChar(5, 11, end);
    
    // 엘리베이터 상태바 내부 색 채우기 설정
    // 왼쪽
    LCD_SetBrushColor(RGB_BLUE);
    LCD_DrawFillRect(15, blue_pointy, 10, 11 * blue_flag);
    
    // 오른쪽
    LCD_SetBrushColor(RGB_RED);
    LCD_DrawFillRect(125, red_pointy, 10, 11 * red_flag);
    ////////////////////////////////////////////////////////////
    
    // 엘리베이터 상태바 테두리 출력
    LCD_SetPenColor(RGB_GREEN);
    LCD_DrawRectangle(15, blue_pointy, 10, 11 * blue_flag);     // 왼쪽
    LCD_DrawRectangle(125, red_pointy, 10, 11 * red_flag);      // 오른쪽
    ////////////////////////////////////////////////////////////////
    
}

uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7 
{ 
	uint16_t key;
	key = GPIOH->IDR & 0xFF00;	// any key pressed ?
	if(key == 0xFF00)		// if no key, check key off
	{  	if(key_flag == 0)
        		return key;
      		else
		{	DelayMS(10);
        		key_flag = 0;
        		return key;
        	}
    	}
  	else				// if key input, check continuous key
	{	if(key_flag != 0)	// if continuous key, treat as no key input
        		return 0xFF00;
      		else			// if new key,delay for debounce
		{	key_flag = 1;
			DelayMS(10);
 			return key;
        	}
	}
}

void BEEP(void)			/* beep for 30 ms */
{ 	
	GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);		// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}

void DelayMS(unsigned short wMS)
{
    register unsigned short i;
    for (i=0; i<wMS; i++)
        DelayUS(1000);         		// 1000us => 1ms
}

void DelayUS(unsigned short wUS)
{
    volatile int Dly = (int)wUS*17;
    for(; Dly; Dly--);
}