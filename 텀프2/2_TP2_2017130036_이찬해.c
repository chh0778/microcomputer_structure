#include "stm32f4xx.h"
#include "GLCD.h"
#include "FRAM.h"

#define SW0    0xFE00  //PH8
#define SW1    0xFD00  //PH9
#define SW2    0xFB00  //PH10
#define SW3    0xF700  //PH11
#define SW4    0xEF00  //PH8
#define SW5    0xDF00  //PH9
#define SW6    0xBF00  //PH10
#define SW7    0x7F00  //PH11

#define LED0  0x0001    //PG0
#define LED1  0x0002    //PG1
#define LED2  0x0004    //PG2
#define LED3  0x0008    //PG3
#define LED4  0x0010    //PG4
#define LED5  0x0020    //PG5
#define LED6  0x0040    //PG6
#define LED7  0x0080    //PG7

#define NAVI_PUSH	0x03C0  //PI5 0000 0011 1100 0000 
#define NAVI_UP		0x03A0  //PI6 0000 0011 1010 0000 
#define NAVI_DOWN        0x0360  //PI7 0000 0011 0110 0000 
#define NAVI_RIGHT	0x02E0  //PI8 0000 0010 1110 0000 
#define NAVI_LEFT	0x01E0  //PI9 0000 0001 1110 0000 

void _GPIO_Init(void);                  
void _EXTI_Init(void);

void DisplayInitScreen(void);
uint16_t KEY_Scan(void);
uint16_t JOY_Scan(void);
uint8_t  SW0_Flag, SW7_Flag, Joy_Push_Flag;

//부저함수들
void BEEP(void);
void BEEP3(void);
void BEEP5(void);

//대기함수들
void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

//승리버튼 누를 시, 프로그램재시작하는 함수
void ProgramRestart(void);

//적돌과 청돌의 좌표 변수들
int red_stone_x=5;
int red_stone_y=5;
int blue_stone_x=5;
int blue_stone_y=5;
int count=0;

//해당좌표에 돌이 있는지 여부를 확인하는 2차원 배열
int check_stone[10][10]={0};
//적돌과 청돌의 게임 점수를 기록하는 변수
int red_score;
int blue_score;

int main(void)
{
  _GPIO_Init(); 	          // GPIO (LED,SW,Buzzer)초기화
  _EXTI_Init();                  //EXTI 초기화
  LCD_Init();	                 // LCD 모듈 초기화
  Fram_Init();                    // FRAM 초기화 H/W 초기화
  Fram_Status_Config();          // FRAM 초기화 S/W 초기화
  DelayMS(10);                  //0.01초 대기
  red_score=Fram_Read(300);     //Fram300주소에 저장돼있는 적돌의 점수를 red_score에 입력
  blue_score=Fram_Read(301);    //Fram301주소에 저장돼있는 청돌의 점수를 blue_score에 입력
  DisplayInitScreen();            // LCD 초기화
  while(1){
    EXTI->IMR  &= ~0x000020;         // EXTI5인터럽트 mask (Interrupt Disable) 설정, 적돌, 청돌 선택대기상태일 때 JoyStick (EXTI5)키 입력방지
    if(SW0_Flag==1)                     //적돌 차례
    {
      EXTI->IMR  &= ~0x008000;         // EXTI15인터럽트 mask (Interrupt Disable) 설정, 적돌 차례에서 SW7 청돌 차례 키 입력 방지
      if(count%2==0)
      {
        LCD_SetBrushColor(RGB_RED);                  //브러쉬 색: RED
        while(1)        
        {
          if(KEY_Scan()==SW1)                            //SW1 을 누르면 적돌 승
          {
            if(red_score==9)                                  //점수가 9점이었다면 0으로 돌아감
            {
              red_score=0;
              Fram_Write(300,0);                              //Fram 300번 주소에 red_score저장
            }//if(red_score==9)
            else                                              //점수가 9점이 아니라면 1승을 추가한다.
            {
              red_score++;
              Fram_Write(300,red_score);                      //Fram 300번 주소에 red_score저장
            }
            BEEP5();                                          //부저 5회 울리기
            DelayMS(5000);                                    //5초 대기
            ProgramRestart();                                 //프로그램 재시작 함수
            break;
          }//if(!(GPIOH->IDR & ~SW1))
          if(KEY_Scan()==SW6)                            //SW6 을 누르면 청돌 승
          {
            if(blue_score==9)                                  //점수가 9점이었다면 0으로 돌아감
            {
              blue_score=0;
              Fram_Write(301,0);                               //Fram 300번 주소에 blue_score저장
            }//if(blue_score==9)
            else                                               //점수가 9점이 아니라면 1승을 추가한다.
            {
              blue_score++;
              Fram_Write(301,blue_score);                     //Fram 300번 주소에 blue_score저장
            }//if(blue_score!=9)
            BEEP5();                                          //부저 5회 울리기
            DelayMS(5000);                                    //5초 대기
            ProgramRestart();                                 //프로그램 재시작 함수
            break;
          }//if(!(GPIOH->IDR & ~SW2))
          EXTI->IMR |= 0x000020;         // EXTI5인터럽트 mask (Interrupt Enable) 설정, JoyStick 입력모드에서 (EXTI5)활성화
          switch(JOY_Scan())                      //적돌 JoyStick 조작영역
          {
          case NAVI_UP:                                 //Joy Stick UP 버튼 누름
            BEEP();                                       //부저 1회 울리기
            if(red_stone_y>0 && red_stone_y<=9)           //착돌 전 y좌표가 1~9일때에만 동작
              red_stone_y--;
            LCD_DisplayChar(9,4,red_stone_y+48);                //GLCD 적돌 y좌표값 수정
            break;
            
          case NAVI_DOWN:                                //Joy Stick DOWN 버튼 누름
            BEEP();                                       //부저 1회 울리기
            if(red_stone_y>=0 && red_stone_y<9)           //착돌 전 y좌표가 0~8일때에만 동작
              red_stone_y++;
            LCD_DisplayChar(9,4,red_stone_y+48);                //GLCD 적돌 y좌표값 수정
            break;
            
          case NAVI_LEFT:                                //Joy Stick LEFT 버튼 누름
            BEEP();                                       //부저 1회 울리기
            if(red_stone_x>0 && red_stone_x<=9)           //착돌 전 x좌표가 1~9일때에만 동작
              red_stone_x--;
            LCD_DisplayChar(9,2,red_stone_x+48);                 //GLCD 적돌 x좌표값 수정
            break;
            
          case NAVI_RIGHT:                               //Joy Stick RIGHT 버튼 누름
            BEEP();                                       //부저 1회 울리기
            if(red_stone_x>=0 && red_stone_x<9)           //착돌 전 x좌표가 0~8일때에만 동작
              red_stone_x++;
            LCD_DisplayChar(9,2,red_stone_x+48);                 //GLCD 적돌 x좌표값 수정
            break;
          }//switch(JOY_Scan)
          if(Joy_Push_Flag==1)                          //EXTI5 발생, 적돌 착돌 시도
          {
            if(check_stone[red_stone_x][red_stone_y]==1)         //그 자리에 돌이 있으면 경고음을 울리고 좌표지정을 다시한다.
            {
              DelayMS(100);                     //1초 대기
              BEEP3();                          //부저3번울리기
              Joy_Push_Flag=0;                  //JoyStick Push 수행동작이 끝나고 대기상태로 전환
            }// if(check_stone[red_stone_x][red_stone_y]==1)
            else                                                  //그 자리에 돌이 없으면 착돌
            {
              BEEP();                                    //부저 울리기
              check_stone[red_stone_x][red_stone_y]=1;   //착돌 좌표정보 저장, 이 좌표로 착돌을 방지
              LCD_DrawFillRect(35+8*red_stone_x-3, 31+8*red_stone_y-3, 7, 7);    //해당 좌표값에 적돌 착돌
              Joy_Push_Flag=0;                                                    //JoyStick Push 수행동작이 끝나고 대기상태로 전환
              count++;
              EXTI->IMR  &= ~0x000020;         // EXTI5인터럽트 mask (Interrupt Disable) 설정, JoyStickPush가 눌린 후(EXTI5) 비활성화
              break;
            }//else if(check_stone[red_stone_x][red_stone_y]!=1)
          }//if(Joy_Push_Flag==1)
        }//while(1) in SW0, 착돌 전까지 조이스틱 조작 반복
      }//if(count/2==0)
      SW0_Flag=0;
      EXTI->IMR  |= 0x008000;         // EXTI15인터럽트 mask (Interrupt Enable) 설정, 적돌 차례가 끝난 후 청돌 차례 키(EXTI15) 활성화
    }//if(SW0_Flag==1)
    else if(SW7_Flag==1)                  //청돌 조이스틱 조작 시작
    {
      EXTI->IMR  &= ~0x000100;         // EXTI8인터럽트 mask (Interrupt Disable) 설정, 청돌 차례에서 적돌 차례 키(EXTI8) 비활성화
      if(count%2==1)
      {
        LCD_SetBrushColor(RGB_BLUE);                  //브러쉬 색: BLUE
        while(1)
        {
          if(KEY_Scan()==SW1)                            //SW1 을 누르면 적돌 승
          {
            if(red_score==9)                                  //점수가 9점이었다면 0으로 돌아감
            {
              red_score=0;
              Fram_Write(300,0);                              //Fram 300번 주소에 red_score저장
            }//if(red_score==9)
            else                                              //점수가 9점이 아니라면 1승을 추가한다.
            {
              red_score++;
              Fram_Write(300,red_score);                      //Fram 300번 주소에 red_score저장
            }
            BEEP5();                                          //부저 5회 울리기
            DelayMS(5000);                                    //5초 대기
            ProgramRestart();                                 //프로그램 재시작 함수
            break;
          }//if(!(GPIOH->IDR & ~SW1))
          if(KEY_Scan()==SW6)                            //SW6 을 누르면 청돌 승
          {
            if(blue_score==9)                                  //점수가 9점이었다면 0으로 돌아감
            {
              blue_score=0;
              Fram_Write(301,0);                               //Fram 300번 주소에 blue_score저장
            }//if(blue_score==9)
            else                                               //점수가 9점이 아니라면 1승을 추가한다.
            {
              blue_score++;
              Fram_Write(301,blue_score);                     //Fram 300번 주소에 blue_score저장
            }//if(blue_score!=9)
            BEEP5();                                          //부저 5회 울리기
            DelayMS(5000);                                    //5초 대기
            ProgramRestart();                                 //프로그램 재시작 함수
            break;
          }//if(!(GPIOH->IDR & ~SW2))
          EXTI->IMR |= 0x000020;         // EXTI5인터럽트 mask (Interrupt Enable) 설정, JoyStickPush 키(EXTI5) 활성화
          switch(JOY_Scan())                      //청돌 JoyStick 조작영역
          {
          case NAVI_UP:                                 //Joy Stick UP 버튼 누름
            BEEP();                                       //부저 1회 울리기
            if(blue_stone_y>0 && blue_stone_y<=9)         //착돌 전 y좌표가 1~9일 때에만 동작
              blue_stone_y--;
            LCD_DisplayChar(9,16,blue_stone_y+48);                //GLCD 청돌 x좌표값 수정
            break;
            
          case NAVI_DOWN:                                //Joy Stick DOWN 버튼 누름
            BEEP();                                       //부저 1회 울리기
            if(blue_stone_y>=0 && blue_stone_y<9)         //착돌 전 y좌표가 0~8일 때에만 동작
              blue_stone_y++;
            LCD_DisplayChar(9,16,blue_stone_y+48);                //GLCD적돌 y좌표값 수정
            break;
            
          case NAVI_LEFT:                                //Joy Stick LEFT 버튼 누름
            BEEP();                                       //부저 1회 울리기
            if(blue_stone_x>0 && blue_stone_x<=9)         //착돌 전 x좌표가 1~9일 때에만 동작
              blue_stone_x--;
            LCD_DisplayChar(9,14,blue_stone_x+48);                 //GLCD적돌 X좌표값 수정
            break;
            
          case NAVI_RIGHT:                               //Joy Stick RIGHT 버튼 누름
            BEEP();                                       //부저 1회 울리기
            if(blue_stone_x>=0 && blue_stone_x<9)         //착돌 전 x좌표가 0~8일 때에만 동작
              blue_stone_x++;
            LCD_DisplayChar(9,14,blue_stone_x+48);                 //GLCD적돌 X좌표값 수정
            break;
          }//switch(JOY_Scan)
          if(Joy_Push_Flag==1)                          //EXTI5 발생, 청돌 착돌 시도
          {
            if(check_stone[blue_stone_x][blue_stone_y]==1)         //그 자리에 돌이 있으면 경고음을 울리고 좌표지정을 다시한다.
            {
              DelayMS(100);                     //1초 대기
              BEEP3();                          //부저3번울리기
              Joy_Push_Flag=0;                    //JoyStick Push 수행동작이 끝나고 대기상태로 전환
            }// if(check_stone[blue_stone_x][blue_stone_y]==1)
            else                                                  //그 자리에 돌이 없으면 착돌
            {
              BEEP();                                    //부저 울리기
              check_stone[blue_stone_x][blue_stone_y]=1;   //착돌 좌표정보 저장, 이 좌표로 착돌을 방지
              LCD_DrawFillRect(35+8*blue_stone_x-3, 31+8*blue_stone_y-3, 7, 7);    //해당 좌표값에 청돌 착돌
              Joy_Push_Flag=0;                            //JoyStick Push 수행동작이 끝나고 대기상태로 전환
              count++;
              EXTI->IMR  &= ~0x000020;         // EXTI5인터럽트 mask (Interrupt Disable) 설정, 착돌 후 JoyStickPush(EXTI5) 비활성화
              break;
            }//else if(check_stone[blue_stone_x][blue_stone_y]!=1)
          }//if(Joy_Push_Flag==1)
        }//while(1) in SW7, 청돌 착돌 전까지 조이스틱 조작 반복
      }//if(count/2!=0)
      SW7_Flag=0;
      EXTI->IMR  |= 0x000100;         // EXTI8인터럽트 mask (Interrupt Enable) 설정, 청돌 차례 종료 후 적돌 차례 (EXTI8)활성화
    }//if(SW7_Flag==1)
    if(KEY_Scan()==SW1)                            //SW1 을 누르면 적돌 승
    {
      if(red_score==9)                                  //점수가 9점이었다면 0으로 돌아감
      {
        red_score=0;
        Fram_Write(300,0);                              //Fram 300번 주소에 red_score저장
      }//if(red_score==9)
      else                                              //점수가 9점이 아니라면 1승을 추가한다.
      {
        red_score++;
        Fram_Write(300,red_score);                      //Fram 300번 주소에 red_score저장
      }
      BEEP5();                                          //부저 5회 울리기
      DelayMS(5000);                                    //5초 대기
      ProgramRestart();                                 //프로그램 재시작 함수
    }//if(!(GPIOH->IDR & ~SW1))
    if(KEY_Scan()==SW6)                            //SW6 을 누르면 청돌 승
    {
      if(blue_score==9)                                  //점수가 9점이었다면 0으로 돌아감
      {
        blue_score=0;
        Fram_Write(301,0);                               //Fram 300번 주소에 blue_score저장
      }//if(blue_score==9)
      else                                               //점수가 9점이 아니라면 1승을 추가한다.
      {
        blue_score++;
        Fram_Write(301,blue_score);                     //Fram 300번 주소에 blue_score저장
      }//if(blue_score!=9)
      BEEP5();                                          //부저 5회 울리기
      DelayMS(5000);                                    //5초 대기
      ProgramRestart();                                 //프로그램 재시작 함수
    }//if(!(GPIOH->IDR & ~SW2))
  }// while(1)
  
}//main(void)

//GLCD 초기화면 지정
void DisplayInitScreen(void)
{
  LCD_Clear(RGB_YELLOW);		// 화면 클리어
  LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
  LCD_SetBackColor(RGB_YELLOW);	// 글자배경색 : YELLOW
  
  LCD_SetTextColor(RGB_BLACK);	// 글자색 :BLACK
  
  ///// Title(MeCha OMOK) /////
  LCD_DisplayText(0,0,"MeCha OMOK(LCH)");
  
  ///// 오목판 그리기 /////
  LCD_DisplayText(1,3,"0");
  LCD_DisplayText(1,9,"5");
  LCD_DisplayText(1,13,"9");
  LCD_DisplayText(5,3,"5");
  LCD_DisplayText(8,3,"9");
  LCD_SetPenColor(RGB_BLACK);           //펜 색: BLACK
  //세로축//
  for(int i=35; i<=107;i+=8)
  {
    LCD_DrawVerLine(i, 31, 72);
  }
  //가로축//
  for(int i=31;i<=103;i+=8)
  {
    LCD_DrawHorLine(35, i, 72);
  }
  //(5,5) 위치 표시 사각형//
  LCD_SetBrushColor(RGB_BLACK);
  LCD_DrawFillRect(73, 69, 5, 5);
  
  ///// 상황판 /////
  LCD_DisplayText(9,9,"vs");
  LCD_SetTextColor(RGB_RED);           //펜 색: RED
  LCD_DisplayText(9,1,"( , )");                
  LCD_DisplayChar(9,2,red_stone_x+48);         //적돌 x좌표에 따라 좌표 값 갱신
  LCD_DisplayChar(9,4,red_stone_y+48);         //적돌 y좌표에 따라 좌표 값 갱신
  LCD_DisplayChar(9,8,red_score+48);                             //0은 fram
  LCD_SetTextColor(RGB_BLUE);         //펜 색: BLUE
  LCD_DisplayChar(9,11,blue_score+48);                          //0은 fram
  LCD_DisplayText(9,13,"( , )");                
  LCD_DisplayChar(9,14,blue_stone_x+48);       //청돌 x좌표에 따라 좌표 값 갱신
  LCD_DisplayChar(9,16,blue_stone_y+48);       //청돌 y좌표에 따라 좌표 값 갱신
}

//GPIO 초기설정
void _GPIO_Init(void)
{
  // LED (GPIO G) 설정
  RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
  GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
  GPIOG->OTYPER	&= ~0x000000FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
  GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
  
  // SW (GPIO H) 설정 
  RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
  GPIOH->MODER 	&=  ~0xFFFF0000; // remove after class        //~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
  GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state
  
  // Buzzer (GPIO F) 설정 
  RCC->AHB1ENR	|=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
  GPIOF->MODER 	|=  0x00040050;	// GPIOF 9 : Output mode (0b01)						
  GPIOF->OTYPER 	&= ~0x00000200;	// GPIOF 9 : Push-pull  	
  GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
  
  //Joy Stick SW(PORT I) 설정
  RCC->AHB1ENR 	|= 0x00000100;	// RCC_AHB1ENR GPIOI Enable
  GPIOI->MODER 	&= ~0x000FFC00;	// GPIOI 5~9 : Input mode (reset state)
  GPIOI->PUPDR    &= ~0x000FFC00;	// GPIOI 5~9 : Floating input (No Pull-up, pull-down) (reset state)
}

//EXTI 인터럽트 초기설정
void _EXTI_Init(void)
{
  RCC->AHB1ENR 	|= 0x00000180;	// RCC_AHB1ENR GPIOH, GPIOI Enable
  RCC->APB2ENR 	|= 0x00004000;	// Enable System Configuration Controller Clock
  // |=(1<<14);
  GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH PIN8~PIN15 Input mode (reset state)	
  GPIOI->MODER    &= ~0x000FFC00;
  SYSCFG->EXTICR[1] |= 0x0080; 	//EXTI5에 대한 소스 입력은 GPIOI로 설정 
  SYSCFG->EXTICR[2] |= 0x0007; 	// EXTI8에 대한 소스 입력은 GPIOH로 설정
  SYSCFG->EXTICR[3] |= 0x7000;	//EXTI15에 대한 소스 입력은 GPIOH로 설정
  EXTI->FTSR |= 0x008120;		// EXTI8,10,15: Falling Trigger Enable 
  EXTI->IMR  |= 0x008120;         // EXTI8,10,15인터럽트 mask (Interrupt Enable) 설정
  
  NVIC->ISER[0] |= ( 1<<23 );  // Enable 'Global Interrupt EXTI8
  // Vector table Position 참조
  NVIC->ISER[1] |= ( 1<<(40-32) );
}

/* EXTI10~15 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)
{		
  if(EXTI->PR & 0x8000) 			// EXTI15 Interrupt Pending(발생) 여부?, 청돌 차례
  {
    EXTI->PR |= 0x8000; 		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
    if(count==0)
    {
      count++;
    }
    SW7_Flag = 1;			// SW7_Flag: EXTI15이 발생되었음을 알리기 위해 만든 변수(main문의 mission에 사용) 
    GPIOG->ODR |=0x80;
    GPIOG->ODR &=~0x01;
    LCD_DisplayText(9,0," ");
    LCD_SetTextColor(RGB_BLUE);           //펜 색: BLUE
    LCD_DisplayText(9,18,"*");
  }
}

/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{		
  if(EXTI->PR & 0x0100) 			// EXTI8 Interrupt Pending(발생) 여부, 적돌 차례
  {
    EXTI->PR |= 0x0100; 		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
    SW0_Flag = 1;			// SW0_Flag: EXTI8이 발생되었음을 알리기 위해 만든 변수(main문의 mission에 사용
    GPIOG->ODR |=0x01;
    GPIOG->ODR &=~0x80;
    LCD_DisplayText(9,18," ");
    LCD_SetTextColor(RGB_RED);           //펜 색: RED
    LCD_DisplayText(9,0,"*");
    
  }
  else if(EXTI->PR & 0x0020)              //EXTI5 Interrupt Pending (발생) 여부, 착돌 버튼
  {
    EXTI->PR |= 0x0020; 		// Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
    Joy_Push_Flag = 1;	        // J-P_Flag: JOYSTICK PUSH가 발생되었음을 알리기 위해 만든 변수(main문의 mission에 사용) 
    
  }
}

/* JoyStick이 입력되었는지를 여부와 어떤 JoyStick이 입력되었는지의 정보를 return하는 함수  */ 
uint8_t joy_flag = 0;
uint16_t JOY_Scan(void)	// input joy stick NAVI_* 
{ 
  uint16_t key;
  key = GPIOI->IDR & 0x03E0;	// any key pressed ?
  if(key == 0x03E0)		// if no key, check key off
  {  	if(joy_flag == 0)
    return key;
  else
  {	DelayMS(10); 
  joy_flag = 0;
  return key;
  }
  }
  else				// if key input, check continuous key
  {	if(joy_flag != 0)	// if continuous key, treat as no key input
    return 0x03E0;
  else			// if new key,delay for debounce
  {	joy_flag = 1;
  DelayMS(10);
  return key;
  }
  }
}
/* Switch가 입력되었는지를 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */ 
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
//부저울리는 함수
void BEEP(void)
{ 	
  GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
  DelayMS(30);		// Delay 30 ms
  GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}
//부저를 3번 울리는 함수
void BEEP3(void)
{
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
}

//부저를 5번 울리는 함수
void BEEP5(void)
{
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
  BEEP();
  DelayMS(200);
}

//milli second 단위로 대기하는 함수
void DelayMS(unsigned short wMS)
{
  register unsigned short i;
  for (i=0; i<wMS; i++)
    DelayUS(1000);         		// 1000us => 1ms
}

//micro second 단위로 대기하는 함수
void DelayUS(unsigned short wUS)
{
  volatile int Dly = (int)wUS*17;
  for(; Dly; Dly--);
}

//프로그램 재시작하는 함수
void ProgramRestart(void)
{
  red_score=Fram_Read(300);             //Fram 300번 주소에 저장되어있는 적돌 승리 횟수를 red_score에 저장
  blue_score=Fram_Read(301);            //Fram 301번 주소에 저장되어있는 청돌 승리 횟수를 blue_score에 저장
  DisplayInitScreen();                  //초기화면 설정
  for(int i=0; i<10;i++)
    for(int j=0; j<10;j++)
      check_stone[i][j]=0;              //좌표에 돌이 있는지 여부를 저장하는 배열 초기화
  GPIOG->ODR &=~0x01;                   //LED0 OFF
  GPIOG->ODR &=~0x80;                   //LED7 OFF
}