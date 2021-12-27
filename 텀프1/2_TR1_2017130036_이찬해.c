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
//���ͷ�Ʈ(EXTI8, 15) �߻��� ��Ÿ���� Flag
//0�̸� �߻���������, 1�̸� �߻�
int RL_Flag;                            
//RL_Flag: R �������������� L�������������� �����ϴ�  Flag
//16�̸� L-E, 125�̸� R-E
int Start_des_select_Flag=0;
//Start_des_select_Flag: ������� ��ǥ���� �Է��ϴ� ������ �޸��ϴ� Flag
//0�̸� ����� �Է»���, 1�̸� ��ǥ�� �Է�

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
  _GPIO_Init(); 	                 // GPIO (LED,SW,Buzzer)�ʱ�ȭ
  _EXTI_Init();                        //EXTI �ʱ�ȭ
  LCD_Init();	                         // LCD ��� �ʱ�ȭ
  Fram_Init();                    // FRAM �ʱ�ȭ H/W �ʱ�ȭ
  Fram_Status_Config();   // FRAM �ʱ�ȭ S/W �ʱ�ȭ
  R_cur_fl=Fram_Read(1002);     //FRAM 1002���� ����� L-E�� ������ ������ ����
  DelayMS(10);
  L_cur_fl=Fram_Read(1001);     //FRAM 1001���� ����� L-E�� ������ ������ ����
  DisplayInitScreen();                  // LCD �ʱ�ȭ��
  GPIOG->ODR &= ~0x007F;	        // LED �ʱⰪ: LED0~6 Off
  GPIOG->ODR |=  0x0080;               // LED7 on
  
  
  /*FRAM 1001���� ����� L-E �� ������ ���������� ���� ǥ��*/
  LCD_SetBrushColor(RGB_BLUE);          //ä��� ��: BLUE
  LCD_DrawFillRect(16,91-13*(Fram_Read(1001)-1),10,13*Fram_Read(1001));         
  LCD_SetPenColor(RGB_GREEN);           //�� ��: GREEN
  LCD_DrawRectangle(16,91-13*(Fram_Read(1001)-1),10,13*Fram_Read(1001));
  
  /*FRAM 1002���� ����� R-E �� ������ ���������� ���� ǥ��*/
  LCD_SetBrushColor(RGB_GREEN);         //ä��� ��: GREEN
  LCD_DrawFillRect(125,91-13*(Fram_Read(1002)-1),10,13*Fram_Read(1002));
  LCD_SetPenColor(RGB_BLUE);            //�� ��: BLUE
  LCD_DrawRectangle(125,91-13*(Fram_Read(1002)-1),10,13*Fram_Read(1002));
  
  /*FRAM 1001������ ����� L-E �� ������ �ʱ� �� ���� GLCDǥ��*/
  LCD_SetTextColor(RGB_RED);                  //���ڻ�: RED
  LCD_DisplayChar(6,7,Fram_Read(1001)+48);          //Fram 1001������ ����� L-E �� �����͸� ���� GLCD ����� ǥ��
  LCD_DisplayChar(6,11,Fram_Read(1001)+48);          //Fram 1002������ ����� L-E �� �����͸� ���� GLCD ����� ǥ��
  
/////////////////////////////////�� �Է� ����////////////////////////////////////////
  
  /*������� �Է� �� EXTI8 ���ͷ�Ʈ�� �߻����� �ʵ��� mask����*/
  EXTI->IMR  &= ~0x000100;  		// EXTI8 ���ͷ�Ʈ mask (Interrupt Disable) ����
  EXTI->IMR  &= ~0x008000;  		// EXTI15 ���ͷ�Ʈ mask (Interrupt Disable) ����
  start_fl=SW_TO_START_FLOOR();         //����ġ �Է¿� ���� ����� �Է�
  prev_start_fl=start_fl;               //���� ����� ����
  BEEP();                               //����
  GPIOG->ODR |= FLOOR_TO_LED(start_fl);       //���� �Էµ� start LED on
  LCD_SetTextColor(RGB_RED);                  //���ڻ�: RED
  LCD_DisplayChar(6,7,start_fl+48);          //GLCD ����� ǥ��
  while(1)
  {
    des_fl=SW_TO_DES_FLOOR();                   //����ġ �Է¿� ���� ��ǥ�� �Է�
    if(start_fl==des_fl)
      continue;
    else
    {
      BEEP();                                   //����
      GPIOG->ODR |= FLOOR_TO_LED(des_fl);         // des LED on
      LCD_SetTextColor(RGB_RED);                  //���ڻ�: RED
      LCD_DisplayChar(6,11,des_fl+48);          //GLCD ��ǥ�� ǥ��
      prev_des_fl=des_fl;                       //���� ��ǥ�� ����
      break;
    }
  }//while(1)
  EXTI->IMR  |= 0x000100;  		// EXTI8 ���ͷ�Ʈ mask (Interrupt Enable) ����
  
  while(1)
  {
    if(Start_des_select_Flag==0)                              //����� �Է´����� 
    {
      start_fl=SW_TO_START_FLOOR();                       //������ġ ����ġ�� ������ ������ ������ ����ȴ�.
      if(SW0_Flag==1)                                   //EXTI8 ���ͷ�Ʈ �߻�
      {
        MoveElevator();                                 //���������� ����
        continue;                                       //while ó������ ���ư��� ��������� �Է¹���
      }
      if(des_fl!=start_fl)                              //��ǥ���� ������� �ٸ����
      {
        BEEP();                                 //����
        GPIOG->ODR &= ~0x007F;	                // LED �ʱⰪ: LED0~6 Off
        if(des_fl>=1 && des_fl<=6)
          GPIOG->ODR |= FLOOR_TO_LED(des_fl);         // des LED on
        GPIOG->ODR |= FLOOR_TO_LED(start_fl);       //���� �Էµ� start LED on
        LCD_SetTextColor(RGB_RED);                  //���ڻ�: RED
        LCD_DisplayChar(6,7,start_fl+48);          //GLCD ����� ǥ��
        Start_des_select_Flag=1;                             //��ǥ�� �Է´����·� ����
        prev_start_fl=start_fl;                 //���� ����� ����
      }//if(des_fl!=start_fl)
    }//if(Start_des_select_Flag==0
    
    else if(Start_des_select_Flag==1)                        //��ǥ�� �Է´�����
    {
      des_fl=SW_TO_DES_FLOOR();                 //����ġ �Է¿� ���� ��ǥ�� ����
      if(SW0_Flag==1)                           //EXTI8 ���ͷ�Ʈ �߻�
      {
        MoveElevator();                         //���������� ���� ����
        continue;
      }
      if(start_fl!=des_fl)                      //��ǥ���� ������� �ٸ����
      {
        BEEP();
        GPIOG->ODR &= ~0x007F;	                // LED �ʱⰪ: LED0~6 Off
        GPIOG->ODR |= FLOOR_TO_LED(des_fl);         // des LED on
        GPIOG->ODR |= FLOOR_TO_LED(start_fl);       //���� �Էµ� start LED on
        LCD_SetTextColor(RGB_RED);                  //���ڻ�: RED
        LCD_DisplayChar(6,11,des_fl+48);          //GLCD ��ǥ�� ǥ��
        Start_des_select_Flag=0;                             //����� �Է´����·� ����
        prev_des_fl=des_fl;
      }//if(start_fl!=des_fl)
    }//else if(Start_des_select_Flag==1)
    
  }//while(1) 
}//main()

/* GLCD �ʱ�ȭ�� ���� */
//y,x ����
void DisplayInitScreen(void)
{
  LCD_Clear(RGB_YELLOW);		// ȭ�� Ŭ����
  LCD_SetFont(&Gulim8);		// ��Ʈ : ���� 8
  LCD_SetBackColor(RGB_YELLOW);	// ���ڹ��� : YELLOW
  
  LCD_SetTextColor(RGB_BLACK);	// ���ڻ� :BLACK
  LCD_DisplayText(0,0,"MC Elevator(LCH)");  // Title
  LCD_DisplayText(6,9,">");
  //GLCD: ���� ���������� ���� ǥ��
  LCD_SetTextColor(RGB_BLUE);    // ���ڻ� :BLUE
  LCD_DisplayText(2,4,"6");    
  LCD_DisplayText(3,4,"5");
  LCD_DisplayText(4,4,"4");
  LCD_DisplayText(5,4,"3");
  LCD_DisplayText(6,4,"2");
  LCD_DisplayText(7,4,"1");
  
  //GLCD: ���� ���������� ����ǥ��
  LCD_SetBrushColor(RGB_BLUE);
  LCD_DrawFillRect(16,91,10,13);
  LCD_SetPenColor(RGB_GREEN);
  LCD_DrawRectangle(16,91 ,10,13);
  
  //GLCD: ��� ���������� ����
  LCD_DisplayText(4,8,"L-E");
  LCD_SetTextColor(RGB_RED);    // ���ڻ� :RED
  LCD_DisplayText(2,9,"FL");
  LCD_DisplayText(5,9,"S");
  //LCD_DisplayText(6,7,"1");
  //LCD_DisplayText(6,11,"1");
  LCD_DisplayChar(6,7,Fram_Read(1001)+48);
  LCD_DisplayChar(6,11,Fram_Read(1001)+48);
  
  //������ ���������� ����ǥ��
  LCD_SetTextColor(RGB_GREEN);    // ���ڻ� :BLUE
  LCD_DisplayText(2,14,"6");
  LCD_DisplayText(3,14,"5");
  LCD_DisplayText(4,14,"4");
  LCD_DisplayText(5,14,"3");
  LCD_DisplayText(6,14,"2");
  LCD_DisplayText(7,14,"1");

  //������ ���������� ����ǥ��
  LCD_SetBrushColor(RGB_GREEN);
  LCD_DrawFillRect(125,91,10,13);
  LCD_SetPenColor(RGB_BLUE);
  LCD_DrawRectangle(125,91 ,10,13);
  
  //LCD_DisplayText(2,0,"Des FL: ");
  //LCD_SetTextColor(RGB_RED); 
  //LCD_DisplayText(1,8,"0");
  //LCD_DisplayText(2,8,"-");
}
/* ����ǥ�ÿ����� ���� ����ϴ� �Լ��� */ 
//void LCD_DrawHorLine(UINT16 x, UINT16 y, UINT16 width)  
//void LCD_DrawVerLine(UINT16 x, UINT16 y, UINT16 height)
//void LCD_DrawRectangle(UINT16 x, UINT16 y, UINT16 width, UINT16 height)  
//void LCD_DrawFillRect(UINT16 x, UINT16 y, UINT16 width, UINT16 height)  
//void LCD_DrawLine(UINT16 x1, UINT16 y1, UINT16 x2, UINT16 y2)

/* GPIO (GPIOG(LED), GPIOH(Switch), GPIOF(Buzzer), GPIOI(Joy stick)) �ʱ� ����	*/
void _GPIO_Init(void)
{
  // LED (GPIO G) ����
  RCC->AHB1ENR	|=  0x00000040;	// RCC_AHB1ENR : GPIOG(bit#6) Enable							
  GPIOG->MODER 	|=  0x00005555;	// GPIOG 0~7 : Output mode (0b01)						
  GPIOG->OTYPER	&= ~0x00FF;	// GPIOG 0~7 : Push-pull  (GP8~15:reset state)	
  GPIOG->OSPEEDR 	|=  0x00005555;	// GPIOG 0~7 : Output speed 25MHZ Medium speed 
  
  // SW (GPIO H) ���� 
  RCC->AHB1ENR    |=  0x00000080;	// RCC_AHB1ENR : GPIOH(bit#7) Enable							
  GPIOH->MODER	&= ~0xFFFF0000;	// GPIOH 8~15 : Input mode (reset state)				
  GPIOH->PUPDR 	&= ~0xFFFF0000;	// GPIOH 8~15 : Floating input (No Pull-up, pull-down) :reset state
  
  // Buzzer (GPIO F) ���� 
  RCC->AHB1ENR    |=  0x00000020; // RCC_AHB1ENR : GPIOF(bit#5) Enable							
  GPIOF->MODER    |=  0x00040000;	// GPIOF 9 : Output mode (0b01)						
  GPIOF->OTYPER 	&= ~0x0200;	// GPIOF 9 : Push-pull  	
  GPIOF->OSPEEDR 	|=  0x00040000;	// GPIOF 9 : Output speed 25MHZ Medium speed 
}	

/* EXTI (EXTI8(GPIOH.8, SW0), EXTI15(GPIOH.15, SW7)) �ʱ� ����  */
void _EXTI_Init(void)
{
  RCC->AHB1ENR 	|= 0x00000080  ;	// RCC_AHB1ENR GPIOH  
  RCC->APB2ENR 	|= 0x00004000  ;	// Enable System Configuration Controller Clock
  //|=(1<<14);
  GPIOH->MODER 	&= ~0xFFFF0000;	// GPIOH PIN8,PIN15 Input mode (reset state)			 
  
  SYSCFG->EXTICR[2] |= 0x0007; 	// EXTI8�� ���� �ҽ� �Է��� GPIOH�� ����
  // EXTI8 <- PH8
  // EXTICR3(EXTICR[2])�� �̿� 
  // reset value: 0x0000
  SYSCFG->EXTICR[3] |= 0x7000; 	// EXTI15�� ���� �ҽ� �Է��� GPIOH�� ����	
  //EXTI15 <- PH15
  //EXTICR4(EXTICR[3])�� �̿�
  //reset value: 0x0000
  EXTI->FTSR |= 0x000100;		        // EXTI8: Falling Trigger Enable
  EXTI->FTSR |= 0x008000;		        // EXTI15: Falling Trigger Enable
  
  EXTI->IMR  |= 0x000100;  		// EXTI8 ���ͷ�Ʈ mask (Interrupt Enable) ����
  EXTI->IMR  |= 0x008000;  		// EXTI15 ���ͷ�Ʈ mask (Interrupt Enable) ����	
  
  NVIC->ISER[0] |= (1<<23 );            // Enable 'Global Interrupt EXTI8'
  NVIC->ISER[1] |= (1<<(40-32) );       // Enable 'Global Interrupt EXTI15'		
  // Vector table Position ����
}

/* EXTI5~9 ���ͷ�Ʈ �ڵ鷯(ISR: Interrupt Service Routine) */
void EXTI9_5_IRQHandler(void)
{		
  if(EXTI->PR & 0x0100) 			// EXTI8 Interrupt Pending(�߻�) ����?
  {
    EXTI->PR |= 0x0100;                                   // Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
    SW0_Flag=1;
    start_fl=prev_start_fl;
  }//if(EXTI->PR & 0x0100) 
}//void EXTI9_5_IRQHandler(void)

/*EXTI15~10 ���ͷ�Ʈ �ڵ鷯(ISR: interrupt Service Routine) */
void EXTI15_10_IRQHandler(void)
{
  if(EXTI-> PR & 0x8000)                              // EXTI15 Interrupt Pending(�߻�) ����?
  {
    EXTI->PR |= 0x8000;                               // Pending bit Clear (clear�� ���ϸ� ���ͷ�Ʈ ������ �ٽ� ���ͷ�Ʈ �߻�)
    LCD_DisplayText(2,9,"HD");                     //GLCD FL->HD ����
    GPIOG->ODR &=~0x0001;                          //LED0 off
    BEEP5s();
    LCD_DisplayText(2,9,"FL");                     //GLCD HD->FL ����
    GPIOG->ODR |=0x0001;                          //LED0 on 
    GPIOG->ODR &=~0x0080;                          //LED7 off
  }
}

/* Switch�� �ԷµǾ������� ���ο� � switch�� �ԷµǾ������� ������ return�ϴ� �Լ�  */ 
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
      EXTI->IMR  |= 0x000100;           //EXTI8 ���ͷ�Ʈ�� �����ϵ��� �ϴ� ����ũ
      return key;
    }
    else
    {
      DelayMS(10);
      key_flag = 0;
      EXTI->IMR  |= 0x000100;           //EXTI8 ���ͷ�Ʈ�� �����ϵ��� �ϴ� ����ũ
      return key;
    }
  }
  else				// if key input, check continuous key
  {
    if(key_flag != 0)	// if continuous key, treat as no key input
    {
      EXTI->IMR  |= 0x000100;           //EXTI8 ���ͷ�Ʈ�� �����ϵ��� �ϴ� ����ũ
      return 0xFF00;
    }
    else			// if new key,delay for debounce
    {
      key_flag = 1;
      DelayMS(10);
      EXTI->IMR  |= 0x000100;           //EXTI8 ���ͷ�Ʈ�� �����ϵ��� �ϴ� ����ũ
      return key;
    }
  }
}

//���ϴ� ���� ����ġ�� ������ �ش� ������ ������ ��ȯ�ϴ� �Լ�
int SW_TO_DES_FLOOR(void){
  int a, key_flag=0;
  while(1){
    switch(KEY_Scan()){
    case SW1:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=1;
      key_flag=1;
      break; 
    case SW2:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=2;
      key_flag=1;
      break; 
    case SW3:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=3;
      key_flag=1;
      break; 
    case SW4:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=4;
      key_flag=1;
      break; 
    case SW5:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=5;
      key_flag=1;
      break; 
    case SW6: 
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=6;
      key_flag=1;
      break;
    }//switch(KEY_Scan)  
    if(key_flag==1)
    {
      key_flag=0;
      
      return a;                         //�Է¹��� ��ǥ�� ��ȯ
    }//if(key_flag==1)
    if(SW0_Flag==1){

      return prev_des_fl;               //�Է¹ޱ� �� ���ͷ�Ʈ �߻��� ���� ��ǥ�� ��ȯ
    }
  }//while(1)
}

//���ϴ� ���� ����ġ�� ������ �ش� ������ ������ ��ȯ�ϴ� �Լ�
int SW_TO_START_FLOOR(void){
  int a, key_flag=0;
  while(1){
    switch(KEY_Scan()){
    case SW1:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=1;
      key_flag=1;
      break; 
    case SW2:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=2;
      key_flag=1;
      break; 
    case SW3:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=3;
      key_flag=1;
      break; 
    case SW4:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;  
      a=4;
      key_flag=1;
      break; 
    case SW5:
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=5;
      key_flag=1;
      break; 
    case SW6: 
      if(SW0_Flag==1)                   //SW0_Flag==1: EXTI8 ���ͷ�Ʈ �߻�
        break;
      a=6;
      key_flag=1;
      break;
    }//switch(KEY_Scan)  
    if(key_flag==1)
    {
      key_flag=0;
      return a;                         //�Է¹��� ����� ���� ��ȯ
    }//if(key_flag==1)
    if(SW0_Flag==1)
      return prev_start_fl;             //�Է¹ޱ� �� ���ͷ�Ʈ �߻��� ���� ����� ��ȯ
  }//while(1)
}

//�������� �ٲپ��ִ� �Լ�
int ABS(int a, int b)
{
  if(a-b<0)
    return b-a;
  else
    return a-b;
}

//R�� L�� ������������ ����
int Select_Elevator(int L_cur_fl, int R_cur_fl, int start_fl)
{

  if(ABS(L_cur_fl,start_fl)<=ABS(R_cur_fl, start_fl))           //�������� ��������� �Ÿ��񱳷� L-E, R-E����
    return 16;                   //L-E ����
  else
    return 125;                   //R-E ����
}

//�� ���� ������ �ش� ���� GPIOG-> ODR�ּҸ� ����Ѵ�.
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
  return a;             //�ش����� GPIOG->ODR �ּ� ��ȯ
}

//RL_Flag�� ���� ���� �������ִ� �Լ�
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

//���������͸� �����̴� �Լ�
void MoveElevator(void)
{
  EXTI->IMR  |= 0x008000;  		// EXTI15   ���ͷ�Ʈ mask (InterruptEnable) ����
    LCD_SetTextColor(RGB_RED);             //���ڻ�: RED
    LCD_DisplayText(2,9,"EX");            //FL-->EX ������� ����
    GPIOG->ODR &=  ~0x0080;               // LED7 off
    GPIOG->ODR |=  0x0001;                 // LED0 on
    
    RL_Flag=Select_Elevator(L_cur_fl, R_cur_fl, start_fl);        
    //L-E, R-E�� ����
    //L-E�̸� RL_Flag �� 16
    //R-E�̸� RL_Flag �� 125
    
    ////////////////////////////////��������� �̵�/////////////////////////////////////////////   
    
    /////////////////R-E, L-E����/////////////////////////////////
    LCD_SetTextColor(RGB_BLUE);             //���ڻ�: BLUE
    if(RL_Flag==16)                       //L-E ����
    {
      LCD_SetTextColor(RGB_BLUE);             //���ڻ�: BLUE
      LCD_DisplayText(4,8,"L-E");
      cur_fl=L_cur_fl;
    }
    else if(RL_Flag==125)                 //R-E ����
    {
      LCD_SetTextColor(RGB_BLUE);             //���ڻ�: BLUE
      LCD_DisplayText(4,8,"R-E");
      cur_fl=R_cur_fl;
    }
    
    //////////////////U, D, S ����/////////////////
    if(L_cur_fl < start_fl)                  //������� ���������� ������
    {
      LCD_SetTextColor(RGB_RED);             //���ڻ�: RED
      LCD_DisplayText(5,9,"U");              //GLCD: Uǥ��
    }
    else if(L_cur_fl > start_fl)          //������� ���������� ������
    {
      LCD_SetTextColor(RGB_RED);             //���ڻ�: RED
      LCD_DisplayText(5,9,"D");           //GLCD: Dǥ��
    }
    
    //////////////���������� ��������� �̵�/////////////
    if(cur_fl==start_fl)
    {
      //�������� ������� �������
      //�ٷ� U �Ǵ� Dǥ��
    }
    else if(cur_fl < start_fl)             //������� ���������� ����.
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
      DelayMS(500);                       //��� 0.5��
      LCD_SetTextColor(RGB_RED);             //���ڻ�: RED
      LCD_DisplayText(5,9,"S");           //GLCD: Sǥ��
    }//if(cur_fl < start_fl)
    else                                //������� ���������� ����.
    {
      for(i=cur_fl; i>start_fl ;i--)
      {
        DelayMS(500);
        LCD_SetBrushColor(RGB_YELLOW);
        LCD_DrawFillRect(RL_Flag,91-13*(i-1),11,13);
        
        //LCD_SetPenColor(RGB_GREEN);
        //LCD_DrawHorLine(RL_Flag,91-13*(i-1),13);
      }//for(i=cur_fl; i>start_fl+1 ;i--)
      DelayMS(500);                       //��� 0.5��
      LCD_SetTextColor(RGB_RED);             //���ڻ�: RED
      LCD_DisplayText(5,9,"S");           //GLCD: Sǥ��
    }//if(cur_fl > start_fl)
    cur_fl=start_fl;
    
    ////////////////////////////////////����� ����///////////////////////////////////////////////
    
    DelayMS(500);                       //��� 0.5��
    
    //////////////////////////////////��ǥ������ �̵�/////////////////////////////////////////////
    
    ////////////////////U, D ����//////////////////////
    if(cur_fl < des_fl)                  //������� ���������� ������
    {
      LCD_SetTextColor(RGB_RED);             //���ڻ�: RED
      LCD_DisplayText(5,9,"U");              //GLCD: Uǥ��
    }
    else if(cur_fl > des_fl)          //������� ���������� ������
    {
      LCD_SetTextColor(RGB_RED);             //���ڻ�: RED
      LCD_DisplayText(5,9,"D");           //GLCD: Dǥ��
    }
    
    //////////////���������� ��ǥ������ �̵�//////////////////
    
    if(cur_fl < des_fl)             //��ǥ���� ���������� ����.
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
    else                                //��ǥ���� ���������� ����.
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
    
    ///////////////////////////////////////�� ���ø��� ����///////////////////////////////////
    if(RL_Flag==16)                       //L-E ����
      L_cur_fl=cur_fl;
    else if(RL_Flag==125)                 //R-E ����
      R_cur_fl=cur_fl;
    LCD_SetTextColor(RGB_RED);             //���ڻ�: RED
    LCD_DisplayText(5,9,"S");              //GLCD: Sǥ��
    LCD_SetTextColor(RGB_RED);             //���ڻ�: RED
    LCD_DisplayText(2,9,"FL");            //EX-->FL : �����ø��� ����
    BEEP3();
    
    GPIOG->ODR &= ~0x00FF;	           //LED0~7 off
    GPIOG->ODR |= 0x0080;	           //LED7 on
    start_fl=des_fl=7;
    
    Fram_Write(1001,L_cur_fl);
    Fram_Write(1002,R_cur_fl);
    SW0_Flag=0;
    

    EXTI->IMR  &= ~0x008000;  		// EXTI15  ���ͷ�Ʈ mask (Interrupt Enable) ����
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
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
  BEEP200ms();                                           //���� on
  DelayMS(300);                                    //��� 0.3��
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