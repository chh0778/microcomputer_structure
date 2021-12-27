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

uint8_t  SW0_Flag, SW7_Flag;
//인터럽트(EXTI8, 15) 발생을 나타내는 Flag
//0이면 발생이전상태, 1이면 발생
int RL_Flag;                            
//RL_Flag: R 엘리베이터인지 L엘리베이터인지 구분하는  Flag
//16이면 L-E, 125이면 R-E
int Start_des_select_Flag=0;
//Start_des_select_Flag: 출발층과 목표층을 입력하는 시점을 달리하는 Flag
//0이면 출발층 입력상태, 1이면 목표층 입력

int L_cur_fl;
int R_cur_fl;
int cur_fl;
int start_fl;
int des_fl;
int prev_start_fl;
int prev_des_fl;
int i;

void BEEP(void);
void BEEP3(void);
void BEEP5s(void);
void BEEP200ms(void);

void DelayMS(unsigned short wMS);
void DelayUS(unsigned short wUS);

uint16_t FLOOR_TO_LED(int);
void Select_Color(int);
int SW_TO_DES_FLOOR(void);
int SW_TO_START_FLOOR(void);

int ABS(int, int);
int Select_Elevator(int, int, int);

void MoveElevator(void);

int main(void)
{
  _GPIO_Init(); 	                 // GPIO (LED,SW,Buzzer)초기화
  _EXTI_Init();                        //EXTI 초기화
  LCD_Init();	                         // LCD 모듈 초기화
  Fram_Init();                    // FRAM 초기화 H/W 초기화
  Fram_Status_Config();   // FRAM 초기화 S/W 초기화
  R_cur_fl=Fram_Read(1002);     //FRAM 1002번지 저장된 L-E의 현재층 정보를 저장
  DelayMS(10);
  L_cur_fl=Fram_Read(1001);     //FRAM 1001번지 저장된 L-E의 현재층 정보를 저장
  DisplayInitScreen();                  // LCD 초기화면
  GPIOG->ODR &= ~0x007F;	        // LED 초기값: LED0~6 Off
  GPIOG->ODR |=  0x0080;               // LED7 on
  
  
  /*FRAM 1001번지 저장된 L-E 층 정보로 엘리베이터 막대 표현*/
  LCD_SetBrushColor(RGB_BLUE);          //채우기 색: BLUE
  LCD_DrawFillRect(16,91-13*(Fram_Read(1001)-1),10,13*Fram_Read(1001));         
  LCD_SetPenColor(RGB_GREEN);           //펜 색: GREEN
  LCD_DrawRectangle(16,91-13*(Fram_Read(1001)-1),10,13*Fram_Read(1001));
  
  /*FRAM 1002번지 저장된 R-E 층 정보로 엘리베이터 막대 표현*/
  LCD_SetBrushColor(RGB_GREEN);         //채우기 색: GREEN
  LCD_DrawFillRect(125,91-13*(Fram_Read(1002)-1),10,13*Fram_Read(1002));
  LCD_SetPenColor(RGB_BLUE);            //펜 새: BLUE
  LCD_DrawRectangle(125,91-13*(Fram_Read(1002)-1),10,13*Fram_Read(1002));
  
  /*FRAM 1001번지에 저장된 L-E 층 정보로 초기 층 정보 GLCD표시*/
  LCD_SetTextColor(RGB_RED);                  //글자색: RED
  LCD_DisplayChar(6,7,Fram_Read(1001)+48);          //Fram 1001번지에 저장된 L-E 층 데이터를 통한 GLCD 출발층 표시
  LCD_DisplayChar(6,11,Fram_Read(1001)+48);          //Fram 1002번지에 저장된 L-E 층 데이터를 통한 GLCD 출발층 표시
  
/////////////////////////////////층 입력 시작////////////////////////////////////////
  
  /*출발층만 입력 후 EXTI8 인터럽트가 발생하지 않도록 mask설정*/
  EXTI->IMR  &= ~0x000100;  		// EXTI8 인터럽트 mask (Interrupt Disable) 설정
  EXTI->IMR  &= ~0x008000;  		// EXTI15 인터럽트 mask (Interrupt Disable) 설정
  start_fl=SW_TO_START_FLOOR();         //스위치 입력에 따른 출발층 입력
  prev_start_fl=start_fl;               //이전 출발층 저장
  BEEP();                               //부저
  GPIOG->ODR |= FLOOR_TO_LED(start_fl);       //새로 입력된 start LED on
  LCD_SetTextColor(RGB_RED);                  //글자색: RED
  LCD_DisplayChar(6,7,start_fl+48);          //GLCD 출발층 표시
  while(1)
  {
    des_fl=SW_TO_DES_FLOOR();                   //스위치 입력에 따른 목표층 입력
    if(start_fl==des_fl)
      continue;
    else
    {
      BEEP();                                   //부저
      GPIOG->ODR |= FLOOR_TO_LED(des_fl);         // des LED on
      LCD_SetTextColor(RGB_RED);                  //글자색: RED
      LCD_DisplayChar(6,11,des_fl+48);          //GLCD 목표층 표시
      prev_des_fl=des_fl;                       //이전 목표층 저장
      break;
    }
  }//while(1)
  EXTI->IMR  |= 0x000100;  		// EXTI8 인터럽트 mask (Interrupt Enable) 설정
  
  while(1)
  {
    if(Start_des_select_Flag==0)                              //출발층 입력대기상태 
    {
      start_fl=SW_TO_START_FLOOR();                       //시작위치 스위치를 누르면 시작층 정보가 저장된다.
      if(SW0_Flag==1)                                   //EXTI8 인터럽트 발생
      {
        MoveElevator();                                 //엘리베이터 실행
        continue;                                       //while 처음으로 돌아가서 출발층부터 입력받음
      }
      if(des_fl!=start_fl)                              //목표층과 출발층이 다른경우
      {
        BEEP();                                 //부저
        GPIOG->ODR &= ~0x007F;	                // LED 초기값: LED0~6 Off
        if(des_fl>=1 && des_fl<=6)
          GPIOG->ODR |= FLOOR_TO_LED(des_fl);         // des LED on
        GPIOG->ODR |= FLOOR_TO_LED(start_fl);       //새로 입력된 start LED on
        LCD_SetTextColor(RGB_RED);                  //글자색: RED
        LCD_DisplayChar(6,7,start_fl+48);          //GLCD 출발층 표시
        Start_des_select_Flag=1;                             //목표층 입력대기상태로 변경
        prev_start_fl=start_fl;                 //이전 출발층 저장
      }//if(des_fl!=start_fl)
    }//if(Start_des_select_Flag==0
    
    else if(Start_des_select_Flag==1)                        //목표층 입력대기상태
    {
      des_fl=SW_TO_DES_FLOOR();                 //스위치 입력에 따른 목표층 저장
      if(SW0_Flag==1)                           //EXTI8 인터럽트 발생
      {
        MoveElevator();                         //엘리베이터 동작 실행
        continue;
      }
      if(start_fl!=des_fl)                      //목표층과 출발층이 다를경우
      {
        BEEP();
        GPIOG->ODR &= ~0x007F;	                // LED 초기값: LED0~6 Off
        GPIOG->ODR |= FLOOR_TO_LED(des_fl);         // des LED on
        GPIOG->ODR |= FLOOR_TO_LED(start_fl);       //새로 입력된 start LED on
        LCD_SetTextColor(RGB_RED);                  //글자색: RED
        LCD_DisplayChar(6,11,des_fl+48);          //GLCD 목표층 표시
        Start_des_select_Flag=0;                             //출발층 입력대기상태로 변경
        prev_des_fl=des_fl;
      }//if(start_fl!=des_fl)
    }//else if(Start_des_select_Flag==1)
    
  }//while(1) 
}//main()

/* GLCD 초기화면 설정 */
//y,x 순서
void DisplayInitScreen(void)
{
  LCD_Clear(RGB_YELLOW);		// 화면 클리어
  LCD_SetFont(&Gulim8);		// 폰트 : 굴림 8
  LCD_SetBackColor(RGB_YELLOW);	// 글자배경색 : YELLOW
  
  LCD_SetTextColor(RGB_BLACK);	// 글자색 :BLACK
  LCD_DisplayText(0,0,"MC Elevator(LCH)");  // Title
  LCD_DisplayText(6,9,">");
  //GLCD: 왼쪽 엘리베이터 층수 표시
  LCD_SetTextColor(RGB_BLUE);    // 글자색 :BLUE
  LCD_DisplayText(2,4,"6");    
  LCD_DisplayText(3,4,"5");
  LCD_DisplayText(4,4,"4");
  LCD_DisplayText(5,4,"3");
  LCD_DisplayText(6,4,"2");
  LCD_DisplayText(7,4,"1");
  
  //GLCD: 왼쪽 엘리베이터 막대표시
  LCD_SetBrushColor(RGB_BLUE);
  LCD_DrawFillRect(16,91,10,13);
  LCD_SetPenColor(RGB_GREEN);
  LCD_DrawRectangle(16,91 ,10,13);
  
  //GLCD: 가운데 엘리베이터 정보
  LCD_DisplayText(4,8,"L-E");
  LCD_SetTextColor(RGB_RED);    // 글자색 :RED
  LCD_DisplayText(2,9,"FL");
  LCD_DisplayText(5,9,"S");
  //LCD_DisplayText(6,7,"1");
  //LCD_DisplayText(6,11,"1");
  LCD_DisplayChar(6,7,Fram_Read(1001)+48);
  LCD_DisplayChar(6,11,Fram_Read(1001)+48);
  
  //오른쪽 엘리베이터 숫자표시
  LCD_SetTextColor(RGB_GREEN);    // 글자색 :BLUE
  LCD_DisplayText(2,14,"6");
  LCD_DisplayText(3,14,"5");
  LCD_DisplayText(4,14,"4");
  LCD_DisplayText(5,14,"3");
  LCD_DisplayText(6,14,"2");
  LCD_DisplayText(7,14,"1");

  //오른쪽 엘리베이터 막대표시
  LCD_SetBrushColor(RGB_GREEN);
  LCD_DrawFillRect(125,91,10,13);
  LCD_SetPenColor(RGB_BLUE);
  LCD_DrawRectangle(125,91 ,10,13);
  
  //LCD_DisplayText(2,0,"Des FL: ");
  //LCD_SetTextColor(RGB_RED); 
  //LCD_DisplayText(1,8,"0");
  //LCD_DisplayText(2,8,"-");
}
/* 문자표시용으로 많이 사용하는 함수들 */ 
//void LCD_DrawHorLine(UINT16 x, UINT16 y, UINT16 width)  
//void LCD_DrawVerLine(UINT16 x, UINT16 y, UINT16 height)
//void LCD_DrawRectangle(UINT16 x, UINT16 y, UINT16 width, UINT16 height)  
//void LCD_DrawFillRect(UINT16 x, UINT16 y, UINT16 width, UINT16 height)  
//void LCD_DrawLine(UINT16 x1, UINT16 y1, UINT16 x2, UINT16 y2)

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
  RCC->AHB1ENR 	|= 0x00000080  ;	// RCC_AHB1ENR GPIOH  
  RCC->APB2ENR 	|= 0x00004000  ;	// Enable System Configuration Controller Clock
  //|=(1<<14);
  GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH PIN8,PIN15 Input mode (reset state)			 
  
  SYSCFG->EXTICR[2] |= 0x0007; 	// EXTI8에 대한 소스 입력은 GPIOH로 설정
  // EXTI8 <- PH8
  // EXTICR3(EXTICR[2])를 이용 
  // reset value: 0x0000
  SYSCFG->EXTICR[3] |= 0x7000; 	// EXTI15에 대한 소스 입력은 GPIOH로 설정	
  //EXTI15 <- PH15
  //EXTICR4(EXTICR[3])를 이용
  //reset value: 0x0000
  EXTI->FTSR |= 0x000100;		        // EXTI8: Falling Trigger Enable
  EXTI->FTSR |= 0x008000;		        // EXTI15: Falling Trigger Enable
  
  EXTI->IMR  |= 0x000100;  		// EXTI8 인터럽트 mask (Interrupt Enable) 설정
  EXTI->IMR  |= 0x008000;  		// EXTI15 인터럽트 mask (Interrupt Enable) 설정	
  
  NVIC->ISER[0] |= (1<<23 );            // Enable 'Global Interrupt EXTI8'
  NVIC->ISER[1] |= (1<<(40-32) );       // Enable 'Global Interrupt EXTI15'		
  // Vector table Position 참조
}

/* EXTI5~9 인터럽트 핸들러(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{		
  if(EXTI->PR & 0x0100) 			// EXTI8 Interrupt Pending(발생) 여부?
  {
    EXTI->PR |= 0x0100;                                   // Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
    SW0_Flag=1;
    start_fl=prev_start_fl;
  }//if(EXTI->PR & 0x0100) 
}//void EXTI9_5_IRQHandler(void)

/*EXTI15~10 인터럽트 핸들러(ISR: interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)
{
  if(EXTI-> PR & 0x8000)                              // EXTI15 Interrupt Pending(발생) 여부?
  {
    EXTI->PR |= 0x8000;                               // Pending bit Clear (clear를 안하면 인터럽트 수행후 다시 인터럽트 발생)
    LCD_DisplayText(2,9,"HD");                     //GLCD FL->HD 변경
    GPIOG->ODR &=~0x0001;                          //LED0 off
    BEEP5s();
    LCD_DisplayText(2,9,"FL");                     //GLCD HD->FL 변경
    GPIOG->ODR |=0x0001;                          //LED0 on 
    GPIOG->ODR &=~0x0080;                          //LED7 off
  }
}

/* Switch가 입력되었는지를 여부와 어떤 switch가 입력되었는지의 정보를 return하는 함수  */ 
uint8_t key_flag = 0;
uint16_t KEY_Scan(void)	// input key SW0 - SW7
{ 
  EXTI->IMR  &= ~0x000100;
  uint16_t key;
  key = GPIOH->IDR & 0xFF00;	// any key pressed ?
  if(key == 0xFF00)		// if no key, check key off
  {
    if(key_flag == 0)
    {
      EXTI->IMR  |= 0x000100;           //EXTI8 인터럽트가 가능하도록 하는 마스크
      return key;
    }
    else
    {
      DelayMS(10);
      key_flag = 0;
      EXTI->IMR  |= 0x000100;           //EXTI8 인터럽트가 가능하도록 하는 마스크
      return key;
    }
  }
  else				// if key input, check continuous key
  {
    if(key_flag != 0)	// if continuous key, treat as no key input
    {
      EXTI->IMR  |= 0x000100;           //EXTI8 인터럽트가 가능하도록 하는 마스크
      return 0xFF00;
    }
    else			// if new key,delay for debounce
    {
      key_flag = 1;
      DelayMS(10);
      EXTI->IMR  |= 0x000100;           //EXTI8 인터럽트가 가능하도록 하는 마스크
      return key;
    }
  }
}

//원하는 층의 스위치를 누르면 해당 시작층 정수를 반환하는 함수
int SW_TO_DES_FLOOR(void){
  int a, key_flag=0;
  while(1){
    switch(KEY_Scan()){
    case SW1:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=1;
      key_flag=1;
      break; 
    case SW2:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=2;
      key_flag=1;
      break; 
    case SW3:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=3;
      key_flag=1;
      break; 
    case SW4:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=4;
      key_flag=1;
      break; 
    case SW5:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=5;
      key_flag=1;
      break; 
    case SW6: 
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=6;
      key_flag=1;
      break;
    }//switch(KEY_Scan)  
    if(key_flag==1)
    {
      key_flag=0;
      
      return a;                         //입력받은 목표층 반환
    }//if(key_flag==1)
    if(SW0_Flag==1){

      return prev_des_fl;               //입력받기 전 인터럽트 발생시 이전 목표층 반환
    }
  }//while(1)
}

//원하는 층의 스위치를 누르면 해당 시작층 정수를 반환하는 함수
int SW_TO_START_FLOOR(void){
  int a, key_flag=0;
  while(1){
    switch(KEY_Scan()){
    case SW1:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=1;
      key_flag=1;
      break; 
    case SW2:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=2;
      key_flag=1;
      break; 
    case SW3:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=3;
      key_flag=1;
      break; 
    case SW4:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;  
      a=4;
      key_flag=1;
      break; 
    case SW5:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=5;
      key_flag=1;
      break; 
    case SW6: 
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 인터럽트 발생
        break;
      a=6;
      key_flag=1;
      break;
    }//switch(KEY_Scan)  
    if(key_flag==1)
    {
      key_flag=0;
      return a;                         //입력받은 출발층 정보 반환
    }//if(key_flag==1)
    if(SW0_Flag==1)
      return prev_start_fl;             //입력받기 전 인터럽트 발생시 이전 출발층 반환
  }//while(1)
}

//절댓값으로 바꾸어주는 함수
int ABS(int a, int b)
{
  if(a-b<0)
    return b-a;
  else
    return a-b;
}

//R과 L의 엘리베이터중 선택
int Select_Elevator(int L_cur_fl, int R_cur_fl, int start_fl)
{

  if(ABS(L_cur_fl,start_fl)<=ABS(R_cur_fl, start_fl))           //현재층과 출발층간의 거리비교로 L-E, R-E선정
    return 16;                   //L-E 선택
  else
    return 125;                   //R-E 선택
}

//층 수를 넣으면 해당 층의 GPIOG-> ODR주소를 출력한다.
uint16_t FLOOR_TO_LED(int floor)
{
  uint16_t a;
  switch(floor)
  {
  case 1:
    a=LED1;
    break; 
  case 2:
    a=LED2;
    break; 
  case 3:
    a=LED3;
    break; 
  case 4:
    a=LED4;
    break; 
  case 5:
    a=LED5;
    break; 
  case 6:
    a=LED6;
    break; 
  }
  return a;             //해당층의 GPIOG->ODR 주소 반환
}

//RL_Flag에 따라 색을 지정해주는 함수
void Select_Color(int RL_Flag)
{
  if(RL_Flag==16)
  {
    LCD_SetBrushColor(RGB_BLUE);
    LCD_SetPenColor(RGB_GREEN);
  }
  else if(RL_Flag==125)
  {
    LCD_SetBrushColor(RGB_GREEN);
    LCD_SetPenColor(RGB_BLUE);
  }
}

//엘리베이터를 움직이는 함수
void MoveElevator(void)
{
  EXTI->IMR  |= 0x008000;  		// EXTI15   인터럽트 mask (InterruptEnable) 설정
    LCD_SetTextColor(RGB_RED);             //글자색: RED
    LCD_DisplayText(2,9,"EX");            //FL-->EX 실행모드로 변경
    GPIOG->ODR &=  ~0x0080;               // LED7 off
    GPIOG->ODR |=  0x0001;                 // LED0 on
    
    RL_Flag=Select_Elevator(L_cur_fl, R_cur_fl, start_fl);        
    //L-E, R-E중 선택
    //L-E이면 RL_Flag 는 16
    //R-E이면 RL_Flag 는 125
    
    ////////////////////////////////출발층으로 이동/////////////////////////////////////////////   
    
    /////////////////R-E, L-E결정/////////////////////////////////
    LCD_SetTextColor(RGB_BLUE);             //글자색: BLUE
    if(RL_Flag==16)                       //L-E 선택
    {
      LCD_SetTextColor(RGB_BLUE);             //글자색: BLUE
      LCD_DisplayText(4,8,"L-E");
      cur_fl=L_cur_fl;
    }
    else if(RL_Flag==125)                 //R-E 선택
    {
      LCD_SetTextColor(RGB_BLUE);             //글자색: BLUE
      LCD_DisplayText(4,8,"R-E");
      cur_fl=R_cur_fl;
    }
    
    //////////////////U, D, S 설정/////////////////
    if(L_cur_fl < start_fl)                  //출발층이 현재층보다 높을때
    {
      LCD_SetTextColor(RGB_RED);             //글자색: RED
      LCD_DisplayText(5,9,"U");              //GLCD: U표시
    }
    else if(L_cur_fl > start_fl)          //출발층이 현재층보다 낮을때
    {
      LCD_SetTextColor(RGB_RED);             //글자색: RED
      LCD_DisplayText(5,9,"D");           //GLCD: D표시
    }
    
    //////////////현재층에서 출발층으로 이동/////////////
    if(cur_fl==start_fl)
    {
      //현재층과 출발층이 같을경우
      //바로 U 또는 D표시
    }
    else if(cur_fl < start_fl)             //출발층이 현재층보다 높다.
    {
      for(i=cur_fl; i<start_fl ;i++)
      {
        DelayMS(500);
        Select_Color(RL_Flag);
        LCD_DrawFillRect(RL_Flag,91-13*(i),10,14);
        LCD_DrawVerLine(RL_Flag,91-13*(i) ,13);
        LCD_DrawVerLine(RL_Flag+10,91-13*(i) ,13);
      }//for(i=cur_fl; i<=start_fl+1 ;i++)
      LCD_DrawHorLine(RL_Flag,91-13*(i-1),10);
      DelayMS(500);                       //대기 0.5초
      LCD_SetTextColor(RGB_RED);             //글자색: RED
      LCD_DisplayText(5,9,"S");           //GLCD: S표시
    }//if(cur_fl < start_fl)
    else                                //출발층이 현재층보다 낮다.
    {
      for(i=cur_fl; i>start_fl ;i--)
      {
        DelayMS(500);
        LCD_SetBrushColor(RGB_YELLOW);
        LCD_DrawFillRect(RL_Flag,91-13*(i-1),11,13);
        
        //LCD_SetPenColor(RGB_GREEN);
        //LCD_DrawHorLine(RL_Flag,91-13*(i-1),13);
      }//for(i=cur_fl; i>start_fl+1 ;i--)
      DelayMS(500);                       //대기 0.5초
      LCD_SetTextColor(RGB_RED);             //글자색: RED
      LCD_DisplayText(5,9,"S");           //GLCD: S표시
    }//if(cur_fl > start_fl)
    cur_fl=start_fl;
    
    ////////////////////////////////////출발층 도착///////////////////////////////////////////////
    
    DelayMS(500);                       //대기 0.5초
    
    //////////////////////////////////목표층으로 이동/////////////////////////////////////////////
    
    ////////////////////U, D 설정//////////////////////
    if(cur_fl < des_fl)                  //출발층이 현재층보다 높을때
    {
      LCD_SetTextColor(RGB_RED);             //글자색: RED
      LCD_DisplayText(5,9,"U");              //GLCD: U표시
    }
    else if(cur_fl > des_fl)          //출발층이 현재층보다 낮을때
    {
      LCD_SetTextColor(RGB_RED);             //글자색: RED
      LCD_DisplayText(5,9,"D");           //GLCD: D표시
    }
    
    //////////////현재층에서 목표층으로 이동//////////////////
    
    if(cur_fl < des_fl)             //목표층이 현재층보다 높다.
    {
      for(i=cur_fl; i<des_fl ;i++)
      {
        DelayMS(500);
        Select_Color(RL_Flag);
        LCD_DrawFillRect(RL_Flag,91-13*(i),10,14);
        LCD_DrawVerLine(RL_Flag,91-13*(i) ,13);
        LCD_DrawVerLine(RL_Flag+10,91-13*(i) ,13);
      }//for(i=cur_fl; i<des_fl ;i++)
      LCD_DrawHorLine(RL_Flag,91-13*(i-1),10);
    }//else if(cur_fl < des_fl)
    else                                //목표층이 현재층보다 낮다.
    {
      for(i=cur_fl; i>des_fl ;i--)
      {
        DelayMS(500);
        LCD_SetBrushColor(RGB_YELLOW);
        LCD_DrawFillRect(RL_Flag,91-13*(i-1),11,13);
      }//for(i=cur_fl; i>des_fl+1 ;i--)
      LCD_DrawHorLine(RL_Flag,91-13*(i-1),10);
    }//if(cur_fl > des_fl)
    cur_fl=des_fl;
    
    ///////////////////////////////////////층 선택모드로 변경///////////////////////////////////
    if(RL_Flag==16)                       //L-E 선택
      L_cur_fl=cur_fl;
    else if(RL_Flag==125)                 //R-E 선택
      R_cur_fl=cur_fl;
    LCD_SetTextColor(RGB_RED);             //글자색: RED
    LCD_DisplayText(5,9,"S");              //GLCD: S표시
    LCD_SetTextColor(RGB_RED);             //글자색: RED
    LCD_DisplayText(2,9,"FL");            //EX-->FL : 층선택모드로 변경
    BEEP3();
    
    GPIOG->ODR &= ~0x00FF;	           //LED0~7 off
    GPIOG->ODR |= 0x0080;	           //LED7 on
    start_fl=des_fl=7;
    
    Fram_Write(1001,L_cur_fl);
    Fram_Write(1002,R_cur_fl);
    SW0_Flag=0;
    

    EXTI->IMR  &= ~0x008000;  		// EXTI15  인터럽트 mask (Interrupt Enable) 설정
}
/* Buzzer: Beep for 30 ms */
void BEEP(void)
{ 	
  GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
  DelayMS(30);		// Delay 30 ms
  GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}

void BEEP200ms(void)
{ 	
  GPIOF->ODR |=  0x0200;	// PF9 'H' Buzzer on
  DelayMS(200);		// Delay 30 ms
  GPIOF->ODR &= ~0x0200;	// PF9 'L' Buzzer off
}
void BEEP5s(void)
{
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
  BEEP200ms();                                           //부저 on
  DelayMS(300);                                    //대기 0.3초
}
void BEEP3()
{
  BEEP();
  DelayMS(300);
  BEEP();
  DelayMS(300);
  BEEP();
  DelayMS(300);
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