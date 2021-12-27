/*
HW3: 엘리베이터
과목: 마이크로컴퓨터구조
담당교수: 남윤석 교수님
2017130036 이찬해
제출일: 2021.05.18
*/
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

void _GPIO_Init(void);                  
void _EXTI_Init(void);

void DisplayInitScreen(void);
uint16_t KEY_Scan(void);
void BEEP(void);
void BEEP3(void); 

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint8_t  SW0_Flag, SW7_Flag;            //인터럽트(EXTI8, 15) 중복 키입력 방지를 위한 Flag

uint16_t cur_led=0;                              //현재 led 주소저장 (초기값: 0)

int main(void)
{
	_GPIO_Init(); 	                 // GPIO (LED,SW,Buzzer)초기화
        _EXTI_Init();                        //EXTI 초기화
	LCD_Init();	                 // LCD 모듈 초기화
	DelayMS(10);

	Fram_Init();                           // FRAM 초기화 H/W 초기화
	Fram_Status_Config();          // FRAM 초기화 S/W 초기화
 
	DisplayInitScreen();            // LCD 초기화면  
        
//FRAM이용해서 전원 끈 시점의 층을 LED랑 GLCD로 표현해야한다.
       switch(Fram_Read(36))
       {
       case 0:                                          //Cur_FL: 0
          LCD_DisplayText(1,8,"0");     
          cur_led=LED0;                       //cur_led에 LED0  값 저장
          break;
       case 1:                                          //Cur_FL: 1
         LCD_DisplayText(1,8,"1");
         cur_led=LED1;                          //cur_led에 LED1  값 저장
          break;
       case 2:                                          //Cur_FL: 2
         LCD_DisplayText(1,8,"2");
         cur_led=LED2;                          //cur_led에 LED2  값 저장
          break;
       case 3:                                           //Cur_FL: 3
         LCD_DisplayText(1,8,"3");
         cur_led=LED3;                          //cur_led에 LED3  값 저장
          break;
       case 4:                                          //Cur_FL: 4
         LCD_DisplayText(1,8,"4");
         cur_led=LED4;                          //cur_led에 LED4  값 저장
          break;
       case 5:                                          //Cur_FL: 5
         LCD_DisplayText(1,8,"5");
         cur_led=LED5;                          //cur_led에 LED5  값 저장
          break;
       case 6:                                          //Cur_FL: 6
         LCD_DisplayText(1,8,"6");
         cur_led=LED6;                          //cur_led에 LED6  값 저장
          break;
       case 7:                                          //Cur_FL: 7
         LCD_DisplayText(1,8,"7");
         cur_led=LED7;                          //cur_led에 LED7  값 저장
         break;
       default:                                         //과제 내 층수 이외의 값
         LCD_DisplayText(1,8,"0");
         cur_led=LED0;                          //cur_led에 LED0  값 저장
       }
       
      	GPIOG->ODR &= ~0x00FF;	         // LED 초기값: LED0~7 Off
       GPIOG->ODR |= cur_led;                 //Fram 에서 읽어온 현재층의 LED  on
          
	while(1)
	{
            if(SW0_Flag)	        // 인터럽트 (EXTI8) 발생
            {
                  if(cur_led!=LED0)                            //목표층과 현재층이 다를 경우
                  {
                      BEEP();                                           //부저 on
                      LCD_DisplayText(2,8,"0");                       //목표층 0층       
                      while(cur_led!=LED0)                                      //목표층과 현재층이 같아지면 whiile문탈출
                      {
                        DelayMS(500);                                 //대기 0.5초
                          GPIOG->ODR &=~cur_led;              //이전 층 LED off
                          
                          if(cur_led >LED0)                         //목표층이 현재층보다 낮을 경우
                              cur_led>>=1;                                  //아랫층 led주소로 시프트 이동
                          else                                            //목표층이 현재층보다 높을 경우
                              cur_led<<=1;                                      //윗층 led주소로 시프트 이동
                          GPIOG->ODR |=cur_led;               //다음층 LED on
                      }
                      BEEP3();                                          //부저3회  반복
                      LCD_DisplayText(1,8,"0");                 //GLCD 현재층 표시
                      LCD_DisplayText(2,8,"-");                    //sw대기상태
                      Fram_Write(36,0);                                 //FRAM36번지에 0저장
                   }
                  SW0_Flag = 0;
            }
            if(SW7_Flag)                  //인터럽트 (EXTI15) 발생
            {
                if(cur_led!=LED7)                                     //목표층과 헌재층이 다른 경우  
                {
                    BEEP();                                                     //부저 on
                    LCD_DisplayText(2,8,"7");                           //GLCD 목표층 7층
                    while(cur_led!=LED7)                                        //목표층과 현재층이 같아지면 whiile문탈출
                    {
                      DelayMS(500);                                         //대기 0.5초
                          GPIOG->ODR &=~cur_led;               //이전 층 LED off
                          
                          if(cur_led >LED7)                         //목표층이 현재층보다 낮을 경우
                          {
                              cur_led>>=1;                                      //아랫층 led주소로 시프트 이동
                          }
                          else                                            //목표층이 현재층보다 높을 경우
                          {
                              cur_led<<=1;                                      //윗층led주소로 시프트 이동
                          }
                          GPIOG->ODR |=cur_led;                //다음층 LED on
                    }
                    BEEP3();                                            //부저3회 반복
                    LCD_DisplayText(1,8,"7");                           //GLCD 현재층 표시
                    LCD_DisplayText(2,8,"-");                           //sw대기상태
                    Fram_Write(36,7);                                   //FRAM36번지에 7저장
                } 
                SW7_Flag = 0;
            }
		switch(KEY_Scan())	// 입력된 Switch 정보 분류
		{
        		case SW1 : 	                //SW1 입력
                           if(cur_led==LED1)                            //목표층과 현재층이 같을 경우
                           {
                              break;
                           }
                           else                                                 //목표층과 현재층이 다를 경우
                          {
                              BEEP();                                           //부저 on
                              LCD_DisplayText(2,8,"1");                         //GLCD 목표층 1층
                              while(cur_led!=LED1)                              //목표층과 현재층이 같아지면 whiile문탈출                         
                              {
                                DelayMS(500);                                 //대기 0.5초
                                  GPIOG->ODR &=~cur_led;               //이전 층 LED off
                                  
                                  if(cur_led >LED1)                         //목표층이 현재층보다 낮을 경우
                                  {
                                      cur_led>>=1;                           //아랫층 led주소로 시프트 이동
                                  }
                                  else                                             //목표층이 현재층보다 높을 경우
                                  {
                                      cur_led <<=1;                             //윗층led주소로 시프트 이동
                                  }

                                  GPIOG->ODR |=cur_led;                //다음층 LED on
                               }
                             BEEP3();                                               //부저3회 반복
                              LCD_DisplayText(1,8,"1");                         //GLCD 현재층 표시
                              LCD_DisplayText(2,8,"-");                        //sw대기상태
                              Fram_Write(36,1);                                   //FRAM36번지에 1저장
                              break;
                          }
                          case SW2:                         //SW2 입력         
                          if(cur_led==LED2)                                     //목표층과 현재층이 같을 경우
                          {
                                break;
                          }
                          else                                                 //목표층과 현재층이 다를 경우
                          {
                            BEEP();                                             //부저 on
                            LCD_DisplayText(2,8,"2");                   //GLCD 목표층 2층
                            while(cur_led!=LED2)                                //목표층과 현재층이 같아지면 whiile문탈출
                            {
                              DelayMS(500);                                   //대기 0.5초
                                GPIOG->ODR &=~cur_led;               //이전 층 LED off
                                
                                if(cur_led >LED2)                         //목표층이 현재층보다 낮을 경우
                                {
                                    cur_led>>=1;                                //아랫층 led주소로 시프트 이동
                                }
                                else                                             //목표층이 현재층보다 높을 경우{
                                {
                                    cur_led<<=1;                                    //윗층led주소로 시프트 이동
                                }
                                GPIOG->ODR |=cur_led;                //다음층 LED on
                            }
                            BEEP3();                                                //부저3회 반복
                            LCD_DisplayText(1,8,"2");                            //GLCD 현재층 표시
                            LCD_DisplayText(2,8,"-");                            //sw대기상태
                            Fram_Write(36,2);                                   //FRAM36번지에 2저장
                            break;
                          }
        		case SW3 : 	                //SW3 입력
                          if(cur_led==LED3)                                  //목표층과 현재층이 같을 경우
                          {
                              break;
                          }
                          else                                                 //목표층과 현재층이 다를 경우
                          {
                            BEEP();
                            LCD_DisplayText(2,8,"3");                   //GLCD 목표층 3층
                            while(cur_led!=LED3)                                //목표층과 현재층이 같아지면 whiile문탈출
                            {
                                DelayMS(500);                                     //대기 0.5초
                                GPIOG->ODR &=~cur_led;               //이전 층 LED off
                                
                                if(cur_led >LED3)                         //목표층이 현재층보다 낮을 경우
                                {
                                    cur_led>>=1;                                //아랫층 led주소로 시프트 이동
                                }
                                else                                             //목표층이 현재층보다 높을 경우
                                {
                                    cur_led<<=1;                                    //윗층led주소로 시프트 이동
                                }
                                GPIOG->ODR |=cur_led;                //다음층 LED on
                            }
                           BEEP3();                                                 //부저3회 반복   
                            LCD_DisplayText(1,8,"3");                            //GLCD 현재층 표시
                            LCD_DisplayText(2,8,"-");                            //sw대기상태
                            Fram_Write(36,3);                                               //FRAM36번지에 3저장
                            break;
                          }
                        case SW4: 	               //SW4 입력
                         if(cur_led==LED4)                                  //목표층과 현재층이 같을 경우
                         {
                            break;
                         }
                          else                                                //목표층과 현재층이 다를 경우
                          {
                            BEEP();
                            LCD_DisplayText(2,8,"4");                   //GLCD 목표층 4층
                            while(cur_led!=LED4)                                //목표층과 현재층이 같아지면 whiile문탈출
                            {
                                DelayMS(500);                                     //대기 0.5초
                                GPIOG->ODR &=~cur_led;               //이전 층 LED off
                                
                                if(cur_led >LED4)                         //목표층이 현재층보다 낮을 경우
                                {
                                    cur_led>>=1;                                //아랫층 led주소로 시프트 이동
                                }
                                else                                             //목표층이 현재층보다 높을 경우
                                {
                                    cur_led<<=1;                                    //윗층led주소로 시프트 이동
                                }
                                GPIOG->ODR |=cur_led;                //다음층 LED on
                            }
                            BEEP3();                                                //부저3회 반복
                            LCD_DisplayText(1,8,"4");                            //GLCD 현재층 표시
                            LCD_DisplayText(2,8,"-");                            //sw대기상태
                            Fram_Write(36,4);                                       //FRAM36번지에 4저장
                            break;
                          }
                        case SW5: 	               //SW5 입력
                           if(cur_led==LED5)                                  //목표층과 현재층이 같을 경우
                           {
                              break;
                           }
                          else                                                 //목표층과 현재층이 다를 경우
                            {
                            BEEP();
                            LCD_DisplayText(2,8,"5");                           //GLCD 목표층 5층
                            while(cur_led!=LED5)                                //목표층과 현재층이 같아지면 whiile문탈출
                            {
                                DelayMS(500);                                     //대기 0.5초
                                GPIOG->ODR &=~cur_led;               //이전 층 LED off
                                
                                if(cur_led >LED5)                         //목표층이 현재층보다 낮을 경우
                                {
                                    cur_led>>=1;                                        //아랫층 led주소로 시프트 이동        
                                }
                                else                                             //목표층이 현재층보다 높을 경우
                                {
                                    cur_led<<=1;                                            //윗층led주소로 시프트 이동
                                }
                                GPIOG->ODR |=cur_led;                //다음층 LED on
                            }
                            BEEP3();                                                //부저3회 반복
                            LCD_DisplayText(1,8,"5");                            //GLCD 현재층 표시
                            LCD_DisplayText(2,8,"-");                            //sw대기상태
                         Fram_Write(36,5);                                          //FRAM36번지에 5저장
                            break;
                          }
                        case SW6: 	                //SW6 입력
                           if(cur_led==LED6)                                      //목표층과 현재층이 같을 경우
                           {
                              break;
                           }
                           else                                                //목표층과 현재층이 다를 경우
                           {
                              BEEP();
                              LCD_DisplayText(2,8,"6");                         //GLCD 목표층 6층
                              while(cur_led!=LED6)                                      //목표층과 현재층이 같아지면 whiile문탈출
                              {
                                  DelayMS(500);                                   //대기 0.5초
                                  GPIOG->ODR &=~cur_led;               //이전 층 LED off
                                
                                  if(cur_led >LED6)                         //목표층이 현재층보다 낮을 경우
                                  {
                                    cur_led>>=1;                                //아랫층 led주소로 시프트 이동
                                  }
                                  else                                             //목표층이 현재층보다 높을 경우
                                  {
                                    cur_led<<=1;
                                  }
                                  GPIOG->ODR |=cur_led;                //다음층 LED on
                               }
                               BEEP3();                                                     //부저3회 반복
                               LCD_DisplayText(1,8,"6");                                 //GLCD 현재층 표시
                              LCD_DisplayText(2,8,"-");                          //sw대기상태
                              Fram_Write(36,6);                                     //FRAM36번지에 6저장
                               break;
                          }
                  }  // switch(KEY_Scan())
    	}  // while(1)
}

/* GLCD 초기화면 설정 */
void DisplayInitScreen(void)
{
        LCD_Clear(RGB_YELLOW);		// 화면 클리어
        LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
        LCD_SetBackColor(RGB_YELLOW);	// 글자배경색 : YELLOW
        LCD_SetTextColor(RGB_BLUE);	// 글자색 :BLUE
        LCD_DisplayText(0,0,"MC Elevator (LCH)");  // Title
        LCD_SetTextColor(RGB_BLACK);    // 글자색 :BLACK
        LCD_DisplayText(1,0,"Cur FL: ");  // subtitle
        LCD_DisplayText(2,0,"Des FL: ");  // subtitle
        LCD_SetTextColor(RGB_RED);      //글자색: RED
        LCD_DisplayText(1,8,"0");
        LCD_DisplayText(2,8,"-");
}

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer), GPIOI(Joy stick)) 초기 설정	*/
void _GPIO_Init(void)
{
        // LED (GPIO G) 설정
    	RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
	GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
	GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
 	GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
    
	// SW (GPIO H) 설정 
	RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
	GPIOH->MODER	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
	GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state

	// Buzzer (GPIO F) 설정 
    	RCC->AHB1ENR    |=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
	GPIOF->MODER    |=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
	GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
 	GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}	

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI15(GPIOH.15, SW7)) 초기 설정  */
void _EXTI_Init(void)
{
    	RCC->AHB1ENR 	|= 0x00000080  ;	// RCC_AHB1ENR GPIOH Enable
	RCC->APB2ENR 	|= 0x00004000  ;	// Enable System Configuration Controller Clock
                                          //|=(1<<14);
	GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH PIN8,PIN15 Input mode (reset state)			 
	
	SYSCFG->EXTICR[2] |= 0x0007; 	// EXTI8에 대한 소스 입력은 GPIOH로 설정
					// EXTI8 <- PH8, 
					// EXTICR3(EXTICR[2])를 이용 
					// reset value: 0x0000
       SYSCFG->EXTICR[3] |= 0x7000; 	// EXTI15에 대한 소스 입력은 GPIOH로 설정	
                                        //EXTI15 <- PH15.
                                        //EXTICR4(EXTICR[3])를 이용
                                        //reset value: 0x0000
	EXTI->FTSR |= 0x000100;		        // EXTI8: Falling Trigger Enable
        EXTI->FTSR |= 0x008000;		        // EXTI15: Falling Trigger Enable
       
    	EXTI->IMR  |= 0x000100;  		// EXTI8 인터럽트 mask (Interrupt Enable) 설정
	EXTI->IMR  |= 0x008000;  		// EXTI15 인터럽트 mask (Interrupt Enable) 설정	
        
	NVIC->ISER[0] |= (1<<23 );  // Enable 'Global Interrupt EXTI8'
	NVIC->ISER[1] |= (1<<(40-32) );  // Enable 'Global Interrupt EXTI15'		
        // Vector table Position 참조
}

/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{		
    if(EXTI->PR & 0x0100) 			// EXTI8 Interrupt Pending(발생) 여부?
    {
      EXTI->PR |= 0x0100;                     // Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
      if(SW0_Flag ==0)
      {
        SW0_Flag = 1;			// SW0_Flag: EXTI8이 발생되었음을 알리기 위해 만든 변수(main문의 mission에 사용) 
      }
    }
}

/*EXTI15~10 인터럽트 핸들러(ISR: interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)
{
    if(EXTI-> PR & 0x8000)                              // EXTI15 Interrupt Pending(발생) 여부?
    {
      EXTI->PR |= 0x8000;                               // Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
      if(SW7_Flag ==0)
      { 
         SW7_Flag = 1;			// SW7_Flag: EXTI15가 발생되었음을 알리기 위해 만든 변수(main문의 mission에 사용) 
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

/* Buzzer: Beep for 30 ms */
void BEEP(void)			
{ 	
	GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
	DelayMS(30);		// Delay 30 ms
	GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}
void BEEP3(void)
{
    BEEP();                                           //부저 on
    DelayMS(100);                                    //대기
    BEEP();                                           //부저 on
    DelayMS(100);                                    //대기
    BEEP();                                           //부저 on
    DelayMS(100);                                     //대기
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